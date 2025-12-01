#include <WaterSimulation/Rendering/OpaquePass.h>

#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/MaterialComponent.h>
#include <WaterSimulation/Components/ShaderComponent.h>

#include <Magnum/Math/Vector2.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/Shaders/FlatGL.h>


using namespace Magnum;

void WaterSimulation::OpaquePass::recreateTextures(const Magnum::Vector2i& windowSize){
    m_depthTexture = GL::Texture2D{};
    m_depthTexture.setStorage(1, GL::TextureFormat::DepthComponent32F, windowSize)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge);

    m_colorTexture = GL::Texture2D{};
    m_colorTexture.setStorage(1, GL::TextureFormat::RGBA8, windowSize)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge);
}

void WaterSimulation::OpaquePass::init(const Magnum::Vector2i& windowSize){
	m_fb = GL::Framebuffer{{{}, windowSize}};

    recreateTextures(windowSize);
    
    m_fb.attachTexture(GL::Framebuffer::BufferAttachment::Depth, m_depthTexture, 0);
    m_fb.attachTexture(GL::Framebuffer::ColorAttachment{0}, m_colorTexture, 0);

    m_fb.mapForDraw({{0, GL::Framebuffer::ColorAttachment{0}}});
}

void WaterSimulation::OpaquePass::resize(const Magnum::Vector2i& windowSize){

    recreateTextures(windowSize);

    m_fb.attachTexture(GL::Framebuffer::BufferAttachment::Depth, m_depthTexture, 0);
    m_fb.attachTexture(GL::Framebuffer::ColorAttachment{0}, m_colorTexture, 0);
    
    m_fb.setViewport({{}, windowSize});
}

void WaterSimulation::OpaquePass::render(Registry & registry, const Magnum::Matrix4 & viewMatrix,const Magnum::Matrix4 & projMatrix){
	m_fb.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth).bind();

	const Matrix4 viewProj = projMatrix * viewMatrix;
    
    auto view = registry.view<MeshComponent, TransformComponent>();

    Magnum::GL::Renderer::setPolygonMode(Magnum::GL::Renderer::PolygonMode::Line);

	for (Entity entity : view) {
        MeshComponent& meshComp = registry.get<MeshComponent>(entity);
        TransformComponent& transformComp = registry.get<TransformComponent>(entity);
        MaterialComponent& materialComp = registry.get<MaterialComponent>(entity);

		Matrix4 mvp = viewProj * transformComp.globalModel;

        if(registry.has<ShaderComponent>(entity)) {
            auto& shaderComp = registry.get<ShaderComponent>(entity);
            shaderComp.shaderPtr.get()->draw(meshComp.glMesh, mvp, materialComp, {}); // TODO : ajouter lights
        }else { // si pas defini de shader component on en a un de base TODO : le rendre mieux ? 
            m_opaqueShader.setMVP(mvp);
            if(materialComp.albedo) {
                m_opaqueShader.bindAlbedoTexture(*materialComp.albedo);
            }
            m_opaqueShader.draw(meshComp.glMesh);
        }

	} 

    Magnum::GL::Renderer::setPolygonMode(Magnum::GL::Renderer::PolygonMode::Fill);
}

