#pragma once

#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Rendering/CustomShader/DepthShader.h>
#include <WaterSimulation/Rendering/CustomShader/DepthDebugShader.h>
#include<WaterSimulation/Camera.h>

#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/Texture.h>


namespace WaterSimulation {
	class ShadowMapPass
	{
		public:
			void init(const Magnum::Vector2i& windowSize);
			void resize(const Magnum::Vector2i& windowSize);
			void render(Registry& registry, Camera& mainCamera, Magnum::Matrix4 & lightViewProj);

			Magnum::GL::Texture2D& getDepthTexture() { return m_depthTexture; }
			Magnum::GL::Texture2D& getColorTexture() { return m_colorTexture; }

		private:
			Magnum::GL::Framebuffer m_fb{Magnum::NoCreate};
			Magnum::GL::Texture2D m_depthTexture{Magnum::NoCreate};
			Magnum::GL::Texture2D m_colorTexture{Magnum::NoCreate};
			
			void recreateTextures(const Magnum::Vector2i& windowSize);
			void renderDepth(Registry& registry, const Magnum::Matrix4& viewProj);
			void rendeWaterMask(Registry& registry, const Magnum::Matrix4& viewProj);

			DepthShader m_depthShader{};
			
	};
	
}