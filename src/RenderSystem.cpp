#include <WaterSimulation/RenderSystem.h>

#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>

using namespace Magnum;
using namespace Math::Literals;

namespace WaterSimulation {

void RenderSystem::render(
    Registry& registry,
    const Matrix4& viewMatrix,
    const Matrix4& projectionMatrix) {
    
    const Matrix4 viewProj = projectionMatrix * viewMatrix;
    
    auto view = registry.view<MeshComponent>();
    for (Entity entity : view) {
        MeshComponent& meshComp = registry.get<MeshComponent>(entity);
        
        renderMesh(meshComp, meshComp.transform, viewProj);
    }
}

void RenderSystem::renderMesh(
    MeshComponent& meshComp,
    const Matrix4& transformation,
    const Matrix4& viewProj) {
    
    if (!meshComp.shader) return;
    
    const Matrix4 mvp = viewProj * transformation;
    
    if (auto* flatShader = dynamic_cast<Shaders::FlatGL3D*>(meshComp.shader)) {
        flatShader->setTransformationProjectionMatrix(mvp)
                  .setColor(0x2f83cc_rgbf)
                  .draw(meshComp.glMesh);
    } 

    // TODO: ajouter notre shader
    // if (auto* shaderPerso =  dynamic_cast<Shaders::ClasseDeNotreSahder*>(meshComp.shader))....
}

} // namespace WaterSimulation