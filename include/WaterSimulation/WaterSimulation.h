#pragma once



#include "Corrade/Tags.h"
#include <WaterSimulation/UIManager.h>
#include <WaterSimulation/Camera.h>
#include <WaterSimulation/Mesh.h>
#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Systems/RenderSystem.h>
#include <WaterSimulation/Systems/TransformSystem.h>


#include <memory>
#include <unordered_set>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/GL/GL.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Timeline.h>
#include <Magnum/Trade/ImageData.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <WaterSimulation/ShallowWater.h>
#include <WaterSimulation/Systems/RenderSystem.h>


namespace WaterSimulation {

	class Application : public Magnum::Platform::Sdl2Application {
		public:
			explicit Application(const Arguments& arguments);
			~Application();

			Camera& camera() { return *m_camera; }
			Magnum::ImGuiIntegration::Context & getContext() {return m_imgui;};
			bool cursorLocked() { return m_cursorLocked;};

			bool simulationPaused = false;
			ShallowWater& shallowWaterSimulation() { return m_shallowWaterSimulation; }

			Registry & registry(){ return m_registry; };

			RenderSystem m_renderSystem;

		private:
			Magnum::Timeline m_timeline;
			float m_deltaTime{};

			std::unique_ptr<UIManager> m_UIManager;
			std::unique_ptr<Camera> m_camera;
			bool m_cursorLocked{false};

			// ECS registry and systems 
			Registry m_registry;
			TransformSystem m_transform_System;

			
			std::unique_ptr<Mesh> m_testMesh;
			Magnum::GL::Texture2D m_testAlbedo;

			std::unique_ptr<Mesh> m_waterMesh;

			std::unique_ptr<Mesh> m_terrainMesh;
			ShallowWater m_shallowWaterSimulation; // simulation de l'eau
			Magnum::GL::Texture2D m_heightTexture; // carte des hauteurs de l'eau, affiché dans imgui
			Magnum::GL::Texture2D m_momentumTexture; // carte des velocités u ou des q
			Magnum::GL::Texture2D m_terrainHeightmap; // heightmap du terrain
			Magnum::Shaders::FlatGL3D m_testFlatShader{Magnum::NoCreate};
			
			DisplayShader debugShader; // 

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
