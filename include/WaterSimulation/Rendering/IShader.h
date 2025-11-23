#pragma once

#include <WaterSimulation/Components/MaterialComponent.h>
#include <WaterSimulation/Components/LightComponent.h>

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Matrix4.h>


namespace WaterSimulation
{
	class IShader : public Magnum::GL::AbstractShaderProgram {
	public:
		virtual ~IShader() = default;
		virtual void draw(Magnum::GL::Mesh& mesh, 
			const Magnum::Matrix4& mvp, 
			MaterialComponent& material, 
			const std::vector<LightComponent>& lights) = 0;
	};
	
} // namespace WaterSimulation
