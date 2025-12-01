#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Matrix4.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/Shaders/GenericGL.h>

namespace WaterSimulation
{
	class DepthShader : public Magnum::GL::AbstractShaderProgram // TODO : renommer en shadow map shader truc comme ça 
	{

	private:
		Magnum::Int m_uMVPLoc;
		Magnum::Int m_uHeightMapSamplerLoc;
		Magnum::Int m_uHasHeightMapLoc;

	public:
	
		explicit DepthShader(){
			Corrade::Utility::Resource rs{"WaterSimulationResources"};

			Magnum::GL::Shader vert{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Vertex};
			Magnum::GL::Shader frag{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Fragment};

			vert.addSource(Corrade::Containers::StringView{rs.getString("depthShader.vs")});
			frag.addSource(Corrade::Containers::StringView{rs.getString("depthShader.fs")});	
					
			if(!vert.compile()) {
				Corrade::Utility::Error{} << "depthShader: vertex shader compilation failed:\n" ;
			}
			if(!frag.compile()) {
				Corrade::Utility::Error{} << "depthShader: fragment shader compilation failed:\n" ;
			}
			CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
			CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

			attachShaders({vert, frag});			
            bindAttributeLocation(Magnum::Shaders::GenericGL3D::Position::Location, "position");
			bindAttributeLocation(Magnum::Shaders::GenericGL3D::TextureCoordinates::Location, "uv");
            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

			   m_uMVPLoc = uniformLocation("mvp");
			   m_uHeightMapSamplerLoc = uniformLocation("uHeightMap"); 
			   m_uHasHeightMapLoc = uniformLocation("hasHeightMap");
		}


		   DepthShader& setMVP(const Magnum::Matrix4& mvp) {
			   setUniform(m_uMVPLoc, mvp);
			   return *this;
		   }

		   DepthShader& setHasHeightMap(bool has) {
			   setUniform(m_uHasHeightMapLoc, Magnum::Int(has ? 1 : 0));
			   return *this;
		   }

		DepthShader& bindHeightMapTexture(Magnum::GL::Texture2D & heigthmap){
            heigthmap.bind(0);
            setUniform(m_uHeightMapSamplerLoc, 0);
            return *this;
        }



	};
} // namespace WaterSimulation
