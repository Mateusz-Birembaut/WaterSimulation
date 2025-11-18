#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Corrade/Containers/StringView.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>

#include <WaterSimulation/WaterSimulation.h>

using namespace Magnum;


WaterSimulation::Application::Application(const Arguments& arguments): 
    Platform::Application{arguments, Configuration{}
        .setTitle("Water Simulation App")
        .setSize({1200, 800})
        .addWindowFlags(Configuration::WindowFlag::Resizable)
    } 

{
    {
        using namespace Math::Literals;
        GL::Renderer::setClearColor(0xa5c9ea_rgbf);
    }

    Debug{} << "This application is running on"
            << GL::Context::current().version() << "using"
            << GL::Context::current().rendererString();
}

void WaterSimulation::Application::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
}

void WaterSimulation::Application::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    /* TODO: Add your drawing code here */

    swapBuffers();
}

MAGNUM_APPLICATION_MAIN(WaterSimulation::Application)
