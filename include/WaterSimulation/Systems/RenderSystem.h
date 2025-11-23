// RenderSystem.h
#pragma once

#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Rendering/OpaquePass.h>
#include <WaterSimulation/Rendering/CustomShader/FullscreenTextureShader.h>
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
				   m_opaquePass.init(windowSize);
			}

			void resize(const Magnum::Vector2i& windowSize){
				   m_opaquePass.resize(windowSize);
			}

			void render(Registry & registry, Camera & cam);

			bool m_renderDepthOnly{false};

		private:
			OpaquePass m_opaquePass;
			FullscreenTextureShader m_fullScreenTextureShader;

			Magnum::GL::Mesh m_fullscreenTriangle{createFullscreenTriangle()};

			void drawFullscreenTexture(Magnum::GL::Texture2D& texture, float, float);

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
						GL::Attribute<0, Vector2>{},      // positions
						GL::Attribute<1, Vector2>{}       // uvs
					);

				return mesh;
			}



	};


} // namespace WaterSimulation