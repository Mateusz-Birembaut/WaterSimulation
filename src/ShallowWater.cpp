#include "Corrade/Utility/Debug.h"
#include "Magnum/GL/GL.h"
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/Math/Color.h>
#include <Magnum/PixelFormat.h>
#include <WaterSimulation/ShallowWater.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>

//get h at face ij with upwinding and hydrostatic reconstruction
float ShallowWater::upwinded_h_x(int i, int j, float qxij) {
    int il = std::max(0, i - 1);
    int ir = std::min(nx - 1, i);
    
    float eta_l = h[idc(il, j)] + terrain[idc(il, j)];
    float eta_r = h[idc(ir, j)] + terrain[idc(ir, j)];

    float terrain_max = std::max(terrain[idc(il, j)], terrain[idc(ir, j)]);
    float h_l_recon = std::max(0.0f, eta_l - terrain_max);
    float h_r_recon = std::max(0.0f, eta_r - terrain_max);

    return (qxij >= 0.0f) ? h_l_recon : h_r_recon;
}

float ShallowWater::upwinded_h_y(int i, int j, float qyij) {
    int jl = std::max(0, j - 1);
    int jr = std::min(ny - 1, j);
    
    float eta_b = h[idc(i, jl)] + terrain[idc(i, jl)];
    float eta_t = h[idc(i, jr)] + terrain[idc(i, jr)];
    
    float terrain_max = std::max(terrain[idc(i, jl)], terrain[idc(i, jr)]);
    float h_b_recon = std::max(0.0f, eta_b - terrain_max);
    float h_t_recon = std::max(0.0f, eta_t - terrain_max);
    
    return (qyij >= 0.0f) ? h_b_recon : h_t_recon;
}

bool ShallowWater::isWetFaceX(int i, int j) const {
    int il = std::max(0, i - 1);
    int ir = std::min(nx - 1, i);
    
    float etaL = h[idc(il, j)] + terrain[idc(il, j)];
    float etaR = h[idc(ir, j)] + terrain[idc(ir, j)];
    float zmax = std::max(terrain[idc(il, j)], terrain[idc(ir, j)]);
    return (etaL > zmax + dryEps) || (etaR > zmax + dryEps);

}

bool ShallowWater::isWetFaceY(int i, int j) const {
    int jl = std::max(0, j - 1);
    int jr = std::min(ny - 1, j);

    float etaB = h[idc(i, jl)] + terrain[idc(i, jl)];
    float etaT = h[idc(i, jr)] + terrain[idc(i, jr)];
    float zmax = std::max(terrain[idc(i, jl)], terrain[idc(i, jr)]);
    return (etaB > zmax + dryEps) || (etaT > zmax + dryEps);
}

void ShallowWater::computeVelocities() {

    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx + 1; ++i) {

            float qxij = qx[idsx(i, j)];
            float h_upwind = upwinded_h_x(i, j, qxij);

            if (h_upwind > dryEps) {
                ux[idsx(i, j)] = qxij / h_upwind;
            } else {
                ux[idsx(i, j)] = 0.0f;
                //qx[idsx(i, j)] = 0.0f;
            }
        }
    }

    for (int j = 0; j < ny + 1; ++j) {
        for (int i = 0; i < nx; ++i) {

            float qyij = qy[idsy(i, j)];
            float h_upwind = upwinded_h_y(i, j, qyij);

            if (h_upwind > dryEps) {
                uy[idsy(i, j)] = qyij / h_upwind;
            } else {
                uy[idsy(i, j)] = 0.0f;
                //qy[idsy(i, j)] = 0.0f;
            }
        }
    }
}

