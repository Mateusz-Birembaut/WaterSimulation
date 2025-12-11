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

    if (!airyWavesEnabled) {
        m_updateFluxesProgram.bindStates(&m_stateTexture, &m_tempTexture)
            .bindTerrain(&m_terrainTexture)
            .run(groupx, groupy);

        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

        m_updateHeightSimpleProgram.bindStates(&m_tempTexture, &m_stateTexture)
            .run(groupx, groupy);

        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
            
        ping = !ping;
        return;
    }

    m_CopyRGBAProgram.bindCopyRGBA(&m_stateTexture, &m_prevStateTexture).run(groupx, groupy);
    Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    // Decompisition Bulk (shallow) + surface (airy) 

    runDecomposition(&m_stateTexture);
    // Shallow water pass

    runSW(&m_bulkTexture, &m_tempTexture);

    m_CopyRGBAProgram.bindCopyRGBA(&m_tempTexture, &m_visBulkUpdated).run(groupx, groupy);
    Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    float N = nx+1;
    Magnum::GL::Texture2D * fftHeight; Magnum::GL::Texture2D * ifftHeight;
    Magnum::GL::Texture2D * fftQx; Magnum::GL::Texture2D * ifftQx;
    Magnum::GL::Texture2D * fftQy; Magnum::GL::Texture2D * ifftQy;
    // Airy Waves
    {   
        
        // FFT pass
        fftHeight = runFFT(&m_surfaceHeightTexture, &m_surfaceHeightPong, 1);
        
        fftQx = runFFT(&m_surfaceQxTexture, &m_surfaceQxPong, 1);
        fftQy = runFFT(&m_surfaceQyTexture, &m_surfaceQyPong, 1);

        m_copyProgram.bindCopy(fftHeight, &m_visFFTHeight).run(groupx, groupy);
        m_copyProgram.bindCopy(fftQx, &m_visFFTQx).run(groupx, groupy);
        m_copyProgram.bindCopy(fftQy, &m_visFFTQy).run(groupx, groupy);
        Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

        m_airywavesProgram.bindAiry(&m_tempTexture, fftHeight, fftQx, fftQy)
            .setFloatUniform("dt", dt)
            .setFloatUniform("gravity", gravity)
            .setFloatUniform("dx", dx)
            .setFloatUniform("hBar", airyHBar)
            .setIntUniform("N", static_cast<int>(N)).run(groupx, groupy);

        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

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

        m_copyProgram.bindCopy(ifftQx, &m_visIFFTQx).run(groupx, groupy);
        m_copyProgram.bindCopy(ifftQy, &m_visIFFTQy).run(groupx, groupy);
        Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

        m_fftOutput = ifftQx;
        m_ifftOutput = ifftQy;


        // IFFT pass, m_fftOutput can be surface Height pong, add an if 
        ifftHeight = runFFT(fftHeight, &m_surfaceHeightPong, -1); // Should just copy the original before fft instead of this 

        m_normalizedProgram.bindReadWrite(ifftHeight, Magnum::GL::ImageFormat::RG32F).setFloatUniform("norm", 1.0f/(N*N)).run(groupx, groupy);

        m_copyProgram.bindCopy(ifftHeight, &m_visIFFTHeight).run(groupx, groupy);
        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

        Magnum::GL::Renderer::setMemoryBarrier(
            Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
    }

    
    {//Surface Transportt

        m_transportSurfaceFlowProgram.bindTransportSurfaceFlow(ifftQx, ifftQy, &m_tempTexture, &m_bulkTexture, &m_tempTexture2)
            .setFloatUniform("dt", dt)
            .setFloatUniform("dx", dx)
            .setFloatUniform("transportGamma", transportGamma)
            .run(groupx, groupy);

        Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
        m_CopyRGBAProgram.bindCopyRGBA(&m_tempTexture2, &m_visTransportedFlow).run(groupx, groupy);
        Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

        m_transportSurfaceHeightProgram.bindTransportSurfaceHeight(ifftHeight, &m_bulkTexture, &m_tempTexture2)
            .setFloatUniform("dt", dt)
            .setFloatUniform("dx", dx)
            .setFloatUniform("transportGamma", transportGamma)
            .run(groupx, groupy);

        Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
        m_CopyRGBAProgram.bindCopyRGBA(&m_tempTexture2, &m_visTransportedHeight).run(groupx, groupy);
        Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

        m_semiLagrangianAdvectionProgram.bindAdvection(&m_tempTexture2, &m_tempTexture3, &m_tempTexture)
            .setFloatUniform("dt", dt)
            .setFloatUniform("dx", dx)
            .run(groupx, groupy);

        Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
        m_CopyRGBAProgram.bindCopyRGBA(&m_tempTexture3, &m_visAdvectedHeight).run(groupx, groupy);
        Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    }

    m_updateWaterHeightProgram.bindUpdateHeight(&m_tempTexture, &m_tempTexture3, &m_prevStateTexture, &m_stateTexture)
        .setFloatUniform("dt", dt)
        .setFloatUniform("dx", dx)
        .setFloatUniform("dryEps", dryEps)
        .run(groupx, groupy);
   

    
    /* m_recomposeProgram.bindRecompose(&m_stateTexture, &m_tempTexture, &m_surfaceHeightTexture, 
                                      &m_surfaceQxTexture, &m_surfaceQyTexture)
        .run(groupx, groupy); */

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

}

