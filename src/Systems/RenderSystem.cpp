#include "Corrade/Utility/Debug.h"
#include <WaterSimulation/Systems/RenderSystem.h>
#include <WaterSimulation/Components/TransformComponent.h>

#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/GL/Texture.h>


using namespace Magnum;
using namespace Math::Literals;

namespace WaterSimulation {

void RenderSystem::render(
    Registry& registry,
    const Matrix4& viewMatrix,
    const Matrix4& projectionMatrix) {
    
    const Matrix4 viewProj = projectionMatrix * viewMatrix;
    
    auto view = registry.view<MeshComponent, TransformComponent>();
    for (Entity entity : view) {
        MeshComponent& meshComp = registry.get<MeshComponent>(entity);
        TransformComponent& transformComp = registry.get<TransformComponent>(entity);

        Matrix4 mvp = viewProj * transformComp.globalModel;

        renderMesh(meshComp, mvp);
    }
}

void RenderSystem::renderMesh(
    MeshComponent& meshComp,
    const Matrix4& mvp) {
    
    if (!meshComp.shader) return;
    
    if (auto* flatShader = dynamic_cast<Shaders::FlatGL3D*>(meshComp.shader)) {
        flatShader->setTransformationProjectionMatrix(mvp)
                  .setColor(0x2f83cc_rgbf)
                  .draw(meshComp.glMesh);
    }
    // Custom Shader
    else if (auto* displayShader = dynamic_cast<DisplayShader*>(meshComp.shader)) {
        displayShader->setTransformationMatrix(mvp)
                     .draw(meshComp.glMesh);
    }

    // TODO: ajouter notre shader par exemple
    // if (auto* shaderPerso =  dynamic_cast<Shaders::ClasseDeNotreSahder*>(meshComp.shader))....
}


DisplayShader::DisplayShader(const char * vertex_shader_file, const char * fragment_shader_file ) {
    GL::Shader vert{GL::Version::GL430, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL430, GL::Shader::Type::Fragment};

    vert.addFile(vertex_shader_file);
    frag.addFile(fragment_shader_file);

    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
    CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT_OUTPUT(link());
}

DisplayShader& DisplayShader::bind(Magnum::GL::Texture2D* tex, int location) {
    if (tex) {
        tex->bind(location);
    } else {
        Corrade::Utility::Error{} << "Could not bind texture to shader";
    }

    return *this;
}

DisplayShader& DisplayShader::setTransformationMatrix(const Matrix4& matrix) {
    setUniform(uniformLocation("mvp"), matrix);
    return *this;
}

} // namespace WaterSimulation