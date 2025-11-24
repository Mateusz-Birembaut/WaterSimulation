#pragma once

#include <WaterSimulation/Rendering/IShader.h>

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
	class DebugShader : public IShader
	{
	private:
		Magnum::Int m_uMVPLoc;
        Magnum::Int m_uHeightMapSamplerLoc;
		Magnum::Int m_uAlbedoSamplerLoc;
	public:
		explicit DebugShader(){
			Corrade::Utility::Resource rs{"WaterSimulationResources"};

			Magnum::GL::Shader vert{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Vertex};
			Magnum::GL::Shader frag{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Fragment};

			vert.addSource(Corrade::Containers::StringView{rs.getString("water.vs")});
			frag.addSource(Corrade::Containers::StringView{rs.getString("water.fs")});	
					
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
            m_uHeightMapSamplerLoc = uniformLocation("uHeightMap"); 
			m_uAlbedoSamplerLoc = uniformLocation("uAlbedoTexture"); 

		}

		DebugShader& setMVP(const Magnum::Matrix4& mvp) {
            setUniform(m_uMVPLoc, mvp);
            return *this;
        }

		DebugShader& bindHeightMapTexture(Magnum::GL::Texture2D & heigthmap){
            heigthmap.bind(0);
            setUniform(m_uHeightMapSamplerLoc, 0);
            return *this;
        }

        DebugShader& bindAlbedoTexture(Magnum::GL::Texture2D & albedo){
            albedo.bind(1);
            setUniform(m_uAlbedoSamplerLoc, 1);
            return *this;
        }



		void draw(  Magnum::GL::Mesh& mesh, 
					const Magnum::Matrix4& mvp, 
					MaterialComponent& material, 
					const std::vector<LightComponent>& lights) override {

				//TODO gerer lights

				Magnum::GL::Texture2D* albedoTex = material.albedo.get();
				Magnum::GL::Texture2D* heightMapTex = material.heightmap.get();

				setMVP(mvp);
				if(albedoTex) bindAlbedoTexture(*albedoTex); // TODO : remettre albedoTex
				if(heightMapTex) bindHeightMapTexture(*heightMapTex);

				Magnum::GL::AbstractShaderProgram::draw(mesh);
		}
	};
	


} // namespace WaterSimulation
