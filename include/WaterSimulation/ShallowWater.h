#pragma once

#include "Corrade/Containers/String.h"
#include "Corrade/Utility/Debug.h"
#include "Corrade/Utility/DebugAssert.h"
#include "Corrade/Utility/Resource.h"
#include <Corrade/Utility/ConfigurationGroup.h>
#include "Magnum/GL/AbstractShaderProgram.h"
#include "Magnum/GL/GL.h"
#include "Magnum/GL/ImageFormat.h"
#include "Magnum/GL/OpenGL.h"
#include "Magnum/GL/Renderer.h"
#include "Magnum/GL/Shader.h"
#include "Magnum/GL/TextureArray.h"
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Image.h>
#include <Magnum/Magnum.h>
#include <Magnum/Trade/ImageData.h>
#include <cstddef>
#include <cstdint>
#include <vector>

class ShallowWater {
  private:
    // Dimensions de la simulation
    int nx, ny;            // nombre de cellules sur chaque axe
    float dx;              // l'écart entre les cellules
    float dt;              // le pas de temps
    float gravity = 9.81f; // la gravité

    // stabilité
    float dryEps = 1e-3f; // valeur de h a partir de laquelle une cellule est
                          // considéré comme sec
    float limitCFL; // coefficient CFL
    float friction_coef = 0.01f;

    // Compute Shaders

    bool ping = false;

    Magnum::GL::Texture2D m_stateTexturePing; // RGB 32f texture that contains (h, qx, qy)
    Magnum::GL::Texture2D m_stateTexturePong; // ping pong setup
    Magnum::GL::Texture2D m_terrainTexture;   // Terrain R 32f texture

    int groupx, groupy;

    struct ComputeProgram : public Magnum::GL::AbstractShaderProgram {

        ComputeProgram() = default;

        ComputeProgram(Magnum::Containers::String filepath) {
            Magnum::GL::Shader compute(Magnum::GL::Version::GL430,
                                       Magnum::GL::Shader::Type::Compute);

            Corrade::Utility::Resource rs{"WaterSimulationResources"};
            compute.addSource(rs.getString(filepath));

            CORRADE_INTERNAL_ASSERT_OUTPUT(compute.compile());

            attachShader(compute);
            link();
        }

        ComputeProgram &bindStates(Magnum::GL::Texture2D *input,
                                   Magnum::GL::Texture2D *output) {
            input->bindImage(0, 0, Magnum::GL::ImageAccess::ReadOnly,
                             Magnum::GL::ImageFormat::RGBA32F);
            output->bindImage(1, 0, Magnum::GL::ImageAccess::WriteOnly,
                              Magnum::GL::ImageFormat::RGBA32F);
            return *this;
        }

        ComputeProgram &bindTerrain(Magnum::GL::Texture2D *input) {
            input->bindImage(2, 0, Magnum::GL::ImageAccess::ReadOnly,
                             Magnum::GL::ImageFormat::R32F);
            return *this;
        }

        ComputeProgram &setParametersUniforms(ShallowWater &sim) {
            setUniform(uniformLocation("dx"), sim.dx);
            setUniform(uniformLocation("dt"), sim.dt);
            setUniform(uniformLocation("gravity"), sim.gravity);
            setUniform(uniformLocation("dryEps"), sim.dryEps);
            setUniform(uniformLocation("friction_coef"), sim.friction_coef);
            return *this;
        }

        ComputeProgram &setFloatUniform(const char *name, float value) {
            setUniform(uniformLocation(name), value);
            return *this;
        }

        ComputeProgram &setIntUniform(const char *name, int value) {
            setUniform(uniformLocation(name), value);
            return *this;
        }

        ComputeProgram &run(unsigned int nx, unsigned int ny) {
            dispatchCompute({nx, ny, 1});
            return *this;
        }
    };

    ComputeProgram m_computeVelocitiesProgram;
    ComputeProgram m_updateFluxesProgram;
    ComputeProgram m_updateWaterHeightProgram;

    ComputeProgram m_initProgram;

  public:
    ShallowWater() = default;

    ShallowWater(size_t nx_, size_t ny_, float dx_, float dt_,
                 int groups = 16) {
        nx = nx_;
        ny = ny_;
        dx = dx_;
        dt = dt_;

        groupx = (nx + groups - 1) / groups;
        groupy = (ny + groups - 1) / groups;

        limitCFL = dx / (4.0f * dt);

        //

        m_stateTexturePing
            .setStorage(
                1, Magnum::GL::TextureFormat::RGBA32F,
                {nx + 1, ny + 1}) // Store only 3 components but there a no
                                  // RGB32F image format for compute binding
            .setMinificationFilter(Magnum::GL::SamplerFilter::Nearest)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Nearest);

        m_stateTexturePong
            .setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Nearest)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Nearest);

        m_terrainTexture
            .setStorage(1, Magnum::GL::TextureFormat::R32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Nearest)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Nearest);

        m_updateFluxesProgram =
            ComputeProgram("updateFluxes.comp");
        m_updateWaterHeightProgram =
            ComputeProgram("updateWaterHeight.comp");
        m_initProgram = ComputeProgram("init.comp");

        m_updateFluxesProgram.setParametersUniforms(*this);
        m_updateWaterHeightProgram.setParametersUniforms(*this);
    }

    void step();

    // helper functions
    int getnx() const { return nx; }
    int getny() const { return ny; }

    Magnum::GL::Texture2D &getStateTexture() {
        return ping ? m_stateTexturePing : m_stateTexturePong;
    }

    Magnum::GL::Texture2D &getTerrainTexture() { return m_terrainTexture; }

    // initialisation
    void initBump();
    void initDamBreak();

    void loadTerrainHeightMap(Magnum::Trade::ImageData2D *tex,
                              float scaling = 1.0f);

    // debug
    float minh;
    float maxh;
    float minux;
    float maxux;
    float minuy;
    float maxuy;
};