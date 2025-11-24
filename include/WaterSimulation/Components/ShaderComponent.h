#pragma once

#include <WaterSimulation/Rendering/IShader.h>

#include <memory>

namespace WaterSimulation
{
	struct ShaderComponent
	{
		std::shared_ptr<IShader> shaderPtr;

		void onAttach(Registry& registry [[maybe_unused]], Entity entity [[maybe_unused]]) {} // [[maybe_unused]] evite warnings
		void onDetach(Registry& registry [[maybe_unused]], Entity entity [[maybe_unused]]) {} 
	};
	
	
} // namespace WaterSimulation
