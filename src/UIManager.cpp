
#include "Magnum/GL/GL.h"
#include "WaterSimulation/ShallowWater.h"
#include "WaterSimulation/WaterSimulation.h"
#include <WaterSimulation/UIManager.h>

#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Corrade/Containers/StringView.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Time.h>

using namespace Magnum;
using namespace Math::Literals;

void WaterSimulation::UIManager::drawUI(Magnum::Platform::Sdl2Application & _app, Magnum::ImGuiIntegration::Context & _imgui){
    _imgui.newFrame();

    /* Enable text input, if needed */
    if(ImGui::GetIO().WantTextInput && !_app.isTextInputActive())
        _app.startTextInput();
    else if(!ImGui::GetIO().WantTextInput && _app.isTextInputActive())
        _app.stopTextInput();

    paramWindow(_app);

    _imgui.updateApplicationCursor(_app);

    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

    _imgui.drawFrame();

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void WaterSimulation::UIManager::paramWindow(Magnum::Platform::Sdl2Application & _app){

    auto* app = dynamic_cast<WaterSimulation::Application*>(& _app);

    // test fenêtre
    {
        ImGui::Text("Hello, world!");
        if(ImGui::Button("Test Button"))
            Debug{} << "Button Pressed";
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));
    }

    // texture 
    {
        Magnum::GL::Texture2D * heightTexture = &(app->m_heightTexture);
        ShallowWater * simulation = &(app->m_shallowWaterSimulation); 

        if (heightTexture) {
            ImVec2 textureSize(512, 512);
            ImGui::Text("Water height:");
            ImGui::Image(reinterpret_cast<void*>(heightTexture->id()), textureSize);
        } else {
            ImGui::Text("texture error");
        }
    }
}