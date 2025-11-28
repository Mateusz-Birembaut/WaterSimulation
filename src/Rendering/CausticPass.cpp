#include <WaterSimulation/Rendering/CausticPass.h>

#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/MaterialComponent.h>
#include <WaterSimulation/Components/ShaderComponent.h>
#include <WaterSimulation/Components/WaterComponent.h>
#include <WaterSimulation/Systems/RenderSystem.h>
#include <WaterSimulation/Mesh.h> 

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/GL/Renderer.h>



using namespace Magnum;

void WaterSimulation::CausticPass::recreateTextures(const Magnum::Vector2i& windowSize){
    m_causticMap = GL::Texture2D{};
    m_causticMap.setStorage(1, GL::TextureFormat::RGBA16F, windowSize)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setWrapping(GL::SamplerWrapping::ClampToEdge);
}

void WaterSimulation::CausticPass::init(const Magnum::Vector2i& windowSize){
	m_fb = GL::Framebuffer{{{}, windowSize}};

    recreateTextures(windowSize);

    m_fb.attachTexture(GL::Framebuffer::ColorAttachment{0}, m_causticMap, 0);

    m_fb.mapForDraw({{0, GL::Framebuffer::ColorAttachment{0}}});
}

void WaterSimulation::CausticPass::resize(const Magnum::Vector2i& windowSize){

    recreateTextures(windowSize);

    m_fb.attachTexture(GL::Framebuffer::ColorAttachment{0}, m_causticMap, 0);
    
    m_fb.setViewport({{}, windowSize});
}

void WaterSimulation::CausticPass::setupPhotonGrid() {
	Mesh gridData = Mesh::createGrid(500, 500, 2.0);
	// gridMeshComp = MeshComponent({{0.0f, &gridData}});

	m_photonBuffer = Magnum::GL::Buffer{};
	m_photonGrid = Magnum::GL::Mesh{};

	m_photonGrid.setCount(gridData.vertices.size());
	m_photonGrid.setPrimitive( Magnum::GL::MeshPrimitive::Points);

	Corrade::Utility::Debug{} << "Creating vertex buffer...";
	m_photonBuffer.setData(Corrade::Containers::arrayView(gridData.vertices)); // Containers::arrayView permet
	m_photonGrid.addVertexBuffer(m_photonBuffer, 0, DisplayShader::Position{});
}


void WaterSimulation::CausticPass::render(
	Registry& registry, 
	Camera & camera, 
	GL::Texture2D & shadowMap, 
	GL::Texture2D & waterMask, 
	GL::Texture2D& terrainHeightMap, 
	Vector3 & lightPosition,
	Matrix4 & lightViewProj
){
	GL::Renderer::setClearColor(Color4{0.0f, 0.0f, 0.0f, 0.0f});
	m_fb.clear(GL::FramebufferClear::Color).bind();

	glDepthMask(GL_FALSE);

	glEnable(GL_DEPTH_TEST); 
    glDepthFunc(GL_LESS);

	glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

	glEnable(GL_PROGRAM_POINT_SIZE);


	auto viewWater = registry.view<MeshComponent, TransformComponent, WaterComponent, MaterialComponent>();

	if(viewWater.begin() != viewWater.end()){
		Entity waterEntity = *viewWater.begin();
		auto & heightMap = registry.get<MaterialComponent>(waterEntity).heightmap;
		auto& transform = registry.get<TransformComponent>(waterEntity);

		const auto camViewProj = camera.projectionMatrix() * camera.viewMatrix();

		Matrix4 invLightViewProj = lightViewProj.inverted();

		m_causticShader.bindShadowMapTexture(shadowMap)
						.bindWaterHeightTexture(*heightMap)
						.bindWaterMaskTexture(waterMask)
						.bindTerrainHeightMapTexture(terrainHeightMap)
						.setInvVPLight(invLightViewProj)
						.setVPLight(lightViewProj)
						.setVPCamera(camViewProj)
						.setCameraPos(camera.position())
						.setLightPos(lightPosition)
						.setMatrixWorldPosToWaterUV(transform.inverseGlobalModel)
						.draw(m_photonGrid);
	}else {
		Debug {} << "Water entity not found, couldn't compute caustics";
	}

	glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}