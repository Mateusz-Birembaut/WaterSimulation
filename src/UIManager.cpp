
#include <WaterSimulation/WaterSimulation.h>
#include <WaterSimulation/Camera.h>

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

void WaterSimulation::UIManager::drawUI(Application & app){
    auto & imgui = app.getContext();

    imgui.newFrame();

    /* Enable text input, if needed */
    if(ImGui::GetIO().WantTextInput && !app.isTextInputActive())
        app.startTextInput();
    else if(!ImGui::GetIO().WantTextInput && app.isTextInputActive())
        app.stopTextInput();


    cameraWindow(app.camera());
    //perfWindow();

    if(!app.cursorLocked()) imgui.updateApplicationCursor(app);


    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

    imgui.drawFrame();

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void WaterSimulation::UIManager::perfWindow(){
    // test fenêtre
    {
        ImGui::Begin("Perfs");

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));
            
        ImGui::End();
    }
}

void WaterSimulation::UIManager::cameraWindow(Camera & cam){
    // camera test fenêtre
    {
        ImGui::Begin("Camera");

        ImGui::Text("Aspect: %.3f", static_cast<double>(cam.aspectRatio()));

        Magnum::Deg fovDeg = cam.FOV();              
        float fov = static_cast<float>(fovDeg);                  
        ImGui::Text("FOV: %.1f deg", static_cast<double>(fov));

        Magnum::Vector3 pos = cam.position();
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", static_cast<double>(pos.x()), static_cast<double>(pos.y()), static_cast<double>(pos.z()));

        Magnum::Vector3 dir = cam.direction();
        ImGui::Text("Target Direction: (%.2f, %.2f, %.2f)", static_cast<double>(dir.x()), static_cast<double>(dir.y()), static_cast<double>(dir.z()));
        ImGui::End();

    }
}