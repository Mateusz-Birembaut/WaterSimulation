#pragma once

#include "Corrade/Containers/String.h"
#include "Corrade/Utility/DebugAssert.h"
#include "Magnum/GL/AbstractShaderProgram.h"
#include "Magnum/GL/GL.h"
#include "Magnum/GL/Shader.h"
#include "Magnum/GL/TextureArray.h"
#include "Magnum/GL/ImageFormat.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <Magnum/Magnum.h>
#include <Magnum/Image.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>

class ShallowWater{
private:
    //Dimensions de la simulation
    int nx, ny; //nombre de cellules sur chaque axe
    float dx; //l'écart entre les cellules
    float dt; //le pas de temps
    float gravity = 9.81f; //la gravité

    //Textures des quantités
    //h calculé a t + dt/2, stocké au centre des cellules
    std::vector<float> h; //calculé a partir de la divergence des q, représente la hauteur de l'eau uniquement (hauteur total = h + terrain)

    //q calculé a t + dt, stockés sur les parois des cellules !
    std::vector<float> qx; //q = h * u
    std::vector<float> qy;
    std::vector<float> terrain; // la hauteur du terrain

    //secondaire, temporaire
    std::vector<float> ux; //velocité sur l'axe x calculé a partir de qx
    std::vector<float> uy; //velocité sur l'axe y


    //stabilité
    float dryEps = 1e-3f; //valeur de h a partir de laquelle une cellule est considéré comme sec
    float limitCFL; //coefficient CFL
    float friction_coef = 0.2f; 

    //coeur de la simu
    void computeVelocities(); //calcul de u et v a partir de q et h
    void computeVelocitiesDelta(); // calcul des du/dt
    void updateFluxes(); //mise a jour des q 
    void updateWaterHeight(); //calcul des h a partir de la divergence des q

    void applyBarrier(); //gère les quantités en bordure

    inline bool isDry(int i, int j){ return h[idc(i, j)] <= dryEps;}

    void enforceCFL(); //Pour garder la simulation stable il ne faut pas que la velocité (ou le pas de temps) soit trop grande

    //helper func
    float upwinded_h_x(int i, int j,float qxij); // récupère la hauteur sur les faces
    float upwinded_h_y(int i, int j,float qyij);

    // Compute Shaders

    bool ping = false;

    Magnum::GL::Texture2D m_stateTexturePing; // RGB 32f texture that contains (h, qx, qy)
    Magnum::GL::Texture2D m_stateTexturePong; //ping pong setup
    Magnum::GL::Texture2D m_terrainTexture; // Terrain R 32f texture 

    

    struct ComputeProgram : public Magnum::GL::AbstractShaderProgram {

        ComputeProgram() = default;

        ComputeProgram(Magnum::Containers::String filepath){
            Magnum::GL::Shader compute(Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Compute);
        
            compute.addSource(filepath);
            CORRADE_INTERNAL_ASSERT_OUTPUT(compute.compile());

            attachShader(compute);
            link();
        }


        ComputeProgram& bindStates(Magnum::GL::Texture2D& input, Magnum::GL::Texture2D& output){
            input.bindImage(0,0, Magnum::GL::ImageAccess::ReadOnly, Magnum::GL::ImageFormat::RGBA32F);
            output.bindImage(1,0, Magnum::GL::ImageAccess::WriteOnly, Magnum::GL::ImageFormat::RGBA32F);
            return *this;
        }

        ComputeProgram& bindTerrain(Magnum::GL::Texture2D& input){
            input.bindImage(2, 0, Magnum::GL::ImageAccess::ReadOnly, Magnum::GL::ImageFormat::R32F);
            return *this;
        }

        ComputeProgram& setParametersUniforms(ShallowWater& sim){
            setUniform(uniformLocation("dx"), sim.dx);
            setUniform(uniformLocation("dt"), sim.dt);
            setUniform(uniformLocation("gravity"), sim.gravity);
            setUniform(uniformLocation("dryEps"), sim.dryEps);
            setUniform(uniformLocation("friction_coef"), sim.friction_coef);
            return *this;
        }


    };

