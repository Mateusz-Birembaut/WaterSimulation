

#include <WaterSimulation/WaterSimulation.h>
#include <WaterSimulation/Camera.h>
#include <WaterSimulation/ShallowWater.h>
#include <WaterSimulation/UIManager.h>
#include <WaterSimulation/ECS.h>

#include <WaterSimulation/Components/LightComponent.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/DirectionalLightComponent.h>
#include <WaterSimulation/Components/ShadowCasterComponent.h>

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
    visualWindow(app);
    sunWindow(app.registry());
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

        ImGui::Text("Debug depth : ");
        ImGui::SliderFloat("Linearize Range", &app->m_renderSystem.m_linearizeRange, 0.01f, 1000.0f);


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

        float near = cam.near();                 
        if(ImGui::SliderFloat("near", &near, 0.01f, 100.0f)) {
            cam.setNear(near);
        }

        float far = cam.far();                
        if(ImGui::SliderFloat("Far", &far, 5.0f, 1000.0f)) {
            cam.setFar(far);
        }

        ImGui::End();

    }
}

void WaterSimulation::UIManager::sunWindow(Registry & registry){
    // camera test fenêtre
    {
        ImGui::Begin("Sun");

        auto sunView = registry.view<DirectionalLightComponent, ShadowCasterComponent, LightComponent>();
        if (sunView.begin() != sunView.end()) {

            Entity sunEntity = *sunView.begin();

            auto& sunDirection = sunView.get<DirectionalLightComponent>(sunEntity);
            auto& sunLight = sunView.get<LightComponent>(sunEntity);
            auto& shadowCastData = sunView.get<ShadowCasterComponent>(sunEntity);


            ImGui::Text("Color:");
            float color[3] = {sunLight.color.r(), sunLight.color.g(), sunLight.color.b()};
            if (ImGui::ColorEdit3("Sun Color", color)) {
                sunLight.color = Magnum::Color3{color[0], color[1], color[2]};
            }
            float intensity = sunLight.intensity;
            if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f)) {
                sunLight.intensity = intensity;
            }

            ImGui::Text("Direction:");
            if (ImGui::DragFloat3("Direction", sunDirection.direction.data(), 0.01f, -1.0f, 1.0f)) {
                sunDirection.direction = sunDirection.direction.normalized();
            }
            ImGui::DragFloat("Offset", &sunDirection.offset, 0.1f, 0.1f, 10000.0f);
            

            ImGui::Text("Shadow Map:");
            ImGui::DragFloat2("Shadow Size", shadowCastData.projectionSize.data(), 1.0f, 1.0f, 1000.0f);
            ImGui::DragFloat("Shadow Near", &shadowCastData.near, 1.0f,-1000.01f, 1000.0f);
            ImGui::DragFloat("Shadow Far", &shadowCastData.far, 1.0f, -5000.0f, 5000.0f);

        }

        ImGui::End();
    }
}

void WaterSimulation::UIManager::visualWindow(Magnum::Platform::Sdl2Application & _app){

    auto* app = dynamic_cast<WaterSimulation::Application*>(& _app);
    ImGui::Begin("Visuals");
    {   

        auto & causticPass = (app->m_renderSystem.causticPass());

        ImGui::Text("Caustic Parameters:");
        ImGui::DragFloat("S Min", &causticPass.m_S_MIN, 0.1f);
        ImGui::DragFloat("S Max", &causticPass.m_S_MAX, 0.1f);
        ImGui::DragFloat("Photon Intensity C", &causticPass.m_photonIntensity, 0.01f);
        ImGui::DragFloat("Water Attenuation", &causticPass.m_waterAttenuation, 0.01f);

    }

    {   

        auto & godrayPass = (app->m_renderSystem.godrayPass());

        ImGui::Text("God Rays Parameters:");
        ImGui::DragFloat("G", &godrayPass.m_g, 0.1f);
        ImGui::DragFloat("Gamma", &godrayPass.m_gamma, 0.1f);
        ImGui::DragFloat("Photon Intensity GD", &godrayPass.m_intensity, 0.1f);
        ImGui::DragFloat("Ray width", &godrayPass.m_rayWidth, 0.1f);

    }
    ImGui::End();
}


