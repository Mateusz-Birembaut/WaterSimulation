#include <WaterSimulation/Rendering/ShadowMapPass.h>

#include <WaterSimulation/ECS.h>

#include <WaterSimulation/Components/DirectionalLightComponent.h>
#include <WaterSimulation/Components/MaterialComponent.h>
#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Components/ShadowCasterComponent.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/WaterComponent.h>

#include <WaterSimulation/Camera.h>

#include <Magnum/Math/Angle.h> 
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/GL/Renderer.h>

using namespace Magnum;

void WaterSimulation::ShadowMapPass::recreateTextures(const Magnum::Vector2i& windowSize){
    m_depthTexture = GL::Texture2D{};
    m_depthTexture.setStorage(1, GL::TextureFormat::DepthComponent32F, windowSize)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge);

    m_colorTexture = GL::Texture2D{};
    m_colorTexture.setStorage(1, GL::TextureFormat::RGBA32F, windowSize)
        .setMinificationFilter(GL::SamplerFilter::Nearest)
        .setMagnificationFilter(GL::SamplerFilter::Nearest)
        .setWrapping(GL::SamplerWrapping::ClampToEdge);
}

void WaterSimulation::ShadowMapPass::init(const Magnum::Vector2i& windowSize){
	m_fb = GL::Framebuffer{{{}, windowSize}};

    recreateTextures(windowSize);
    
    m_fb.attachTexture(GL::Framebuffer::BufferAttachment::Depth, m_depthTexture, 0);
    m_fb.attachTexture(GL::Framebuffer::ColorAttachment{0}, m_colorTexture, 0);

    m_fb.mapForDraw({{0, GL::Framebuffer::ColorAttachment{0}}});
}

void WaterSimulation::ShadowMapPass::resize(const Magnum::Vector2i& windowSize){

    recreateTextures(windowSize);

    m_fb.attachTexture(GL::Framebuffer::BufferAttachment::Depth, m_depthTexture, 0);
    m_fb.attachTexture(GL::Framebuffer::ColorAttachment{0}, m_colorTexture, 0);
    
    m_fb.setViewport({{}, windowSize});
}


void WaterSimulation::ShadowMapPass::renderDepth(Registry& registry, const Matrix4& viewProj) {
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); 
    glDepthMask(GL_TRUE);

	auto view = registry.view<MeshComponent, TransformComponent>();
	for (Entity entity : view) {

		if (registry.has<WaterComponent>(entity)) continue; // draw pas l'eau ici 

		MeshComponent& meshComp = registry.get<MeshComponent>(entity);
		TransformComponent& transformComp = registry.get<TransformComponent>(entity);
		MaterialComponent& materialComp = registry.get<MaterialComponent>(entity);

		Matrix4 mvp = viewProj * transformComp.globalModel;

		m_depthShader.setMVP(mvp);
		if (materialComp.heightmap) {
			m_depthShader.bindHeightMapTexture(*materialComp.heightmap);
			m_depthShader.setHasHeightMap(true);
		} else {
			m_depthShader.setHasHeightMap(false);
		}
		m_depthShader.draw(meshComp.glMesh);
	}
}

void WaterSimulation::ShadowMapPass::rendeWaterPosTexture(Registry& registry, const Matrix4& viewProj) {
	glDepthMask(GL_FALSE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	auto view = registry.view<MeshComponent, TransformComponent, WaterComponent>();
	if (view.begin() != view.end()) {
		auto entity = *view.begin();
		MeshComponent& meshComp = registry.get<MeshComponent>(entity);
		TransformComponent& transformComp = registry.get<TransformComponent>(entity);
		MaterialComponent& materialComp = registry.get<MaterialComponent>(entity);

		m_waterPosShader.bindHeightMapTexture(*materialComp.heightmap)
						.setLightVP(viewProj)
						.setModel(transformComp.globalModel)
						.draw(meshComp.glMesh);

	}
}

void WaterSimulation::ShadowMapPass::render(Registry& registry, Camera& mainCamera, Matrix4& lightViewProj) {
    m_fb.bind();
	using namespace Math::Literals;
	Magnum::GL::Renderer::setClearColor(Magnum::Color4{0.0f, 0.0f, 0.0f, 0.0f}); // si on met pas alpha a 0 on avait un petit soucis dans la passe de caustic
    m_fb.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);


	renderDepth(registry, lightViewProj);
	rendeWaterPosTexture(registry, lightViewProj);
		
	
	glDepthMask(GL_TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	Magnum::GL::Renderer::setClearColor(0xa5c9ea_rgbf);

}