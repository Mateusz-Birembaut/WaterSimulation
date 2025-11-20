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
#include <MagnumPlugins/StbImageImporter/StbImageImporter.h>



#include <WaterSimulation/WaterSimulation.h>


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

    // Plugins setup

    Corrade::PluginManager::Manager<Magnum::Trade::AbstractImporter> manager; //manager des plugins, sert a instantier les plugins de chargements d'assets nottament

    auto importer = manager.loadAndInstantiate("StbImageImporter");

    Debug{} << "Available plugins : " << manager.pluginList();

    if(!importer){
        Error{} << "Could not load STB Image plugins, have you initialized all git submodules ?";
    }else{
        Debug{} << "Plugin STB Image Importer loaded ";
    }

    importer->openFile("ressources/heightmaps/h1.png");
    auto image = importer->image2D(0);

    // ImGui setup
    m_imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(),windowSize(), framebufferSize());

    // Shallow Water simulation setup
    m_shallowWaterSimulation = ShallowWater(128,128, 1.0f, 1.0f/144.0f);
    m_shallowWaterSimulation.initBump();

    m_heightTexture = GL::Texture2D{};
    m_heightTexture.setWrapping(GL::SamplerWrapping::ClampToEdge)
                   .setMinificationFilter(GL::SamplerFilter::Linear)
                   .setMagnificationFilter(GL::SamplerFilter::Nearest)
                   .setStorage(1, GL::TextureFormat::R8, {m_shallowWaterSimulation.getnx(), m_shallowWaterSimulation.getny()});


    // OpenGL setup

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);

    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    GL::Renderer::setClearColor(0xa5c9ea_rgbf);

    Debug{} << "This application is running on"
            << GL::Context::current().version() << "using"
            << GL::Context::current().rendererString();
}

void WaterSimulation::Application::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    m_imgui.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
        event.windowSize(), event.framebufferSize());
}

//main draw loop
void WaterSimulation::Application::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    m_shallowWaterSimulation.step();

    m_shallowWaterSimulation.updateHeightTexture(&m_heightTexture);

    m_UIManager.drawUI(*this, m_imgui);

    swapBuffers();
    redraw();
}


void WaterSimulation::Application::keyPressEvent(KeyEvent& event) {
    if(m_imgui.handleKeyPressEvent(event)) return;
}

void WaterSimulation::Application::keyReleaseEvent(KeyEvent& event) {
    if(m_imgui.handleKeyReleaseEvent(event)) return;
}

void WaterSimulation::Application::pointerPressEvent(PointerEvent& event) {
    if(m_imgui.handlePointerPressEvent(event)) return;
}

void WaterSimulation::Application::pointerReleaseEvent(PointerEvent& event) {
    if(m_imgui.handlePointerReleaseEvent(event)) return;
}

void WaterSimulation::Application::pointerMoveEvent(PointerMoveEvent& event) {
    if(m_imgui.handlePointerMoveEvent(event)) return;
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

MAGNUM_APPLICATION_MAIN(WaterSimulation::Application)
