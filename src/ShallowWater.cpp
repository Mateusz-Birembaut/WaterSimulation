#include "Magnum/GL/Renderer.h"
#include "Magnum/Trade/Trade.h"
#include <Corrade/Utility/Debug.h>
#include <Magnum/GL/GL.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/Math/Color.h>
#include <Magnum/PixelFormat.h>
#include <WaterSimulation/ShallowWater.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <Magnum/GL/Renderer.h>
#include <cstdint>


void ShallowWater::step() {

    Magnum::GL::Texture2D * inputTex;
    Magnum::GL::Texture2D * outputTex;

    int groupx = (nx+15)/16;
    int groupy = (ny+15)/16;
    
    if(ping){
        inputTex = &m_stateTexturePing;
        outputTex = &m_stateTexturePong;
    }else{
        inputTex = &m_stateTexturePong;
        outputTex = &m_stateTexturePing;
    }

    m_updateFluxesProgram.bindStates(inputTex, outputTex).run(groupx,groupy);

    // wait for all imageStore to be done before continuing
    Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    m_updateWaterHeightProgram.bindStates(outputTex, inputTex).run(groupx,groupy);

    Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);

    //ping = !ping;

}





void ShallowWater::loadTerrainHeightMap(Magnum::Trade::ImageData2D * img, float scaling) {
    m_terrainTexture = Magnum::GL::Texture2D{};
    m_terrainTexture.setStorage(1, Magnum::GL::textureFormat(img->format()), img->size())
                    .setSubImage(0, {}, *img);

    m_updateFluxesProgram.bindTerrain(&m_terrainTexture);
}

void ShallowWater::initBump() {
    ping = false;

    m_initProgram.bindStates(&m_stateTexturePing, &m_stateTexturePong)
        .setParametersUniforms(*this)
        .setIntUniform("init_type", 0)
        .run(groupx, groupy);

        Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}

void ShallowWater::initDamBreak() {
    ping = false;

    m_initProgram.bindStates(&m_stateTexturePing, &m_stateTexturePong)
        .setParametersUniforms(*this)
        .setIntUniform("init_type", 1)
        .run(groupx, groupy);

        Magnum::GL::Renderer::setMemoryBarrier(Magnum::GL::Renderer::MemoryBarrier::ShaderImageAccess);
}