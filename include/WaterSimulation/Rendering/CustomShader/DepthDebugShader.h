#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Corrade/Utility/Resource.h>

namespace WaterSimulation
{
	class DepthDebugShader : public Magnum::GL::AbstractShaderProgram
	{

	private:

	public:
	
		explicit DepthDebugShader(){
			Corrade::Utility::Resource rs{"WaterSimulationResources"};

			Magnum::GL::Shader vert{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Vertex};
			Magnum::GL::Shader frag{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Fragment};

			vert.addSource(Corrade::Containers::StringView{rs.getString("depthDebugShader.vs")});
			frag.addSource(Corrade::Containers::StringView{rs.getString("depthDebugShader.fs")});	
					
			if(!vert.compile()) {
				Corrade::Utility::Error{} << "DepthDebugShader: vertex shader compilation failed:\n" ;
			}
			if(!frag.compile()) {
				Corrade::Utility::Error{} << "DepthDebugShader: fragment shader compilation failed:\n" ;
			}
			CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
			CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

			attachShaders({vert, frag});			
			CORRADE_INTERNAL_ASSERT_OUTPUT(link());
		}

		DepthDebugShader& bindDepthTexture(Magnum::GL::Texture2D & depthTexture){
			depthTexture.bind(0);
			return *this;
		}

		DepthDebugShader& setNear(float near){
			setUniform(0, near);
			return *this;
		}

		DepthDebugShader& setFar(float far){
			setUniform(1, far);
			return *this;
		}

	};
} // namespace WaterSimulation
