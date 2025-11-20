#pragma once

#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/ImGuiIntegration/Context.hpp>

namespace WaterSimulation {
    class UIManager {
    public:
        void drawUI(Magnum::Platform::Sdl2Application & _app, Magnum::ImGuiIntegration::Context& _context);

    private:
        void paramWindow(Magnum::Platform::Sdl2Application & _app);
    };
} // namespace WaterSimulation