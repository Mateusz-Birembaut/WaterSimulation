#include "Magnum/GL/Renderer.h"
#include "Magnum/Trade/Trade.h"
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Debug.h>
#include <Magnum/GL/GL.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/Math/Color.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/ImageFormat.h>
#include <WaterSimulation/ShallowWater.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>

void ShallowWater::step() {

    Magnum::GL::Texture2D *inputTex;
    Magnum::GL::Texture2D *outputTex;

    

    // Decompisition Bulk (shallow) + surface (airy) 

    runDecomposition(&m_stateTexture);

    // Shallow water pass

    runSW(&m_bulkTexture);


    // Airy Waves
    {   
        float N = nx+1;
        Magnum::GL::Texture2D * fftHeight; Magnum::GL::Texture2D * ifftHeight;
        Magnum::GL::Texture2D * fftQx; Magnum::GL::Texture2D * ifftQx;
        Magnum::GL::Texture2D * fftQy; Magnum::GL::Texture2D * ifftQy;
        /* m_copyProgram.bindCopy(&m_surfaceHeightTexture,&m_surfaceHeightPing).run(groupx,groupy);

        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

        Corrade::Utility::Debug{} << "running fft pass"; */
        // FFT pass
        fftHeight = runFFT(&m_surfaceHeightTexture, &m_surfaceHeightPong, 1);
        
        fftQx = runFFT(&m_surfaceQxTexture, &m_surfaceQxPong, 1);
        fftQy = runFFT(&m_surfaceQyTexture, &m_surfaceQyPong, 1);

        //m_airywavesProgram.bindAiry(&m_bulkTexture, fftHeight, fftQx, fftQy );

        // Turn qx and qy back into spatial domain
        ifftQx = runFFT(fftQx, &m_surfaceQxPong, -1);
        ifftQy = runFFT(fftQy, &m_surfaceQyPong, -1);

        // Normalize the IFFT outputs
        m_normalizedProgram.bindReadWrite(ifftQx, Magnum::GL::ImageFormat::RG32F)
            .setFloatUniform("norm", 1.0f / (N * N))
            .run(groupx, groupy);

        m_normalizedProgram.bindReadWrite(ifftQy, Magnum::GL::ImageFormat::RG32F)
            .setFloatUniform("norm", 1.0f / (N * N))
            .run(groupx, groupy);

        m_fftOutput = ifftQx;
        m_ifftOutput = ifftQy;
        //m_fftOutput = ifftQx;
        /* Magnum::GL::Texture2D* ifftPong = (m_fftOutput == &m_surfaceHeightPing) 
                                        ? &m_surfaceHeightPong 
                                        : &m_surfaceHeightPing;

        
        m_copyProgram.bindCopy(m_fftOutput,&m_surfaceQxPong).run(groupx,groupy);

        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess); */

        //runAiryWaves(m_fftOutput);

        // IFFT pass, m_fftOutput can be surface Height pong, add an if 
        ifftHeight = runFFT(fftHeight, &m_surfaceHeightPong, -1);

        m_normalizedProgram.bindReadWrite(ifftHeight, Magnum::GL::ImageFormat::RG32F).setFloatUniform("norm", 1.0f/(N*N)).run(groupx, groupy);

        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

        m_recomposeProgram.bindRecompose(&m_stateTexture, &m_bulkTexture, ifftHeight, ifftQx, ifftQy)
            .run(groupx, groupy);

        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
    }

    ping = !ping;
}

void ShallowWater::runDecomposition(Magnum::GL::Texture2D *inputTex) {
    // Initialisation
    m_decompositionProgram
        .bindDecompose(inputTex, &m_terrainTexture, &m_bulkTexture,
                       &m_surfaceHeightTexture, &m_surfaceQxTexture,
                       &m_surfaceQyTexture, &m_tempTexture, &m_tempTexture2)
        .setIntUniform("stage", 0)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    // Diffusion
    const int diffusionIterations = 64;
    Magnum::GL::Texture2D *tempIn = &m_tempTexture2;
    Magnum::GL::Texture2D *tempOut = &m_tempTexture;

    for (int i = 0; i < diffusionIterations; i++) {
        m_decompositionProgram
            .bindDecompose(inputTex, &m_terrainTexture, &m_bulkTexture,
                           &m_surfaceHeightTexture, &m_surfaceQxTexture,
                           &m_surfaceQyTexture, tempIn, tempOut)
            .setIntUniform("stage", 1)
            .run(groupx, groupy);

        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

        std::swap(tempIn, tempOut);
    }

    // Compute final values
    m_decompositionProgram
        .bindDecompose(inputTex, &m_terrainTexture, &m_bulkTexture,
                       &m_surfaceHeightTexture, &m_surfaceQxTexture,
                       &m_surfaceQyTexture, tempIn, tempOut)
        .setIntUniform("stage", 2)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}

void ShallowWater::runSW(Magnum::GL::Texture2D *inputTex) {
    m_updateFluxesProgram.bindStates(inputTex, &m_tempTexture)
        .bindTerrain(&m_terrainTexture)
        .run(groupx, groupy);

    // Wait for all imageStore to be done before continuing
    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    m_updateWaterHeightProgram.bindStates(&m_tempTexture, inputTex)
        .bindTerrain(&m_terrainTexture)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}

