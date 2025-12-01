
#include <WaterSimulation/WaterSimulation.h>

#include <WaterSimulation/Systems/RenderSystem.h>
#include <WaterSimulation/Systems/TransformSystem.h>
#include <WaterSimulation/Systems/PhysicsSystem.h>

#include <WaterSimulation/Camera.h>
#include <WaterSimulation/Mesh.h>
#include <WaterSimulation/UIManager.h>
#include <WaterSimulation/ECS.h>

#include <WaterSimulation/Rendering/CustomShader/TerrainShader.h>
#include <WaterSimulation/Rendering/CustomShader/DebugShader.h>

#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/MaterialComponent.h>
#include <WaterSimulation/Components/ShaderComponent.h>
#include <WaterSimulation/Components/DirectionalLightComponent.h>
#include <WaterSimulation/Components/LightComponent.h>
#include <WaterSimulation/Components/ShadowCasterComponent.h>
#include <WaterSimulation/Components/ShaderComponent.h>
#include <WaterSimulation/Components/WaterComponent.h>
#include <WaterSimulation/Components/TerrainComponent.h>
#include <WaterSimulation/Components/BuoyancyComponent.h>

#include <Corrade/Containers/StringView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Debug.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/ImageView.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Shaders/VertexColorGL.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include <memory>

using namespace Magnum;
using namespace Math::Literals;
using namespace Corrade::Utility;

