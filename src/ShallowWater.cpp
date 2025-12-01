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

    //m_fftOutput = &m_surfaceHeightTexture;
    //m_ifftOutput = &m_surfaceHeightTexture;

    Corrade::Utility::Debug{} << "running fft pass";
    m_fftOutput = runFFT(&m_surfaceHeightTexture, &m_surfaceHeightPong, 1, 1.0f );
    Corrade::Utility::Debug{} << "FFT output : " << m_fftOutput;

    

    Corrade::Utility::Debug{} << "\n running ifft pass";
    m_ifftOutput = runFFT(m_fftOutput, &m_surfaceHeightPing, -1, 1.0/static_cast<float>(nx+1));
    Corrade::Utility::Debug{} << "IFFT output : " << m_ifftOutput;
    //
    m_debugAlphaProgram.bindReadWrite(m_ifftOutput).run(groupx, groupy); // removing this has no real effect, this is not the issue 

    Corrade::Utility::Debug{} << "-----------------------";

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

    // Compute final values
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

    // Wait for all imageStore to be done before continuing
    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    m_updateWaterHeightProgram.bindStates(&m_tempTexture, inputTex)
        .bindTerrain(&m_terrainTexture)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}

Magnum::GL::Texture2D* ShallowWater::runFFT(Magnum::GL::Texture2D* pingTex, Magnum::GL::Texture2D* pongTex, int direction, float normalization) {

    bool temp_as_input = false;

    int N = nx + 1;
    int numStages = static_cast<int>(std::log2(N));
    Corrade::Utility::Debug{} << "N : "<< N << ", Number of stages: " << numStages;
    
    Corrade::Utility::Debug{} << "Horizontal";
    Corrade::Utility::Debug{} << "Input : " << pingTex;
    Corrade::Utility::Debug{} << "Output : " << pongTex;
    
    Magnum::GL::Texture2D* input = pingTex;
    Magnum::GL::Texture2D* output = pongTex;

    for (int p = 1; p < N; p <<= 1) {

        if(temp_as_input){
            m_fftHorizontalProgram.bindFFT(pongTex, pingTex);
        }else{
            m_fftHorizontalProgram.bindFFT(pingTex, pongTex);
        }

        m_fftHorizontalProgram.setIntUniform("u_total_count", N)
            .setIntUniform("u_subseq_count", p)
            .setIntUniform("u_direction", direction)
            .setFloatUniform("u_normalization", normalization)
            .run(N,1);
        
        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
        
        //std::swap(input, output);

        temp_as_input = !temp_as_input;
    }
    Corrade::Utility::Debug{} << "Vertical";
    Corrade::Utility::Debug{} << "Input : " << input;
    Corrade::Utility::Debug{} << "Output : " << output;

    //return (numStages % 2 == 0) ? input : output;

    for (int p = 1; p < N; p <<= 1) {

        if(temp_as_input){
            m_fftVerticalProgram.bindFFT(pongTex, pingTex);
            //Corrade::Utility::Debug{} << "output is ping ";
        }else{
            m_fftVerticalProgram.bindFFT(pingTex, pongTex);
            //Corrade::Utility::Debug{} << "output is pong";
        }
        
        m_fftVerticalProgram
            .setIntUniform("u_total_count", N)
            .setIntUniform("u_subseq_count", p)
            .setIntUniform("u_direction", direction)
            .setFloatUniform("u_normalization", normalization)
            .run(N,1);
        
        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

        temp_as_input = !temp_as_input;
        
        
        /* Corrade::Utility::Debug{} << "Before last swap";
        Corrade::Utility::Debug{} << "Input : " << input;
        Corrade::Utility::Debug{} << "Output : " << output; */
        //std::swap(input, output);
        /* Corrade::Utility::Debug{} << "After last swap";
        Corrade::Utility::Debug{} << "Input : " << input;
        Corrade::Utility::Debug{} << "Output : " << output; */

        //Corrade::Utility::Debug{} << "use temp as input ? " << temp_as_input;
        //Corrade::Utility::Debug{} << temp_as_input;
    }
    
    //return (numStages % 2 == 1) ? input : output;
    Magnum::GL::Texture2D* lastOutput = temp_as_input ? pongTex : pingTex;
    return lastOutput;
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

        m_debugAlphaProgram = ComputeProgram("debugAlpha.comp");

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