#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/Shaders/GenericGL.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Matrix4.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/Math/Color.h>

namespace WaterSimulation
{
	class OpaqueShader : public Magnum::GL::AbstractShaderProgram
	{
	private:
		
		Magnum::Int m_uMVPLoc;
        Magnum::Int m_uColorLoc;
        Magnum::Int m_uUseTextureLoc; 
        Magnum::Int m_uAlbedoSamplerLoc;

	public:

		explicit OpaqueShader(){
			Corrade::Utility::Resource rs{"WaterSimulationResources"};

			Magnum::GL::Shader vert{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Vertex};
			Magnum::GL::Shader frag{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Fragment};

			vert.addSource(Corrade::Containers::StringView{rs.getString("opaqueShader.vs")});
			frag.addSource(Corrade::Containers::StringView{rs.getString("opaqueShader.fs")});	
					
			if(!vert.compile()) {
				Corrade::Utility::Error{} << "DepthDebugShader: vertex shader compilation failed:\n" ;
			}
			if(!frag.compile()) {
				Corrade::Utility::Error{} << "DepthDebugShader: fragment shader compilation failed:\n" ;
			}
			CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
			CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

			attachShaders({vert, frag});	
			
			bindAttributeLocation(Magnum::Shaders::GenericGL3D::Position::Location, "position");
			bindAttributeLocation(Magnum::Shaders::GenericGL3D::Normal::Location, "normal");
			bindAttributeLocation(Magnum::Shaders::GenericGL3D::TextureCoordinates::Location, "uv");
				
			CORRADE_INTERNAL_ASSERT_OUTPUT(link());

			m_uMVPLoc = uniformLocation("uMVP");
            m_uColorLoc = uniformLocation("uColor");
            m_uUseTextureLoc = uniformLocation("uHasAlbedo");
            m_uAlbedoSamplerLoc = uniformLocation("uAlbedoTexture"); 

		}

		OpaqueShader& setMVP(const Magnum::Matrix4& mvp) {
            setUniform(m_uMVPLoc, mvp);
            return *this;
        }

        OpaqueShader& setColor(const Magnum::Color4 & uColor){
            setUniform(m_uColorLoc, uColor);
            return *this;
        }

        OpaqueShader& bindAlbedoTexture(Magnum::GL::Texture2D & albedo){
            albedo.bind(0);
            setUniform(m_uAlbedoSamplerLoc, 0);
            setUniform(m_uUseTextureLoc, true);
            return *this;
        }

        OpaqueShader& unbindAlbedoTexture() {
            setUniform(m_uUseTextureLoc, false);
            return *this;
        }
    };
	
} // namespace WaterSimulation
