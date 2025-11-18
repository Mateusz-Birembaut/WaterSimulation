#pragma once

#include <Magnum/Platform/Sdl2Application.h>

namespace WaterSimulation {

	class Application : public Magnum::Platform::Application {
		public:
			explicit Application(const Arguments& arguments);

		private:
			void drawEvent() override;
			void viewportEvent(ViewportEvent& event) override;
	};

} // namespace WaterSimulation
