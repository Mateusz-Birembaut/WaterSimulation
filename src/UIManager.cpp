

#include <WaterSimulation/WaterSimulation.h>
#include <WaterSimulation/Camera.h>
#include <WaterSimulation/ShallowWater.h>
#include <WaterSimulation/UIManager.h>

#include <Corrade/Containers/StringView.h>
#include <Magnum/GL/GL.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Time.h>
#include <imgui.h>

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


    paramWindow(app);


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

void WaterSimulation::UIManager::sceneGraph(){
    // test fenêtre
    {
        ImGui::Begin("Scene Graph");

        ImGui::End();
    }
}

void WaterSimulation::UIManager::paramWindow(Magnum::Platform::Sdl2Application & _app){

    auto* app = dynamic_cast<WaterSimulation::Application*>(& _app);

    // texture 
    {   
        ShallowWater * simulation = &(app->shallowWaterSimulation());

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));

        if (ImGui::Button(app->simulationPaused ? "Resume" : "Pause")) {
            app->simulationPaused = !app->simulationPaused;
        }

        if (ImGui::Button("Init Dam Break")) {
            simulation->initDamBreak();
        }

        if (ImGui::Button("Init Bump")) {
            simulation->initBump();
        }

        /* const char* items[] = { "h3.png", "h6.png" };
        static int currentItem = 0;
        if (ImGui::Combo("Texture", &currentItem, items, IM_ARRAYSIZE(items))) {
                        auto heightmapData = app->resourceManager().getRaw(items[currentItem]);
                        app->importer()->openData(heightmapData);
                        auto image = app->importer()->image2D(0);
                        simulation->loadTerrainHeightMap(&*image, 10.0f);  } */

        ImGui::InputInt("Step Number", &(app->step_number), 1, 10);

    }
}

void WaterSimulation::UIManager::cameraWindow(Camera & cam){
    // camera test fenêtre
    {
        ImGui::Begin("Camera");

        ImGui::Text("Aspect: %.3f", static_cast<double>(cam.aspectRatio()));

        Magnum::Deg fovDeg = cam.FOV();              
        float fov = static_cast<float>(fovDeg);                  
        if(ImGui::SliderFloat("FOV", &fov, 10.0f, 170.0f)) {
            cam.setFOV(fov);
        }

        Magnum::Vector3 pos = cam.position();
        if(ImGui::DragFloat3("Position", pos.data(), 0.1f)) {
            cam.setPos(pos); 
        }

        Magnum::Vector3 dir = cam.direction();
        ImGui::Text("Target Direction: (%.2f, %.2f, %.2f)", static_cast<double>(dir.x()), static_cast<double>(dir.y()), static_cast<double>(dir.z()));



        ImGui::End();

    }
}