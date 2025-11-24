#pragma once

#include <WaterSimulation/ECS.h>
#include <Magnum/Math/Vector3.h>

namespace WaterSimulation
{
	struct DirectionalLightComponent
	{
		Magnum::Vector3 direction{1.0f, 1.0f, 1.0f};
		float intensity = 1.0f;

		void onAttach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
    	void onDetach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
	};
	
} // namespace WaterSimulation
