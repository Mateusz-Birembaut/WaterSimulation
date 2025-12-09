
#include <WaterSimulation/Systems/PhysicsSystem.h>

#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/BuoyancyComponent.h>
#include <WaterSimulation/Components/WaterComponent.h>
#include <WaterSimulation/Components/MaterialComponent.h>
#include <WaterSimulation/PhysicsUtils.h>

#include <Corrade/Utility/Debug.h>
#include <Magnum/Math/Constants.h>
#include <Magnum/Math/Functions.h>

#include <cmath>
#include <limits>
#include <cstring>

namespace WaterSimulation
{
	


//ajoute un collider a un rigidbody et recalcule les vars necessaire
void RigidBodyComponent::addCollider(Collider* collider){
    assert(collider != nullptr);
    colliders.push_back(collider);

    mass = 0.0f;
    inverseMass = 0.0f;

    localCentroid = Magnum::Vector3{0.0f};
    for(Collider* colptr : colliders){
        const Collider& col = *colptr;

        localCentroid += col.mass * col.localCentroid;

        mass += col.mass;
    }

    inverseMass = 1.0f / mass;

    localCentroid *= inverseMass;

    localInertiaTensor = Magnum::Matrix3{0.0f};

    for(Collider* colptr : colliders){
        const Collider& col = *colptr;

        const Magnum::Vector3 r = localCentroid - col.localCentroid;
        const float rDotR = Magnum::Math::dot(r,r);
        Magnum::Matrix3 rOutR{
            {r.x()*r.x(), r.x()*r.y(), r.x()*r.z()},
            {r.y()*r.x(), r.y()*r.y(), r.y()*r.z()},
            {r.z()*r.x(), r.z()*r.y(), r.z()*r.z()}
        };

        localInertiaTensor += col.localInertiaTensor + col.mass * (rDotR * Magnum::Matrix3{1.0f} - rOutR);
    }

    localInverseInertiaTensor = localInertiaTensor.inverted();
}

//recalcule l'aabb a partir de la liste des colliders de l'objet
void PhysicsSystem::recomputeAABB(Registry& registry){

    auto view = registry.view<RigidBodyComponent, TransformComponent>();

    for(Entity entity : view){
        RigidBodyComponent& rigidBody = view.get<RigidBodyComponent>(entity);
        TransformComponent& transform = view.get<TransformComponent>(entity);
        if (rigidBody.bodyType == PhysicsType::STATIC)
            continue;

        Magnum::Vector3 min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        Magnum::Vector3 max{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

        for(Collider* colptr : rigidBody.colliders){

            const Collider& collider = *colptr;

            if(collider.type == ColliderType::OBB){
                const OBBCollider& obb = (const OBBCollider&) collider;
                Magnum::Vector3 vertices[8];
                Magnum::Vector3 globalPos = transform.globalModel.transformPoint(obb.localCentroid);
                obb.getVertices(globalPos, transform.right(), transform.up(), transform.forward(), vertices);

                for (const Magnum::Vector3& vertex : vertices) {
                    min.x() = std::min(min.x(), vertex.x());
                    min.y() = std::min(min.y(), vertex.y());
                    min.z() = std::min(min.z(), vertex.z());

                    max.x() = std::max(max.x(), vertex.x());
                    max.y() = std::max(max.y(), vertex.y());
                    max.z() = std::max(max.z(), vertex.z());
                }

                float offset = 0.5f; //on veut que l'aabb soit un peu plus grande
                rigidBody.aabbCollider.min = min - Magnum::Vector3{offset};
                rigidBody.aabbCollider.max = max + Magnum::Vector3{offset};
            }else if(collider.type == ColliderType::SPHERE){

                const SphereCollider& sphere = (const SphereCollider&) collider;

                Magnum::Vector3 sphereCenter = transform.globalModel.transformPoint(sphere.localCentroid);
                float sphereRadius = sphere.radius * transform.scale.x();
                min.x() = std::min(min.x(), sphereCenter.x() - sphereRadius);
                min.y() = std::min(min.y(), sphereCenter.y() - sphereRadius);
                min.z() = std::min(min.z(), sphereCenter.z() - sphereRadius);

                max.x() = std::max(max.x(), sphereCenter.x() + sphereRadius);
                max.y() = std::max(max.y(), sphereCenter.y() + sphereRadius);
                max.z() = std::max(max.z(), sphereCenter.z() + sphereRadius);

                rigidBody.aabbCollider.min = min;
                rigidBody.aabbCollider.max = max;

            }else if(collider.type == ColliderType::PLANE){

                const PlaneCollider& plane = (const PlaneCollider&) collider;
                Magnum::Vector3 planePosition = transform.globalModel.transformPoint(plane.localCentroid);
                Magnum::Vector3 planeNormal = transform.globalModel.transformVector(plane.normal);
                planeNormal = planeNormal.normalized();

                Magnum::Vector3 maxValue{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
                Magnum::Vector3 minValue{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

                auto perpendicular = [&](const Magnum::Vector3& normal){
                    if(std::abs(normal.x()) <= std::abs(normal.y()) && std::abs(normal.x()) <= std::abs(normal.z())){
                        return Magnum::Vector3{0, -normal.z(), normal.y()};
                    }else if (std::abs(normal.y()) <= std::abs(normal.z())){
                        return Magnum::Vector3{-normal.z(), 0, normal.x()};
                    }else{
                        return Magnum::Vector3{-normal.y(), normal.x(), 0};
                    }
                };

                Magnum::Vector3 tangent = perpendicular(planeNormal).normalized();
                Magnum::Vector3 bitangent = Magnum::Math::cross(planeNormal, tangent).normalized();

                Magnum::Vector3 corner1 = planePosition + tangent * maxValue + bitangent * maxValue;
                Magnum::Vector3 corner2 = planePosition + tangent * maxValue + bitangent * minValue;
                Magnum::Vector3 corner3 = planePosition + tangent * minValue + bitangent * maxValue;
                Magnum::Vector3 corner4 = planePosition + tangent * minValue + bitangent * minValue;

                Magnum::Vector3 corners[4] = { corner1, corner2, corner3, corner4 };

                for (const Magnum::Vector3& corner : corners) {
                    min.x() = std::min(min.x(), corner.x());
                    min.y() = std::min(min.y(), corner.y());
                    min.z() = std::min(min.z(), corner.z());

                    max.x() = std::max(max.x(), corner.x());
                    max.y() = std::max(max.y(), corner.y());
                    max.z() = std::max(max.z(), corner.z());
                }
 
                 rigidBody.aabbCollider.min = min;
                 rigidBody.aabbCollider.max = max;

            }else if(collider.type == ColliderType::CYLINDER){

                const CylinderCollider& cylinderCollider = (const CylinderCollider&) collider;

                Magnum::Vector3 worldAxis = transform.globalModel.transformVector(cylinderCollider.axis).normalized();
                Magnum::Vector3 worldPos = transform.globalModel.transformPoint(cylinderCollider.localCentroid);
                float worldRadius = cylinderCollider.radius * std::max(std::max(transform.scale.x(), transform.scale.y()), transform.scale.z());
                float worldHeight = cylinderCollider.halfSize * std::max(std::max(transform.scale.x(), transform.scale.y()), transform.scale.z());

                Magnum::Vector3 start = worldPos + worldAxis * worldHeight;
                Magnum::Vector3 end = worldPos - worldAxis * worldHeight;

                min.x() = std::min(start.x(), end.x()) - worldRadius;
                min.y() = std::min(min.y(), std::min(start.y(), end.y()) - worldRadius);
                min.z() = std::min(min.z(), std::min(start.z(), end.z()) - worldRadius);

                max.x() = std::max(max.x(), std::max(start.x(), end.x()) + worldRadius);
                max.y() = std::max(max.y(), std::max(start.y(), end.y()) + worldRadius);
                max.z() = std::max(max.z(), std::max(start.z(), end.z()) + worldRadius);

                rigidBody.aabbCollider.min = min;
                rigidBody.aabbCollider.max = max;

            }else if(collider.type == ColliderType::MESH){

                const MeshCollider& meshCollider = (const MeshCollider&) collider;
                if(meshCollider.vertices.empty())
                    continue;

                for(const Magnum::Vector3& vertex : meshCollider.vertices){
                    Magnum::Vector3 worldVertex = transform.globalModel.transformPoint(vertex);
                    min.x() = std::min(min.x(), worldVertex.x());
                    min.y() = std::min(min.y(), worldVertex.y());
                    min.z() = std::min(min.z(), worldVertex.z());

                    max.x() = std::max(max.x(), worldVertex.x());
                    max.y() = std::max(max.y(), worldVertex.y());
                    max.z() = std::max(max.z(), worldVertex.z());
                }

                rigidBody.aabbCollider.min = min;
                rigidBody.aabbCollider.max = max;



            }

        }

    }

}

//integration numerique 
void PhysicsSystem::integrate(Registry& registry, float deltaTime){

    auto view = registry.view<RigidBodyComponent, TransformComponent>();

    for(Entity entity : view){
        RigidBodyComponent& rigidBody = view.get<RigidBodyComponent>(entity);
        TransformComponent& transform = view.get<TransformComponent>(entity);

        //-----
        Magnum::Matrix3 rotation = transform.rotation.toMatrix();
        rigidBody.globalInverseInertiaTensor = rotation * rigidBody.localInverseInertiaTensor * rotation.transposed();         
        rigidBody.globalCentroid = transform.globalModel.transformPoint(rigidBody.localCentroid);
        //-----
        
        if(rigidBody.isPaused) continue;

        if(rigidBody.bodyType == PhysicsType::STATIC){
            rigidBody.linearVelocity = Magnum::Vector3{0.0f};
            rigidBody.angularVelocity = Magnum::Vector3{0.0f};
            continue;
        }

        
        Magnum::Vector3 acceleration = rigidBody.forceAccumulator * rigidBody.inverseMass;
        acceleration += gravity;
        if(acceleration.length() > 100) {
            std::cout << "Acceleration: (" << acceleration.x() << ", " << acceleration.y() << ", " << acceleration.z() << ")" << std::endl;
            std::cout << "DeltaTime: " << deltaTime << std::endl;
        }
        rigidBody.linearVelocity += acceleration * deltaTime;

        rigidBody.linearVelocity *= (1.0f - (rigidBody.linearDamping * deltaTime));

        if(rigidBody.linearVelocity.length() > 0.01){ //permet de limiter les tremblements
            transform.position += rigidBody.linearVelocity * deltaTime;
        }

        Magnum::Vector3 angularAcceleration = rigidBody.globalInverseInertiaTensor * rigidBody.torqueAccumulator;
        rigidBody.angularVelocity += angularAcceleration * deltaTime;

        rigidBody.angularVelocity *= (1.0f - (rigidBody.angularDamping * deltaTime));

        //on essaie de limiter la casse encore
        float maxAngularSpeedXZ = float(Magnum::Deg(30.0f));
        //rigidBody.angularVelocity.x() = Magnum::Math::clamp(rigidBody.angularVelocity.x(), -maxAngularSpeedXZ, maxAngularSpeedXZ);
        //rigidBody.angularVelocity.z() = Magnum::Math::clamp(rigidBody.angularVelocity.z(), -maxAngularSpeedXZ, maxAngularSpeedXZ);

        float angle = rigidBody.angularVelocity.length();
        if (angle > 0.001f) {
            Magnum::Vector3 axis = rigidBody.angularVelocity / angle;
            float halfAngle = 0.5f * angle * deltaTime;
            Magnum::Quaternion deltaRot = Magnum::Quaternion::rotation(Magnum::Rad{halfAngle}, axis);
            transform.rotation = (deltaRot * transform.rotation).normalized();
        }

        rigidBody.forceAccumulator = Magnum::Vector3{0.0f};
        rigidBody.torqueAccumulator = Magnum::Vector3{0.0f};

        Magnum::Matrix4 updatedModel = transform.model();
        transform.globalModel = updatedModel;
        transform.inverseGlobalModel = updatedModel.inverted();
    }

}

//broad phase utilise les aabb classique
void PhysicsSystem::broadCollisionDetection(Registry& registry){

    collisionList.clear();
    //Console::getInstance().clearLogs();

    auto view = registry.view<RigidBodyComponent, TransformComponent>();

    for(Entity entityA : view){

        RigidBodyComponent& rigidBodyA = view.get<RigidBodyComponent>(entityA);
        TransformComponent& transformA = view.get<TransformComponent>(entityA);

        for(Entity entityB : view){

            if(entityA == entityB) break;

            RigidBodyComponent& rigidBodyB = view.get<RigidBodyComponent>(entityB);
            TransformComponent& transformB = view.get<TransformComponent>(entityB);

            if (rigidBodyA.bodyType == PhysicsType::STATIC && rigidBodyB.bodyType == PhysicsType::STATIC){
                //Console::getInstance().addLog("skipping");
                continue;
            }

            bool collisionX = rigidBodyA.aabbCollider.min.x() <= rigidBodyB.aabbCollider.max.x() &&
                               rigidBodyA.aabbCollider.max.x() >= rigidBodyB.aabbCollider.min.x();

            bool collisionY = rigidBodyA.aabbCollider.min.y() <= rigidBodyB.aabbCollider.max.y() &&
                               rigidBodyA.aabbCollider.max.y() >= rigidBodyB.aabbCollider.min.y();

            bool collisionZ = rigidBodyA.aabbCollider.min.z() <= rigidBodyB.aabbCollider.max.z() &&
                               rigidBodyA.aabbCollider.max.z() >= rigidBodyB.aabbCollider.min.z();

            if (collisionX && collisionY && collisionZ) {
                //Console::getInstance().addLog("broad phase collision e " + std::to_string(entityA) + " e " + std::to_string(entityB));

                narrowCollisionDetection(entityA, rigidBodyA, transformA,entityB, rigidBodyB, transformB);
            }else{
                //Console::getInstance().addLog("no broad Collision");
            }


        }

    }


}

//narrow phase
void PhysicsSystem::narrowCollisionDetection(Entity entityA, RigidBodyComponent& rigidBodyA, TransformComponent& transformA, Entity entityB ,RigidBodyComponent& rigidBodyB, TransformComponent& transformB){

    for(const Collider* colliderAptr : rigidBodyA.colliders){
        for(const Collider* colliderBptr : rigidBodyB.colliders){
            CollisionInfo collisionInfo;
            collisionInfo.entityA = entityA;
            collisionInfo.entityB = entityB;

            const Collider& colliderA = *colliderAptr;
            const Collider& colliderB = *colliderBptr;

            if (handleSphereTerrainCollision(entityA, rigidBodyA, transformA, colliderA,
                                             entityB, rigidBodyB, transformB, colliderB, collisionInfo)) {
                collisionList.push_back(collisionInfo);
                continue;
            }

            CollisionDetection::testCollision(entityA, colliderA, transformA, entityB, colliderB, transformB, collisionInfo);
            if(collisionInfo.isColliding){
                collisionList.push_back(collisionInfo);
                //Console::getInstance().addLog("collision narrow phase e " + std::to_string(entityA) + " e " + std::to_string(entityB));

                //Console::getInstance().addLog("collision info normal = (" + std::to_string(collisionInfo.normal.x) + ", " + std::to_string(collisionInfo.normal.y) + ", " + std::to_string(collisionInfo.normal.z) + "), pen depth = " + std::to_string(collisionInfo.penetrationDepth));
            }else{
                //Console::getInstance().addLog("no narrow collision");
            }
        }
    }

}

bool PhysicsSystem::handleSphereTerrainCollision(Entity entityA,
                                      RigidBodyComponent& rigidBodyA,
                                      TransformComponent& transformA,
                                      const Collider& colliderA,
                                      Entity entityB,
                                      RigidBodyComponent& rigidBodyB,
                                      TransformComponent& transformB,
                                      const Collider& colliderB,
                                      CollisionInfo& collisionInfo) const {
    if (!m_heightmapReadback)
        return false;

    const SphereCollider* sphereCollider = nullptr;
    const MeshCollider* meshCollider = nullptr;
    TransformComponent* sphereTransform = nullptr;
    TransformComponent* meshTransform = nullptr;
    bool sphereIsEntityA = false;

    if (colliderA.type == ColliderType::SPHERE && colliderB.type == ColliderType::MESH) {
        sphereCollider = static_cast<const SphereCollider*>(&colliderA);
        meshCollider = static_cast<const MeshCollider*>(&colliderB);
        sphereTransform = &transformA;
        meshTransform = &transformB;
        sphereIsEntityA = true;
    } else if (colliderA.type == ColliderType::MESH && colliderB.type == ColliderType::SPHERE) {
        sphereCollider = static_cast<const SphereCollider*>(&colliderB);
        meshCollider = static_cast<const MeshCollider*>(&colliderA);
        sphereTransform = &transformB;
        meshTransform = &transformA;
        sphereIsEntityA = false;
    } else {
        return false;
    }

    if (!meshCollider || meshCollider->vertices.empty())
        return false;

    const auto& terrainHeights = m_heightmapReadback->terrainHeightmap();
    const Magnum::Vector2i terrainSize = meshCollider->resolution;
    if (terrainHeights.empty() || terrainSize.x() <= 1 || terrainSize.y() <= 1)
        return false;

    const std::size_t expectedTexels = std::size_t(terrainSize.x()) * std::size_t(terrainSize.y());
    if (terrainHeights.size() < expectedTexels)
        return false;

    const float minX = meshCollider->localMin.x();
    const float maxX = meshCollider->localMax.x();
    const float minZ = meshCollider->localMin.z();
    const float maxZ = meshCollider->localMax.z();

    const float spanX = maxX - minX;
    const float spanZ = maxZ - minZ;
    if (spanX <= 0.0f || spanZ <= 0.0f)
        return false;

    Magnum::Matrix4 sphereModel = sphereTransform->globalModel;
    Magnum::Vector3 sphereCenter = (sphereModel * Magnum::Vector4{sphereCollider->localCentroid, 1.0f}).xyz();
    const float radiusScale = std::max({sphereTransform->scale.x(), sphereTransform->scale.y(), sphereTransform->scale.z()});
    const float sphereRadius = sphereCollider->radius * radiusScale;

    const Magnum::Matrix4& terrainInverse = meshTransform->inverseGlobalModel;
    Magnum::Vector3 localCenter = (terrainInverse * Magnum::Vector4{sphereCenter, 1.0f}).xyz();

    const float normalizedX = (localCenter.x() - minX) / spanX;
    const float normalizedZ = (localCenter.z() - minZ) / spanZ;

    if (normalizedX < 0.0f || normalizedX > 1.0f || normalizedZ < 0.0f || normalizedZ > 1.0f)
        return false;

    const float gridX = normalizedX * float(terrainSize.x() - 1);
    const float gridZ = normalizedZ * float(terrainSize.y() - 1);

    const float cellSizeX = spanX / float(terrainSize.x() - 1);
    const float cellSizeZ = spanZ / float(terrainSize.y() - 1);

    auto sampleHeightAndNormal = [&](float gx, float gz) -> std::pair<float, Magnum::Vector3> {
        gx = Magnum::Math::clamp(gx, 0.0f, float(terrainSize.x() - 1));
        gz = Magnum::Math::clamp(gz, 0.0f, float(terrainSize.y() - 1));

        const int x0 = int(Magnum::Math::floor(gx));
        const int z0 = int(Magnum::Math::floor(gz));
        const int x1 = Magnum::Math::min(x0 + 1, terrainSize.x() - 1);
        const int z1 = Magnum::Math::min(z0 + 1, terrainSize.y() - 1);

        const float tx = gx - float(x0);
        const float tz = gz - float(z0);

        auto index = [&](int x, int z) -> std::size_t {
            return std::size_t(z) * std::size_t(terrainSize.x()) + std::size_t(x);
        };

        const float h00 = terrainHeights[index(x0, z0)] * 1.5f;
        const float h10 = terrainHeights[index(x1, z0)] * 1.5f;
        const float h01 = terrainHeights[index(x0, z1)] * 1.5f;
        const float h11 = terrainHeights[index(x1, z1)] * 1.5f;

        // Local positions of the cell corners (x horizontal, z depth)
        const float lx0 = minX + float(x0) * cellSizeX;
        const float lx1 = minX + float(x1) * cellSizeX;
        const float lz0 = minZ + float(z0) * cellSizeZ;
        const float lz1 = minZ + float(z1) * cellSizeZ;

        float height = 0.0f;
        Magnum::Vector3 p0, p1, p2;

        // Choose the triangle of the quad based on tx/tz and interpolate on that triangle.
        if (tx + tz <= 1.0f) {
            // Triangle (x0,z0)-(x1,z0)-(x0,z1)
            height = h00 + tx * (h10 - h00) + tz * (h01 - h00);
            p0 = {lx0, h00, lz0};
            p1 = {lx1, h10, lz0};
            p2 = {lx0, h01, lz1};
        } else {
            // Triangle (x1,z1)-(x1,z0)-(x0,z1)
            const float tx1 = 1.0f - tx;
            const float tz1 = 1.0f - tz;
            height = h11 + tx1 * (h01 - h11) + tz1 * (h10 - h11);
            p0 = {lx1, h11, lz1};
            p1 = {lx1, h10, lz0};
            p2 = {lx0, h01, lz1};
        }

        Magnum::Vector3 normalLocal = Magnum::Math::cross(p1 - p0, p2 - p0);
        const float nLen = normalLocal.length();
        if (nLen > 1.0e-6f) {
            normalLocal /= nLen;
            if (normalLocal.y() < 0.0f) normalLocal = -normalLocal; // keep y-up
        } else {
            normalLocal = Magnum::Vector3{0.0f, 1.0f, 0.0f};
        }

        return {height, normalLocal};
    };

    const auto [heightLocal, triNormalLocal] = sampleHeightAndNormal(gridX, gridZ);
    Magnum::Vector3 localSurface{localCenter.x(), heightLocal, localCenter.z()};
    Magnum::Vector3 worldSurface = (meshTransform->globalModel * Magnum::Vector4{localSurface, 1.0f}).xyz();

    // Compute contact relative to the terrain plane normal to avoid under-estimating penetration on slopes
    Magnum::Matrix3 normalMatrix = meshTransform->globalModel.rotationScaling().inverted().transposed();
    Magnum::Vector3 normalWorld = (normalMatrix * triNormalLocal).normalized();

    // Signed distance from sphere center to the terrain plane along the plane normal
    const float distanceToPlane = Magnum::Math::dot(sphereCenter - worldSurface, normalWorld);
    float penetration = sphereRadius - distanceToPlane;
    const float verticalPenetration = worldSurface.y() - (sphereCenter.y() - sphereRadius);
    if (verticalPenetration > 0.0f)
        penetration = Magnum::Math::min(penetration, verticalPenetration);
    if (penetration <= 0.0f)
        return false;

    Magnum::Vector3 sphereContact = sphereCenter - normalWorld * sphereRadius;

    collisionInfo.isColliding = true;
    collisionInfo.penetrationDepth = penetration;

    if (sphereIsEntityA) {
        collisionInfo.normal = normalWorld;
        collisionInfo.collisionPointA = sphereContact;
        collisionInfo.collisionPointB = worldSurface - normalWorld * distanceToPlane;
    } else {
        collisionInfo.normal = -normalWorld;
        collisionInfo.collisionPointA = worldSurface - normalWorld * distanceToPlane;
        collisionInfo.collisionPointB = sphereContact;
    }

    return true;
}

//solver a impulsion avec la rotation (ne marche pas avec tous les objets)
void PhysicsSystem::collisionResolution(Registry& registry) {
    for (CollisionInfo& collision : collisionList) {
        RigidBodyComponent& rigidBodyA = registry.get<RigidBodyComponent>(collision.entityA);
        RigidBodyComponent& rigidBodyB = registry.get<RigidBodyComponent>(collision.entityB);

        TransformComponent& transformA = registry.get<TransformComponent>(collision.entityA);
        TransformComponent& transformB = registry.get<TransformComponent>(collision.entityB);

        Magnum::Vector3 correction = collision.normal * collision.penetrationDepth * 0.5f;

        if (rigidBodyA.bodyType != PhysicsType::STATIC && rigidBodyB.bodyType != PhysicsType::STATIC) {
            transformA.position += correction;
            transformB.position -= correction;
            Magnum::Matrix4 modelA = transformA.model();
            transformA.globalModel = modelA;
            transformA.inverseGlobalModel = modelA.inverted();
            Magnum::Matrix4 modelB = transformB.model();
            transformB.globalModel = modelB;
            transformB.inverseGlobalModel = modelB.inverted();
        } else if (rigidBodyA.bodyType != PhysicsType::STATIC) {
            transformA.position += correction * 2.0f;
            Magnum::Matrix4 modelA = transformA.model();
            transformA.globalModel = modelA;
            transformA.inverseGlobalModel = modelA.inverted();
        } else if (rigidBodyB.bodyType != PhysicsType::STATIC) {
            transformB.position -= correction * 2.0f;
            Magnum::Matrix4 modelB = transformB.model();
            transformB.globalModel = modelB;
            transformB.inverseGlobalModel = modelB.inverted();
        }

        Magnum::Vector3 rA = collision.collisionPointA - rigidBodyA.globalCentroid;
        Magnum::Vector3 rB = collision.collisionPointB - rigidBodyB.globalCentroid;

        Magnum::Vector3 vA = rigidBodyA.linearVelocity + Magnum::Math::cross(rigidBodyA.angularVelocity, rA);
        Magnum::Vector3 vB = rigidBodyB.linearVelocity + Magnum::Math::cross(rigidBodyB.angularVelocity, rB);
        Magnum::Vector3 relativeVelocity = vB - vA;
        float velocityAlongNormal = Magnum::Math::dot(relativeVelocity, collision.normal);

        if (velocityAlongNormal < 0)
            continue;

        Magnum::Vector3 raCrossN = Magnum::Math::cross(rA, collision.normal);
        Magnum::Vector3 rbCrossN = Magnum::Math::cross(rB, collision.normal);

        Magnum::Vector3 IA_raCrossN = rigidBodyA.globalInverseInertiaTensor * raCrossN;
        Magnum::Vector3 IB_rbCrossN = rigidBodyB.globalInverseInertiaTensor * rbCrossN;

        float denom = rigidBodyA.inverseMass + rigidBodyB.inverseMass + Magnum::Math::dot(Magnum::Math::cross(IA_raCrossN, rA) + Magnum::Math::cross(IB_rbCrossN, rB), collision.normal);

        float restitution = std::min(rigidBodyA.restitution, rigidBodyB.restitution);
        float impulseStrength = -(1.0f + restitution) * velocityAlongNormal / denom;

        Magnum::Vector3 impulse = impulseStrength * collision.normal;

        float linearDenom = rigidBodyA.inverseMass + rigidBodyB.inverseMass;
        float linearImpulseStrength = -(1.0f + restitution) * velocityAlongNormal / linearDenom;
        Magnum::Vector3 linearImpulse = linearImpulseStrength * collision.normal;
        impulse = linearImpulse;

        if (rigidBodyA.bodyType != PhysicsType::STATIC) {
            rigidBodyA.linearVelocity -= impulse * rigidBodyA.inverseMass;
            rigidBodyA.angularVelocity -= IA_raCrossN * impulseStrength;
        }
        if (rigidBodyB.bodyType != PhysicsType::STATIC) {
            rigidBodyB.linearVelocity += impulse * rigidBodyB.inverseMass;
            rigidBodyB.angularVelocity += IB_rbCrossN * impulseStrength;
        }

        Magnum::Vector3 tangent = relativeVelocity - Magnum::Math::dot(relativeVelocity, collision.normal) * collision.normal;
        if (tangent.length() > 0.1f) tangent = tangent.normalized();
        else continue;

        Magnum::Vector3 raCrossT = Magnum::Math::cross(rA, tangent);
        Magnum::Vector3 rbCrossT = Magnum::Math::cross(rB, tangent);

        Magnum::Vector3 IA_raCrossT = rigidBodyA.globalInverseInertiaTensor * raCrossT;
        Magnum::Vector3 IB_rbCrossT = rigidBodyB.globalInverseInertiaTensor * rbCrossT;

        float denomFriction = rigidBodyA.inverseMass + rigidBodyB.inverseMass + Magnum::Math::dot(Magnum::Math::cross(IA_raCrossT, rA) + Magnum::Math::cross(IB_rbCrossT, rB), tangent);

        float impulseFriction;
        Magnum::Vector3 frictionImpulse{0.0f};
        if (denomFriction > 0.0f) {
            //Console::getInstance().addLog("applying friction ");
            impulseFriction = -Magnum::Math::dot(relativeVelocity, tangent) / denomFriction;
            float mu = std::min(rigidBodyA.friction, rigidBodyB.friction);
            float maxFriction = std::abs(impulseStrength) * mu;
            impulseFriction = Magnum::Math::clamp(impulseFriction, -maxFriction, maxFriction);
            frictionImpulse = impulseFriction * tangent;

            if (rigidBodyA.bodyType != PhysicsType::STATIC) {
                rigidBodyA.linearVelocity -= frictionImpulse * rigidBodyA.inverseMass;
                //Console::getInstance().addLog("Friction Impulse: (" + std::to_string(frictionImpulse.x()) + ", " + std::to_string(frictionImpulse.y()) + ", " + std::to_string(frictionImpulse.z()) + ")");
                Magnum::Vector3 deltaAngularVelocity = IA_raCrossT * impulseFriction;
                rigidBodyA.angularVelocity -= deltaAngularVelocity;
                //Console::getInstance().addLog("Delta Angular Velocity: (" + std::to_string(deltaAngularVelocity.x()) + ", " + std::to_string(deltaAngularVelocity.y()) + ", " + std::to_string(deltaAngularVelocity.z()) + ")");

            }
            if (rigidBodyB.bodyType != PhysicsType::STATIC) {
                rigidBodyB.linearVelocity += frictionImpulse * rigidBodyB.inverseMass;
                rigidBodyB.angularVelocity += IB_rbCrossT * impulseFriction;
            }
        }
        /* Console::getInstance().addLog("Collision detected between Entity " + std::to_string(collision.entityA) + " and Entity " + std::to_string(collision.entityB));
        Console::getInstance().addLog("Collision Point A: (" + std::to_string(collision.collisionPointA.x) + ", " + std::to_string(collision.collisionPointA.y) + ", " + std::to_string(collision.collisionPointA.z) + ")");
        Console::getInstance().addLog("Collision Point B: (" + std::to_string(collision.collisionPointB.x) + ", " + std::to_string(collision.collisionPointB.y) + ", " + std::to_string(collision.collisionPointB.z) + ")");
        Console::getInstance().addLog("Collision Normal: (" + std::to_string(collision.normal.x) + ", " + std::to_string(collision.normal.y) + ", " + std::to_string(collision.normal.z) + ")");
        Console::getInstance().addLog("Penetration Depth: " + std::to_string(collision.penetrationDepth));
        Console::getInstance().addLog("Impulse: (" + std::to_string(impulse.x) + ", " + std::to_string(impulse.y) + ", " + std::to_string(impulse.z) + ")");
        Console::getInstance().addLog("Friction Impulse: (" + std::to_string(frictionImpulse.x) + ", " + std::to_string(frictionImpulse.y) + ", " + std::to_string(frictionImpulse.z) + ")");
     */

    }
}

//solver qui met a jour seulement la vitesse lineaire
void PhysicsSystem::collisionResolutionLinear(Registry& registry) {
    for (CollisionInfo& collision : collisionList) {
        RigidBodyComponent& rigidBodyA = registry.get<RigidBodyComponent>(collision.entityA);
        RigidBodyComponent& rigidBodyB = registry.get<RigidBodyComponent>(collision.entityB);

        TransformComponent& transformA = registry.get<TransformComponent>(collision.entityA);
        TransformComponent& transformB = registry.get<TransformComponent>(collision.entityB);

        Magnum::Vector3 correction = collision.normal * collision.penetrationDepth * 0.5f;

        if (rigidBodyA.bodyType != PhysicsType::STATIC && rigidBodyB.bodyType != PhysicsType::STATIC) {
            transformA.position += correction;
            transformB.position -= correction;
            Magnum::Matrix4 modelA = transformA.model();
            transformA.globalModel = modelA;
            transformA.inverseGlobalModel = modelA.inverted();
            Magnum::Matrix4 modelB = transformB.model();
            transformB.globalModel = modelB;
            transformB.inverseGlobalModel = modelB.inverted();
        } else if (rigidBodyA.bodyType != PhysicsType::STATIC) {
            transformA.position += correction * 2.0f;
            Magnum::Matrix4 modelA = transformA.model();
            transformA.globalModel = modelA;
            transformA.inverseGlobalModel = modelA.inverted();
        } else if (rigidBodyB.bodyType != PhysicsType::STATIC) {
            transformB.position -= correction * 2.0f;
            Magnum::Matrix4 modelB = transformB.model();
            transformB.globalModel = modelB;
            transformB.inverseGlobalModel = modelB.inverted();
        }

        Magnum::Vector3 vA = rigidBodyA.linearVelocity;
        Magnum::Vector3 vB = rigidBodyB.linearVelocity;
        Magnum::Vector3 relativeVelocity = vB - vA;
        float velocityAlongNormal = Magnum::Math::dot(relativeVelocity, collision.normal);

        if (velocityAlongNormal < 0)
            continue;

        float restitution = std::min(rigidBodyA.restitution, rigidBodyB.restitution);
        float denom = rigidBodyA.inverseMass + rigidBodyB.inverseMass;

        if (denom == 0.0f)
            continue;

        float impulseStrength = -(1.0f + restitution) * velocityAlongNormal / denom;
        Magnum::Vector3 impulse = impulseStrength * collision.normal;

        if (rigidBodyA.bodyType != PhysicsType::STATIC) {
            rigidBodyA.linearVelocity -= impulse * rigidBodyA.inverseMass;
        }
        if (rigidBodyB.bodyType != PhysicsType::STATIC) {
            rigidBodyB.linearVelocity += impulse * rigidBodyB.inverseMass;
        }
    }
}

void PhysicsSystem::applyBuoyancy(Registry& registry) {

    m_disturbances.clear();

    auto waterView = registry.view<MeshComponent, TransformComponent, WaterComponent>();

    if (waterView.begin() == waterView.end()) {
        return; 
    }

    auto waterEntity = *waterView.begin();
    TransformComponent& transformComp = registry.get<TransformComponent>(waterEntity);
    MaterialComponent& materialComp = registry.get<MaterialComponent>(waterEntity);
    WaterComponent& wC = registry.get<WaterComponent>(waterEntity);

    if (m_heightmapReadback)
        m_heightmapReadback->fetchLatestCPUCopy();

    const bool hasHeightmap = m_heightmapReadback && m_heightmapReadback->hasCpuData();
    const Magnum::Vector2i heightmapSize = hasHeightmap ? m_heightmapReadback->size() : Magnum::Vector2i{0};
    
    auto view = registry.view<TransformComponent, RigidBodyComponent, BuoyancyComponent>();
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& rb = view.get<RigidBodyComponent>(entity);
        auto& b = view.get<BuoyancyComponent>(entity);

        if (rb.bodyType == PhysicsType::STATIC) continue;

        if (!hasHeightmap || heightmapSize.x() <= 0 || heightmapSize.y() <= 0) continue;

        const float gravityMagnitude = std::abs(gravity.y());
        const float fluidDensity = b.flotability > 0.0f ? b.flotability : 1000.0f;

        for (Collider* colliderPtr : rb.colliders) {

            if (!colliderPtr || colliderPtr->type != ColliderType::SPHERE) continue;

            const SphereCollider& sphere = static_cast<const SphereCollider&>(*colliderPtr);

            const float radiusScale = std::max({transform.scale.x(), transform.scale.y(), transform.scale.z()});
            const float radius = sphere.radius * radiusScale;
            
            if (radius <= 0.0f) continue;

            Magnum::Vector3 sphereCenter = transform.globalModel.transformPoint(sphere.localCentroid);
            Magnum::Vector4 centerWorld{sphereCenter, 1.0f};
            Magnum::Vector3 waterSpacePoint = (transformComp.inverseGlobalModel * centerWorld).xyz();

            const float u = (waterSpacePoint.x() + wC.scale * 0.5f) / wC.scale;
            const float v = (waterSpacePoint.z() + wC.scale * 0.5f) / wC.scale;

            if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) continue;

            const int px = Magnum::Math::clamp(int(u * float(heightmapSize.x() - 1)), 0, heightmapSize.x() - 1);
            const int py = Magnum::Math::clamp(int(v * float(heightmapSize.y() - 1)), 0, heightmapSize.y() - 1);

            const float waterHeightLocal = m_heightmapReadback->heightAt(px, py);
            const Magnum::Vector3 waterState = m_heightmapReadback->stateAt(px, py);
            Magnum::Vector4 surfaceLocal{waterSpacePoint.x(), waterHeightLocal, waterSpacePoint.z(), 1.0f};
            const float waterHeightWorld = (transformComp.globalModel * surfaceLocal).y();

            const float sphereBottom = sphereCenter.y() - radius;

            const float waterDepth = waterState.x();
            if (waterDepth <= 1.0e-3f) continue;

            if (waterHeightWorld <= sphereBottom) continue;

            const float maxSubmersion = radius * 2.0f;
            const float submergedHeight = Magnum::Math::clamp(waterHeightWorld - sphereBottom, 0.0f, maxSubmersion);

            if (submergedHeight <= 0.0f) continue;

            const float submergedVolume = Magnum::Constants::pi() * submergedHeight * submergedHeight * (radius - submergedHeight / 3.0f);
            const float buoyantForceMagnitude = fluidDensity * gravityMagnitude * submergedVolume;

            if (buoyantForceMagnitude <= 0.0f) continue;

            rb.globalCentroid = transform.globalModel.transformPoint(rb.localCentroid);
            //rb.addForceAt({0.0f, buoyantForceMagnitude, 0.0f}, sphereCenter);
            rb.forceAccumulator += Magnum::Vector3{0.0f, buoyantForceMagnitude, 0.0f};

            Magnum::Vector3 waterVelocityLocal{0.0f};
            if (waterDepth > 1.0e-4f) {
                const float invDepth = 1.0f / waterDepth;
                waterVelocityLocal.x() = waterState.y() * invDepth;
                waterVelocityLocal.z() = waterState.z() * invDepth;
            }

            Magnum::Matrix3 waterRotation = transformComp.globalModel.rotationScaling();
            Magnum::Vector3 waterVelocityWorld = waterRotation * waterVelocityLocal;

            Magnum::Vector3 bodyVelocityAtPoint = rb.getVelocityAt(sphereCenter);
            Magnum::Vector3 relativeVelocity = waterVelocityWorld - bodyVelocityAtPoint;
            
            const float submersionRatio = submergedHeight / maxSubmersion;

            const float relSpeed = relativeVelocity.length();
            if (relSpeed > 1.0e-3f) {
                const float referenceArea = Magnum::Constants::pi() * radius * radius;
                const float dragCoefficient = b.waterDrag > 0.0f ? b.waterDrag : 1.0f;
                const float dragMagnitude = 0.5f * fluidDensity * referenceArea * relSpeed * relSpeed * dragCoefficient * submersionRatio;
                Magnum::Vector3 dragForce = dragMagnitude * (relativeVelocity / relSpeed);
                
                rb.forceAccumulator += dragForce;

                const float sphereTop = sphereCenter.y() + radius;
                const float depth = waterHeightWorld - sphereTop;
                float depthAttenuation = 1.0f;

                if (depth > 0.0f) {
                    depthAttenuation = Magnum::Math::clamp(1.0f - (depth / radius), 0.0f, 1.0f);
                }

                if (depthAttenuation > 0.0f) {
                    const float speed = relativeVelocity.length();

                    const float strengthFactor = 0.25f;
                    const float wakeStrength = speed * submersionRatio * strengthFactor * depthAttenuation; 
                    m_disturbances.push_back({px, py, wakeStrength, 0.0f});
                }
            }
        }
    }
}


void PhysicsSystem::update(Registry& registry, float deltaTime) {
    this->deltaTime = deltaTime;

    integrate(registry, deltaTime);

    recomputeAABB(registry);

    broadCollisionDetection(registry);

    collisionResolution(registry);

	applyBuoyancy(registry);

}


} // namespace WaterSimulation
