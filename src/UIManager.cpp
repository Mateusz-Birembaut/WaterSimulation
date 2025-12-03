

#include <WaterSimulation/Camera.h>
#include <WaterSimulation/ShallowWater.h>
#include <WaterSimulation/UIManager.h>
#include <WaterSimulation/WaterSimulation.h>

#include <Corrade/Containers/StringView.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/GL.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Version.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <imgui.h>

using namespace Magnum;
using namespace Math::Literals;

void WaterSimulation::UIManager::drawUI(Application &app) {
    auto &imgui = app.getContext();

    imgui.newFrame();

    /* Enable text input, if needed */
    if (ImGui::GetIO().WantTextInput && !app.isTextInputActive())
        app.startTextInput();
    else if (!ImGui::GetIO().WantTextInput && app.isTextInputActive())
        app.stopTextInput();

    paramWindow(app);

    cameraWindow(app.camera());
    // perfWindow();

    if (!app.cursorLocked())
        imgui.updateApplicationCursor(app);

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

void WaterSimulation::UIManager::sceneGraph() {
    // test fenêtre
    {
        ImGui::Begin("Scene Graph");

        ImGui::End();
    }
}

void WaterSimulation::UIManager::paramWindow(
    Magnum::Platform::Sdl2Application &_app) {

    auto *app = dynamic_cast<WaterSimulation::Application *>(&_app);

    // texture
    {
        ShallowWater *simulation = &(app->shallowWaterSimulation());

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0 / Double(ImGui::GetIO().Framerate),
                    Double(ImGui::GetIO().Framerate));

        if (ImGui::Button(app->simulationPaused ? "Resume" : "Pause")) {
            app->simulationPaused = !app->simulationPaused;
        }
        if (ImGui::Button("Step")) {
            simulation->step();
        }

        if (ImGui::Button("Init Dam Break")) {
            simulation->initDamBreak();
        }
        ImGui::SameLine();
        if (ImGui::Button("Init Bump")) {
            simulation->initBump();
        }
        ImGui::SameLine();
        if (ImGui::Button("Init Tsunami")) {
            simulation->initTsunami();
        }
        ImGui::Checkbox("Airy waves", &simulation->airyWavesEnabled);

        ImGui::InputInt("Step Number", &(app->step_number), 1, 10);

        ImGui::Text("State, Terrain and Bulk Texture");
        ImGui::Image(
            reinterpret_cast<void *>(simulation->getStateTexture().id()),
            ImVec2(256, 256));
        ImGui::SameLine();
        ImGui::Image(
            reinterpret_cast<void *>(simulation->getTerrainTexture().id()),
            ImVec2(256, 256));
        ImGui::SameLine();
        ImGui::Image(
            reinterpret_cast<void *>(simulation->getBulkTexture().id()),
            ImVec2(256, 256));


        Magnum::GL::Texture2D * fftoutput = simulation->getFFTOutput();
        Magnum::GL::Texture2D * ifftoutput = simulation->getIFFTOutput();

        ImGui::Text("Surface Height, FFT, IFFT, Qx and Qy");
        ImGui::Image(
            reinterpret_cast<void *>(simulation->getSurfaceHeightTexture().id()),
            ImVec2(256, 256));
        ImGui::SameLine();
        if(fftoutput){
            ImGui::Image(
            reinterpret_cast<void *>(fftoutput->id()),
            ImVec2(256, 256));
            
        }else{
            ImGui::Text("no fftoutput");
        }
        ImGui::SameLine();
        if (ifftoutput) {
            ImGui::Image(
                reinterpret_cast<void *>(ifftoutput->id()),
                ImVec2(256, 256));
        } else {
            ImGui::Text("no ifftoutput");
        }  
        ImGui::SameLine();
        ImGui::Image(
            reinterpret_cast<void *>(simulation->getSurfaceQxTexture().id()),
            ImVec2(256, 256));
        ImGui::SameLine();
        ImGui::Image(
            reinterpret_cast<void *>(simulation->getSurfaceQyTexture().id()),
            ImVec2(256, 256));

        /* const char* items[] = { "h3.png", "h6.png" };
        static int currentItem = 0;
        if (ImGui::Combo("Texture", &currentItem, items, IM_ARRAYSIZE(items))) {
                        auto heightmapData =
        app->resourceManager().getRaw(items[currentItem]);
                        app->importer()->openData(heightmapData);
                        auto image = app->importer()->image2D(0);
                        simulation->loadTerrainHeightMap(&*image, 10.0f);  } */
    }
}

void WaterSimulation::UIManager::cameraWindow(Camera &cam) {
    // camera test fenêtre
    {
        ImGui::Begin("Camera");

        ImGui::Text("Aspect: %.3f", static_cast<double>(cam.aspectRatio()));

        Magnum::Deg fovDeg = cam.FOV();
        float fov = static_cast<float>(fovDeg);
        if (ImGui::SliderFloat("FOV", &fov, 10.0f, 170.0f)) {
            cam.setFOV(fov);
        }

        Magnum::Vector3 pos = cam.position();
        if (ImGui::DragFloat3("Position", pos.data(), 0.1f)) {
            cam.setPos(pos);
        }

        Magnum::Vector3 dir = cam.direction();
        ImGui::Text("Target Direction: (%.2f, %.2f, %.2f)",
                    static_cast<double>(dir.x()), static_cast<double>(dir.y()),
                    static_cast<double>(dir.z()));

        ImGui::End();
    }
}