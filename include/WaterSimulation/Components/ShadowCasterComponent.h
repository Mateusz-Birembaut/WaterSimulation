#pragma once

#include <Magnum/Math/Vector2.h>

namespace WaterSimulation
{
	struct ShadowCasterComponent
	{
		Magnum::Vector2 projectionSize{100.0f, 100.0f};
		float near = 0.1f;
		float far = 1000.0f;

		void onAttach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
    	void onDetach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
	};
	
} // namespace WaterSimulation
