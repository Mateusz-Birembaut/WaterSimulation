#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Rendering/CustomShader/FullscreenTextureShader.h>
#pragma once

#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector2.h>

namespace WaterSimulation {

	class CompositionPass {
	public:
		void init(const Magnum::Vector2i& windowSize);
		void resize(const Magnum::Vector2i& windowSize);

		void render(
			Magnum::GL::Texture2D& opaqueColor,
			Magnum::GL::Texture2D& caustics,
			Magnum::GL::Texture2D& godrays,
			Magnum::GL::Texture2D& opaqueDepth,
			Registry& registry,
			const Magnum::Matrix4& viewMatrix,
			const Magnum::Matrix4& projMatrix
		);

		Magnum::GL::Texture2D& getColorTexture() { return m_colorTexture; }

	private:
		Magnum::GL::Mesh createFullscreenTriangle();
		Magnum::GL::Mesh m_fullscreenTriangle{Magnum::NoCreate};
		FullscreenTextureShader m_fullscreenShader;

		Magnum::GL::Framebuffer m_fb{Magnum::NoCreate};
		Magnum::GL::Texture2D m_colorTexture{Magnum::NoCreate};
	};
}
