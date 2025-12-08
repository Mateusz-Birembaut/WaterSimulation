#pragma once

#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Mesh.h>

#include <Magnum/Math/Vector3.h>

#include <vector>

namespace WaterSimulation
{	
	struct BuoyancyComponent
	{
		
		float flotability{};
		float waterDrag = 0.1f; 
		//float angularDrag = 0.5f;

		void onAttach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
    	void onDetach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};


	};
	
} // namespace WaterSimulation
