#pragma once



#include "Corrade/Tags.h"
#include <WaterSimulation/UIManager.h>
#include <WaterSimulation/Camera.h>
#include <WaterSimulation/Mesh.h>
#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Systems/RenderSystem.h>
#include <WaterSimulation/Systems/TransformSystem.h>
#include <WaterSimulation/Systems/PhysicsSystem.h>
#include <WaterSimulation/Rendering/HeightmapReadback.h>
#include <WaterSimulation/Rendering/CustomShader/PBRShader.h>

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/GL/GL.h>
#include <Magnum/GL/Texture.h>
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
			int step_number = 1; //number of shallow water steps for a single time step, increasing this increases water speed

			ShallowWater& shallowWaterSimulation() { return m_shallowWaterSimulation; }

			Registry & registry(){ return m_registry; };

			Entity createSphereEntity(const Magnum::Vector3& position, float radius = 1.0f, float mass = 600.0f, float flotability = 300.0f, float waterDrag = 30.0f, std::shared_ptr<Magnum::GL::Texture2D> albedo = nullptr, std::shared_ptr<Magnum::GL::Texture2D> arm = nullptr);
			Entity spawnSphereAt(const Magnum::Vector3& position, const Magnum::Vector3& impulseDir = Magnum::Vector3{0.0f});

			Entity createTerrain(float scale);

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
			PhysicsSystem m_physicSystem;
			
			std::unique_ptr<Mesh> m_testMesh;
			Magnum::GL::Texture2D m_testAlbedo;
			std::unique_ptr<Mesh> m_waterMesh;
			std::unique_ptr<Mesh> m_terrainMesh;

			HeightmapReadback m_heightmapReadback;

			ShallowWater m_shallowWaterSimulation; // simulation de l'eau
			Magnum::GL::Texture2D m_heightTexture; // carte des hauteurs de l'eau, affiché dans imgui
			Magnum::GL::Texture2D m_momentumTexture; // carte des velocités u ou des q
			Magnum::GL::Texture2D m_terrainHeightmap; // heightmap du terrain
			Magnum::Shaders::FlatGL3D m_testFlatShader{Magnum::NoCreate};
			
			DisplayShader debugShader; //
			std::shared_ptr<PBRShader> m_pbrShader;

			// Preloaded textures for metal spheres
			std::shared_ptr<Magnum::GL::Texture2D> m_metalAlbedo;
			std::shared_ptr<Magnum::GL::Texture2D> m_metalArm;
			std::shared_ptr<Magnum::GL::Texture2D> m_metalNormal;

			bool m_nextHeavyShot{true};

			std::unordered_set<Magnum::Platform::Sdl2Application::Key> m_keysPressed;
			void handleCameraInputs();

			Magnum::ImGuiIntegration::Context m_imgui{Magnum::NoCreate};

			void drawEvent() override;

			std::shared_ptr<Magnum::GL::Texture2D> loadTex(const std::string& path);
			std::shared_ptr<Magnum::GL::Texture2D> loadTexCached(const std::string& path);
			std::unordered_map<std::string, std::weak_ptr<Magnum::GL::Texture2D>> m_textureCache;

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
