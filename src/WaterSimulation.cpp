#include <WaterSimulation/WaterSimulation.h>
#include <WaterSimulation/UIManager.h>
#include <WaterSimulation/Camera.h>
#include <WaterSimulation/Mesh.h>
#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Systems/RenderSystem.h>
#include <WaterSimulation/Systems/TransformSystem.h>

#include <Magnum/Magnum.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Corrade/Containers/StringView.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Time.h>
#include <Magnum/GL/TextureFormat.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Corrade/Utility/Debug.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/ImageView.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Shaders/VertexColorGL.h>
#include <Magnum/Shaders/FlatGL.h>
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

    importer->openFile("ressources/heightmaps/h3.png");
    auto image = importer->image2D(0);

    converter->configuration().setValue("size", "256 256");
    auto resized = converter->convert(*image);
    auto allo = resized->format();
    Debug{} << "FORMAT IS : " << allo;
    Debug{} << "SIZE IS : " << resized->size();
    
    //ImageView2D resized_converted_heightmap = ImageView2D{PixelFormat::R8Unorm, resized->size(), resized->data()};
    

    // Shallow Water simulation setup
    m_shallowWaterSimulation = ShallowWater(255,255, .25f, 1.0f/60.0f);
    

    m_heightTexture = GL::Texture2D{};
    m_heightTexture.setWrapping(GL::SamplerWrapping::ClampToEdge)
                   .setMinificationFilter(GL::SamplerFilter::Linear)
                   .setMagnificationFilter(GL::SamplerFilter::Nearest)
                   .setStorage(1, GL::TextureFormat::R32F, {m_shallowWaterSimulation.getnx(), m_shallowWaterSimulation.getny()});

    m_momentumTexture = GL::Texture2D{};
    m_momentumTexture.setWrapping(GL::SamplerWrapping::ClampToEdge)
                   .setMinificationFilter(GL::SamplerFilter::Linear)
                   .setMagnificationFilter(GL::SamplerFilter::Nearest)
                   .setStorage(1, GL::TextureFormat::RG32F, {m_shallowWaterSimulation.getnx(), m_shallowWaterSimulation.getny()});

    m_terrainHeightmap = GL::Texture2D{};
    m_terrainHeightmap.setWrapping(GL::SamplerWrapping::ClampToEdge)
                   .setMinificationFilter(GL::SamplerFilter::Linear)
                   .setMagnificationFilter(GL::SamplerFilter::Nearest)
                   .setStorage(1, GL::TextureFormat::R8, {m_shallowWaterSimulation.getnx(), m_shallowWaterSimulation.getny()})
                   .setSubImage(0, {}, *resized);

    
    //m_shallowWaterSimulation.loadTerrainHeightMap(&*resized, 10.0f);

    m_shallowWaterSimulation.initDamBreak();

    debugShader = DisplayShader("./ressources/shaders/debug.vs", "./ressources/shaders/debug.fs");
    

    // ImGui setup
    m_imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(),windowSize(), framebufferSize());

    // OpenGL setup

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);

    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    GL::Renderer::setClearColor(0xa5c9ea_rgbf);

    Debug{} << "This application is running on"
            << GL::Context::current().version() << "using"
            << GL::Context::current().rendererString();

    m_UIManager = std::make_unique<UIManager>();
    m_camera = std::make_unique<Camera>(windowSize());

    Debug{} << "Camera pos:" << m_camera->position();
    
    // test ECS et rendu avec shader de base
    m_testFlatShader = Shaders::FlatGL3D{};
    
    m_testMesh = std::make_unique<Mesh>("./ressources/assets/Meshes/quad.obj");
    Entity testEntity = m_registry.create();
    m_registry.emplace<TransformComponent>(
        testEntity,
        Magnum::Vector3{0.0f, 0.0f, -1.0f},
        Quaternion::rotation(-90.0_degf, Vector3::yAxis())
    );
    m_registry.emplace<MeshComponent>(
        testEntity,
        std::vector<std::pair<float, Mesh*>>{{0.0f, m_testMesh.get()}},
        &debugShader
    );
    
}

WaterSimulation::Application::~Application() {
    m_registry.clear(); 
}

void WaterSimulation::Application::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

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

    debugShader.bind(&m_shallowWaterSimulation.getTerrainTexture(), 1);
    debugShader.bind(&m_shallowWaterSimulation.getStateTexture(), 0);
    

    if(!simulationPaused) m_shallowWaterSimulation.step();

    m_transform_System.update(m_registry);

    auto view = m_camera->viewMatrix();
    auto proj = m_camera->projectionMatrix();
    m_renderSystem.render(m_registry, view, proj);

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
