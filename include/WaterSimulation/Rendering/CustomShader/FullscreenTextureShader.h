#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Corrade/Utility/Resource.h>

namespace WaterSimulation
{
	class FullscreenTextureShader : public Magnum::GL::AbstractShaderProgram
	{

	private:
		Magnum::Int m_textureSamplerLoc;
        Magnum::Int m_uNear;
        Magnum::Int m_uFar;
		Magnum::Int m_uIsUnderwater;
		Magnum::Int m_uFogColor;
		Magnum::Int m_uFogDensity;

	public:
	
		explicit FullscreenTextureShader(){
			Corrade::Utility::Resource rs{"WaterSimulationResources"};

			Magnum::GL::Shader vert{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Vertex};
			Magnum::GL::Shader frag{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Fragment};

			vert.addSource(Corrade::Containers::StringView{rs.getString("FullscreenTextureShader.vs")});
			frag.addSource(Corrade::Containers::StringView{rs.getString("FullscreenTextureShader.fs")});	
					
			if(!vert.compile()) {
				Corrade::Utility::Error{} << "FullscreenTextureShader: vertex shader compilation failed:\n" ;
			}
			if(!frag.compile()) {
				Corrade::Utility::Error{} << "FullscreenTextureShader: fragment shader compilation failed:\n" ;
			}
			CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
			CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

			attachShaders({vert, frag});			
			CORRADE_INTERNAL_ASSERT_OUTPUT(link());

			m_textureSamplerLoc = uniformLocation("uTexture");
            m_uNear = uniformLocation("uNear");
            m_uFar = uniformLocation("uFar");
			m_uIsUnderwater = uniformLocation("uIsUnderwater");
			m_uFogColor = uniformLocation("uFogColor");
			m_uFogDensity = uniformLocation("uFogDensity");

			/* set stable defaults */
			setUniform(m_textureSamplerLoc, 0);
			setUniform(m_uIsUnderwater, 0);
			setUniform(m_uFogColor, Magnum::Color3{0.0f});
			setUniform(m_uFogDensity, 0.0f);
		}

		FullscreenTextureShader& bindDepthTexture(Magnum::GL::Texture2D & depthTexture){
			depthTexture.bind(0);
			return *this;
		}

		FullscreenTextureShader& setNear(float uNear){
			setUniform(m_uNear, uNear);
			return *this;
		}

		FullscreenTextureShader& setFar(float uFar){
			setUniform(m_uFar, uFar);
			return *this;
		}

		FullscreenTextureShader& setIsUnderwater(bool isUnderwater){
			setUniform(m_uIsUnderwater, isUnderwater ? 1 : 0);
			return *this;
		}

		FullscreenTextureShader& setFogColor(const Magnum::Color3& fogColor){
			setUniform(m_uFogColor, fogColor);
			return *this;
		}

		FullscreenTextureShader& setFogDensity(float fogDensity){
			setUniform(m_uFogDensity, fogDensity);
			return *this;
		}

	};
} // namespace WaterSimulation
