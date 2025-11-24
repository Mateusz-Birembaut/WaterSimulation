#pragma once

#include <Magnum/Math/Vector2.h>

namespace WaterSimulation
{
	struct ShadowCasterComponent
	{
		Magnum::Vector2 projectionSize{256.0f, 256.0f};
		float zNear = 1.0f;
		float zFar = 500.0f;

		void onAttach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
    	void onDetach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
	};
	
} // namespace WaterSimulation
