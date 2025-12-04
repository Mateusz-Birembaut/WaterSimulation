#pragma once 

#include <WaterSimulation/Rendering/CustomShader/BlurShader.h>

#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Sampler.h>
#include <Magnum/GL/TextureFormat.h> 
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Color.h>

#include <algorithm>
#include <cmath>


namespace WaterSimulation {

class CausticUtil {
private:
    Magnum::GL::Mesh m_meshFullscreenUtils;
    Magnum::GL::Buffer m_vertexBufferFullscreenUtils;
    Magnum::GL::Texture2D m_tempTexture;

public:

    void blurTexture(Magnum::GL::Texture2D& texture, float kernelRadius) {
        Magnum::Vector2i size = texture.imageSize(0);


        if(m_tempTexture.id() == 0 || m_tempTexture.imageSize(0) != size) {
            m_tempTexture = Magnum::GL::Texture2D{};
            m_tempTexture.setStorage(1, Magnum::GL::TextureFormat::RGBA16F, size)
                .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge);
        }

        const float clampedRadius = std::max(kernelRadius, 0.0f);
        if(clampedRadius <= 0.0f)
            return;

        Magnum::GL::Framebuffer fbTemp{{{}, size}};
        fbTemp.attachTexture(Magnum::GL::Framebuffer::ColorAttachment{0}, m_tempTexture, 0);
        fbTemp.setViewport({{}, size});

        Magnum::GL::Renderer::setClearColor(Magnum::Color4{0.0f, 0.0f, 0.0f, 0.0f});
        fbTemp.clear(Magnum::GL::FramebufferClear::Color);

        Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::Blending);
        Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::DepthTest);

        BlurShader blur;


        if(m_meshFullscreenUtils.count() == 0) {
            
            struct Vertex {
                Magnum::Vector2 position;
                Magnum::Vector2 uv;
            };

            constexpr Vertex vertices[]{
                {{-1.0f, -1.0f}, {0.0f, 0.0f}},
                {{3.0f, -1.0f}, {2.0f, 0.0f}},
                {{-1.0f, 3.0f}, {0.0f, 2.0f}}
            };

            m_vertexBufferFullscreenUtils.setData(vertices);
            
            m_meshFullscreenUtils.setPrimitive(Magnum::GL::MeshPrimitive::Triangles)
                .setCount(3)
                .addVertexBuffer(
                    m_vertexBufferFullscreenUtils,
                    0,
                    Magnum::GL::Attribute<0, Magnum::Vector2>{}, 
                    Magnum::GL::Attribute<1, Magnum::Vector2>{}
                );
        }

        Magnum::Vector2 texelSize{1.0f / float(size.x()), 1.0f / float(size.y())};

        // horizontal temp text
        fbTemp.bind();
        blur.setTexelSize(texelSize)
            .setRadius(clampedRadius)
            .setDirection({1.0f, 0.0f})
            .bindTexture(texture)
            .draw(m_meshFullscreenUtils);

        // vertical texture de sortie
        Magnum::GL::Framebuffer fbOriginal{{{}, size}};
        fbOriginal.attachTexture(Magnum::GL::Framebuffer::ColorAttachment{0}, texture, 0);
        fbOriginal.setViewport({{}, size});
        fbOriginal.bind();
        Magnum::GL::Renderer::setClearColor(Magnum::Color4{0.0f, 0.0f, 0.0f, 0.0f});
        fbOriginal.clear(Magnum::GL::FramebufferClear::Color);

        blur.setTexelSize(texelSize)
            .setRadius(clampedRadius)
            .setDirection({0.0f, 1.0f})
            .bindTexture(m_tempTexture)
            .draw(m_meshFullscreenUtils);

        Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::DepthTest);
        Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::Blending);
    }
};
}