    ComputeProgram m_computeVelocitiesProgram;
    ComputeProgram m_updateFluxesProgram;
    ComputeProgram m_updateWaterHeightProgram;
    ComputeProgram m_applyBarrierProgram;


public:

    ShallowWater() = default;

    ShallowWater(size_t nx_, size_t ny_, float dx_, float dt_){
        nx = nx_; ny = ny_; dx = dx_; dt = dt_;

        h.assign( nx * ny, 0.0f);
        terrain.assign(nx*ny, 0.0f);
        
        qx.assign((nx+1) * ny, 0.0f);
        qy.assign(nx*(ny+1), 0.0f);

        ux.assign((nx+1) * ny, 0.0f);
        uy.assign(nx*(ny+1), 0.0f);
        
        limitCFL = dx / (5.0f*dt);


        //

        m_stateTexturePing
            .setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx+1, ny+1}) // Store only 3 components but there a no RGB32F image format for compute binding
            .setMinificationFilter(Magnum::GL::SamplerFilter::Nearest)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Nearest);

        m_stateTexturePong
            .setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx+1, ny+1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Nearest)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Nearest);

        m_terrainTexture
            .setStorage(1, Magnum::GL::TextureFormat::R32F, {nx, ny})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Nearest)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Nearest);

        m_computeVelocitiesProgram = ComputeProgram("ressources/shaders/compute/computeVelocities.comp");
        m_updateFluxesProgram = ComputeProgram("ressources/shaders/compute/updateFluxes.comp");
        m_updateWaterHeightProgram = ComputeProgram("ressources/shaders/compute/updateWaterHeight.comp");
        m_applyBarrierProgram = ComputeProgram("ressources/shaders/compute/applyBarrier.comp");

        m_computeVelocitiesProgram.bindStates(m_stateTexturePing, m_stateTexturePong).bindTerrain(m_terrainTexture).setParametersUniforms(*this);
        m_updateFluxesProgram.bindStates(m_stateTexturePing, m_stateTexturePong).bindTerrain(m_terrainTexture).setParametersUniforms(*this);
        m_updateWaterHeightProgram.bindStates(m_stateTexturePing, m_stateTexturePong).bindTerrain(m_terrainTexture).setParametersUniforms(*this);
        m_applyBarrierProgram.bindStates(m_stateTexturePing, m_stateTexturePong).bindTerrain(m_terrainTexture).setParametersUniforms(*this);
    }

    void step();


    //helper functions

    inline int idc(int i, int j) const {return j * nx + i;} //retourne l'index pour les grilles centrés (comme la hauteur)
    inline int idsx(int i, int j) const { return j * (nx+1) + i;} //index side x, retourne l'index pour les valeurs de qx et ux
    inline int idsy(int i, int j) const { return j * nx + i;} //index side y, retourne l'index pour les valeurs de qy et uy
    bool isWetFaceX(int i,int j) const;
    bool isWetFaceY(int i,int j) const;

    int getnx() const {return nx;}
    int getny() const {return ny;}
    void updateHeightTexture(Magnum::GL::Texture2D* texture);
    void updateMomentumTexture(Magnum::GL::Texture2D* texture);

    Magnum::GL::Texture2D& getStateTexture() { 
        return ping ? m_stateTexturePing : m_stateTexturePong; 
    }
    
    Magnum::GL::Texture2D& getTerrainTexture() { 
        return m_terrainTexture; 
    }

    //initialisation
    void initBump();
    void initTop();

    void loadTerrainHeightMap(Magnum::Trade::ImageData2D * img, float scaling = 1.0f);

    //debug
    float minh;
    float maxh;
    float minux;
    float maxux;
    float minuy;
    float maxuy;
};