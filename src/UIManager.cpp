

#include <WaterSimulation/Camera.h>
#include <WaterSimulation/ShallowWater.h>
#include <WaterSimulation/UIManager.h>
#include <WaterSimulation/ECS.h>

#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/DirectionalLightComponent.h>
#include <WaterSimulation/Components/ShadowCasterComponent.h>
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
    visualWindow(app);
    sunWindow(app.registry());

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

        ImGui::Begin("Simulation Parameters");

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0 / Double(ImGui::GetIO().Framerate),
                    Double(ImGui::GetIO().Framerate));

        if (ImGui::Button(app->simulationPaused ? "Resume" : "Pause")) {
            app->simulationPaused = !app->simulationPaused;
        }
        ImGui::SameLine();
        if (ImGui::Button("Step")) {
            simulation->step();
        }

        /* if (ImGui::Button("Init Dam Break")) {
            simulation->initDamBreak();
        }
        ImGui::SameLine();
        if (ImGui::Button("Init Bump")) {
            simulation->initBump();
        }
        ImGui::SameLine();
        if (ImGui::Button("Init Tsunami")) {
            simulation->initTsunami();
        } */
        ImGui::SameLine();
        if (ImGui::Button("Clear All Water")) {
            simulation->initEmpty();
        }
        ImGui::Text("Create Water");
        static ImVec2 position(256.0f, 256.0f);
        static float radius = 1.0f;
        static float quantity = 1.0f;
        static bool fill = false;

        ImGui::Checkbox("Put water everywhere",&fill);
        

        if(!fill){
            ImGui::InputFloat2("Position", &position.x);
            ImGui::SliderFloat("Radius", &radius, 0.1f, 50.0f, "%.2f");
            ImGui::SliderFloat("Quantity", &quantity, 0.1f, 1.0f, "%.2f");
            
            if (ImGui::Button("Create (you can hold this button)") || ImGui::IsItemActive()) {
                simulation->createWater(position.x, position.y, radius, quantity);
            }  
        }else{
            ImGui::SliderFloat("Quantity", &quantity, 0.1f, 100.0f, "%.2f");
            if (ImGui::Button("Fill")) {
                simulation->createWater(.0f,.0f,100000, quantity);
            }  
        }
        
        ImGui::Separator();
        ImGui::Text("Send a Wave");
        static int waveSide = 0;  // 0=bottom, 1=top, 2=left, 3=right
        static float waveWidth = 20.0f;
        static float waveWallQuantity = 2.0f;
        
        const char* sideNames[] = { "Bottom (Y=0)", "Top (Y=max)", "Left (X=0)", "Right (X=max)" };
        ImGui::Combo("Wave Side", &waveSide, sideNames, 4);
        ImGui::SliderFloat("Wave Width", &waveWidth, 5.0f, 100.0f, "%.1f");
        ImGui::SliderFloat("Wave Height", &waveWallQuantity, 0.1f, 10.0f, "%.2f");
        
        if (ImGui::Button("Send Wave Wall")) {
            simulation->sendWaveWall(waveSide, waveWidth, waveWallQuantity);
        }
        ImGui::Separator();
        
        ImGui::Checkbox("Airy Waves Enabled", &simulation->airyWavesEnabled);
        ImGui::InputInt("Step Number", &(app->step_number), 1, 10);

        ImGui::Separator();
        ImGui::Text("Base Parameters");
        
        ImGui::SliderFloat("Gravity", &simulation->gravity, 0.1f, 100.0f, "%.2f");
        ImGui::SliderFloat("Dry Epsilon (Threshold when a cell is considered dry)", &simulation->dryEps, 1e-6f, 0.1f, "%.6f", ImGuiSliderFlags_Logarithmic);
        
        ImGui::Separator();
        ImGui::Text("Airy Waves Parameters");
        
        ImGui::SliderFloat("Decomposition D (The higher it is, the less airy waves we have) ", &simulation->decompositionD, 0.000001f, 1.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderInt("Diffusion Iterations", &simulation->diffusionIterations, 1, 512);
        //ImGui::SliderFloat("Airy h_bar", &simulation->airyHBar, 0.1f, 20.0f, "%.2f");
        //ImGui::SliderFloat("Transport Gamma", &simulation->transportGamma, 0.0f, 1.0f, "%.3f");


        if (ImGui::Button("Load Mountain")) {
            loadMap("mountain.jpg", 3, 30.0f, simulation);
            app->createTerrain(200.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Volcano")) {
            loadMap("volcano.png", 3, 20.0f, simulation);
            app->createTerrain(200.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Ridge")) {
            loadMap("ridge.png", 3, 20.0f, simulation);
            app->createTerrain(200.0f); 
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Canyon")) {
            loadMap("canyon.png", 3, 20.0f, simulation);
            app->createTerrain(200.0f);
        }

        ImGui::End();
    }

    // Visualization window
    {
        ShallowWater *simulation = &(app->shallowWaterSimulation());
        
        ImGui::Begin("Algorithm Visualization");
        
        ImVec2 texSize(512, 512);

        if (ImGui::CollapsingHeader("1. Input State", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("State");
            ImGui::SameLine(100);
            ImGui::Text("Terrain");
            
            ImGui::Image(
                reinterpret_cast<void *>(simulation->getStateTexture().id()),
                texSize);
            ImGui::SameLine();
            ImGui::Image(
                reinterpret_cast<void *>(simulation->getTerrainTexture().id()),
                texSize);
        }

        if (ImGui::CollapsingHeader("2. Decomposition", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Bulk Flow");
            ImGui::SameLine(100);
            ImGui::Text("Surface Height");
            ImGui::SameLine(220);
            ImGui::Text("Surface Qx");
            ImGui::SameLine(340);
            ImGui::Text("Surface Qy");

            ImGui::Image(
                reinterpret_cast<void *>(simulation->getBulkTexture().id()),
                texSize);
            ImGui::SameLine();
            ImGui::Image(
                reinterpret_cast<void *>(simulation->getSurfaceHeightTexture().id()),
                texSize);
            ImGui::SameLine();
            ImGui::Image(
                reinterpret_cast<void *>(simulation->getSurfaceQxTexture().id()),
                texSize);
            ImGui::SameLine();
            ImGui::Image(
                reinterpret_cast<void *>(simulation->getSurfaceQyTexture().id()),
                texSize);
        }

        if (ImGui::CollapsingHeader("3. Shallow Water (Bulk)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Bulk Updated");
            ImGui::Image(
                reinterpret_cast<void *>(simulation->getVisBulkUpdated().id()),
                texSize);
        }

        if (ImGui::CollapsingHeader("4. FFT (On Airy Waves Quantities)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("FFT Height");
            ImGui::SameLine(100);
            ImGui::Text("FFT Qx");
            ImGui::SameLine(220);
            ImGui::Text("FFT Qy");

            ImGui::Image(
                reinterpret_cast<void *>(simulation->getVisFFTHeight().id()),
                texSize);
            ImGui::SameLine();
            ImGui::Image(
                reinterpret_cast<void *>(simulation->getVisFFTQx().id()),
                texSize);
            ImGui::SameLine();
            ImGui::Image(
                reinterpret_cast<void *>(simulation->getVisFFTQy().id()),
                texSize);
        }

        if (ImGui::CollapsingHeader("5. IFFT (Back to Spatial)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("IFFT Height");
            ImGui::SameLine(100);
            ImGui::Text("IFFT Qx");
            ImGui::SameLine(220);
            ImGui::Text("IFFT Qy");

            ImGui::Image(
                reinterpret_cast<void *>(simulation->getVisIFFTHeight().id()),
                texSize);
            ImGui::SameLine();
            ImGui::Image(
                reinterpret_cast<void *>(simulation->getVisIFFTQx().id()),
                texSize);
            ImGui::SameLine();
            ImGui::Image(
                reinterpret_cast<void *>(simulation->getVisIFFTQy().id()),
                texSize);
        }

        if (ImGui::CollapsingHeader("6. Surface Transport", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Transported Flow");
            ImGui::SameLine(150);
            ImGui::Text("Transported Height");
            ImGui::SameLine(320);
            ImGui::Text("Advected Surface");

            ImGui::Image(
                reinterpret_cast<void *>(simulation->getVisTransportedFlow().id()),
                texSize);
            ImGui::SameLine();
            ImGui::Image(
                reinterpret_cast<void *>(simulation->getVisTransportedHeight().id()),
                texSize);
            ImGui::SameLine();
            ImGui::Image(
                reinterpret_cast<void *>(simulation->getVisAdvectedHeight().id()),
                texSize);
        }

        ImGui::End();
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

        auto sunView = registry.view<DirectionalLightComponent, ShadowCasterComponent>();
        if (sunView.begin() != sunView.end()) {

            Entity sunEntity = *sunView.begin();

            auto& sun = sunView.get<DirectionalLightComponent>(sunEntity);
            auto& shadowCastData = sunView.get<ShadowCasterComponent>(sunEntity);


            ImGui::Text("Color:");
            float color[3] = {sun.color.r(), sun.color.g(), sun.color.b()};
            if (ImGui::ColorEdit3("Sun Color", color)) {
                sun.color = Magnum::Color3{color[0], color[1], color[2]};
            }
            float intensity = sun.intensity;
            if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f)) {
                sun.intensity = intensity;
            }

            ImGui::Text("Direction:");
            if (ImGui::DragFloat3("Direction", sun.direction.data(), 0.01f, -1.0f, 1.0f)) {
                sun.direction = sun.direction.normalized();
            }
            ImGui::DragFloat("Offset", &sun.offset, 0.1f, 0.1f, 10000.0f);
            

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

    auto& causticPass = app->m_renderSystem.causticPass();
    ImGui::Text("Caustic Parameters:");
    ImGui::DragFloat("S Min", &causticPass.m_S_MIN, 0.1f);
    ImGui::DragFloat("S Max", &causticPass.m_S_MAX, 0.1f);
    ImGui::DragFloat("Photon Intensity C", &causticPass.m_photonIntensity, 0.01f);
    ImGui::DragFloat("Water Attenuation", &causticPass.m_waterAttenuation, 0.01f);
    ImGui::DragFloat("Floor Offset", &causticPass.m_floorOffset, 0.01f);
    ImGui::Checkbox("Blur Caustics", &causticPass.m_enableBlur);
    ImGui::SliderFloat("Caustics Blur Radius", &causticPass.m_blurRadius, 0.0f, 10.0f);

    auto& godrayPass = app->m_renderSystem.godrayPass();
    ImGui::Separator();
    ImGui::Text("God Rays Parameters:");
    ImGui::DragFloat("G", &godrayPass.m_g, 0.005f);
    ImGui::DragFloat("Gamma", &godrayPass.m_gamma, 0.005f);
    ImGui::DragFloat("Photon Intensity GD", &godrayPass.m_intensity, 0.1f);
    ImGui::DragFloat("Ray width", &godrayPass.m_rayWidth, 0.1f);
    ImGui::Checkbox("Blur God Rays", &godrayPass.m_enableBlur);
    ImGui::SliderFloat("God Rays Blur Radius", &godrayPass.m_blurRadius, 0.0f, 10.0f);

    ImGui::End();
}


void WaterSimulation::UIManager::loadMap(const char * filename, int channels, float scaling, ShallowWater *simulation){

    int nx = simulation->getnx();
    int ny = simulation->getny();

    Corrade::Utility::Resource rs{"WaterSimulationResources"};

    converter->configuration().setValue("size", std::to_string(nx+1) + " " + std::to_string(ny+1));

    auto heightmapData = rs.getRaw(filename);
    importer->openData(heightmapData);

    auto image = importer->image2D(0);
    auto resized = converter->convert(*image);
    auto allo = resized->format();
    Debug{} << "FORMAT IS : " << allo;
    Debug{} << "SIZE IS : " << resized->size();

    simulation->loadTerrainHeightMap(&*resized, scaling, channels);

}