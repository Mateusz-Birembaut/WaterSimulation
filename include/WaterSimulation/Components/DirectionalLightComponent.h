#pragma once

#include <WaterSimulation/ECS.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Color.h>

namespace WaterSimulation
{
	struct DirectionalLightComponent
	{
		Magnum::Color3 color{1.0f, 1.0f, 1.0f};
		float intensity = 1.0f;
		Magnum::Vector3 direction{Magnum::Vector3(-0.1f, -0.9f, -0.0f).normalized()};
		float offset = 100.0f;

		void onAttach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
    	void onDetach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
	};
	
} // namespace WaterSimulation
