#pragma once

#include <WaterSimulation/UIManager.h>
#include <WaterSimulation/Camera.h>

#include <memory>
#include <unordered_set>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Timeline.h>

namespace WaterSimulation {

	class Application : public Magnum::Platform::Sdl2Application {
		public:
			explicit Application(const Arguments& arguments);
			Camera& camera() { return *m_camera; }
			Magnum::ImGuiIntegration::Context & getContext() {return m_imgui;};
			bool cursorLocked() { return m_cursorLocked;};
		private:
			Magnum::Timeline m_timeline;
			float m_deltaTime{};

			std::unique_ptr<UIManager> m_UIManager;
			std::unique_ptr<Camera> m_camera;
			bool m_cursorLocked{false};


			std::unordered_set<Magnum::Platform::Sdl2Application::Key> m_keysPressed;
			void handleCameraInputs();

			Magnum::ImGuiIntegration::Context m_imgui{Magnum::NoCreate};

			void drawEvent() override;

			void viewportEvent(ViewportEvent& event) override;

			void keyPressEvent(KeyEvent& event) override;
			void keyReleaseEvent(KeyEvent& event) override;

			void pointerPressEvent(PointerEvent& event) override;
			void pointerReleaseEvent(PointerEvent& event) override;
			void pointerMoveEvent(PointerMoveEvent& event) override;
			void scrollEvent(ScrollEvent& event) override;
			void textInputEvent(TextInputEvent& event) override;


	};

} // namespace WaterSimulation
