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
	class GodRayShader : public Magnum::GL::AbstractShaderProgram {

	      private:
		Magnum::Int m_uLightViewProjLoc;
		Magnum::Int m_uInvLightViewProjLoc;
		Magnum::Int m_uCameraViewProjLoc;
		Magnum::Int m_uCameraPosLoc;
		Magnum::Int m_uShadowMapSamplerLoc;
		Magnum::Int m_uWaterMaskSamplerLoc;
		Magnum::Int m_uCamDepthBufferLoc;
		Magnum::Int m_uTimeLoc;
		Magnum::Int m_uLightPosLoc;
		Magnum::Int m_uIntensityLoc;
		Magnum::Int m_uGLoc;
		Magnum::Int m_uFogDensityLoc;
		Magnum::Int m_uLightFarLoc;
		Magnum::Int m_uRayWidthLoc;

	      public:
		explicit GodRayShader() {
			Corrade::Utility::Resource rs{"WaterSimulationResources"};

			Magnum::GL::Shader vert{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Vertex};
			Magnum::GL::Shader frag{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Fragment};
			Magnum::GL::Shader geom{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Geometry};

			vert.addSource(Corrade::Containers::StringView{rs.getString("godray.vs")});
			frag.addSource(Corrade::Containers::StringView{rs.getString("godray.fs")});
			geom.addSource(Corrade::Containers::StringView{rs.getString("godray.geom")});

			if (!vert.compile()) {
				Corrade::Utility::Error{} << "godray: vertex shader compilation failed:\n";
			}
			if (!geom.compile()) {
				Corrade::Utility::Error{} << "godray: geom shader compilation failed:\n";
			}
			if (!frag.compile()) {
				Corrade::Utility::Error{} << "godray: fragment shader compilation failed:\n";
			}

			CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
			CORRADE_INTERNAL_ASSERT_OUTPUT(geom.compile());
			CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

			attachShaders({vert, geom, frag});

			bindAttributeLocation(Magnum::Shaders::GenericGL3D::Position::Location, "position");

			CORRADE_INTERNAL_ASSERT_OUTPUT(link());

			m_uLightViewProjLoc = uniformLocation("uVPLight");

			m_uInvLightViewProjLoc = uniformLocation("uInvVPLight");

			m_uCameraViewProjLoc = uniformLocation("uVPCamera");
			m_uCameraPosLoc = uniformLocation("uCamPos");

			m_uWaterMaskSamplerLoc = uniformLocation("uWaterMask");

			m_uShadowMapSamplerLoc = uniformLocation("uShadowMap");

			m_uCamDepthBufferLoc = uniformLocation("uCamDepthBuffer");

			m_uTimeLoc = uniformLocation("uTime");

			m_uLightPosLoc = uniformLocation("uLightPos");

			m_uIntensityLoc = uniformLocation("uIntensity");

			m_uGLoc = uniformLocation("uG");

			m_uFogDensityLoc = uniformLocation("uGamma");

			m_uLightFarLoc = uniformLocation("uLightFar");

			m_uRayWidthLoc = uniformLocation("uRayWidth");
		}

		GodRayShader& setVPLight(const Magnum::Matrix4& VPLight) {
			setUniform(m_uLightViewProjLoc, VPLight);
			return *this;
		}

		GodRayShader& setInvVPLight(const Magnum::Matrix4& invVPLight) {
			setUniform(m_uInvLightViewProjLoc, invVPLight);
			return *this;
		}

		GodRayShader& setVPCamera(const Magnum::Matrix4& uVPCamera) {
			setUniform(m_uCameraViewProjLoc, uVPCamera);
			return *this;
		}

		GodRayShader& setCameraPos(const Magnum::Vector3& camPos) {
			setUniform(m_uCameraPosLoc, camPos);
			return *this;
		}

		GodRayShader& setLightPos(const Magnum::Vector3& lightPos) {
			setUniform(m_uLightPosLoc, lightPos);
			return *this;
		}

		GodRayShader& bindWaterMaskTexture(Magnum::GL::Texture2D& waterMask) {
			waterMask.bind(1);
			setUniform(m_uWaterMaskSamplerLoc, 1);
			return *this;
		}

		GodRayShader& bindShadowMapTexture(Magnum::GL::Texture2D& shadowMap) {
			shadowMap.bind(2);
			setUniform(m_uShadowMapSamplerLoc, 2);
			return *this;
		}

		GodRayShader& bindCamDepthBufferTexture(Magnum::GL::Texture2D& CamDepthBuffer) {
			CamDepthBuffer.bind(4);
			setUniform(m_uCamDepthBufferLoc, 4);
			return *this;
		}

		GodRayShader& setUtime(float time) {
			setUniform(m_uTimeLoc, time);
			return *this;
		}


		GodRayShader& setIntensity(float intensity) {
			setUniform(m_uIntensityLoc, intensity);
			return *this;
		}

		GodRayShader& setG(float g) {
			setUniform(m_uGLoc, g);
			return *this;
		}

		GodRayShader& setFogDensity(float fogI) {
			setUniform(m_uFogDensityLoc, fogI);
			return *this;
		}

		GodRayShader& setLightFar(float lightFar) {
			setUniform(m_uLightFarLoc, lightFar);
			return *this;
		}

		GodRayShader& setRayWidth(float rayWidth) {
			setUniform(m_uRayWidthLoc, rayWidth);
			return *this;
		}
	};
} // namespace WaterSimulation
