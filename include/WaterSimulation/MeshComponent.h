#pragma once

#include <WaterSimulation/Mesh.h>
#include <WaterSimulation/ECS.h>

#include <vector>
#include <string>
#include <memory>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Matrix4.h>

namespace WaterSimulation {

struct MeshComponent {
    std::vector<std::pair<float, Mesh*>> meshLODs;
    Mesh* activeMesh;

    Magnum::GL::Buffer vertexBuffer;
    Magnum::GL::Buffer normalBuffer;
    Magnum::GL::Buffer uvBuffer;
    Magnum::GL::Buffer indexBuffer;

    Magnum::GL::Mesh glMesh{Magnum::NoCreate};
    Magnum::GL::AbstractShaderProgram* shader = nullptr;

    std::vector<std::string> textureFiles;
    std::vector<std::string> textureUniforms;
    std::string materialPath = "";
    
    Magnum::Matrix4 transform;
    
    MeshComponent() = default;

    // GL::Buffer et mesh pas copiable du coup on doit descativer la copie des mesh components
    MeshComponent(const MeshComponent&) = delete; 
    MeshComponent& operator=(const MeshComponent&) = delete;

    MeshComponent(MeshComponent&&) = default;
    MeshComponent& operator=(MeshComponent&&) = default;
    
    MeshComponent(
        const std::vector<std::pair<float, Mesh*>>& lods,
        Magnum::GL::AbstractShaderProgram* shaderProgram = nullptr,
        const std::vector<std::string>& texFiles = {},
        const std::vector<std::string>& texUniforms = {},
        const std::string& material = ""
    ) : 
        meshLODs(lods), 
        activeMesh(lods.empty() ? nullptr : lods[0].second),
        transform{Magnum::Math::IdentityInit},
        shader(shaderProgram)
    {
        if (material.empty()) {
            textureFiles = texFiles;
            textureUniforms = texUniforms;
        } else {
            materialPath = material;
            textureFiles = {
                material + "/albedo.png", 
                material + "/ao.png", 
                material + "/metallic.png",
                material + "/normal.png", 
                material + "/roughness.png"
            };
            textureUniforms = {"albedoMap", "aoMap", "metallicMap", "normalMap", "roughnessMap"};
        }
        
        if (activeMesh) {
            setupBuffers();
        }
    }
    
    void setupBuffers();
    void clearBuffers();
    void checkLOD(const Magnum::Vector3& cameraPos, const Magnum::Vector3& entityPos);
    
    void onAttach(Registry& registry, Entity entity) {}
    void onDetach(Registry& registry, Entity entity) {}
};

} // namespace WaterSimulation