WaterSimulation::Application::Application(const Arguments& arguments): 
    Platform::Application{arguments, Configuration{}
        .setTitle("Water Simulation App")
        .setSize({1200, 800})
        .addWindowFlags(Configuration::WindowFlag::Resizable)
    } 
{   
	Corrade::Utility::Resource rs{"WaterSimulationResources"};

    Debug{} << "Creating application";
    
    m_timeline.start();
    // Plugins setup

    Corrade::PluginManager::Manager<Magnum::Trade::AbstractImporter> importerManager; //manager des plugins, sert a instantier les plugins de chargements d'assets nottament
    Corrade::PluginManager::Manager<Magnum::Trade::AbstractImageConverter> converterManager; //manager des plugins, sert a instantier les plugins de chargements d'assets nottament

    Debug{} << "Available importer plugins : " << importerManager.pluginList();
    Debug{} << "Available converter plugins : " << converterManager.pluginList();

    auto importer = importerManager.loadAndInstantiate("StbImageImporter");

    if(!importer){
        Error{} << "Could not load STB Image Importer plugin, have you initialized all git submodules ?";
    }else{
        Debug{} << "Plugin STB Image Importer loaded ";
    }

    auto converter = converterManager.loadAndInstantiate("StbResizeImageConverter");

    if(!importer){
        Error{} << "Could not load STB Resize Image Converter plugin, have you initialized all git submodules ?";
    }else{
        Debug{} << "Plugin STB Image Resizer and Converter loaded ";
    }

    auto heightmapData = rs.getRaw("h3.png");
    importer->openData(heightmapData);
    auto image = importer->image2D(0);

    converter->configuration().setValue("size", "512 512");
    auto resized = converter->convert(*image);
    auto allo = resized->format();
    Debug{} << "FORMAT IS : " << allo;
    Debug{} << "SIZE IS : " << resized->size();
    
    //ImageView2D resized_converted_heightmap = ImageView2D{PixelFormat::R8Unorm, resized->size(), resized->data()};
    
    // Shallow Water simulation setup
    m_shallowWaterSimulation = ShallowWater(511,511, 0.25f, 1.0f/30.0f);
    
    m_shallowWaterSimulation.loadTerrainHeightMap(&*resized, 10.0f);

    m_shallowWaterSimulation.initDamBreak();

    debugShader = DisplayShader("debug.vs", "debug.fs");
    

    // ImGui setup
    m_imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(),windowSize(), framebufferSize());

    // OpenGL setup

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);

    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    GL::Renderer::setClearColor(0xa5c9ea_rgbf);

    m_renderSystem.init(framebufferSize());

    Debug{} << "This application is running on"
            << GL::Context::current().version() << "using"
            << GL::Context::current().rendererString();

    m_UIManager = std::make_unique<UIManager>();
    m_camera = std::make_unique<Camera>(windowSize());
    m_camera.get()->setPos({0.0, 1.0, 0.0f});
    m_camera.get()->setSpeed(10.0f);
    m_camera.get()->setRotSpeed(5.0f);
    m_camera.get()->setFar(500.0f);
    
    // test ECS et rendu avec shader de base
    // test sphere avec texture

    auto grassData = rs.getRaw("grass.png");
    importer->openData(grassData);
    auto imageTest = importer->image2D(0);
    m_testAlbedo.setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
                    .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
                    .setMagnificationFilter(Magnum::GL::SamplerFilter::Nearest)
                    .setStorage(1, Magnum::GL::TextureFormat::RGBA8, imageTest->size())
                    .setSubImage(0, {}, Magnum::ImageView2D{*imageTest});   

    auto albedoPtr = std::make_shared<Magnum::GL::Texture2D>(std::move(m_testAlbedo));
    
    //test maillage
    Entity testEntity = m_registry.create();
    auto & mat = m_registry.emplace<MaterialComponent>(
        testEntity
    );
    mat.setAlbedo(albedoPtr);
    auto & testTransform = m_registry.emplace<TransformComponent>(
        testEntity
    );
    testTransform.position = Magnum::Vector3(0.0f, 20.0f, -15.0f);
    testTransform.scale = Magnum::Vector3(10.0f, 10.0f, 10.0f);
    m_testMesh = std::make_unique<Mesh>("./resources/assets/Meshes/sphereLOD1.obj");
    auto& testMeshComp = m_registry.emplace<MeshComponent>(
        testEntity,
        std::vector<std::pair<float, Mesh*>>{{0.0f, m_testMesh.get()}}
    );
    auto& rigidBody = m_registry.emplace<RigidBodyComponent>(testEntity);
    rigidBody.mass = 1.0f;
    rigidBody.mesh = testMeshComp.activeMesh;
    rigidBody.addCollider(new SphereCollider(10.0f));
    auto& b = m_registry.emplace<BuoyancyComponent>(testEntity);
    b.createTestPointsFromMesh(*testMeshComp.activeMesh);

    float scale = 250.0f;
    
    // terrain test avec heightmap et texture pas pbr
    Entity testTerrain = m_registry.create();
    auto & matTerrain = m_registry.emplace<MaterialComponent>(
        testTerrain
    );
    auto heightmapPtr = std::shared_ptr<Magnum::GL::Texture2D>(&m_shallowWaterSimulation.getTerrainTexture(), [](Magnum::GL::Texture2D*){});
    matTerrain.setAlbedo(albedoPtr);
    matTerrain.setHeightMap(heightmapPtr);
    m_terrainMesh = std::make_unique<Mesh>(Mesh::createGrid(512, 512, scale));
    m_registry.emplace<MeshComponent>(
        testTerrain,
        std::vector<std::pair<float, Mesh*>>{{0.0f, m_terrainMesh.get()}}
    );
    m_registry.emplace<TransformComponent>(
        testTerrain,
        Magnum::Vector3{0.0f, -1.0f, -3.0f}
    );
    auto shaderPtr = std::make_shared<TerrainShader>();
    m_registry.emplace<ShaderComponent>(
        testTerrain,
        shaderPtr
    ); 
    m_registry.emplace<TerrainComponent>(testTerrain);

    //visu eau rapide
    auto waterHeightTexPtr = std::shared_ptr<Magnum::GL::Texture2D>(&m_shallowWaterSimulation.getStateTexture(), [](Magnum::GL::Texture2D*){});
    auto waterAlbedoTexPtr = std::shared_ptr<Magnum::GL::Texture2D>(&m_shallowWaterSimulation.getStateTexture(), [](Magnum::GL::Texture2D*){});
    
    auto waterShader = std::make_shared<DebugShader>();
    m_registry.emplace<ShaderComponent>(
        testTerrain,
        shaderPtr
    ); 

    m_waterMesh = std::make_unique<Mesh>(Mesh::createGrid(512, 512, scale)); 
    Entity waterEntity = m_registry.create();
    m_registry.emplace<MeshComponent>(
        waterEntity,
        std::vector<std::pair<float, Mesh*>>{{0.0f, m_waterMesh.get()}}
    );
    m_registry.emplace<TransformComponent>(
        waterEntity,
        Magnum::Vector3{0.0f, -1.0f, -3.0f} 
    );
    auto& waterMat = m_registry.emplace<MaterialComponent>(waterEntity);
    waterMat.setHeightMap(waterHeightTexPtr);
    waterMat.setAlbedo(waterAlbedoTexPtr);
    m_registry.emplace<ShaderComponent>(
        waterEntity,
        waterShader
    ); 
    m_registry.emplace<WaterComponent>(waterEntity, 512, 512, scale);
    
 
    // sun light en cours
    Magnum::Vector3 sunPos{50.0f, 100.0f, 100.0f};
    Magnum::Vector3 target{128.0f, 0.0f, 128.0f};

    auto sunEntity = m_registry.create();
    m_registry.emplace<LightComponent>(sunEntity, Color3{1.0f, 0.95f, 0.8f}, 5.0f);
    m_registry.emplace<DirectionalLightComponent>(sunEntity);
    m_registry.emplace<ShadowCasterComponent>(sunEntity);
}

WaterSimulation::Application::~Application() {
    m_registry.clear(); 
}

void WaterSimulation::Application::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    m_renderSystem.resize(event.framebufferSize());
    m_imgui.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
        event.windowSize(), event.framebufferSize());

    m_camera->setWindowSize(event.windowSize());
}

