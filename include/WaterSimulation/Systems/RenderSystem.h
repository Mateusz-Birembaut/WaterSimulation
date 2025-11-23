// RenderSystem.h
#pragma once

#include "Magnum/Magnum.h"
#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Components/MeshComponent.h>

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/Math/Matrix4.h>

namespace WaterSimulation {

	class RenderSystem {
	      public:
		void render(Registry& registry,
			    const Magnum::Matrix4& viewMatrix,
			    const Magnum::Matrix4& projectionMatrix);

	      private:
		void renderMesh(MeshComponent& meshComp,
				const Magnum::Matrix4& viewProj);
	};

	
	class DisplayShader : public Magnum::GL::AbstractShaderProgram {
	public:

		typedef Magnum::GL::Attribute<0, Magnum::Vector3> Position;
        typedef Magnum::GL::Attribute<1, Magnum::Vector2> TextureCoordinates;
		typedef Magnum::GL::Attribute<2, Magnum::Vector3> Normal;


		DisplayShader() = default;
		explicit DisplayShader(const char * vert, const char * frag );
		DisplayShader& bind(Magnum::GL::Texture2D * tex, int location);
		DisplayShader& setTransformationMatrix(const Magnum::Matrix4 & m);
	};

} // namespace WaterSimulation