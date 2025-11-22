// RenderSystem.h
#pragma once

#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Rendering/DepthPass.h>
#include <WaterSimulation/Rendering/CustomShader/DepthDebugShader.h>
#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Camera.h>

#include <Magnum/GL/Shader.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/Math/Matrix4.h>

#include <Magnum/Primitives/Plane.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Buffer.h>


namespace WaterSimulation {

	class RenderSystem {
		public:
			void init(const Magnum::Vector2i& windowSize){
				m_depthPass.init(windowSize);
			}

			void resize(const Magnum::Vector2i& windowSize){
				m_depthPass.resize(windowSize);
			}

			void render(Registry & registry, Camera & cam);

		private:
			DepthPass m_depthPass;
			DepthDebugShader m_depthDebugShader;
			bool m_renderDepth{true};

			Magnum::GL::Mesh m_fullscreenTriangle{createFullscreenTriangle()};

			Magnum::GL::Mesh createFullscreenTriangle()
			{
				using namespace Magnum;

				struct Vertex {
					Vector2 position;
					Vector2 uv;
				};

				constexpr Vertex vertices[]{
					{{-1.0f, -1.0f}, {0.0f, 0.0f}},   
					{{ 3.0f, -1.0f}, {2.0f, 0.0f}},   
					{{-1.0f,  3.0f}, {0.0f, 2.0f}}   
				};

				GL::Buffer vertexBuffer;
				vertexBuffer.setData(vertices);

				GL::Mesh mesh;
				mesh.setPrimitive(GL::MeshPrimitive::Triangles)
					.setCount(3)
					.addVertexBuffer(
						std::move(vertexBuffer),
						0,
						GL::Attribute<0, Vector2>{},      // position
						GL::Attribute<1, Vector2>{}       // uv
					);

				return mesh;
			}

	};


} // namespace WaterSimulation