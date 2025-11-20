#include <WaterSimulation/Systems/TransformSystem.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Components/HierarchyComponent.h>

#include <Magnum/Math/Matrix4.h>

void WaterSimulation::TransformSystem::update(WaterSimulation::Registry & registry){
	
	auto view = registry.view<TransformComponent>();
    for (Entity entity : view) {
        if (!registry.has<HierarchyComponent>(entity)) {
            computeGlobalTransform(entity, registry, Magnum::Matrix4{Magnum::Math::IdentityInit});
        } else {
            const auto & hierarchy = registry.get<HierarchyComponent>(entity);
            if (hierarchy.parent == INVALID) {
                computeGlobalTransform(entity, registry, Magnum::Matrix4{Magnum::Math::IdentityInit});
            }
        }
    }
};


void WaterSimulation::TransformSystem::computeGlobalTransform(Entity entity, Registry & registry, const Magnum::Matrix4 & parentModel){
    auto& transform = registry.get<TransformComponent>(entity);
    transform.computeGlobalModelMatrix(parentModel);
    transform.inverseGlobalModel = transform.globalModel.inverted();
    
    if (registry.has<HierarchyComponent>(entity)) {
        auto & hierarchy = registry.get<HierarchyComponent>(entity);
        for (Entity child : hierarchy.children) {
            computeGlobalTransform(child, registry, transform.globalModel);
        }
    }
};