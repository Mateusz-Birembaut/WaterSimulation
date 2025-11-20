// RenderSystem.h
#pragma once

#include <WaterSimulation/ECS.h>
#include <WaterSimulation/MeshComponent.h>

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
				const Magnum::Matrix4& transformation,
				const Magnum::Matrix4& viewProj);
	};

} // namespace WaterSimulation