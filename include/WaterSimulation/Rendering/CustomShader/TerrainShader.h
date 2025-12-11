#pragma once

#include <WaterSimulation/Rendering/IShader.h>

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/Shaders/GenericGL.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Matrix3.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/Math/Color.h>

namespace WaterSimulation
{
	class TerrainShader : public IShader
	{
	private:
		Magnum::Int m_uMVPLoc;
		Magnum::Int m_uModelLoc;
        Magnum::Int m_uHeightMapSamplerLoc;
		Magnum::Int m_uAlbedoSamplerLoc;
		Magnum::Int m_uLightVPLoc;
		Magnum::Int m_uShadowMapSamplerLoc;
		Magnum::Int m_uARMSamplerLoc;
		Magnum::Int m_uNormalMapSamplerLoc;
		Magnum::Int m_uLightDirectionLoc;
		Magnum::Int m_uLightColorLoc;
		Magnum::Int m_uCameraPositionLoc;
		Magnum::Int m_uLightIntensityLoc;
		Magnum::Int m_uUvScaleLoc;


	public:
		explicit TerrainShader(){
			Corrade::Utility::Resource rs{"WaterSimulationResources"};

			Magnum::GL::Shader vert{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Vertex};
			Magnum::GL::Shader frag{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Fragment};

			vert.addSource(Corrade::Containers::StringView{rs.getString("terrainShader.vs")});
			frag.addSource(Corrade::Containers::StringView{rs.getString("terrainShader.fs")});	
					
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
			m_uModelLoc = uniformLocation("uModel");
            m_uHeightMapSamplerLoc = uniformLocation("uHeightMap"); 
			m_uAlbedoSamplerLoc = uniformLocation("uAlbedoTexture"); 
			m_uLightVPLoc = uniformLocation("uLightVP");
			m_uShadowMapSamplerLoc = uniformLocation("uShadowMap");
			m_uARMSamplerLoc = uniformLocation("uARM");
			m_uNormalMapSamplerLoc = uniformLocation("uNormalMap");
			m_uLightDirectionLoc = uniformLocation("uLightDirection");
			m_uLightColorLoc = uniformLocation("uLightColor");
			m_uCameraPositionLoc = uniformLocation("uCameraPosition");
			m_uLightIntensityLoc = uniformLocation("uLightIntensity");
			m_uUvScaleLoc = uniformLocation("uUvScale");

		}

		TerrainShader& setModel(const Magnum::Matrix4& model) {
			setUniform(m_uModelLoc, model);
			return *this;
		}

		TerrainShader& setMVP(const Magnum::Matrix4& mvp) {
            setUniform(m_uMVPLoc, mvp);
            return *this;
        }

		TerrainShader& setLightVP(const Magnum::Matrix4& lightVP) {
			setUniform(m_uLightVPLoc, lightVP);
			return *this;
		}

		TerrainShader& bindHeightMapTexture(Magnum::GL::Texture2D & heigthmap){
            heigthmap.bind(0);
            setUniform(m_uHeightMapSamplerLoc, 0);
            return *this;
        }

        TerrainShader& bindAlbedoTexture(Magnum::GL::Texture2D & albedo){
            albedo.bind(1);
            setUniform(m_uAlbedoSamplerLoc, 1);
            return *this;
        }

		
		TerrainShader& bindARM(Magnum::GL::Texture2D & arm){
            arm.bind(2);
            setUniform(m_uARMSamplerLoc, 2);
            return *this;
        }

		TerrainShader& bindNormalMap(Magnum::GL::Texture2D & normalMap){
            normalMap.bind(3);
            setUniform(m_uNormalMapSamplerLoc, 3);
            return *this;
        }
		

		TerrainShader& bindShadowMap(Magnum::GL::Texture2D & shadowMap){
			shadowMap.bind(4);
			setUniform(m_uShadowMapSamplerLoc, 4);
			return *this;
		}

		TerrainShader& setLightDirection(const Magnum::Vector3& dir) {
			setUniform(m_uLightDirectionLoc, dir);
			return *this;
		}
		TerrainShader& setLightColor(const Magnum::Color3& color) {
			setUniform(m_uLightColorLoc, color);
			return *this;
		}
		TerrainShader& setLightIntensity(float intensity) {
			setUniform(m_uLightIntensityLoc, intensity);
			return *this;
		}

		TerrainShader& setUvScale(float scale) {
			setUniform(m_uUvScaleLoc, scale);
			return *this;
		}

		TerrainShader& setCameraPosition(const Magnum::Vector3& pos) {
			setUniform(m_uCameraPositionLoc, pos);
			return *this;
		}

		 void draw(  Magnum::GL::Mesh& mesh, 
			 const Magnum::Matrix4& model,
			 const Magnum::Matrix4& mvp, 
			 MaterialComponent& material, 
			 Magnum::Matrix4 lightVP,
			 Magnum::GL::Texture2D & shadowMap,
			 DirectionalLightComponent& sunLight,
			 Magnum::Vector3 camPos) override{

				Magnum::GL::Texture2D* albedoTex = material.albedo.get();
				Magnum::GL::Texture2D* heightMapTex = material.heightmap.get();
				Magnum::GL::Texture2D* armTex = material.arm.get();
				Magnum::GL::Texture2D* normalTex = material.normal.get();

				setModel(model);
				setMVP(mvp);
				if(albedoTex) bindAlbedoTexture(*albedoTex); 
				if(heightMapTex) bindHeightMapTexture(*heightMapTex);
				if(armTex) bindARM(*armTex);
				if(normalTex) bindNormalMap(*normalTex);

				setLightDirection(sunLight.direction);
				setLightColor(sunLight.color);
				setLightIntensity(sunLight.intensity);
				setUvScale(4.0f);
				setCameraPosition(camPos);

				bindShadowMap(shadowMap);
				setLightVP(lightVP);

		    	Magnum::GL::AbstractShaderProgram::draw(mesh);
		}
	};
	


} // namespace WaterSimulation
