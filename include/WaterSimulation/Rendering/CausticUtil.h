#pragma once 

#include <WaterSimulation/Rendering/CustomShader/BlurShader.h>

#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h> 
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Color.h>


namespace WaterSimulation {

class CausticUtil {
private:
    Magnum::GL::Mesh m_meshFullscreenUtils;
    Magnum::GL::Buffer m_vertexBufferFullscreenUtils;
    Magnum::GL::Texture2D m_tempTexture;

public:

    void blurTexture(Magnum::GL::Texture2D& texture) {
        Magnum::Vector2i size = texture.imageSize(0);


        if(m_tempTexture.id() == 0 || m_tempTexture.imageSize(0) != size) {
            m_tempTexture = Magnum::GL::Texture2D{};
            m_tempTexture.setStorage(1, Magnum::GL::TextureFormat::RGBA16F, size);
        }

        Magnum::GL::Framebuffer fb{{{}, size}};
        fb.attachTexture(Magnum::GL::Framebuffer::ColorAttachment{0}, m_tempTexture, 0);
        
        Magnum::GL::Renderer::setClearColor(Magnum::Color4{0.0f, 0.0f, 0.0f, 0.0f});
        fb.clear(Magnum::GL::FramebufferClear::Color);
        fb.bind();

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
        
        blur.bindTexture(texture).draw(m_meshFullscreenUtils);

        Magnum::GL::Framebuffer fbOriginal{{{}, size}};
        fbOriginal.attachTexture(Magnum::GL::Framebuffer::ColorAttachment{0}, texture, 0);
        Magnum::GL::Framebuffer::blit(fb, fbOriginal, {{0, 0}, size}, Magnum::GL::FramebufferBlit::Color);

        Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::DepthTest);
    }
};
}