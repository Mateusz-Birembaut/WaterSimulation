#pragma once

#include <WaterSimulation/Camera.h>
#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/ECS.h>
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
		void init(const Magnum::Vector2i& windowSize);
		void resize(const Magnum::Vector2i& windowSize);

		void render(Registry& registry,
			    Camera& camera,
			    Magnum::GL::Texture2D& shadowMap,
			    Magnum::GL::Texture2D& waterMask,
			    Magnum::GL::Texture2D& terrainHeightMap,
			    Magnum::Vector3& lightPosition,
			    Magnum::Matrix4& lightViewProj);

		void recreateTextures(const Magnum::Vector2i& windowSize);

		Magnum::GL::Texture2D& getCausticTexture() {
			return m_causticMap;
		}

		void setupPhotonGrid();

		CausticPass() {setupPhotonGrid();}

	      private:
		Magnum::GL::Framebuffer m_fb{Magnum::NoCreate};
		Magnum::GL::Texture2D m_causticMap{Magnum::NoCreate};

		Magnum::GL::Buffer m_photonBuffer{Magnum::NoCreate};
		Magnum::GL::Mesh m_photonGrid{Magnum::NoCreate}; // grille de vertices entre -1 et 1

		CausticShader m_causticShader;


	};

} // namespace WaterSimulation
