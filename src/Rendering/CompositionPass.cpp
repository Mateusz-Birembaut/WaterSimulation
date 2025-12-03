
#include <WaterSimulation/Rendering/CompositionPass.h>
#include <WaterSimulation/Components/LightComponent.h>
#include <WaterSimulation/Components/MaterialComponent.h>
#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Components/ShaderComponent.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/WaterComponent.h>
#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Rendering/CustomShader/FullscreenTextureShader.h>

#include <Magnum/GL/Attribute.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Vector2.h>

#include <vector>

using namespace Magnum;
using namespace WaterSimulation;

void CompositionPass::init(const Magnum::Vector2i& windowSize) {
	m_fullscreenTriangle = createFullscreenTriangle();
	m_colorTexture = GL::Texture2D{};
	m_colorTexture.setStorage(1, GL::TextureFormat::RGBA8, windowSize)
		.setMinificationFilter(GL::SamplerFilter::Linear)
		.setMagnificationFilter(GL::SamplerFilter::Linear)
		.setWrapping(GL::SamplerWrapping::ClampToEdge);

	m_fb = GL::Framebuffer{{{}, windowSize}};
	m_fb.attachTexture(GL::Framebuffer::ColorAttachment{0}, m_colorTexture, 0);
	m_fb.setViewport({{}, windowSize});
	m_fb.mapForDraw({{0, GL::Framebuffer::ColorAttachment{0}}});
}

void CompositionPass::resize(const Magnum::Vector2i& windowSize) {
	m_colorTexture = GL::Texture2D{};
	m_colorTexture.setStorage(1, GL::TextureFormat::RGBA8, windowSize)
		.setMinificationFilter(GL::SamplerFilter::Linear)
		.setMagnificationFilter(GL::SamplerFilter::Linear)
		.setWrapping(GL::SamplerWrapping::ClampToEdge);

	m_fb.attachTexture(GL::Framebuffer::ColorAttachment{0}, m_colorTexture, 0);
	m_fb.setViewport({{}, windowSize});
}

void CompositionPass::render(
	GL::Texture2D& opaqueColor,
	GL::Texture2D& caustics,
	GL::Texture2D& godrays,
	GL::Texture2D& opaqueDepth,
	Registry& registry,
	const Matrix4& viewMatrix,
	const Matrix4& projMatrix) {
	const Matrix4 viewProj = projMatrix * viewMatrix;

	GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
	GL::Renderer::disable(GL::Renderer::Feature::Blending);

	m_fb.bind();
	m_fb.clear(GL::FramebufferClear::Color);

	auto drawFullscreen = [&](GL::Texture2D& texture) {
		m_fullscreenShader
		    .bindDepthTexture(texture)
		    .setNear(0.0f)
		    .setFar(1.0f)
		    .draw(m_fullscreenTriangle);
	};

	// Base layer: opaque geometry
	drawFullscreen(opaqueColor);

	// Additive overlays for caustics and godrays
	GL::Renderer::enable(GL::Renderer::Feature::Blending);
	GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One,
	    GL::Renderer::BlendFunction::One);
	drawFullscreen(caustics);
	drawFullscreen(godrays);

	// Attach opaque depth texture so water depth test uses scene depth
	m_fb.attachTexture(GL::Framebuffer::BufferAttachment::Depth, opaqueDepth, 0);

	// Render water meshes with regular alpha blending and depth testing
	GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
	    GL::Renderer::BlendFunction::OneMinusSourceAlpha);

	auto waterView = registry.view<MeshComponent,
	    TransformComponent,
	    WaterComponent,
	    MaterialComponent,
	    ShaderComponent>();

	auto lightView = registry.view<LightComponent>();
	std::vector<LightComponent> lights;
	for (Entity lightEntity : lightView) {
		lights.push_back(lightView.get<LightComponent>(lightEntity));
	}

	GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
	// don't write depth when drawing transparent water
	GL::Renderer::setDepthMask(false);
	GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);

	for (Entity entity : waterView) {
		auto& meshComp = waterView.get<MeshComponent>(entity);
		auto& transform = waterView.get<TransformComponent>(entity);
		auto& material = waterView.get<MaterialComponent>(entity);
		auto& shaderComp = waterView.get<ShaderComponent>(entity);

		if (!shaderComp.shaderPtr) {
			continue;
		}

		const Matrix4 mvp = viewProj * transform.globalModel;
		shaderComp.shaderPtr->draw(
		    meshComp.glMesh,
		    mvp,
		    material,
		    Matrix4{Math::IdentityInit},
		    opaqueColor,
		    lights);
	}


	// restore depth write and face culling state
	GL::Renderer::setDepthMask(true);
	GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
	GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
	GL::Renderer::disable(GL::Renderer::Feature::Blending);

	// Present the composed color buffer to the default framebuffer
	GL::defaultFramebuffer.bind();
	m_fullscreenShader
	    .bindDepthTexture(m_colorTexture)
	    .setNear(0.0f)
	    .setFar(1.0f)
	    .draw(m_fullscreenTriangle);

	GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
}

Magnum::GL::Mesh CompositionPass::createFullscreenTriangle() {
	struct Vertex {
		Vector2 position;
		Vector2 uv;
	};

	constexpr Vertex vertices[]{
	    {{-1.0f, -1.0f}, {0.0f, 0.0f}},
	    {{3.0f, -1.0f}, {2.0f, 0.0f}},
	    {{-1.0f, 3.0f}, {0.0f, 2.0f}}};

	GL::Buffer vertexBuffer;
	vertexBuffer.setData(vertices);

	GL::Mesh mesh;
	mesh.setPrimitive(GL::MeshPrimitive::Triangles)
	    .setCount(3)
	    .addVertexBuffer(
		std::move(vertexBuffer),
		0,
		GL::Attribute<0, Vector2>{},
		GL::Attribute<1, Vector2>{});

	return mesh;
}

