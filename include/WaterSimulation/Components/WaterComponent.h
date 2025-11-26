#pragma once

#include <WaterSimulation/ECS.h>

namespace WaterSimulation
{
	struct WaterComponent
	{
		// vide juste pour l'utiliser comme tag pendant le rendu

		void onAttach(Registry& registry [[maybe_unused]], Entity entity [[maybe_unused]]) {} // [[maybe_unused]] evite warnings
		void onDetach(Registry& registry [[maybe_unused]], Entity entity [[maybe_unused]]) {} 
	};
	
} // namespace WaterSimulation
