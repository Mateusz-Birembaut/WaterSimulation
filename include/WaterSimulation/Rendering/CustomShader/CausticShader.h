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
		Magnum::Int m_uLightViewProjLoc;
		Magnum::Int m_uCameraViewProjLoc;
		Magnum::Int m_uCameraPosLoc;
		Magnum::Int m_uShadowMapSamplerLoc;
		Magnum::Int m_uWaterMaskSamplerLoc;
		Magnum::Int m_uCamDepthBufferLoc;
		Magnum::Int m_uTimeLoc;
		Magnum::Int m_uLightPosLoc;
		Magnum::Int m_uAttenuationLoc;
		Magnum::Int m_uIntensityLoc;
		Magnum::Int m_uALoc;
		Magnum::Int m_uBLoc;
		Magnum::Int m_uLightFarLoc;
		Magnum::Int m_uInvLightViewProjLoc;

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

			m_uLightViewProjLoc = uniformLocation("uVPLight");

			m_uCameraViewProjLoc = uniformLocation("uVPCamera");
			m_uCameraPosLoc = uniformLocation("uCamPos");

			m_uWaterMaskSamplerLoc = uniformLocation("uWaterMask");

			m_uShadowMapSamplerLoc = uniformLocation("uShadowMap");

			m_uCamDepthBufferLoc = uniformLocation("uCamDepthBuffer");

			m_uTimeLoc = uniformLocation("uTime");

			m_uLightPosLoc = uniformLocation("uLightPos");

			m_uAttenuationLoc = uniformLocation("uAttenuation");

			m_uIntensityLoc = uniformLocation("uIntensity");

			m_uALoc = uniformLocation("uA");

			m_uBLoc = uniformLocation("uB");

			m_uLightFarLoc = uniformLocation("uLightFar");

			m_uInvLightViewProjLoc = uniformLocation("uInvVPLight");
		}

		CausticShader& setVPLight(const Magnum::Matrix4& VPLight) {
			setUniform(m_uLightViewProjLoc, VPLight);
			return *this;
		}

		CausticShader& setInvVPLight(const Magnum::Matrix4& invVPLight) {
			setUniform(m_uInvLightViewProjLoc, invVPLight);
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

		CausticShader& setLightPos(const Magnum::Vector3& lightPos) {
			setUniform(m_uLightPosLoc, lightPos);
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

		CausticShader& bindCamDepthBufferTexture(Magnum::GL::Texture2D& CamDepthBuffer) {
			CamDepthBuffer.bind(4);
			setUniform(m_uCamDepthBufferLoc, 4);
			return *this;
		}

		CausticShader& setUtime(float time) {
			setUniform(m_uTimeLoc, time);
			return *this;
		}

		CausticShader& setAttenuation(float attenuation) {
			setUniform(m_uAttenuationLoc, attenuation);
			return *this;
		}

		CausticShader& setIntensity(float intensity) {
			setUniform(m_uIntensityLoc, intensity);
			return *this;
		}

		CausticShader& setA(float a) {
			setUniform(m_uALoc, a);
			return *this;
		}

		CausticShader& setB(float b) {
			setUniform(m_uBLoc, b);
			return *this;
		}

		CausticShader& setLightFar(float lightFar) {
			setUniform(m_uLightFarLoc, lightFar);
			return *this;
		}
	};
} // namespace WaterSimulation
