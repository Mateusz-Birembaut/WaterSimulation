// RenderSystem.h
#pragma once

#include "Magnum/Magnum.h"
#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Rendering/OpaquePass.h>
#include <WaterSimulation/Rendering/ShadowMapPass.h>
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
				   m_shadowMapPass.init(windowSize);
			}

			void resize(const Magnum::Vector2i& windowSize){
				   m_opaquePass.resize(windowSize);
				   m_shadowMapPass.resize(windowSize);
			}

			void render(Registry & registry, Camera & cam);

			bool m_renderDepthOnly{false};
			bool m_renderShadowMapOnly{false};
			bool m_renderWaterMaskOnly{false};

			float m_linearizeRange = 50.0f; 

		   private:
			   OpaquePass m_opaquePass;
			   ShadowMapPass m_shadowMapPass;

			   FullscreenTextureShader m_fullScreenTextureShader;
			   Magnum::GL::Mesh m_fullscreenTriangle{createFullscreenTriangle()};
			   void drawFullscreenTexture(Magnum::GL::Texture2D& texture, float, float);

			// Shader debug depth
			WaterSimulation::DepthDebugShader m_depthDebugShader;
			void drawFullscreenTextureDebugDepth(Magnum::GL::Texture2D& texture, float near, float far, bool isOrtho);

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