#include "Magnum/GL/GL.h"
#include <WaterSimulation/ShallowWater.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/ImageView.h>
#include <Magnum/Image.h>
#include <algorithm>
#include <cstdint>


float ShallowWater::upwinded_h_x(int i, int j,float qxij){
    int il = std::max(0, i - 1);
    int ir = std::min(nx - 1, i);
    return (qxij >= 0.0f) ? h[idc(il, j)] : h[idc(ir, j)];
}


float ShallowWater::upwinded_h_y(int i, int j,float qyij){
    int jl = std::max(0, j - 1);
    int jr = std::min(ny - 1, j);
    return (qyij >= 0.0f) ? h[idc(i, jl)] : h[idc(i, jr)];
}

void ShallowWater::computeVelocities(){

    for(int j = 0; j < ny; ++j){
        for (int i = 0; i < nx + 1; ++i) {
            
            float qxij = qx[idsx(i, j)];
            float h_upwind = upwinded_h_x(i, j, qxij);

            if(h_upwind > dryEps){
                ux[idsx(i, j)] = qxij/h_upwind;
            }else{
                ux[idsx(i, j)] = 0.0f;
            }
        }
    }

    for(int j = 0; j < ny + 1; ++j){
        for (int i = 0; i < nx; ++i) {
            
            float qyij = qy[idsy(i, j)];
            float h_upwind = upwinded_h_y(i, j, qyij);

            if(h_upwind > dryEps){
                uy[idsy(i, j)] = qyij/h_upwind;
            }else{
                uy[idsy(i, j)] = 0.0f;
            }
        }
    }

}

void ShallowWater::updateFluxes(){

    //calcul des du / dt
    for(int j = 0; j < ny; ++j){
        for (int i = 1; i < nx; ++i) {
            
            float qxij = qx[idsx(i, j)];
            float uxij = ux[idsx(i, j)];
            
            float h_upwind = upwinded_h_x(i, j, qxij);

            if(h_upwind <= dryEps){ //pas assez d'eau donc on skip
                continue;
            }

            //ajouter advection

            float Hl = h[idc(i-1, j)] + terrain[idc(i-1,j)];
            float Hr = h[idc(i,j)] + terrain[idc(i,j)];

            float pressure = -gravity * (Hr - Hl) / dx;

            float dudt = pressure;

            ux[idsx(i, j)] += dudt * dt;

        }
    }

    for(int j = 1; j < ny; ++j){
        for (int i = 0; i < nx; ++i) {

            float qyij = qy[idsx(i, j)];
            float uyij = uy[idsy(i, j)];

            float h_upwind = upwinded_h_y(i, j, qyij);

            if(h_upwind <= dryEps){ //pas assez d'eau donc on skip
                continue;
            }

            //ajouter advection

            float Hb = h[idc(i, j-1)] + terrain[idc(i, j-1)];
            float Ht = h[idc(i, j)] + terrain[idc(i, j)];

            float pressure = -gravity * (Ht - Hb) / dx;

            float dudt = pressure;

            uy[idsy(i, j)] += dudt * dt;
            
        }
    }

    //mise a jour de qx et qy
    for(int j = 0; j < ny; ++j){
        for (int i = 0; i < nx + 1; ++i) {
            float uxij = ux[idsx(i, j)];
            float h_upwind = upwinded_h_x(i, j, uxij);
            qx[idsx(i, j)] = ux[idsx(i, j)] * h_upwind;
        }
    }

    for(int j = 0; j < ny + 1; ++j){
        for (int i = 0; i < nx; ++i) {
            float uyij = uy[idsy(i, j)];
            float h_upwind = upwinded_h_y(i, j, uyij);
            qy[idsy(i, j)] = uy[idsy(i, j)] * h_upwind;
        }
    }
}

void ShallowWater::updateWaterHeight(){

    for(int j = 0; j < ny; ++j){
        for (int i = 0; i < nx; ++i) {

            float qxl = qx[idsx(i, j)];
            float qxr = qx[idsx(i+1, j)];
            float qyb = qy[idsy(i, j)];
            float qyt = qy[idsy(i, j+1)];

            float div_x = (qxr - qxl) / dx;
            float div_y = (qyt - qyb) / dx;

            float total_divergence = div_x + div_y;

            h[idc(i, j)] -= total_divergence * dt;

        }
    }

}

void ShallowWater::step(){

    computeVelocities();
    updateFluxes();
    updateWaterHeight();

}

//convertit h en tableau de pixels normalisé
void ShallowWater::updateHeightTexture(Magnum::GL::Texture2D* texture) const {

    auto minmax = std::minmax_element(h.begin(), h.end());
    auto minIt = minmax.first;
    auto maxIt = minmax.second;
    float minHeight = *minIt;
    float maxHeight = *maxIt;

    std::vector<uint8_t> pixels;
    pixels.resize(h.size());
    std::transform(h.begin(), h.end(), pixels.begin(), [&](float height) {
        float normalized = (height - minHeight)/(maxHeight - minHeight);
        return static_cast<uint8_t>(Magnum::Math::clamp(normalized * 255.0f, 0.0f, 255.0f));
    });

    texture->setSubImage(0, {},
        Magnum::ImageView2D{
        Magnum::PixelFormat::R8Unorm,
        {nx, ny},
        {pixels.data(), pixels.size() } }
    );

}

void ShallowWater::initBump(){
    int centerX = nx / 2;
        int centerY = ny / 2;
        float bumpHeight = 1.0f;

        for (int j = 0; j < ny; ++j) {
            for (int i = 0; i < nx; ++i) {
                float distance = std::sqrt((i - centerX) * (i - centerX) + (j - centerY) * (j - centerY));
                float radius = 16.0f;
                h[idc(i, j)] = 2.0f;
                if (distance < radius) {
                    h[idc(i, j)] += bumpHeight * (1.0f - distance / radius);
                }
            }
        }
}