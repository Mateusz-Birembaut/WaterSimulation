#pragma once

#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Shaders/GenericGL.h>

namespace WaterSimulation {
	class WaterPosShader : public Magnum::GL::AbstractShaderProgram 
	{
	      private:
		Magnum::Int m_uModelLoc;
		Magnum::Int m_uLightVpLoc;
		Magnum::Int m_uHeightMapSamplerLoc;
		Magnum::Int m_uAlbedoSamplerLoc;

	      public:
		explicit WaterPosShader() {
			Corrade::Utility::Resource rs{"WaterSimulationResources"};

			Magnum::GL::Shader vert{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Vertex};
			Magnum::GL::Shader frag{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Fragment};

			vert.addSource(Corrade::Containers::StringView{rs.getString("waterPos.vs")});
			frag.addSource(Corrade::Containers::StringView{rs.getString("waterPos.fs")});

			if (!vert.compile()) {
				Corrade::Utility::Error{} << "waterPos: vertex shader compilation failed:\n";
			}
			if (!frag.compile()) {
				Corrade::Utility::Error{} << "waterPos: fragment shader compilation failed:\n";
			}
			CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
			CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

			attachShaders({vert, frag});
			bindAttributeLocation(Magnum::Shaders::GenericGL3D::Position::Location, "position");
			bindAttributeLocation(Magnum::Shaders::GenericGL3D::TextureCoordinates::Location, "uv");
			CORRADE_INTERNAL_ASSERT_OUTPUT(link());

			m_uModelLoc = uniformLocation("uModel");
			m_uLightVpLoc = uniformLocation("uLightVP");
			m_uHeightMapSamplerLoc = uniformLocation("uHeightMap");
			m_uAlbedoSamplerLoc = uniformLocation("uAlbedoTexture");
		}

		WaterPosShader& setModel(const Magnum::Matrix4& model) {
			setUniform(m_uModelLoc, model);
			return *this;
		}

		WaterPosShader& setLightVP(const Magnum::Matrix4& lightVP) {
			setUniform(m_uLightVpLoc, lightVP);
			return *this;
		}

		WaterPosShader& bindHeightMapTexture(Magnum::GL::Texture2D& heigthmap) {
			heigthmap.bind(0);
			setUniform(m_uHeightMapSamplerLoc, 0);
			return *this;
		}

		WaterPosShader& bindAlbedoTexture(Magnum::GL::Texture2D& albedo) {
			albedo.bind(1);
			setUniform(m_uAlbedoSamplerLoc, 1);
			return *this;
		}
	};
} // namespace WaterSimulation
