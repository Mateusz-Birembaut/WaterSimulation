#pragma once

#include <WaterSimulation/Components/MaterialComponent.h>
#include <WaterSimulation/Components/DirectionalLightComponent.h>

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Vector3.h>


namespace WaterSimulation
{
	class IShader : public Magnum::GL::AbstractShaderProgram {
	public:
		virtual ~IShader() = default;

		virtual void draw(  Magnum::GL::Mesh& mesh, 
			    const Magnum::Matrix4& model,
			    const Magnum::Matrix4& mvp, 
			    MaterialComponent& material, 
			    Magnum::Matrix4 lightVP,
			    Magnum::GL::Texture2D & shadowMap,
			    DirectionalLightComponent& sunLight,
				Magnum::Vector3 camPos) = 0;
	};
	
} // namespace WaterSimulation
