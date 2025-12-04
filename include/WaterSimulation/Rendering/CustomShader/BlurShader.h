#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector2.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/Shaders/GenericGL.h>

namespace WaterSimulation
{
	class BlurShader : public Magnum::GL::AbstractShaderProgram 
	{

	private:
		Magnum::Int m_uTextureLoc;
		Magnum::Int m_uTexelSizeLoc;
		Magnum::Int m_uRotationLoc;
		Magnum::Int m_uRadiusLoc;

	public:
	
		explicit BlurShader(){
			Corrade::Utility::Resource rs{"WaterSimulationResources"};

			Magnum::GL::Shader vert{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Vertex};
			Magnum::GL::Shader frag{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Fragment};

			vert.addSource(Corrade::Containers::StringView{rs.getString("blur.vs")});
			frag.addSource(Corrade::Containers::StringView{rs.getString("blur.fs")});	
					
			if(!vert.compile()) {
				Corrade::Utility::Error{} << "blur: vertex shader compilation failed:\n" ;
			}
			if(!frag.compile()) {
				Corrade::Utility::Error{} << "blur: fragment shader compilation failed:\n" ;
			}
			CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
			CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

			attachShaders({vert, frag});			
            bindAttributeLocation(Magnum::Shaders::GenericGL3D::Position::Location, "position");
			bindAttributeLocation(Magnum::Shaders::GenericGL3D::TextureCoordinates::Location, "uv");
            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

			m_uTextureLoc = uniformLocation("uTexture");
			m_uTexelSizeLoc = uniformLocation("uTexelSize");
			m_uRotationLoc = uniformLocation("uRotation");
			m_uRadiusLoc = uniformLocation("uRadius");
		}

		BlurShader& bindTexture(Magnum::GL::Texture2D & texture){
            texture.bind(0);
            setUniform(m_uTextureLoc, 0);
            return *this;
        }

		BlurShader& setTexelSize(const Magnum::Vector2& texelSize) {
			setUniform(m_uTexelSizeLoc, texelSize);
			return *this;
		}

		BlurShader& setRotation(float rotation) {
			setUniform(m_uRotationLoc, rotation);
			return *this;
		}

		BlurShader& setRadius(float radius) {
			setUniform(m_uRadiusLoc, radius);
			return *this;
		}

	};
} // namespace WaterSimulation
