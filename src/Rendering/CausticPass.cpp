#include <WaterSimulation/Rendering/CausticPass.h>

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


void WaterSimulation::CausticPass::init() {
	const Magnum::Vector2i causticResolution = {1500, 1500};
	m_fb = GL::Framebuffer{{{}, causticResolution}};

	m_causticMap = GL::Texture2D{};
	m_causticMap.setStorage(1, GL::TextureFormat::RGBA16F, causticResolution)
	    .setMinificationFilter(GL::SamplerFilter::Linear)
	    .setMagnificationFilter(GL::SamplerFilter::Linear)
	    .setWrapping(GL::SamplerWrapping::ClampToEdge);

	m_fb.attachTexture(GL::Framebuffer::ColorAttachment{0}, m_causticMap, 0);
	m_fb.setViewport({{}, causticResolution});
	m_fb.mapForDraw({{0, GL::Framebuffer::ColorAttachment{0}}});
}



void WaterSimulation::CausticPass::setupPhotonGrid() {
	Mesh gridData = Mesh::createGrid(2000, 2000, 2.0);

	m_photonBuffer = Magnum::GL::Buffer{};
	m_photonGrid = Magnum::GL::Mesh{};

	m_photonGrid.setCount(gridData.vertices.size());
	m_photonGrid.setPrimitive(Magnum::GL::MeshPrimitive::Points);

	Corrade::Utility::Debug{} << "Creating vertex buffer...";
	m_photonBuffer.setData(Corrade::Containers::arrayView(gridData.vertices)); // Containers::arrayView permet
	m_photonGrid.addVertexBuffer(m_photonBuffer, 0, DisplayShader::Position{});
}

void WaterSimulation::CausticPass::render(
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

		float b_calc = (cameraNear * zFar * (m_S_MAX - m_S_MIN)) / (zFar - cameraNear);
		float a_calc = m_S_MIN - (b_calc / zFar);


		m_causticShader.bindShadowMapTexture(shadowMap)
		    .bindWaterMaskTexture(waterWorldPos)
		    .setVPLight(lightViewProj)
		    .setInvVPLight(lightViewProj.inverted())
		    .setVPCamera(camViewProj)
		    .setCameraPos(camera.position())
		    .setLightPos(lightPosition)
		    .setUtime(uTime)
		    .bindCamDepthBufferTexture(opaquePassDepth)
		    .setAttenuation(m_waterAttenuation)
		    .setIntensity(m_photonIntensity)
		    .setA(a_calc)
		    .setB(b_calc)
		    .setLightFar(sunFar)
			.setFloorOffset(m_floorOffset)
		    .draw(m_photonGrid);
	} else {
		Debug{} << "Water entity not found, couldn't compute caustics";
	}

	
	if (m_enableBlur) {
		m_utils.blurTexture(m_causticMap, m_blurRadius);
	}

	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);
}