void ShallowWater::updateFluxes() {
    std::vector<float> qx_new = qx;
    std::vector<float> qy_new = qy;

    for (int j = 1; j < ny - 1; ++j) {
        for (int i = 1; i < nx; ++i) {
            
            if(!isWetFaceX(i, j)){
                qx_new[idsx(i, j)] = 0.0f;
                continue;
            }

            float qxij = qx[idsx(i, j)];
            float h_upwind = upwinded_h_x(i, j, qxij);

            if (h_upwind <= dryEps) {
                qx_new[idsx(i, j)] = 0.0f;
                continue;
            }

            float eta_l = h[idc(i-1, j)] + terrain[idc(i-1, j)];
            float eta_r = h[idc(i, j)] + terrain[idc(i, j)];
            
            float h_avg = 0.5f * (h[idc(i-1, j)] + h[idc(i, j)]);

            float pressure = -gravity * (eta_r - eta_l) / dx;

            //advection
            float qx_center = qxij;
            float qx_left = qx[idsx(i-1, j)];
            float qx_right = qx[idsx(i+1, j)];
            
            float u_face = qxij / h_upwind;
            float advection = 0.0f;
            
            if (u_face > 0.0f) {
                advection = -u_face * (qx_center - qx_left) / dx;
            } else {
                advection = -u_face * (qx_right - qx_center) / dx;
            }

            //

            float friction = -friction_coef * u_face * std::abs(u_face) / h_upwind;

            float accel = pressure + friction;
            
            float dqdt = h_avg * accel + advection;

            qx_new[idsx(i, j)] = qxij + dqdt * dt;
            
            float max_q = h_upwind * limitCFL;
            qx_new[idsx(i, j)] = Magnum::Math::clamp(qx_new[idsx(i, j)], 
                                                      -max_q, max_q);
        }
    }

    for (int j = 1; j < ny; ++j) {
        for (int i = 1; i < nx - 1; ++i) {
            
            if(!isWetFaceY(i, j)){
                qy_new[idsy(i, j)] = 0.0f;
                continue;
            }

            float qyij = qy[idsy(i, j)];
            float h_upwind = upwinded_h_y(i, j, qyij);

            if (h_upwind <= dryEps) {
                qy_new[idsy(i, j)] = 0.0f;
                continue;
            }

            float eta_b = h[idc(i, j-1)] + terrain[idc(i, j-1)];
            float eta_t = h[idc(i, j)] + terrain[idc(i, j)];
            
            float h_avg = 0.5f * (h[idc(i, j-1)] + h[idc(i, j)]);

            float pressure = -gravity * (eta_t - eta_b) / dx;

            //advection

            float qy_center = qyij;
            float qy_bottom = qy[idsy(i, j-1)];
            float qy_top = qy[idsy(i, j+1)];
            
            float v_face = qyij / h_upwind;
            float advection = 0.0f;
            
            if (v_face > 0.0f) {
                advection = -v_face * (qy_center - qy_bottom) / dx;
            } else {
                advection = -v_face * (qy_top - qy_center) / dx;
            }

            //

            float friction = -friction_coef * v_face * std::abs(v_face) / h_upwind;

            float accel = pressure + friction;
            float dqdt = h_avg * accel + advection;

            qy_new[idsy(i, j)] = qyij + dqdt * dt;
            
            float max_q = h_upwind * limitCFL;
            qy_new[idsy(i, j)] = Magnum::Math::clamp(qy_new[idsy(i, j)], 
                                                      -max_q, max_q);
        }
    }

    qx = std::move(qx_new);
    qy = std::move(qy_new);
}

void ShallowWater::applyBarrier() {
    for (int j = 0; j < ny; j++) {
        qx[idsx(0, j)] = 0.0f;
        ux[idsx(0, j)] = 0.0f;
        qx[idsx(nx, j)] = 0.0f;
        ux[idsx(nx, j)] = 0.0f;
    }
    
    for (int i = 0; i < nx; i++) {
        qy[idsy(i, 0)] = 0.0f;
        uy[idsy(i, 0)] = 0.0f;
        qy[idsy(i, ny)] = 0.0f;
        uy[idsy(i, ny)] = 0.0f;
    }
}

void ShallowWater::updateWaterHeight() {

    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {

			//if(isDry(i, j))

            float qxl = qx[idsx(i, j)];
            float qxr = qx[idsx(i + 1, j)];
            float qyb = qy[idsy(i, j)];
            float qyt = qy[idsy(i, j + 1)];

            float div_x = (qxr - qxl) / dx;
            float div_y = (qyt - qyb) / dx;

            float total_divergence = div_x + div_y;

            h[idc(i, j)] -= total_divergence * dt;
            h[idc(i, j)] = std::max(0.0f, h[idc(i, j)]);
        }
    }
}

void ShallowWater::enforceCFL(){



}

void ShallowWater::step() {

    computeVelocities();
	
    updateFluxes();
	applyBarrier();
	enforceCFL();
    updateWaterHeight();
}

