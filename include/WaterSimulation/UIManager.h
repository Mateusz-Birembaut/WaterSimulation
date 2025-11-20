#pragma once

#include <WaterSimulation/Camera.h>

#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/ImGuiIntegration/Context.hpp>

namespace WaterSimulation {
    class Application;

    class UIManager {
    public:
        void drawUI(Application& app);

    private:
        void perfWindow();
        void sceneGraph();
        void cameraWindow(Camera & cam);
    };
} // namespace WaterSimulation