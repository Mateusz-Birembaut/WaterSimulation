#pragma once

#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Rendering/CustomShader/DepthShader.h>
#include <WaterSimulation/Rendering/CustomShader/OpaqueShader.h>

#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector2.h>

namespace WaterSimulation {
	class OpaquePass {
	      public:
		  
		void init(const Magnum::Vector2i& windowSize);
		void resize(const Magnum::Vector2i& windowSize);

		void render(Registry& registry,
			    const Magnum::Matrix4& viewMatrix,
			    const Magnum::Matrix4& projMatrix,
			    const Magnum::Matrix4& lightVP,
			    Magnum::GL::Texture2D& shadowMap);

		void recreateTextures(const Magnum::Vector2i& windowSize);
		Magnum::GL::Texture2D& getDepthTexture() {
			return m_depthTexture;
		}
		Magnum::GL::Texture2D& getColorTexture() {
			return m_colorTexture;
		}

	      private:
		Magnum::GL::Framebuffer m_fb{Magnum::NoCreate};
		Magnum::GL::Texture2D m_depthTexture{Magnum::NoCreate}; // pour refraction ?
		Magnum::GL::Texture2D m_colorTexture{Magnum::NoCreate};

		OpaqueShader m_opaqueShader{};
	};

} // namespace WaterSimulation