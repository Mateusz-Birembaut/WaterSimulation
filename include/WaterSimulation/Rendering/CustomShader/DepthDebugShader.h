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
    class DepthDebugShader : public Magnum::GL::AbstractShaderProgram
    {
    private:
        Magnum::Int m_uMVPLoc;
        Magnum::Int m_uDepthSamplerLoc;
        Magnum::Int m_uNearLoc;
        Magnum::Int m_uFarLoc;
        Magnum::Int m_uIsOrtho;
        Magnum::Int uLinearizeRangeLoc;
    public:
        explicit DepthDebugShader() {
            Corrade::Utility::Resource rs{"WaterSimulationResources"};

            Magnum::GL::Shader vert{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Vertex};
            Magnum::GL::Shader frag{Magnum::GL::Version::GL430, Magnum::GL::Shader::Type::Fragment};

            vert.addSource(Corrade::Containers::StringView{rs.getString("depthDebugShader.vs")});
            frag.addSource(Corrade::Containers::StringView{rs.getString("depthDebugShader.fs")});

            CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
            CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

            attachShaders({vert, frag});
            bindAttributeLocation(Magnum::Shaders::GenericGL3D::Position::Location, "position");
            bindAttributeLocation(Magnum::Shaders::GenericGL3D::TextureCoordinates::Location, "uv");
            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            m_uMVPLoc = uniformLocation("mvp");
            m_uDepthSamplerLoc = uniformLocation("depthTexture");
            m_uNearLoc = uniformLocation("near");
            m_uFarLoc = uniformLocation("far");
            m_uIsOrtho = uniformLocation("isOrthographic");
            uLinearizeRangeLoc = uniformLocation("uLinearizeRange");
        }

        DepthDebugShader& setLinearize(float linearizeValue) {
            setUniform(uLinearizeRangeLoc, linearizeValue);
            return *this;
        }
        DepthDebugShader& setMVP(const Magnum::Matrix4& mvp) {
            setUniform(m_uMVPLoc, mvp);
            return *this;
        }

        DepthDebugShader& setDepthTexture(Magnum::GL::Texture2D& texture) {
            setUniform(m_uDepthSamplerLoc, 0);
            texture.bind(0);
            return *this;
        }

        DepthDebugShader& setNear(float near) {
            setUniform(m_uNearLoc, near);
            return *this;
        }

        DepthDebugShader& setFar(float far) {
            setUniform(m_uFarLoc, far);
            return *this;
        }

		DepthDebugShader& setOrthographic(bool isOrtho) {
            setUniform(m_uIsOrtho, isOrtho);
            return *this;
        }
    };
} // namespace WaterSimulation