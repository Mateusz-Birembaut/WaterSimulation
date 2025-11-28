#include "Magnum/GL/Renderer.h"
#include "Magnum/Trade/Trade.h"
#include <Corrade/Utility/Debug.h>
#include <Magnum/GL/GL.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/Math/Color.h>
#include <Magnum/PixelFormat.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <WaterSimulation/ShallowWater.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>

void ShallowWater::step() {

    Magnum::GL::Texture2D *inputTex;
    Magnum::GL::Texture2D *outputTex;


    runSW(&m_stateTexture);
    
    runDecomposition(&m_stateTexture);

    auto ffth = runFFT(&m_surfaceHeightTexture, &m_surfaceHeightPong, 1, 1.0f );
    
    auto iffth = runFFT(ffth, ffth == &m_surfaceHeightTexture ? &m_surfaceHeightPong : &m_surfaceHeightTexture, -1, 1.0/static_cast<float>(nx+1));

    ping = !ping;
}

void ShallowWater::runDecomposition(Magnum::GL::Texture2D *inputTex){
    // Initialisation
    m_decompositionProgram.bindDecompose(inputTex, &m_terrainTexture, &m_bulkTexture, 
        &m_surfaceHeightTexture, &m_surfaceQxTexture, &m_surfaceQyTexture, 
        &m_tempTexture, &m_tempTexture2)
        .setIntUniform("stage", 0)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    // Diffusion
    const int diffusionIterations = 64;
    Magnum::GL::Texture2D* tempIn = &m_tempTexture2;
    Magnum::GL::Texture2D* tempOut = &m_tempTexture;

    for (int i = 0; i < diffusionIterations; i++) {
        m_decompositionProgram.bindDecompose(inputTex, &m_terrainTexture, &m_bulkTexture, 
            &m_surfaceHeightTexture, &m_surfaceQxTexture, &m_surfaceQyTexture, 
            tempIn, tempOut)
            .setIntUniform("stage", 1)
            .run(groupx, groupy);

        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

        std::swap(tempIn, tempOut);
    }

    //Calcul des valeurs finales
    m_decompositionProgram.bindDecompose(inputTex, &m_terrainTexture, &m_bulkTexture, 
        &m_surfaceHeightTexture, &m_surfaceQxTexture, &m_surfaceQyTexture, 
        tempIn, tempOut)
        .setIntUniform("stage", 2)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}

void ShallowWater::runSW(Magnum::GL::Texture2D *inputTex){
    m_updateFluxesProgram.bindStates(inputTex, &m_tempTexture)
        .bindTerrain(&m_terrainTexture)
        .run(groupx, groupy);

    // wait for all imageStore to be done before continuing
    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    m_updateWaterHeightProgram.bindStates(&m_tempTexture, inputTex)
        .bindTerrain(&m_terrainTexture)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}

Magnum::GL::Texture2D* ShallowWater::runFFT(Magnum::GL::Texture2D* pingTex, Magnum::GL::Texture2D* pongTex, int direction, float normalization) {
    int N = nx + 1;
    int numStages = static_cast<int>(std::log2(N));
    Corrade::Utility::Debug{} << "Number of stages: " << numStages;
        
    Magnum::GL::Texture2D* input = pingTex;
    Magnum::GL::Texture2D* output = pongTex;

    for (int stage = 0; stage < numStages; stage++) {
        int subseqCount = 1 << stage;
        
        m_fftHorizontalProgram
            .bindFFT(input, output)
            .setIntUniform("u_total_count", N)
            .setIntUniform("u_subseq_count", subseqCount)
            .setIntUniform("u_direction", direction)
            .setFloatUniform("u_normalization", normalization)
            .run(N, 1);
        
        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
        
        std::swap(input, output);
    }
    
    for (int stage = 0; stage < numStages; stage++) {
        int subseqCount = 1 << stage;
        
        m_fftVerticalProgram
            .bindFFT(input, output)
            .setIntUniform("u_total_count", N)
            .setIntUniform("u_subseq_count", subseqCount)
            .setIntUniform("u_direction", direction)
            .setFloatUniform("u_normalization", normalization)
            .run(N, 1);
        
        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
        
        std::swap(input, output);
    }
    
    if (output == pongTex) {
        Corrade::Utility::Debug{} << "OUTPUT IS PONG";
    } else if (output == pingTex) {
        Corrade::Utility::Debug{} << "OUTPUT IS PING";
    } else {
        Corrade::Utility::Debug{} << "OUTPUT IS UNKNOWN";
    }

    return (numStages % 2 == 0) ? input : output;
    
}



void ShallowWater::compilePrograms(){
    m_updateFluxesProgram =
            ComputeProgram("updateFluxes.comp");
        m_updateWaterHeightProgram =
            ComputeProgram("updateWaterHeight.comp");
        m_initProgram = ComputeProgram("init.comp");

        m_decompositionProgram = ComputeProgram("decompose.comp");

        m_fftHorizontalProgram = ComputeProgram("CS_FFTHorizontal.comp");
        m_fftVerticalProgram = ComputeProgram("CS_FFTVertical.comp");

        m_updateFluxesProgram.setParametersUniforms(*this);
        m_updateWaterHeightProgram.setParametersUniforms(*this);
        m_decompositionProgram.setParametersUniforms(*this);
}

void ShallowWater::loadTerrainHeightMap(Magnum::Trade::ImageData2D* img,
                                        float scaling, int channels)
{
    CORRADE_INTERNAL_ASSERT(channels >= 1 && channels <= 4);
    //CORRADE_INTERNAL_ASSERT(img->format() == Magnum::PixelFormat::R8Unorm);

    Magnum::Vector2i size = img->size();
    const unsigned char* data = reinterpret_cast<const unsigned char*>(img->data().data());
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

    Magnum::Trade::ImageData2D floatImg{
        Magnum::PixelStorage{},
        Magnum::PixelFormat::R32F,
        size,
        std::move(floatData)
    };

    m_terrainTexture = Magnum::GL::Texture2D{};
    m_terrainTexture
        .setStorage(1, Magnum::GL::TextureFormat::R32F, size)
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

void ShallowWater::initTsunami(){
    ping = false;

    m_initProgram.bindStates(&m_stateTexture, &m_stateTexture)
        .bindTerrain(&m_terrainTexture)
        .setFloatUniform("dryEps", dryEps)
        .setIntUniform("init_type", 3)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}