#pragma once

#include "Corrade/PluginManager/Manager.h"
#include "Magnum/GL/GL.h"
#include <WaterSimulation/UIManager.h>

#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Trade/ImageData.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/Trade/AbstractImporter.h>

#include <WaterSimulation/ShallowWater.h>


namespace WaterSimulation {

	class Application : public Magnum::Platform::Sdl2Application {
		public:
			explicit Application(const Arguments& arguments);

			ShallowWater m_shallowWaterSimulation; // simulation de l'eau
			Magnum::GL::Texture2D m_heightTexture; // carte des hauteurs de l'eau, affiché dans imgui
			Magnum::GL::Texture2D m_momentumTexture; // carte des velocités u ou des q
			Magnum::GL::Texture2D m_terrainHeightmap; // heightmap du terrain


			



		private:
			UIManager m_UIManager{};
			
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