// convertit h en tableau de pixels normalisé
void ShallowWater::updateHeightTexture(Magnum::GL::Texture2D *texture) {

    auto minmax = std::minmax_element(h.begin(), h.end());
    auto minIt = minmax.first;
    auto maxIt = minmax.second;
    float minHeight = *minIt;
    float maxHeight = *maxIt;

    minh = minHeight;
    maxh = maxHeight;

    std::vector<uint8_t> pixels;
    pixels.resize(h.size());
    std::transform(h.begin(), h.end(), pixels.begin(), [&](float height) {
        float normalized = (height - minHeight) / (maxHeight - minHeight);
        return static_cast<uint8_t>(
            Magnum::Math::clamp(normalized * 255.0f, 0.0f, 255.0f));
    });

    texture->setSubImage(0, {},
                         Magnum::ImageView2D{Magnum::PixelFormat::R8Unorm,
                                             {nx, ny},
                                             {pixels.data(), pixels.size()}});
}

// convertit ux et uy  en tableau de pixels
void ShallowWater::updateMomentumTexture(Magnum::GL::Texture2D *texture) {
    float minUx = INFINITY, maxUx = -INFINITY;
    float minUy = INFINITY, maxUy = -INFINITY;

    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            float uxi = ux[idsx(i, j)];
            float uyi = uy[idsy(i, j)];
            minUx = std::min(minUx, uxi);
            maxUx = std::max(maxUx, uxi);
            minUy = std::min(minUy, uyi);
            maxUy = std::max(maxUy, uyi);
        }
    }

    minux = minUx;
    maxux = maxUx;
    minuy = minUy;
    maxuy = maxUy;
    bool normalize = false;

    std::vector<uint8_t> pixels;
    pixels.resize(nx * ny * 3);

    const float rangeUx = normalize ? (maxUx - minUx) : 1.0f;
    const float rangeUy = normalize ? (maxUy - minUy) : 1.0f;
    const float offsetUx = normalize ? minUx : 0.0f;
    const float offsetUy = normalize ? minUy : 0.0f;

    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            float uxi = ux[idsx(i, j)];
            float uyi = uy[idsy(i, j)];

            float normUx = (rangeUx > 0.0f) ? (uxi - offsetUx) / rangeUx : 0.0f;
            float normUy = (rangeUy > 0.0f) ? (uyi - offsetUy) / rangeUy : 0.0f;

            normUx = Magnum::Math::clamp(normUx, 0.0f, 1.0f);
            normUy = Magnum::Math::clamp(normUy, 0.0f, 1.0f);

            size_t idx = (j * nx + i) * 3;
            pixels[idx + 0] = static_cast<uint8_t>(normUx * 255.0f);
            pixels[idx + 1] = static_cast<uint8_t>(normUy * 255.0f);
            pixels[idx + 2] = 0;
        }
    }

    texture->setSubImage(
        0, {},
        Magnum::ImageView2D{
            Magnum::PixelFormat::RGB8Unorm,
            {nx, ny},
            {pixels.data(), pixels.size()}});
}

void ShallowWater::loadTerrainHeightMap(Magnum::Trade::ImageData2D* img, float scaling) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(img->data().data());
	const int channels = 4;

	float minTerrain = INFINITY;
	float maxTerrain = -INFINITY;

	for (int j = 0; j < ny; ++j) {
		for (int i = 0; i < nx; ++i) {
			const uint8_t r = data[(j * nx + i) * channels + 0];
			terrain[idc(i, j)] = r * scaling / 255.0f;
			minTerrain = std::min(minTerrain, terrain[idc(i, j)]);
			maxTerrain = std::max(maxTerrain, terrain[idc(i, j)]);
		}
	}

	Corrade::Utility::Debug{} << "Terrain height - min:" << minTerrain << "max:" << maxTerrain;
}

void ShallowWater::initBump() {
    int centerX = nx / 2;
    int centerY = ny / 2;
    float bumpHeight = 2.0f;
    float baseLevel = 2.0f;

    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            float distance = std::sqrt(static_cast<float>((i - centerX) * (i - centerX) +
                                                          (j - centerY) * (j - centerY)));
            float radius = 16.0f;
            float totalHeight = baseLevel - terrain[idc(i, j)];
            if (totalHeight <= dryEps) {h[idc(i, j)] = 0.0f; continue;}
            h[idc(i, j)] = totalHeight;
            if (distance < radius) {
                h[idc(i, j)] += bumpHeight * (1.0f - distance / radius);
            }

        }
    }
}

void ShallowWater::initTop() {
    float waterLevel = 1.0f;
    int damPosition = ny / 6;
    
    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            if (j < damPosition) {
                float depth = waterLevel - terrain[idc(i, j)];
                h[idc(i, j)] = std::max(0.0f, depth);
            } else {
                h[idc(i, j)] = 0.0f;
            }
        }
    }
}