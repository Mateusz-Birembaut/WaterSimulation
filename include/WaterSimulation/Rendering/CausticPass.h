#pragma once

#include <WaterSimulation/Camera.h>
#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Rendering/CausticUtil.h>
#include <WaterSimulation/Rendering/CustomShader/CausticShader.h>
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
	class CausticPass {

	      public:

		float m_S_MIN = 2.0f;
		float m_S_MAX = 20.0f;
		float m_photonIntensity = 0.09f;
		float m_waterAttenuation = 0.13f;
		float m_blurRadius = 3.25f;

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

		Magnum::GL::Texture2D& getCausticTexture() {
			return m_causticMap;
		}

		void setupPhotonGrid();

		CausticPass() {
			setupPhotonGrid();
		}

	      private:
		Magnum::GL::Framebuffer m_fb{Magnum::NoCreate};
		Magnum::GL::Texture2D m_causticMap{Magnum::NoCreate};

		Magnum::GL::Buffer m_photonBuffer{Magnum::NoCreate};
		Magnum::GL::Mesh m_photonGrid{Magnum::NoCreate};

		CausticShader m_causticShader;

		CausticUtil m_utils;


	};
} // namespace WaterSimulation
