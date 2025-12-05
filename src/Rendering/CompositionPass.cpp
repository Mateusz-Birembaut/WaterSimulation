
#include <WaterSimulation/Rendering/CompositionPass.h>
#include <WaterSimulation/Components/LightComponent.h>
#include <WaterSimulation/Components/MaterialComponent.h>
#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Components/ShaderComponent.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/WaterComponent.h>
#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Rendering/CustomShader/FullscreenTextureShader.h>
#include <WaterSimulation/Rendering/HeightmapReadback.h>

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
#include <Magnum/Math/Color.h>

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
		HeightmapReadback * heightmapReadback,
		const Magnum::Vector3& cameraPosition,
		Magnum::GL::Texture2D& opaqueColor,
		Magnum::GL::Texture2D& caustics,
		Magnum::GL::Texture2D& godrays,
		Magnum::GL::Texture2D& opaqueDepth,
		Registry& registry,
		const Magnum::Matrix4& viewMatrix,
		const Magnum::Matrix4& projMatrix) {
	const Matrix4 viewProj = projMatrix * viewMatrix;
	const Color3 underwaterFogColor{0.0f, 0.35f, 0.55f};

	GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
	GL::Renderer::disable(GL::Renderer::Feature::Blending);

	m_fb.bind();
	m_fb.clear(GL::FramebufferClear::Color);

	const bool isUnderwater = isCameraUnderwater(registry, cameraPosition, heightmapReadback);


	auto drawFullscreen = [&](GL::Texture2D& texture) {
		m_fullscreenShader
		    .bindDepthTexture(texture)
		    .setNear(0.0f)
		    .setFar(1.0f)
		    .draw(m_fullscreenTriangle);
	};

	// Ensure no fog is baked into intermediate passes
	m_fullscreenShader
	    .setIsUnderwater(false)
	    .setFogDensity(0.0f)
	    .setFogColor(underwaterFogColor);

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
	    .setIsUnderwater(isUnderwater)
	    .setFogColor(underwaterFogColor)
	    .setFogDensity(isUnderwater ? 0.6f : 0.0f)
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



bool WaterSimulation::CompositionPass::isCameraUnderwater( 
	Registry& registry,
	const Magnum::Vector3& cameraPosition,
	HeightmapReadback* heightmapReadback
) {
	auto waterView = registry.view<WaterComponent, TransformComponent, MaterialComponent>();

	if (waterView.begin() != waterView.end()) {

		auto waterEntity = *waterView.begin();
		TransformComponent& transformComp = registry.get<TransformComponent>(waterEntity);
		WaterComponent& wC = registry.get<WaterComponent>(waterEntity);
		
		if (heightmapReadback) heightmapReadback->fetchLatestCPUCopy();

		const bool hasHeightmap = heightmapReadback && heightmapReadback->hasCpuData();
		const Magnum::Vector2i heightmapSize = hasHeightmap ? heightmapReadback->size() : Magnum::Vector2i{0};
		
		Magnum::Vector4 centerWorld{cameraPosition, 1.0f};
		Magnum::Vector3 waterSpacePoint = (transformComp.inverseGlobalModel * centerWorld).xyz();

		const float u = (waterSpacePoint.x() + wC.scale * 0.5f) / wC.scale;
		const float v = (waterSpacePoint.z() + wC.scale * 0.5f) / wC.scale;

		if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) return false;

		const int px = Magnum::Math::clamp(int(u * float(heightmapSize.x() - 1)), 0, heightmapSize.x() - 1);
		const int py = Magnum::Math::clamp(int(v * float(heightmapSize.y() - 1)), 0, heightmapSize.y() - 1);

		const float waterHeightLocal = heightmapReadback->heightAt(px, py);
 		Magnum::Vector4 surfaceLocal{waterSpacePoint.x(), waterHeightLocal, waterSpacePoint.z(), 1.0f};
        const float waterHeightWorld = (transformComp.globalModel * surfaceLocal).y();

		if (cameraPosition.y() < waterHeightWorld) return true;
	}
	return false;
}