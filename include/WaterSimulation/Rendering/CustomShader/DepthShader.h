#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Matrix4.h>
#include <Corrade/Utility/Resource.h>

namespace WaterSimulation
{
	class DepthShader : public Magnum::GL::AbstractShaderProgram
	{

	private:

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
			CORRADE_INTERNAL_ASSERT_OUTPUT(link());
		}

		DepthShader& setMVP(const Magnum::Matrix4& mvp) {
			setUniform(0, mvp);
			return *this;
		}

	};
} // namespace WaterSimulation
