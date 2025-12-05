#pragma once

#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/RigidBodyComponent.h>
#include <WaterSimulation/PhysicsUtils.h>
#include <WaterSimulation/Rendering/HeightmapReadback.h>

#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Matrix3.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>


namespace WaterSimulation
{
    


class PhysicsSystem  {

    HeightmapReadback* m_heightmapReadback{nullptr};

    std::vector<std::pair<Entity, Entity>> collisionPairs;
    std::vector<CollisionInfo> collisionList;

    Magnum::Vector3 gravity = Magnum::Vector3{0.0f, -20.0f, 0.0f};

    float deltaTime = 1.0f/60.0f;

    void recomputeAABB(Registry& registry);

    void integrate(Registry& registry, float deltaTime);

    void narrowCollisionDetection(Entity entityA, RigidBodyComponent& rigidBodyA, TransformComponent& transformA, Entity entityB, RigidBodyComponent& rigidBodyB, TransformComponent& transformB);
    void broadCollisionDetection(Registry& registry);

    void collisionResolution(Registry& registry);

    void collisionResolutionLinear(Registry& registry);

    struct Disturbance {
        int px, py;
        float strength;
        float _padding;
    };

    std::vector<Disturbance> m_disturbances;

    void applyBuoyancy(Registry& registry);

    bool handleSphereTerrainCollision(Entity entityA,
                                      RigidBodyComponent& rigidBodyA,
                                      TransformComponent& transformA,
                                      const Collider& colliderA,
                                      Entity entityB,
                                      RigidBodyComponent& rigidBodyB,
                                      TransformComponent& transformB,
                                      const Collider& colliderB,
                                      CollisionInfo& collisionInfo) const;

public : 

    void update(Registry& registry, float deltaTime );
    
    std::vector<CollisionInfo> getCollisionList(){return collisionList;}

    void setHeightmapReadback(HeightmapReadback* hb) { m_heightmapReadback = hb; }

    const std::vector<Disturbance>& getDisturbances() const { return m_disturbances; }
    void clearDisturbances() { m_disturbances.clear(); }


};

} // namespace WaterSimulation

