#include "WaterSimulation/Systems/RenderSystem.h"
#include <WaterSimulation/Components/MeshComponent.h>

#include <Magnum/GL/Buffer.h>
#include <Magnum/Shaders/GenericGL.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/GL/Renderer.h>
#include <Corrade/Utility/Debug.h>
#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/ArrayViewStl.h>

using namespace Magnum;
using namespace WaterSimulation;

void MeshComponent::setupBuffers(GL::MeshPrimitive primitive) {
    Corrade::Utility::Debug{} << "MeshComponent::setupBuffers() called";
    
    if (!activeMesh) {
        Corrade::Utility::Warning{} << "MeshComponent: No active mesh";
        return;
    }
    
    if (activeMesh->vertices.empty()) {
        Corrade::Utility::Warning{} << "MeshComponent: Empty vertices";
        return;
    }
    
    Corrade::Utility::Debug{} << "Setting up buffers for mesh with" 
                              << activeMesh->vertices.size() << "vertices and"
                              << activeMesh->triangles.size() << "indices";
    
    if (glMesh.id() == 0) {
        glMesh = GL::Mesh{};
    }
    
    if(primitive == GL::MeshPrimitive::Patches){
        Corrade::Utility::Debug{} << "SETTING BUFFER FOR TESSELATION";
        Magnum::GL::Renderer::setPatchVertexCount(4);
        glMesh.setCount(activeMesh->vertices.size());
        glMesh.setPrimitive(GL::MeshPrimitive::Patches);
    }else{
        glMesh.setCount(activeMesh->triangles.size());
        glMesh.setPrimitive(GL::MeshPrimitive::Triangles);
    }
    
    Corrade::Utility::Debug{} << "Creating vertex buffer...";
    vertexBuffer.setData(Containers::arrayView(activeMesh->vertices)); // Containers::arrayView permet 
    glMesh.addVertexBuffer(vertexBuffer, 0, DisplayShader::Position{});
    
    if (!activeMesh->normals.empty()) {
        normalBuffer.setData(Containers::arrayView(activeMesh->normals));
        glMesh.addVertexBuffer(normalBuffer, 0, DisplayShader::Normal{});
    }

    if (!activeMesh->uvs.empty()) {
        uvBuffer.setData(Containers::arrayView(activeMesh->uvs));
        glMesh.addVertexBuffer(uvBuffer, 0, DisplayShader::TextureCoordinates{});
    }
    

    if(primitive == GL::MeshPrimitive::Patches){
        
    }else{
        indexBuffer.setData(Containers::arrayView(activeMesh->triangles));
        glMesh.setIndexBuffer(indexBuffer, 0, GL::MeshIndexType::UnsignedInt);

    }

    
}

void MeshComponent::clearBuffers() {
    glMesh = GL::Mesh{Magnum::NoCreate}; // libère ressources gpu
}

void MeshComponent::checkLOD(const Vector3& cameraPos, const Vector3& entityPos) {
    if (meshLODs.size() <= 1) {
        return;
    }
    
    float distance = (cameraPos - entityPos).length();
    
    Mesh* newActiveMesh = meshLODs[0].second; 
    
    for (const auto& [lodDistance, mesh] : meshLODs) {
        if (distance >= lodDistance) {
            newActiveMesh = mesh;
        } else {
            break;
        }
    }
    
    if (newActiveMesh != activeMesh) {
        activeMesh = newActiveMesh;
        setupBuffers(Magnum::GL::MeshPrimitive::Triangles);
    }
}