//main draw loop
void WaterSimulation::Application::drawEvent() {
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
    
    m_timeline.nextFrame();
    m_deltaTime = m_timeline.previousFrameDuration();

    handleCameraInputs();

    
    debugShader.bind(&m_shallowWaterSimulation.getStateTexture(), 0);
    debugShader.bind(&m_shallowWaterSimulation.getTerrainTexture(), 1);
    

    if(!simulationPaused) m_shallowWaterSimulation.step();

    m_transform_System.update(m_registry);
    m_physicSystem.update(m_registry, m_deltaTime);

    m_renderSystem.render(m_registry, *m_camera.get());

    m_UIManager->drawUI(*this);

    if(m_cursorLocked)
        setCursor(Platform::Sdl2Application::Cursor::HiddenLocked);

    swapBuffers();
    redraw();
}


void WaterSimulation::Application::keyPressEvent(KeyEvent& event) {
    if(m_imgui.handleKeyPressEvent(event)) return;

    using Key = Platform::Sdl2Application::Key;
    if(event.key() == Key::Enter) {
        setCursor(Platform::Sdl2Application::Cursor::HiddenLocked);
        m_cursorLocked = true;
        Debug{} << "locked";
        return;
    }
    if(event.key() == Key::Esc) {
        setCursor(Platform::Sdl2Application::Cursor::Arrow);
        m_cursorLocked = false;
        Debug{} << "unlocked";
        return;
    }

    if(event.key() == Key::N) {
        m_renderSystem.m_renderDepthOnly = !m_renderSystem.m_renderDepthOnly;
        Debug{} << "Depth Mode :" << m_renderSystem.m_renderDepthOnly;
        return;
    }
    if(event.key() == Key::B) {
        m_renderSystem.m_renderShadowMapOnly = !m_renderSystem.m_renderShadowMapOnly;
        Debug{} << "Draw shadow map only:" << m_renderSystem.m_renderShadowMapOnly;
        return;
    }
    if(event.key() == Key::V) {
        m_renderSystem.m_renderWaterMaskOnly = !m_renderSystem.m_renderWaterMaskOnly;
        Debug{} << "Draw shadow map only:" << m_renderSystem.m_renderWaterMaskOnly;
        return;
    }
    if(event.key() == Key::C) {
        m_renderSystem.m_renderCausticMapOnly = !m_renderSystem.m_renderCausticMapOnly;
        Debug{} << "Draw shadow map only:" << m_renderSystem.m_renderCausticMapOnly;
        return;
    }
    if(event.key() == Key::X) {
        m_renderSystem.m_renderGodRayMapOnly = !m_renderSystem.m_renderGodRayMapOnly;
        Debug{} << "Draw shadow map only:" << m_renderSystem.m_renderGodRayMapOnly;
        return;
    }

    m_keysPressed.insert(event.key());
}

void WaterSimulation::Application::keyReleaseEvent(KeyEvent& event) {
    if(m_imgui.handleKeyReleaseEvent(event)) return;
    m_keysPressed.erase(event.key());
}

void WaterSimulation::Application::pointerPressEvent(PointerEvent& event) {
    if(m_imgui.handlePointerPressEvent(event)) return;
}

void WaterSimulation::Application::pointerReleaseEvent(PointerEvent& event) {
    if(m_imgui.handlePointerReleaseEvent(event)) return;
}

void WaterSimulation::Application::pointerMoveEvent(PointerMoveEvent& event) {
    if(m_imgui.handlePointerMoveEvent(event)) return;

    if(!m_cursorLocked) return;

    Vector2 delta = event.relativePosition();
    if(delta.isZero()) return;

    delta *= Vector2{-1.0f, -1.0f};
    delta *= m_camera->rotSpeed() * m_deltaTime;
    m_camera->rotate(delta);

}

void WaterSimulation::Application::scrollEvent(ScrollEvent& event) {
    if(m_imgui.handleScrollEvent(event)) {
        /* Prevent scrolling the page */
        event.setAccepted();
        return;
    }
}

void WaterSimulation::Application::textInputEvent(TextInputEvent& event) {
    if(m_imgui.handleTextInputEvent(event)) return;
}

void WaterSimulation::Application::handleCameraInputs(){
    using Key = Platform::Sdl2Application::Key;
    const float distance = m_camera->speed() * m_deltaTime;
    if(m_keysPressed.count(Key::D)) m_camera->move({ distance, 0.0f,  0.0f});
    if(m_keysPressed.count(Key::Q)) m_camera->move({-distance, 0.0f,  0.0f});
    if(m_keysPressed.count(Key::S)) m_camera->move({ 0.0f, 0.0f,  distance});
    if(m_keysPressed.count(Key::Z)) m_camera->move({ 0.0f, 0.0f, -distance});
}

MAGNUM_APPLICATION_MAIN(WaterSimulation::Application)