Magnum::GL::Texture2D *ShallowWater::runFFT(Magnum::GL::Texture2D *pingTex,
                                            Magnum::GL::Texture2D *pongTex,
                                            int direction) {
    int N = nx + 1;
    int numStages = static_cast<int>(std::log2(N));

    //Corrade::Utility::Debug{} << "N:" << N << " stages:" << numStages;

    bool pingIsInput = true;
    unsigned int groupsX = (N + 256 - 1) / 256;
    unsigned int groups16 = (N + 15) / 16;

    // Bit reversal
    m_bitReverseProgram.bindFFT(pingTex, pongTex)
        .setIntUniform("u_length", N)
        .setIntUniform("u_isVertical", 0)
        .run(groups16, groups16);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    pingIsInput = false;

    //Corrade::Utility::Debug{} << "horizontal";

    // Horizontal FFT stages
    for(int s = 0; s < numStages; s++) {
        Magnum::GL::Texture2D* in  = pingIsInput ? pingTex : pongTex;
        Magnum::GL::Texture2D* out = pingIsInput ? pongTex : pingTex;

        m_fftProgram.bindFFT(in, out)
            .setIntUniform("u_stage", s)
            .setIntUniform("u_length", N)
            .setIntUniform("u_direction", direction)
            .setIntUniform("u_isVertical", 0)
            .run(groupsX, N);

        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

        pingIsInput = !pingIsInput;
    }

    // Bit reversal
    Magnum::GL::Texture2D* currentIn = pingIsInput ? pingTex : pongTex;
    Magnum::GL::Texture2D* currentOut = pingIsInput ? pongTex : pingTex;

    m_bitReverseProgram.bindFFT(currentIn, currentOut)
        .setIntUniform("u_length", N)
        .setIntUniform("u_isVertical", 1)
        .run(groups16, groups16);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    pingIsInput = !pingIsInput;

    //Corrade::Utility::Debug{} << "vertical";

    // Vertical fft
    for(int s = 0; s < numStages; s++) {
        Magnum::GL::Texture2D* in  = pingIsInput ? pingTex : pongTex;
        Magnum::GL::Texture2D* out = pingIsInput ? pongTex : pingTex;

        m_fftProgram.bindFFT(in, out)
            .setIntUniform("u_stage", s)
            .setIntUniform("u_length", N)
            .setIntUniform("u_direction", direction)
            .setIntUniform("u_isVertical", 1)
            .run(groupsX, N);

        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

        pingIsInput = !pingIsInput;
    }

    return pingIsInput ? pingTex : pongTex;
}

void ShallowWater::compilePrograms() {
    m_updateFluxesProgram = ComputeProgram("updateFluxes.comp");
    m_updateWaterHeightProgram = ComputeProgram("updateWaterHeight.comp");
    m_initProgram = ComputeProgram("init.comp");

    m_decompositionProgram = ComputeProgram("decompose.comp");

    m_fftHorizontalProgram = ComputeProgram("CS_FFTHorizontal.comp");
    m_fftVerticalProgram = ComputeProgram("CS_FFTVertical.comp");

    m_fftProgram = ComputeProgram("fft.comp");

    m_bitReverseProgram = ComputeProgram("bitreverse.comp");

    m_normalizedProgram = ComputeProgram("normalize.comp");

    m_debugAlphaProgram = ComputeProgram("debugAlpha.comp");

    m_copyProgram = ComputeProgram("copy.comp");

    m_airywavesProgram = ComputeProgram("airywaves.comp");

    m_recomposeProgram = ComputeProgram("recompose.comp");

    m_updateFluxesProgram.setParametersUniforms(*this);
    m_updateWaterHeightProgram.setParametersUniforms(*this);
    m_decompositionProgram.setParametersUniforms(*this);
}

void ShallowWater::loadTerrainHeightMap(Magnum::Trade::ImageData2D *img,
                                        float scaling, int channels) {
    CORRADE_INTERNAL_ASSERT(channels >= 1 && channels <= 4);
    // CORRADE_INTERNAL_ASSERT(img->format() == Magnum::PixelFormat::R8Unorm);

    Magnum::Vector2i size = img->size();
    const unsigned char *data =
        reinterpret_cast<const unsigned char *>(img->data().data());
    Corrade::Containers::Array<float> scaled{std::size_t(size.x() * size.y())};

    for (std::size_t i = 0; i < scaled.size(); ++i) {
        float h = 0.0f;
        for (int c = 0; c < channels; ++c) {
            h += data[i * channels + c] / 255.0f;
        }
        h /= channels;
        scaled[i] = h * scaling;
    }

    Corrade::Containers::Array<char> floatData{scaled.size() * sizeof(float)};
    std::memcpy(floatData.data(), scaled.data(), floatData.size());

    Magnum::Trade::ImageData2D floatImg{Magnum::PixelStorage{},
                                        Magnum::PixelFormat::R32F, size,
                                        std::move(floatData)};

    m_terrainTexture = Magnum::GL::Texture2D{};
    m_terrainTexture.setStorage(1, Magnum::GL::TextureFormat::R32F, size)
        .setMinificationFilter(Magnum::GL::SamplerFilter::Nearest)
        .setMagnificationFilter(Magnum::GL::SamplerFilter::Nearest)
        .setSubImage(0, {}, floatImg);
}

void ShallowWater::initBump() {
    ping = false;

    m_initProgram.bindStates(&m_stateTexture, &m_stateTexture)
        .bindTerrain(&m_terrainTexture)
        .setFloatUniform("dryEps", dryEps)
        .setIntUniform("init_type", 0)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}

void ShallowWater::initDamBreak() {
    ping = false;

    m_initProgram.bindStates(&m_stateTexture, &m_stateTexture)
        .bindTerrain(&m_terrainTexture)
        .setFloatUniform("dryEps", dryEps)
        .setIntUniform("init_type", 1)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}

void ShallowWater::initTsunami() {
    ping = false;

    m_initProgram.bindStates(&m_stateTexture, &m_stateTexture)
        .bindTerrain(&m_terrainTexture)
        .setFloatUniform("dryEps", dryEps)
        .setIntUniform("init_type", 3)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}