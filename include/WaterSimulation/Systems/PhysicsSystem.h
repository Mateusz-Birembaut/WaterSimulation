#pragma once

#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/PhysicsUtils.h>

#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Matrix3.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>


namespace WaterSimulation
{
    

enum class PhysicsType
{
    STATIC,
    DYNAMIC
};

struct RigidBodyComponent {

    TransformComponent * transform;
    Mesh * mesh; //a enlever

    Magnum::Vector3 linearVelocity;
    Magnum::Vector3 angularVelocity;

    Magnum::Vector3 forceAccumulator;
    Magnum::Vector3 torqueAccumulator;

    float mass;
    float inverseMass;

    Magnum::Matrix3 localInertiaTensor; //utile seulement pour la rotation
    Magnum::Matrix3 localInverseInertiaTensor;
    Magnum::Matrix3 globalInverseInertiaTensor;

    Magnum::Vector3 globalCentroid;
    Magnum::Vector3 localCentroid;

    AABBCollider aabbCollider; //chaque objet a une aabb en coordonée globale

    std::vector<Collider*> colliders;

    PhysicsType bodyType = PhysicsType::DYNAMIC;
    bool isPaused = false;
    bool useGravity = true;

    float linearDamping = 0.25f;
    float angularDamping = 0.5f;

    float restitution = 0.25f;
    float friction = 2.0f;

    RigidBodyComponent(){
        mass = 1.0f;
        inverseMass = 1.0f;
        localInertiaTensor = Magnum::Matrix3{Magnum::Math::ZeroInit};
        localInverseInertiaTensor = Magnum::Matrix3{Magnum::Math::ZeroInit};
        globalInverseInertiaTensor = Magnum::Matrix3{Magnum::Math::ZeroInit};
        globalCentroid = Magnum::Vector3{0.0f};
        localCentroid = Magnum::Vector3{0.0f};
        linearVelocity = Magnum::Vector3{0.0f};
        angularVelocity = Magnum::Vector3{0.0f};
        forceAccumulator = Magnum::Vector3{0.0f};
        torqueAccumulator = Magnum::Vector3{0.0f};
    }   

    void onAttach(Registry & registry, Entity entity){
        auto& t = registry.get<TransformComponent>(entity);
        transform = &t;
    };
    void onDetach(Registry & registry, Entity entity){};

    void addCollider(Collider* collider);
    
    void addForceAt(const Magnum::Vector3& force, const Magnum::Vector3& point){
        forceAccumulator += force;
        torqueAccumulator += Magnum::Math::cross(point - globalCentroid, force);
    };

        void addTorque(const Magnum::Vector3& torque){
            torqueAccumulator += torque;
    };

        Magnum::Vector3 getVelocityAt(const Magnum::Vector3& point) const {
            return linearVelocity + Magnum::Math::cross(angularVelocity, point-globalCentroid);
    };

};



class PhysicsSystem  {

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

    void applyBuoyancy(Registry& registry);

public : 

    void update(Registry& registry, float deltaTime );
    
    std::vector<CollisionInfo> getCollisionList(){return collisionList;}



};

} // namespace WaterSimulation

