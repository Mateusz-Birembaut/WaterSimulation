#pragma once

#include <WaterSimulation/ECS.h>
#include <Magnum/Math/Vector3.h>

namespace WaterSimulation
{
	struct DirectionalLightComponent
	{
		Magnum::Vector3 direction{Magnum::Vector3(-0.1f, -0.9f, -0.0f).normalized()};
		float offset = 50.0f;

		void onAttach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
    	void onDetach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
	};
	
} // namespace WaterSimulation
