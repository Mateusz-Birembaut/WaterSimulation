#pragma once

#include <WaterSimulation/Camera.h>
#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Rendering/CausticUtil.h>
#include <WaterSimulation/Rendering/CustomShader/GodRayShader.h>
#include <WaterSimulation/Rendering/CustomShader/DepthShader.h>
#include <WaterSimulation/Rendering/CustomShader/OpaqueShader.h>

#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Shaders/GenericGL.h>

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/ArrayViewStl.h>

namespace WaterSimulation {
	class GodRayPass {

	      public:

		float m_g = 0.930f;
		float m_gamma = 0.105f;
		float m_intensity = 35.0f;
		float m_rayWidth = 0.0f;
		float m_blurRadius = 10.0f;

        bool m_enableBlur = true; // Toggle for blur effect


		void init();

		void render(Registry& registry,
			    Camera& camera,
			    Magnum::GL::Texture2D& shadowMap,
			    Magnum::GL::Texture2D& waterWorldPos,
			    Magnum::GL::Texture2D& opaquePassDepth,
			    Magnum::Vector3& lightPosition,
			    Magnum::Matrix4& lightViewProj,
			    float cameraNear,
			    float cameraFar,
			    float sunFar);

		Magnum::GL::Texture2D& getGodRayTexture() {
			return m_godrayTexture;
		}

		void setupPhotonGrid();

		GodRayPass() {
			setupPhotonGrid();
		}

	      private:
		Magnum::GL::Framebuffer m_fb{Magnum::NoCreate};
		Magnum::GL::Texture2D m_godrayTexture{Magnum::NoCreate};

		Magnum::GL::Buffer m_photonBuffer{Magnum::NoCreate};
		Magnum::GL::Mesh m_photonGrid{Magnum::NoCreate};

		GodRayShader m_godrayShader;

		CausticUtil m_utils;


	};
} // namespace WaterSimulation
