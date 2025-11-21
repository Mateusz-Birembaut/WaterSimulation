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

//first order upwinding with hydrostatic reconstruction
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

void ShallowWater::computeVelocities() {

    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx + 1; ++i) {

            float qxij = qx[idsx(i, j)];
            float h_upwind = upwinded_h_x(i, j, qxij);

            if (h_upwind > dryEps) {
                ux[idsx(i, j)] = qxij / h_upwind;
            } else {
                ux[idsx(i, j)] = 0.0f;
                qx[idsx(i, j)] = 0.0f;
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
                qy[idsy(i, j)] = 0.0f;
            }
        }
    }
}

void ShallowWater::updateFluxes() {

	std::vector<float> ux_current = ux; 
	std::vector<float> uy_current = uy; 

    // calcul des du / dt
    for (int j = 1; j < ny; ++j) { // loop through vertical faces
        for (int i = 1; i < nx; ++i) {

            float qxij = qx[idsx(i, j)];
			float qyij = qy[idsy(i, j)];

            float h_upwind = upwinded_h_x(i, j, qxij);
			
			//Corrade::Utility::Debug{} << h_upwind << "  " << dryEps;

            if (h_upwind <= dryEps) { // pas assez d'eau donc on skip
				//Corrade::Utility::Debug{} << "h too small";
				//qx[idsx(i, j)] = 0.0f;
				ux[idsx(i, j)] = 0.0f;
                continue;
            }

            /* // advection x
			float u_face = (qxij > 0) ? ux_current[idsx(i - 1, j)] : ux_current[idsx(i, j)];
			float q_up = (qxij > 0) ? qx[idsx(i - 1, j)] : qx[idsx(i, j)];
			float advection_x = (qxij - q_up) / h_upwind * u_face;

			// advection y
			float v_face = (qyij > 0) ? uy_current[idsy(i, j - 1)] : uy_current[idsy(i, j)];
			float qy_up = (qyij > 0) ? qy[idsy(i, j - 1)] : qy[idsy(i, j)];
			float advection_y = (qyij - qy_up) / h_upwind * v_face; */


			float uc = ux_current[idsx(i, j)];
			float ul = ux_current[idsx(i-1, j)];

			float ql = qx[idsx(i-1, j)];



			//

            float Hl = h[idc(i - 1, j)] + terrain[idc(i - 1, j)];
            float Hr = h[idc(i, j)] + terrain[idc(i, j)];

            float pressure = -gravity * (Hr - Hl) / dx;

            float dudt = pressure ;//+ advection_x + advection_y;

            ux[idsx(i, j)] += dudt * dt;
            ux[idsx(i, j)] = Magnum::Math::clamp(ux[idsx(i, j)], -limitCFL, limitCFL);
        }
    }

    for (int j = 1; j < ny; ++j) { //loop through horizontal faces
        for (int i = 1; i < nx; ++i) {

            float qyij = qy[idsy(i, j)];
            float uyij = uy_current[idsy(i, j)];

            float h_upwind = upwinded_h_y(i, j, qyij);

            if (h_upwind <= dryEps) { // pas assez d'eau donc on skip
				//qy[idsy(i, j)] = 0.0f;
				uy[idsy(i, j)] = 0.0f;
                continue;
            }

            /* // advection x
			float ul = ux_current[idsx(i - 1, j)];
			float ur = ux_current[idsx(i, j)];
			float ql = qx[idsx(i - 1, j)];
			float qr = qx[idsx(i, j)];
			float advection_x = ((qr - ql) / h_upwind) * (ur - ul) / dx;

			// advection y
			float ub = uy_current[idsy(i, j - 1)];
			float ut = uy_current[idsy(i, j)];
			float qb = qy[idsy(i, j - 1)];
			float qt = qy[idsy(i, j)];
			float advection_y = ((qt - qb) / h_upwind) * (ut - ub) / dx; */
            

            float Hb = h[idc(i, j - 1)] + terrain[idc(i, j - 1)];
            float Ht = h[idc(i, j)] + terrain[idc(i, j)];

            float pressure = -gravity * (Ht - Hb) / dx;

            float dudt = pressure ;//+ advection_x + advection_y;

            uy[idsy(i, j)] += dudt * dt;
            uy[idsy(i, j)] = Magnum::Math::clamp(uy[idsy(i, j)], -limitCFL, limitCFL);
        }
    }

    // mise a jour de qx et qy
    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx + 1; ++i) {
            float uxij = ux[idsx(i, j)];
            float h_upwind = upwinded_h_x(i, j, uxij);
            qx[idsx(i, j)] = Magnum::Math::clamp(ux[idsx(i, j)] * h_upwind, -h_upwind * limitCFL, h_upwind * limitCFL);
        }
    }

    for (int j = 0; j < ny + 1; ++j) {
        for (int i = 0; i < nx; ++i) {
            float uyij = uy[idsy(i, j)];
            float h_upwind = upwinded_h_y(i, j, uyij);
            qy[idsy(i, j)] = Magnum::Math::clamp(uy[idsy(i, j)] * h_upwind, -h_upwind * limitCFL, h_upwind * limitCFL);
        }
    }
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
void ShallowWater::updateHeightTexture(Magnum::GL::Texture2D *texture) const {

    auto minmax = std::minmax_element(h.begin(), h.end());
    auto minIt = minmax.first;
    auto maxIt = minmax.second;
    float minHeight = *minIt;
    float maxHeight = *maxIt;

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
void ShallowWater::updateMomentumTexture(Magnum::GL::Texture2D *texture) const {
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

    std::vector<uint8_t> pixels;
    pixels.resize(nx * ny * 3);

    const float rangeUx = maxUx - minUx;
    const float rangeUy = maxUy - minUy;

    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            float uxi = ux[idsx(i, j)];
            float uyi = uy[idsy(i, j)];

            float normUx = (rangeUx > 0.0f) ? (uxi - minUx) / rangeUx : 0.0f;
            float normUy = (rangeUy > 0.0f) ? (uyi - minUy) / rangeUy : 0.0f;

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