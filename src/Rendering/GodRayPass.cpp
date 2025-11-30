#include <WaterSimulation/Rendering/GodRayPass.h>

#include <WaterSimulation/Components/MaterialComponent.h>
#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Components/ShaderComponent.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/WaterComponent.h>
#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Mesh.h>
#include <WaterSimulation/Systems/RenderSystem.h>
#include <WaterSimulation/Rendering/CausticUtil.h>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Shaders/FlatGL.h>

#include <chrono>

using namespace Magnum;


void WaterSimulation::GodRayPass::init() {
	const Magnum::Vector2i causticResolution = {128, 128};
	m_fb = GL::Framebuffer{{{}, causticResolution}};

	m_godrayTexture = GL::Texture2D{};
	m_godrayTexture.setStorage(1, GL::TextureFormat::RGBA16F, causticResolution)
	    .setMinificationFilter(GL::SamplerFilter::Linear)
	    .setMagnificationFilter(GL::SamplerFilter::Linear)
	    .setWrapping(GL::SamplerWrapping::ClampToEdge);

	m_fb.attachTexture(GL::Framebuffer::ColorAttachment{0}, m_godrayTexture, 0);
	m_fb.setViewport({{}, causticResolution});
	m_fb.mapForDraw({{0, GL::Framebuffer::ColorAttachment{0}}});
}



void WaterSimulation::GodRayPass::setupPhotonGrid() {
	Mesh gridData = Mesh::createGrid(700, 700, 2.0);

	m_photonBuffer = Magnum::GL::Buffer{};
	m_photonGrid = Magnum::GL::Mesh{};

	m_photonGrid.setCount(gridData.vertices.size());
	m_photonGrid.setPrimitive(Magnum::GL::MeshPrimitive::Points);

	Corrade::Utility::Debug{} << "Creating vertex buffer...";
	m_photonBuffer.setData(Corrade::Containers::arrayView(gridData.vertices)); 
	m_photonGrid.addVertexBuffer(m_photonBuffer, 0, DisplayShader::Position{});
}


void WaterSimulation::GodRayPass::render(
    Registry& registry,
    Camera& camera,
    GL::Texture2D& shadowMap,
    GL::Texture2D& waterWorldPos,
    GL::Texture2D& opaquePassDepth,
    Vector3& lightPosition,
    Matrix4& lightViewProj,
    float cameraNear,
    float cameraFar,
	float sunFar
) 
	
	{
	GL::Renderer::setClearColor(Color4{0.0f, 0.0f, 0.0f, 0.0f});
	m_fb.bind();
	m_fb.clear(GL::FramebufferClear::Color);


	static auto startTime = std::chrono::steady_clock::now();
	auto now = std::chrono::steady_clock::now();
	float uTime = std::chrono::duration<float>(now - startTime).count();

	glDisable(GL_CULL_FACE);

	glDepthMask(GL_FALSE);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glEnable(GL_PROGRAM_POINT_SIZE);

	auto viewWater = registry.view<MeshComponent, TransformComponent, WaterComponent, MaterialComponent>();

	if (viewWater.begin() != viewWater.end()) {
		Entity waterEntity = *viewWater.begin();
		auto& heightMap = registry.get<MaterialComponent>(waterEntity).heightmap;
		auto& transform = registry.get<TransformComponent>(waterEntity);

		const auto camViewProj = camera.projectionMatrix() * camera.viewMatrix();

		float zNear = std::clamp(cameraNear, 1.0f, cameraFar);
		float zFar = cameraFar;

		m_godrayShader.bindShadowMapTexture(shadowMap)
		    .bindWaterMaskTexture(waterWorldPos)
		    .setVPLight(lightViewProj)
		    .setVPCamera(camViewProj)
		    .setCameraPos(camera.position())
		    .setLightPos(lightPosition)
		    .setUtime(uTime)
		    .bindCamDepthBufferTexture(opaquePassDepth)
			.setFogDensity(m_gamma)
			.setG(m_g)
		    .setLightFar(sunFar)
			.setIntensity(m_intensity)
			.setRayWidth(m_rayWidth)
		    .draw(m_photonGrid);
	} else {
		Debug{} << "Water entity not found, couldn't compute caustics";
	}

	
	m_utils.blurTexture(m_godrayTexture);

	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
}