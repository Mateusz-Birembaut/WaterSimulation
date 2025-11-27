#pragma once

#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Shaders/GenericGL.h>

namespace WaterSimulation {
	class CausticShader : public Magnum::GL::AbstractShaderProgram {

	      private:
		Magnum::Int m_uInvLightViewProjLoc;
		Magnum::Int m_uLightViewProjLoc;

		Magnum::Int m_uCameraViewProjLoc;
		Magnum::Int m_uCameraPosLoc;

		Magnum::Int m_uShadowMapSamplerLoc;

		Magnum::Int m_uWaterHeightSamplerLoc;
		Magnum::Int m_uWaterMaskSamplerLoc;
		Magnum::Int m_uWorldPosToWaterUVLoc;
		Magnum::Int m_uWaterSizeLoc;

		Magnum::Int m_uTerrainHeightSamplerLoc;


		//Magnum::Int m_uLightPosLoc;
		//Magnum::Int m_uDepthMapSamplerLoc;

	      public:
		explicit CausticShader() {
			Corrade::Utility::Resource rs{"WaterSimulationResources"};

			Magnum::GL::Shader vert{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Vertex};
			Magnum::GL::Shader frag{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Fragment};
			Magnum::GL::Shader geom{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Geometry};

			vert.addSource(Corrade::Containers::StringView{rs.getString("caustic.vs")});
			frag.addSource(Corrade::Containers::StringView{rs.getString("caustic.fs")});
			geom.addSource(Corrade::Containers::StringView{rs.getString("caustic.geom")});

			if (!vert.compile()) {
				Corrade::Utility::Error{} << "causticShader: vertex shader compilation failed:\n";
			}
 			if (!geom.compile()) {
				Corrade::Utility::Error{} << "causticShader: geom shader compilation failed:\n";
			} 
			if (!frag.compile()) {
				Corrade::Utility::Error{} << "causticShader: fragment shader compilation failed:\n";
			}

			CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
			CORRADE_INTERNAL_ASSERT_OUTPUT(geom.compile());
			CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

			attachShaders({vert, geom, frag});

			bindAttributeLocation(Magnum::Shaders::GenericGL3D::Position::Location, "position");

			CORRADE_INTERNAL_ASSERT_OUTPUT(link());

			m_uInvLightViewProjLoc = uniformLocation("uInvVPLight");
			m_uLightViewProjLoc = uniformLocation("uVPLight");

			m_uCameraViewProjLoc = uniformLocation("uVPCamera");
			m_uCameraPosLoc = uniformLocation("uCamPos");

			m_uWaterHeightSamplerLoc = uniformLocation("uHeightWater");
			m_uWaterMaskSamplerLoc = uniformLocation("uWaterMask");
			m_uWorldPosToWaterUVLoc = uniformLocation("uMatrixWorldPosToWaterUV");
			m_uWaterSizeLoc = uniformLocation("uWaterSize");

			m_uShadowMapSamplerLoc = uniformLocation("uShadowMap");

			m_uTerrainHeightSamplerLoc = uniformLocation("uTerrainHeightMap");


			//m_uDepthMapSamplerLoc = uniformLocation("uDepthMap");
			//m_uLightPosLoc = uniformLocation("uLightPos");

		}

		CausticShader& setInvVPLight(const Magnum::Matrix4& invVPLight) {
			setUniform(m_uInvLightViewProjLoc, invVPLight);
			return *this;
		}

		CausticShader& setVPLight(const Magnum::Matrix4& VPLight) {
			setUniform(m_uLightViewProjLoc, VPLight);
			return *this;
		}

		CausticShader& setVPCamera(const Magnum::Matrix4& uVPCamera) {
			setUniform(m_uCameraViewProjLoc, uVPCamera);
			return *this;
		}

		CausticShader& setCameraPos(const Magnum::Vector3& camPos) {
			setUniform(m_uCameraPosLoc, camPos);
			return *this;
		}
/* 
		CausticShader& setLightPos(const Magnum::Vector3& lightPos) {
			setUniform(m_uLightPosLoc, lightPos);
			return *this;
		}
 */
		CausticShader& bindWaterHeightTexture(Magnum::GL::Texture2D& waterHeight) {
			waterHeight.bind(0);
			setUniform(m_uWaterHeightSamplerLoc, 0);
			return *this;
		}

		CausticShader& bindWaterMaskTexture(Magnum::GL::Texture2D& waterMask) {
			waterMask.bind(1);
			setUniform(m_uWaterMaskSamplerLoc, 1);
			return *this;
		}

		CausticShader& bindShadowMapTexture(Magnum::GL::Texture2D& shadowMap) {
			shadowMap.bind(2);
			setUniform(m_uShadowMapSamplerLoc, 2);
			return *this;
		}

		CausticShader& bindTerrainHeightMapTexture(Magnum::GL::Texture2D& terrainHeightMap) {
			terrainHeightMap.bind(3);
			setUniform(m_uTerrainHeightSamplerLoc, 3);
			return *this;
		} 

		CausticShader& setMatrixWorldPosToWaterUV(const Magnum::Matrix4& inverseGlobalModel){
			setUniform(m_uWorldPosToWaterUVLoc, inverseGlobalModel);
			return *this;
		}

		CausticShader& setWaterSize(float size){
			setUniform(m_uWaterSizeLoc, size);
			return *this;
		}

		/* 		
		CausticShader& bindDepthMapTexture(Magnum::GL::Texture2D& depthMap) {
			depthMap.bind(3);
			setUniform(m_uDepthMapSamplerLoc, 3);
			return *this;
		} 
		*/
	};

} // namespace WaterSimulation
