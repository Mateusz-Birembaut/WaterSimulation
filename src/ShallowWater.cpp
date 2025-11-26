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

    int groupx = (nx + 15) / 16;
    int groupy = (ny + 15) / 16;

    if (ping) {
        inputTex = &m_stateTexturePing;
        outputTex = &m_stateTexturePong;
    } else {
        inputTex = &m_stateTexturePong;
        outputTex = &m_stateTexturePing;
    }

    m_updateFluxesProgram.bindStates(inputTex, outputTex)
        .bindTerrain(&m_terrainTexture)
        .run(groupx, groupy);

    // wait for all imageStore to be done before continuing
    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    m_updateWaterHeightProgram.bindStates(outputTex, inputTex)
        .bindTerrain(&m_terrainTexture)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    //m_decompositionProgram.bindDecompose(inputTex, &m_terrainTexture, &m_bulkTexture, &m_surfaceTexture, &m_tempTexture).run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
    

    // ping = !ping;
}

void ShallowWater::compilePrograms(){
    m_updateFluxesProgram =
            ComputeProgram("updateFluxes.comp");
        m_updateWaterHeightProgram =
            ComputeProgram("updateWaterHeight.comp");
        m_initProgram = ComputeProgram("init.comp");

        m_decompositionProgram = ComputeProgram("decompose.comp");

        m_updateFluxesProgram.setParametersUniforms(*this);
        m_updateWaterHeightProgram.setParametersUniforms(*this);
        m_decompositionProgram.setParametersUniforms(*this);
}

void ShallowWater::loadTerrainHeightMap(Magnum::Trade::ImageData2D* img,
                                        float scaling)
{
    CORRADE_INTERNAL_ASSERT(img->format() == Magnum::PixelFormat::R8Unorm);

    Magnum::Vector2i size = img->size();
    const unsigned char* data = reinterpret_cast<const unsigned char*>(img->data().data());
    Corrade::Containers::Array<float> scaled{std::size_t(size.x()*size.y())};

    for(std::size_t i = 0; i < scaled.size(); ++i) {
        float h = data[i] / 255.0f;
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

    m_initProgram.bindStates(&m_stateTexturePing, &m_stateTexturePong)
        .bindTerrain(&m_terrainTexture)
        .setFloatUniform("dryEps", dryEps)
        .setIntUniform("init_type", 0)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}

void ShallowWater::initDamBreak() {
    ping = false;

    m_initProgram.bindStates(&m_stateTexturePing, &m_stateTexturePong)
        .bindTerrain(&m_terrainTexture)
        .setFloatUniform("dryEps", dryEps)
        .setIntUniform("init_type", 1)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}

void ShallowWater::initTsunami(){
    ping = false;

    m_initProgram.bindStates(&m_stateTexturePing, &m_stateTexturePong)
        .bindTerrain(&m_terrainTexture)
        .setFloatUniform("dryEps", dryEps)
        .setIntUniform("init_type", 3)
        .run(groupx, groupy);

    Magnum::GL::Renderer::setMemoryBarrier(
        Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}