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

    Magnum::GL::Texture2D m_stateTexture; // RGB 32f texture that contains (h, qx, qy)
    Magnum::GL::Texture2D m_stateTexturePong; // ping pong setup // texture final
    Magnum::GL::Texture2D m_prevStateTexture; // Previous state for height update
    Magnum::GL::Texture2D m_terrainTexture;   // Terrain R 32f texture

    //
    Magnum::GL::Texture2D m_bulkTexture; 
    Magnum::GL::Texture2D m_surfaceTexture; 

    Magnum::GL::Texture2D m_surfaceHeightTexture;
    Magnum::GL::Texture2D m_surfaceQxTexture;
    Magnum::GL::Texture2D m_surfaceQyTexture;

    Magnum::GL::Texture2D m_surfaceHeightPing;
    Magnum::GL::Texture2D m_surfaceHeightPong;
    Magnum::GL::Texture2D m_surfaceQxPong;
    Magnum::GL::Texture2D m_surfaceQyPong;

    Magnum::GL::Texture2D m_tempTexture;
    Magnum::GL::Texture2D m_tempTexture2;
    Magnum::GL::Texture2D m_tempTexture3;

    // Visualization textures
    Magnum::GL::Texture2D m_visBulkUpdated;
    Magnum::GL::Texture2D m_visFFTHeight;
    Magnum::GL::Texture2D m_visFFTQx;
    Magnum::GL::Texture2D m_visFFTQy;
    Magnum::GL::Texture2D m_visIFFTHeight;
    Magnum::GL::Texture2D m_visIFFTQx;
    Magnum::GL::Texture2D m_visIFFTQy;
    Magnum::GL::Texture2D m_visTransportedFlow;
    Magnum::GL::Texture2D m_visTransportedHeight;
    Magnum::GL::Texture2D m_visAdvectedHeight;

    Magnum::GL::Texture2D * m_fftOutput = nullptr;
    Magnum::GL::Texture2D * m_ifftOutput = nullptr;
    


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

        ComputeProgram &bindDecompose(Magnum::GL::Texture2D *stateIn,
                                   Magnum::GL::Texture2D *terrain,Magnum::GL::Texture2D *bulk,
                                   Magnum::GL::Texture2D *surfaceHeight, 
                                   Magnum::GL::Texture2D *surfaceQx,
                                   Magnum::GL::Texture2D *surfaceQy,
                                   Magnum::GL::Texture2D *tempIn,
                                   Magnum::GL::Texture2D *tempOut){

            stateIn->bindImage(0, 0, Magnum::GL::ImageAccess::ReadOnly,
                             Magnum::GL::ImageFormat::RGBA32F);
            terrain->bindImage(1, 0, Magnum::GL::ImageAccess::ReadOnly,
                              Magnum::GL::ImageFormat::R32F);
            bulk->bindImage(2, 0, Magnum::GL::ImageAccess::WriteOnly,
                             Magnum::GL::ImageFormat::RGBA32F);
            surfaceHeight->bindImage(3, 0, Magnum::GL::ImageAccess::WriteOnly,
                              Magnum::GL::ImageFormat::RG32F);
            surfaceQx->bindImage(4, 0, Magnum::GL::ImageAccess::WriteOnly,
                              Magnum::GL::ImageFormat::RG32F);
            surfaceQy->bindImage(5, 0, Magnum::GL::ImageAccess::WriteOnly,
                              Magnum::GL::ImageFormat::RG32F);
            tempIn->bindImage(6, 0, Magnum::GL::ImageAccess::ReadOnly,
                              Magnum::GL::ImageFormat::RGBA32F);
            tempOut->bindImage(7, 0, Magnum::GL::ImageAccess::WriteOnly,
                              Magnum::GL::ImageFormat::RGBA32F);
            return *this;
        }

        ComputeProgram &bindFFT(Magnum::GL::Texture2D *input,
                                Magnum::GL::Texture2D *output) {
            input->bindImage(0, 0, Magnum::GL::ImageAccess::ReadOnly,
                             Magnum::GL::ImageFormat::RG32F);
            output->bindImage(1, 0, Magnum::GL::ImageAccess::WriteOnly,
                              Magnum::GL::ImageFormat::RG32F);
            return *this;
        }

        ComputeProgram &bindReadWrite(Magnum::GL::Texture2D * input, Magnum::GL::ImageFormat format = Magnum::GL::ImageFormat::RGBA32F){
            input->bindImage(0,0,Magnum::GL::ImageAccess::ReadWrite, format);
            return *this;
        }

        ComputeProgram &bindClear(Magnum::GL::Texture2D * output, Magnum::GL::ImageFormat format = Magnum::GL::ImageFormat::RGBA32F){
            output->bindImage(0,0,Magnum::GL::ImageAccess::WriteOnly, format);
            return *this;
        }

        ComputeProgram &bindCopy(Magnum::GL::Texture2D * input, Magnum::GL::Texture2D * output){
            input->bindImage(0,0,Magnum::GL::ImageAccess::ReadOnly, Magnum::GL::ImageFormat::RG32F);
            output->bindImage(1,0,Magnum::GL::ImageAccess::WriteOnly, Magnum::GL::ImageFormat::RG32F);
            return *this;
        }

        ComputeProgram &bindCopyRGBA(Magnum::GL::Texture2D * input, Magnum::GL::Texture2D * output){
            input->bindImage(0,0,Magnum::GL::ImageAccess::ReadOnly, Magnum::GL::ImageFormat::RGBA32F);
            output->bindImage(1,0,Magnum::GL::ImageAccess::WriteOnly, Magnum::GL::ImageFormat::RGBA32F);
            return *this;
        }

        ComputeProgram &bindAiry(Magnum::GL::Texture2D *bulkState,
                     Magnum::GL::Texture2D *surfaceHeightFFT,
                     Magnum::GL::Texture2D *surfaceQxFFT,
                     Magnum::GL::Texture2D *surfaceQyFFT) {
            bulkState->bindImage(0, 0, Magnum::GL::ImageAccess::ReadOnly,
                     Magnum::GL::ImageFormat::RGBA32F);
            surfaceHeightFFT->bindImage(1, 0, Magnum::GL::ImageAccess::ReadOnly,
                        Magnum::GL::ImageFormat::RG32F);
            surfaceQxFFT->bindImage(2, 0, Magnum::GL::ImageAccess::ReadWrite,
                        Magnum::GL::ImageFormat::RG32F);
            surfaceQyFFT->bindImage(3, 0, Magnum::GL::ImageAccess::ReadWrite,
                        Magnum::GL::ImageFormat::RG32F);
            return *this;
        }

        ComputeProgram &bindRecompose(Magnum::GL::Texture2D *stateOut,
                     Magnum::GL::Texture2D *bulkFlow,
                     Magnum::GL::Texture2D *surfaceHeight,
                     Magnum::GL::Texture2D *surfaceQx,
                     Magnum::GL::Texture2D *surfaceQy) {
            stateOut->bindImage(0, 0, Magnum::GL::ImageAccess::WriteOnly,
                     Magnum::GL::ImageFormat::RGBA32F);
            bulkFlow->bindImage(1, 0, Magnum::GL::ImageAccess::ReadOnly,
                        Magnum::GL::ImageFormat::RGBA32F);
            surfaceHeight->bindImage(2, 0, Magnum::GL::ImageAccess::ReadOnly,
                        Magnum::GL::ImageFormat::RG32F);
            surfaceQx->bindImage(3, 0, Magnum::GL::ImageAccess::ReadOnly,
                        Magnum::GL::ImageFormat::RG32F);
            surfaceQy->bindImage(4, 0, Magnum::GL::ImageAccess::ReadOnly,
                        Magnum::GL::ImageFormat::RG32F);
            return *this;
        }

        ComputeProgram &bindTransportSurfaceFlow(Magnum::GL::Texture2D * surfaceQx,
                     Magnum::GL::Texture2D *surfaceQy,
                     Magnum::GL::Texture2D *bulkFlow,
                     Magnum::GL::Texture2D *bulkFLowOld,
                    Magnum::GL::Texture2D *surfaceOut) {
            surfaceQx->bindImage(3, 0, Magnum::GL::ImageAccess::ReadOnly,
                     Magnum::GL::ImageFormat::RG32F);
            surfaceQy->bindImage(4, 0, Magnum::GL::ImageAccess::ReadOnly,
                     Magnum::GL::ImageFormat::RG32F);

            bulkFlow->bindImage(2, 0, Magnum::GL::ImageAccess::ReadOnly,
                        Magnum::GL::ImageFormat::RGBA32F);
            bulkFLowOld->bindImage(1, 0, Magnum::GL::ImageAccess::ReadOnly,
                        Magnum::GL::ImageFormat::RGBA32F);

            surfaceOut->bindImage(0, 0, Magnum::GL::ImageAccess::WriteOnly,
                        Magnum::GL::ImageFormat::RGBA32F);
            return *this;
        }

        ComputeProgram &bindTransportSurfaceHeight(Magnum::GL::Texture2D * surfaceHeight,
                     Magnum::GL::Texture2D *bulkFlow,
                    Magnum::GL::Texture2D *surfaceOut) {
            surfaceHeight->bindImage(2, 0, Magnum::GL::ImageAccess::ReadOnly,
                     Magnum::GL::ImageFormat::RG32F);

            bulkFlow->bindImage(1, 0, Magnum::GL::ImageAccess::ReadOnly,
                        Magnum::GL::ImageFormat::RGBA32F);

            surfaceOut->bindImage(0, 0, Magnum::GL::ImageAccess::ReadWrite,
                        Magnum::GL::ImageFormat::RGBA32F);
            return *this;
        }

        ComputeProgram &bindAdvection(Magnum::GL::Texture2D *surfaceIn,
                          Magnum::GL::Texture2D *surfaceOut,
                          Magnum::GL::Texture2D *bulkFlow) {
            surfaceIn->bindImage(0, 0, Magnum::GL::ImageAccess::ReadOnly,
                     Magnum::GL::ImageFormat::RGBA32F);
            surfaceOut->bindImage(1, 0, Magnum::GL::ImageAccess::WriteOnly,
                      Magnum::GL::ImageFormat::RGBA32F);
            bulkFlow->bindImage(2, 0, Magnum::GL::ImageAccess::ReadOnly,
                    Magnum::GL::ImageFormat::RGBA32F);
            return *this;
        }

        ComputeProgram &bindUpdateHeight(Magnum::GL::Texture2D *bulk,
                         Magnum::GL::Texture2D *surface,
                         Magnum::GL::Texture2D *stateIn,
                         Magnum::GL::Texture2D *stateOut) {
            bulk->bindImage(0, 0, Magnum::GL::ImageAccess::ReadOnly,
                    Magnum::GL::ImageFormat::RGBA32F);
            surface->bindImage(1, 0, Magnum::GL::ImageAccess::ReadOnly,
                       Magnum::GL::ImageFormat::RGBA32F);
            stateIn->bindImage(2, 0, Magnum::GL::ImageAccess::ReadOnly,
                    Magnum::GL::ImageFormat::RGBA32F);
            stateOut->bindImage(3, 0, Magnum::GL::ImageAccess::WriteOnly,
                    Magnum::GL::ImageFormat::RGBA32F);
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
    ComputeProgram m_updateHeightSimpleProgram;

    ComputeProgram m_decompositionProgram;

    ComputeProgram m_fftHorizontalProgram;
    ComputeProgram m_fftVerticalProgram;

    ComputeProgram m_initProgram;

    ComputeProgram m_debugAlphaProgram;

    ComputeProgram m_fftProgram;

    ComputeProgram m_bitReverseProgram;

    ComputeProgram m_normalizedProgram; // To normalized ifft output

    ComputeProgram m_copyProgram;
    ComputeProgram m_CopyRGBAProgram;

    ComputeProgram m_clearProgram;
    ComputeProgram m_clearRGProgram;

    ComputeProgram m_airywavesProgram;

    ComputeProgram m_recomposeProgram;

    ComputeProgram m_transportSurfaceFlowProgram;
    ComputeProgram m_transportSurfaceHeightProgram;

    ComputeProgram m_semiLagrangianAdvectionProgram;

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

        m_stateTexture
            .setStorage(
                1, Magnum::GL::TextureFormat::RGBA32F,
                {nx + 1, ny + 1}) // Store only 3 components but there a no
                                  // RGB32F image format for compute binding
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_stateTexturePong
            .setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_prevStateTexture
            .setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_terrainTexture
            .setStorage(1, Magnum::GL::TextureFormat::R32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_bulkTexture.setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_surfaceTexture.setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        // Surface textures use RG32F for complex numbers (for FFT)
        m_surfaceHeightTexture.setStorage(1, Magnum::GL::TextureFormat::RG32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setWrapping(Magnum::GL::SamplerWrapping::Repeat);

        m_surfaceQxTexture.setStorage(1, Magnum::GL::TextureFormat::RG32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_surfaceQyTexture.setStorage(1, Magnum::GL::TextureFormat::RG32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_surfaceHeightPing.setStorage(1, Magnum::GL::TextureFormat::RG32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setWrapping(Magnum::GL::SamplerWrapping::Repeat);

        m_surfaceHeightPong.setStorage(1, Magnum::GL::TextureFormat::RG32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setWrapping(Magnum::GL::SamplerWrapping::Repeat);;

        m_surfaceQxPong.setStorage(1, Magnum::GL::TextureFormat::RG32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_surfaceQyPong.setStorage(1, Magnum::GL::TextureFormat::RG32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_tempTexture.setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_tempTexture2.setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_tempTexture2.setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_tempTexture3.setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_visBulkUpdated.setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_visFFTHeight.setStorage(1, Magnum::GL::TextureFormat::RG32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_visFFTQx.setStorage(1, Magnum::GL::TextureFormat::RG32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_visFFTQy.setStorage(1, Magnum::GL::TextureFormat::RG32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_visIFFTHeight.setStorage(1, Magnum::GL::TextureFormat::RG32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_visIFFTQx.setStorage(1, Magnum::GL::TextureFormat::RG32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_visIFFTQy.setStorage(1, Magnum::GL::TextureFormat::RG32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_visTransportedFlow.setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_visTransportedHeight.setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);

        m_visAdvectedHeight.setStorage(1, Magnum::GL::TextureFormat::RGBA32F, {nx + 1, ny + 1})
            .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
            .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear);
        

        compilePrograms();
    }

    void step();

    void compilePrograms();
    
    void clearAllTextures();

    void runSW(Magnum::GL::Texture2D *inputTex,Magnum::GL::Texture2D *outputTex);
    void runDecomposition(Magnum::GL::Texture2D *inputTex);
    Magnum::GL::Texture2D* runFFT(Magnum::GL::Texture2D* pingTex, Magnum::GL::Texture2D* pongTex, 
                        int direction);
    void runIFFT();

    // helper functions
    int getnx() const { return nx; }
    int getny() const { return ny; }

    Magnum::GL::Texture2D &getStateTexture() {
        return m_stateTexture;
    }

    Magnum::GL::Texture2D &getTerrainTexture() { return m_terrainTexture; }

    Magnum::GL::Texture2D &getBulkTexture() { return m_bulkTexture; }
    Magnum::GL::Texture2D &getSurfaceTexture() { return m_surfaceTexture; }
    Magnum::GL::Texture2D &getSurfaceHeightTexture() { return m_surfaceHeightTexture; }
    Magnum::GL::Texture2D &getSurfaceQxTexture() { return m_surfaceQxTexture; }
    Magnum::GL::Texture2D &getSurfaceQyTexture() { return m_surfaceQyTexture; }

    Magnum::GL::Texture2D &getVisBulkUpdated() { return m_visBulkUpdated; }
    Magnum::GL::Texture2D &getVisFFTHeight() { return m_visFFTHeight; }
    Magnum::GL::Texture2D &getVisFFTQx() { return m_visFFTQx; }
    Magnum::GL::Texture2D &getVisFFTQy() { return m_visFFTQy; }
    Magnum::GL::Texture2D &getVisIFFTHeight() { return m_visIFFTHeight; }
    Magnum::GL::Texture2D &getVisIFFTQx() { return m_visIFFTQx; }
    Magnum::GL::Texture2D &getVisIFFTQy() { return m_visIFFTQy; }
    Magnum::GL::Texture2D &getVisTransportedFlow() { return m_visTransportedFlow; }
    Magnum::GL::Texture2D &getVisTransportedHeight() { return m_visTransportedHeight; }
    Magnum::GL::Texture2D &getVisAdvectedHeight() { return m_visAdvectedHeight; }

    Magnum::GL::Texture2D *getFFTOutput() { return m_fftOutput; }
    Magnum::GL::Texture2D *getIFFTOutput() { return m_ifftOutput; }
    

    bool airyWavesEnabled = true;
    // initialisation
    void initBump();
    void initDamBreak();
    void initTsunami();

    void loadTerrainHeightMap(Magnum::Trade::ImageData2D *tex,
                              float scaling = 1.0f, int channels = 1);

    // debug
    float minh;
    float maxh;
    float minux;
    float maxux;
    float minuy;
    float maxuy;
};