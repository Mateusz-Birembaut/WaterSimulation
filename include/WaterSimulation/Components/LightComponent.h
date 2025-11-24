#pragma once

#include <Magnum/Math/Color.h>

namespace WaterSimulation
{
	struct LightComponent
	{
		Magnum::Color3 color{1.0f, 1.0f, 1.0f};
		float intensity = 1.0f;

		void onAttach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
    	void onDetach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
	};
	
} // namespace WaterSimulation
