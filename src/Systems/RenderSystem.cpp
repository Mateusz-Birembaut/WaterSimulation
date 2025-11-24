#include <WaterSimulation/Systems/RenderSystem.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Camera.h>

#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/DefaultFramebuffer.h>


using namespace Magnum;
using namespace Math::Literals;

namespace WaterSimulation {

void RenderSystem::render(Registry& registry,
                          Camera& cam) 
{
    auto viewMatrix = cam.viewMatrix();
    auto projectionMatrix = cam.projectionMatrix();

    //m_shadowMapPass.render(registry, )

    m_opaquePass.render(registry, viewMatrix, projectionMatrix);

    if(m_renderDepthOnly){
        drawFullscreenTexture(m_opaquePass.getDepthTexture(), cam.near(), cam.far());
        return;
    }

    drawFullscreenTexture(m_opaquePass.getColorTexture(), cam.near(), cam.far());
    return;
}


void RenderSystem::drawFullscreenTexture(Magnum::GL::Texture2D& texture, float near, float far) {
    GL::defaultFramebuffer.bind();
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    m_fullScreenTextureShader
        .bindDepthTexture(texture)
        .setNear(near)
        .setFar(far)
        .draw(m_fullscreenTriangle);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
}

} // namespace WaterSimulation