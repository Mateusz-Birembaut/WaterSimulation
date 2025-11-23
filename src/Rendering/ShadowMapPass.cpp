#include <WaterSimulation/Rendering/ShadowMapPass.h>

#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Components/TransformComponent.h>

#include <Magnum/Math/Vector2.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/Shaders/FlatGL.h>


using namespace Magnum;

void WaterSimulation::ShadowMapPass::init(const Magnum::Vector2i& windowSize){
	m_fb = GL::Framebuffer{{{}, windowSize}};

    m_depthTexture = GL::Texture2D{};
    m_depthTexture.setStorage(1, GL::TextureFormat::DepthComponent32F, windowSize)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge);
    
    m_fb.attachTexture(GL::Framebuffer::BufferAttachment::Depth, m_depthTexture, 0);
    m_fb.mapForDraw({});
}

void WaterSimulation::ShadowMapPass::resize(const Magnum::Vector2i& windowSize){
    m_depthTexture = GL::Texture2D{};
    m_depthTexture.setStorage(1, GL::TextureFormat::DepthComponent32F, windowSize)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge);
    
    m_fb.attachTexture(GL::Framebuffer::BufferAttachment::Depth, m_depthTexture, 0);
    m_fb.setViewport({{}, windowSize});
}

void WaterSimulation::ShadowMapPass::render(Registry & registry, const Magnum::Matrix4 & viewMatrix,const Magnum::Matrix4 & projMatrix){
	m_fb.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth).bind();

	const Matrix4 viewProj = projMatrix * viewMatrix;
    
    auto view = registry.view<MeshComponent, TransformComponent>();
    int drawCount = 0;
	for (Entity entity : view) {
        MeshComponent& meshComp = registry.get<MeshComponent>(entity);
        TransformComponent& transformComp = registry.get<TransformComponent>(entity);

		Matrix4 mvp = viewProj * transformComp.globalModel;

		m_depthShader.setMVP(mvp).draw(meshComp.glMesh);
		++drawCount;
	} 
	//Debug{} << "DepthPass: drawn meshes:" << drawCount;
}

