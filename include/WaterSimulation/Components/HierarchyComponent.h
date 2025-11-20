#pragma once

#include <WaterSimulation/ECS.h>

#include <string>
#include <vector>
#include <algorithm>

namespace WaterSimulation {


struct HierarchyComponent {

    std::string name = "default_name";
    Entity parent = INVALID;
    std::vector<Entity> children;

    HierarchyComponent(Entity parentIn, std::vector<Entity> childrenIn) : parent(parentIn), children(std::move(childrenIn)) {}

    explicit HierarchyComponent(std::vector<Entity> childrenIn) : children(std::move(childrenIn)) {}

    HierarchyComponent() = default;

    void addChild(Entity child) {
        children.push_back(child);
    }

    void onAttach(Registry& registry, Entity entity) {
        name = "Entity " + std::to_string(entity);
        if (parent != INVALID) {
            if (registry.has<HierarchyComponent>(parent)) {
                registry.get<HierarchyComponent>(parent).children.push_back(entity);
            } else {
                registry.emplace<HierarchyComponent>(parent).children.push_back(entity);
            }
        }
    }

    void onDetach(Registry& registry, Entity entity) {
        if (parent != INVALID && registry.has<HierarchyComponent>(parent)) {
            auto& siblings = registry.get<HierarchyComponent>(parent).children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), entity), siblings.end());
        }
        parent = INVALID;
    }
};

} // namespace WaterSimulation