void ShallowWater::runDecomposition(Magnum::GL::Texture2D *inputTex) {
    // Initialisation
    m_decompositionProgram
        .bindDecompose(inputTex, &m_terrainTexture, &m_bulkTexture,
                       &m_surfaceHeightTexture, &m_surfaceQxTexture,
                       &m_surfaceQyTexture, &m_tempTexture, &m_tempTexture2)
        .setIntUniform("stage", 0)
        .setFloatUniform("decompositionD", decompositionD)
        .setFloatUniform("dryEps", dryEps)
        .setFloatUniform("dx", dx)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    Magnum::GL::Texture2D *tempIn = &m_tempTexture2;
    Magnum::GL::Texture2D *tempOut = &m_tempTexture;

    for (int i = 0; i < diffusionIterations; i++) {
        m_decompositionProgram
            .bindDecompose(inputTex, &m_terrainTexture, &m_bulkTexture,
                           &m_surfaceHeightTexture, &m_surfaceQxTexture,
                           &m_surfaceQyTexture, tempIn, tempOut)
            .setIntUniform("stage", 1)
            .setFloatUniform("decompositionD", decompositionD)
            .setFloatUniform("dryEps", dryEps)
            .setFloatUniform("dx", dx)
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
        .setFloatUniform("decompositionD", decompositionD)
        .setFloatUniform("dryEps", dryEps)
        .setFloatUniform("dx", dx)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}

void ShallowWater::runSW(Magnum::GL::Texture2D *inputTex, Magnum::GL::Texture2D *outputTex) {
    m_updateFluxesProgram.bindStates(inputTex, outputTex)
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
    m_updateHeightSimpleProgram = ComputeProgram("updateHeightSimple.comp");
    m_initProgram = ComputeProgram("init.comp");

    m_decompositionProgram = ComputeProgram("decompose.comp");

    m_fftHorizontalProgram = ComputeProgram("CS_FFTHorizontal.comp");
    m_fftVerticalProgram = ComputeProgram("CS_FFTVertical.comp");

    m_fftProgram = ComputeProgram("fft.comp");

    m_bitReverseProgram = ComputeProgram("bitreverse.comp");

    m_normalizedProgram = ComputeProgram("normalize.comp");

    m_debugAlphaProgram = ComputeProgram("debugAlpha.comp");

    m_copyProgram = ComputeProgram("copy.comp");
    m_CopyRGBAProgram = ComputeProgram("copyRGBA.comp");

    m_clearProgram = ComputeProgram("clear.comp");
    m_clearRGProgram = ComputeProgram("clearRG.comp");

        m_debugAlphaProgram = ComputeProgram("debugAlpha.comp");
        m_disturbanceProgram = ComputeProgram("disturbance.comp");
    m_airywavesProgram = ComputeProgram("airywaves.comp");

    m_recomposeProgram = ComputeProgram("recompose.comp");

    m_transportSurfaceFlowProgram = ComputeProgram("transportSurfaceFlow.comp");

    m_transportSurfaceHeightProgram = ComputeProgram("transportSurfaceHeight.comp");

    m_semiLagrangianAdvectionProgram = ComputeProgram("advectSurface.comp");

    m_createWaterProgram = ComputeProgram("createWater.comp");

    m_updateFluxesProgram.setParametersUniforms(*this);
    m_updateWaterHeightProgram.setParametersUniforms(*this);
    m_updateHeightSimpleProgram.setParametersUniforms(*this);
    m_decompositionProgram.setParametersUniforms(*this);

    m_airywavesProgram.setParametersUniforms(*this);
    m_recomposeProgram.setParametersUniforms(*this);
    m_transportSurfaceFlowProgram.setParametersUniforms(*this);
    m_transportSurfaceHeightProgram.setParametersUniforms(*this);
    m_semiLagrangianAdvectionProgram.setParametersUniforms(*this);
}

void ShallowWater::clearAllTextures() {
    m_clearProgram.bindClear(&m_stateTexture).run(groupx, groupy);
    m_clearProgram.bindClear(&m_stateTexturePong).run(groupx, groupy);
    m_clearProgram.bindClear(&m_prevStateTexture).run(groupx, groupy);
    m_clearProgram.bindClear(&m_bulkTexture).run(groupx, groupy);
    m_clearProgram.bindClear(&m_surfaceTexture).run(groupx, groupy);
    m_clearProgram.bindClear(&m_tempTexture).run(groupx, groupy);
    m_clearProgram.bindClear(&m_tempTexture2).run(groupx, groupy);
    m_clearProgram.bindClear(&m_tempTexture3).run(groupx, groupy);
    
    m_clearRGProgram.bindClear(&m_surfaceHeightTexture, Magnum::GL::ImageFormat::RG32F).run(groupx, groupy);
    m_clearRGProgram.bindClear(&m_surfaceQxTexture, Magnum::GL::ImageFormat::RG32F).run(groupx, groupy);
    m_clearRGProgram.bindClear(&m_surfaceQyTexture, Magnum::GL::ImageFormat::RG32F).run(groupx, groupy);
    m_clearRGProgram.bindClear(&m_surfaceHeightPing, Magnum::GL::ImageFormat::RG32F).run(groupx, groupy);
    m_clearRGProgram.bindClear(&m_surfaceHeightPong, Magnum::GL::ImageFormat::RG32F).run(groupx, groupy);
    m_clearRGProgram.bindClear(&m_surfaceQxPong, Magnum::GL::ImageFormat::RG32F).run(groupx, groupy);
    m_clearRGProgram.bindClear(&m_surfaceQyPong, Magnum::GL::ImageFormat::RG32F).run(groupx, groupy);
    
    m_clearProgram.bindClear(&m_visBulkUpdated).run(groupx, groupy);
    m_clearRGProgram.bindClear(&m_visFFTHeight, Magnum::GL::ImageFormat::RG32F).run(groupx, groupy);
    m_clearRGProgram.bindClear(&m_visFFTQx, Magnum::GL::ImageFormat::RG32F).run(groupx, groupy);
    m_clearRGProgram.bindClear(&m_visFFTQy, Magnum::GL::ImageFormat::RG32F).run(groupx, groupy);
    m_clearRGProgram.bindClear(&m_visIFFTHeight, Magnum::GL::ImageFormat::RG32F).run(groupx, groupy);
    m_clearRGProgram.bindClear(&m_visIFFTQx, Magnum::GL::ImageFormat::RG32F).run(groupx, groupy);
    m_clearRGProgram.bindClear(&m_visIFFTQy, Magnum::GL::ImageFormat::RG32F).run(groupx, groupy);
    m_clearProgram.bindClear(&m_visTransportedFlow).run(groupx, groupy);
    m_clearProgram.bindClear(&m_visTransportedHeight).run(groupx, groupy);
    m_clearProgram.bindClear(&m_visAdvectedHeight).run(groupx, groupy);
    
    m_fftOutput = nullptr;
    m_ifftOutput = nullptr;
    
    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
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
    
    clearAllTextures();

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
    
    clearAllTextures();

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
    
    clearAllTextures();

    m_initProgram.bindStates(&m_stateTexture, &m_stateTexture)
        .bindTerrain(&m_terrainTexture)
        .setFloatUniform("dryEps", dryEps)
        .setIntUniform("init_type", 3)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}

void ShallowWater::applyDisturbances(const std::vector<Disturbance>& disturbances) {
    if (disturbances.empty())
        return;

    // Upload disturbances to GPU buffer
    m_disturbanceBuffer.setData(
        Corrade::Containers::ArrayView<const Disturbance>{disturbances.data(), disturbances.size()},
        Magnum::GL::BufferUsage::DynamicDraw
    );

    // Bind the buffer to binding point 1
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_disturbanceBuffer.id());

    // Bind the state texture for read-write access
    m_stateTexture.bindImage(0, 0, Magnum::GL::ImageAccess::ReadWrite,
                             Magnum::GL::ImageFormat::RGBA32F);

    // Set uniform for disturbance count
    m_disturbanceProgram.setIntUniform("uDisturbanceCount", int(disturbances.size()));

    // Dispatch compute shader (one work group per disturbance)
    m_disturbanceProgram.dispatchCompute({unsigned(disturbances.size()), 1, 1});

    // Memory barrier to ensure writes are visible
    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}
void ShallowWater::initEmpty() {
    ping = false;
    
    clearAllTextures();

    m_initProgram.bindStates(&m_stateTexture, &m_stateTexture)
        .bindTerrain(&m_terrainTexture)
        .setFloatUniform("dryEps", dryEps)
        .setIntUniform("init_type", 4)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}

void ShallowWater::createWater(float x, float y, float radius, float quantity){

    m_createWaterProgram.bindReadWrite(&m_stateTexture)
        .bindTerrain(&m_terrainTexture)
        .setFloatUniform("posx", x)
        .setFloatUniform("posy", y)
        .setFloatUniform("radius", radius)
        .setFloatUniform("quantity", quantity)
        .setIntUniform("mode", 0)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}



void ShallowWater::sendWaveWall(int side, float width, float quantity){
    m_createWaterProgram.bindReadWrite(&m_stateTexture)
        .bindTerrain(&m_terrainTexture)
        .setIntUniform("mode", 2)
        .setIntUniform("wallSide", side)
        .setFloatUniform("wallWidth", width)
        .setFloatUniform("quantity", quantity)
        .setFloatUniform("gravity", gravity)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}



