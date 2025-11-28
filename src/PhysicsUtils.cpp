
#include <WaterSimulation/PhysicsUtils.h>
#include <WaterSimulation/Components/TransformComponent.h>

#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Vector4.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Functions.h>

#include <limits>

namespace WaterSimulation
{
	
using namespace std;

//tableau qui associe au paire d'objet la bonne fonction
CollisionFn CollisionDetection::collisionDispatchTable[CollisionDetection::NUM_COLLIDER_TYPES][CollisionDetection::NUM_COLLIDER_TYPES] = {
    {collision_sphere_sphere, collision_sphere_cylinder, collision_sphere_aabb, collision_sphere_obb, collision_sphere_plane, nullptr, nullptr},
    {nullptr, collision_cylinder_cylinder, collision_cylinder_aabb, collision_cylinder_obb, collision_cylinder_plane, nullptr, nullptr},
    {nullptr, nullptr, collision_aabb_aabb, collision_aabb_obb, collision_aabb_plane, nullptr, nullptr},
    {nullptr, nullptr, nullptr, collision_obb_obb, collision_obb_plane, nullptr, nullptr},
    {nullptr, nullptr, nullptr, nullptr, collision_plane_plane, nullptr, nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}, 
    {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}  
};

//renvoie l'obb en transformation global
void CollisionDetection::getWorldOBB(const OBBCollider& collider, const TransformComponent& transform, WorldOBB& worldOBB){
    Magnum::Matrix4 transformModel = transform.globalModel;
    Magnum::Matrix4 colliderRotation = Magnum::Matrix4::rotationZ(Magnum::Rad{collider.rotation.z()}) * Magnum::Matrix4::rotationY(Magnum::Rad{collider.rotation.y()}) * Magnum::Matrix4::rotationX(Magnum::Rad{collider.rotation.x()});
    Magnum::Matrix4 newModel = transformModel * colliderRotation;
    worldOBB.globalCentroid = (newModel * Magnum::Vector4{collider.localCentroid, 1.0f}).xyz();
    worldOBB.axes[0] = (newModel.transformVector(Magnum::Vector3{1.0f, 0.0f, 0.0f})).normalized();
    worldOBB.axes[1] = (newModel.transformVector(Magnum::Vector3{0.0f, 1.0f, 0.0f})).normalized();
    worldOBB.axes[2] = (newModel.transformVector(Magnum::Vector3{0.0f, 0.0f, 1.0f})).normalized();
    worldOBB.halfSize = collider.halfSize * transform.scale;
}

void CollisionDetection::collision_obb_obb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo){

    collisionInfo.isColliding = false;

    Magnum::Matrix4 modelA = transformA.globalModel;
    Magnum::Matrix4 modelB = transformB.globalModel;

    const OBBCollider& obbA = (const OBBCollider&) colliderA;
    const OBBCollider& obbB = (const OBBCollider&) colliderB;
    
    WorldOBB wobbA;
    WorldOBB wobbB;
    getWorldOBB(obbA, transformA, wobbA);
    getWorldOBB(obbB, transformB, wobbB);

    float minOverlap = std::numeric_limits<float>::max();
    Magnum::Vector3 minAxis;

    //fonction lambda pour tester un axe, algo SAT
    auto testOverlap = [&](const Magnum::Vector3 axis) {
        if(axis.length() < 0.0001f) return true;
        float projectionA = std::abs(Magnum::Math::dot(axis, wobbA.axes[0])) * wobbA.halfSize.x() + std::abs(Magnum::Math::dot(axis, wobbA.axes[1])) * wobbA.halfSize.y() + std::abs(Magnum::Math::dot(axis, wobbA.axes[2])) * wobbA.halfSize.z();
        float projectionB = std::abs(Magnum::Math::dot(axis, wobbB.axes[0])) * wobbB.halfSize.x() + std::abs(Magnum::Math::dot(axis, wobbB.axes[1])) * wobbB.halfSize.y() + std::abs(Magnum::Math::dot(axis, wobbB.axes[2])) * wobbB.halfSize.z();
        float dist = std::abs(Magnum::Math::dot(wobbB.globalCentroid - wobbA.globalCentroid, axis));
        float overlap = projectionA + projectionB - dist;

        if(overlap < 0) return false;

        if(overlap < minOverlap){
            minOverlap = overlap;
            if(Magnum::Math::dot(wobbB.globalCentroid - wobbA.globalCentroid, axis) < 0){
                minAxis = axis;
            }else{
                minAxis = -axis;
            }
        }
        return true;
    };

    //test des 15 axes
    for(int i = 0; i < 3; i++){ if(!testOverlap(wobbA.axes[i])) return; }
    for(int i = 0; i < 3; i++){ if(!testOverlap(wobbB.axes[i])) return; }
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){ 
            Magnum::Vector3 newAxis = Magnum::Math::cross(wobbA.axes[i], wobbB.axes[j]).normalized();
            if(!testOverlap(newAxis)) return;
        }
    }

    collisionInfo.isColliding = true;
    collisionInfo.normal = minAxis;
    collisionInfo.penetrationDepth = minOverlap;


    //calculer point de contact

    //ContactPointDetection::contact_obb_obb(entityA, wobbA, transformA, entityB, wobbB, transformB, collisionInfo);

    
}

void CollisionDetection::collision_sphere_sphere(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    collisionInfo.isColliding = false;

    //std::cout << entityA << " and " << entityB << std::endl;

    const SphereCollider& sphereA = (const SphereCollider&) colliderA;
    const SphereCollider& sphereB = (const SphereCollider&) colliderB;

    Magnum::Vector3 centerA = (transformA.globalModel * Magnum::Vector4{sphereA.localCentroid, 1.0f}).xyz();
    Magnum::Vector3 centerB = (transformB.globalModel * Magnum::Vector4{sphereB.localCentroid, 1.0f}).xyz();

    float radiusA = sphereA.radius * Magnum::Math::max(transformA.scale.x(), Magnum::Math::max(transformA.scale.y(), transformA.scale.z()));
    float radiusB = sphereB.radius * Magnum::Math::max(transformB.scale.x(), Magnum::Math::max(transformB.scale.y(), transformB.scale.z()));

    Magnum::Vector3 diff = centerB - centerA;
    float distSquared = Magnum::Math::dot(diff, diff);
    float radiusSum = radiusA + radiusB;

    if (distSquared <= radiusSum * radiusSum) {
        collisionInfo.isColliding = true;
        float dist = std::sqrt(distSquared);
        collisionInfo.normal = dist > 0.0f ? (-diff / dist) : Magnum::Vector3{1.0f, 0.0f, 0.0f};
        collisionInfo.penetrationDepth = radiusSum - dist;
        collisionInfo.collisionPointA = centerA + diff / dist * radiusA;
        collisionInfo.collisionPointB = centerB - diff / dist * radiusB;
    }
}

void CollisionDetection::collision_sphere_cylinder(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    const SphereCollider& sphere = (const SphereCollider&)colliderA;
    const CylinderCollider& cylinder = (const CylinderCollider&)colliderB;

    Magnum::Vector3 sphereCenter = (transformA.globalModel * Magnum::Vector4{sphere.localCentroid, 1.0f}).xyz();
    float sphereRadius = sphere.radius * Magnum::Math::max(transformA.scale.x(), Magnum::Math::max(transformA.scale.y(), transformA.scale.z()));

    Magnum::Vector3 cylinderPos = (transformB.globalModel * Magnum::Vector4{cylinder.localCentroid, 1.0f}).xyz();
    Magnum::Vector3 cylinderAxis = (transformB.globalModel * Magnum::Vector4{cylinder.axis, 0.0f}).xyz().normalized();
    float cylinderRadius = cylinder.radius * transformB.scale.x();
    float cylinderHeight = cylinder.halfSize * Magnum::Math::min(transformB.scale.x(), Magnum::Math::min(transformB.scale.y(), transformB.scale.z()));

    Magnum::Vector3 axis = sphereCenter - cylinderPos;
    float t = Magnum::Math::dot(axis, cylinderAxis);

    Magnum::Vector3 radial = axis - t * cylinderAxis;
    float radialDist = radial.length();

    float axialOff = std::abs(t) - cylinderHeight;
    float radialOff = radialDist - cylinderRadius;

    float dist;
    Magnum::Vector3 normal;

    //cote du cylindre
    if (axialOff <= 0.0f && radialOff <= 0.0f) {
        dist = std::abs(radialOff);
        normal = (radialDist > 1e-6f ? radial / radialDist : Magnum::Vector3{1.0f, 0.0f, 0.0f});
    } else if (axialOff > 0.0f && radialOff <= 0.0f) {
        dist = axialOff;
        normal = (t > 0.0f ? cylinderAxis : -cylinderAxis);
    } else {
        dist = std::sqrt(axialOff*axialOff + radialOff*radialOff);
        Magnum::Vector3 dirRadial = (radialDist > 1e-6f ? radial / radialDist : Magnum::Vector3{1.0f, 0.0f, 0.0f});
        Magnum::Vector3 rimDir = (dirRadial * cylinderRadius + (t > 0.0f ? cylinderAxis : -cylinderAxis) * cylinderHeight).normalized();
        normal = rimDir;
    }


    float penetration = sphereRadius - dist;
    if (penetration > 0.0f) {
        collisionInfo.isColliding = true;
        collisionInfo.normal = normal;
        collisionInfo.penetrationDepth = penetration;
        collisionInfo.collisionPointA = sphereCenter - normal * sphereRadius;

        if (axialOff <= 0.0f) {
            Magnum::Vector3 sidePoint = cylinderPos + t * cylinderAxis + normal * cylinderRadius;
            collisionInfo.collisionPointB = sidePoint;
        } else {
            Magnum::Vector3 capCenter = cylinderPos + (t > 0.0f ? cylinderAxis : -cylinderAxis) * cylinderHeight;
            collisionInfo.collisionPointB = capCenter + normal * cylinderRadius;
        }
    }

    
}

void CollisionDetection::collision_sphere_aabb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    // 
}

void CollisionDetection::collision_sphere_obb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    collisionInfo.isColliding = false;

    const SphereCollider& sphere = (const SphereCollider&) colliderA;
    const OBBCollider& obb = (const OBBCollider&) colliderB;

    Magnum::Vector3 sphereCenter = (transformA.globalModel * Magnum::Vector4{sphere.localCentroid, 1.0f}).xyz();
    float sphereRadius = sphere.radius * Magnum::Math::max(transformA.scale.x(), Magnum::Math::max(transformA.scale.y(), transformA.scale.z()));

    WorldOBB wobb;
    getWorldOBB(obb, transformB, wobb);

    Magnum::Vector3 dir = sphereCenter - wobb.globalCentroid;

    
    //on cherche le point sur l'obb le plus proche de la sphere
    Magnum::Vector3 closestPoint = wobb.globalCentroid;

    for (int i = 0; i < 3; ++i) {
        float projection = Magnum::Math::dot(dir, wobb.axes[i]);
        float clamped = Magnum::Math::clamp(projection, -wobb.halfSize[i], wobb.halfSize[i]);
        closestPoint += clamped * wobb.axes[i];
    }

    Magnum::Vector3 diff = sphereCenter - closestPoint;
    float distSquared = Magnum::Math::dot(diff, diff);

    if (distSquared <= sphereRadius * sphereRadius) {
        float distance = std::sqrt(distSquared);
        collisionInfo.isColliding = true;
        collisionInfo.normal = distance > 0.0f ? diff / distance : Magnum::Vector3{1.0f, 0.0f, 0.0f};
        collisionInfo.penetrationDepth = sphereRadius - distance;
        collisionInfo.collisionPointA = sphereCenter - collisionInfo.normal * sphereRadius;
        collisionInfo.collisionPointB = closestPoint;
    }
}

void CollisionDetection::collision_sphere_plane(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    collisionInfo.isColliding = false;

    const SphereCollider& sphere = (const SphereCollider&) colliderA;
    const PlaneCollider& plane = (const PlaneCollider&) colliderB;

    Magnum::Vector3 sphereCenter = (transformA.globalModel * Magnum::Vector4{sphere.localCentroid, 1.0f}).xyz();
    float sphereRadius = sphere.radius * transformA.scale.x();

    Magnum::Vector3 planePosition = (transformB.globalModel * Magnum::Vector4{plane.localCentroid, 1.0f}).xyz();
    Magnum::Vector3 planeNormal = (transformB.globalModel * Magnum::Vector4{plane.normal, 0.0f}).xyz().normalized();

    float distance = Magnum::Math::dot(sphereCenter - planePosition, planeNormal);

    if (distance < sphereRadius) {
        collisionInfo.isColliding = true;
        collisionInfo.penetrationDepth = sphereRadius - std::abs(distance);
        collisionInfo.normal = distance > 0.0f ? planeNormal : -planeNormal;
        collisionInfo.collisionPointA = sphereCenter - collisionInfo.normal * sphereRadius;
        collisionInfo.collisionPointB = sphereCenter - collisionInfo.normal * distance;
    }
}

void CollisionDetection::collision_cylinder_cylinder(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    //
}

void CollisionDetection::collision_cylinder_aabb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    //
}

void CollisionDetection::collision_cylinder_obb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    collisionInfo.isColliding = false;

    const CylinderCollider& cylinder = (const CylinderCollider&) colliderA;
    const OBBCollider& obb = (const OBBCollider&) colliderB;

    Magnum::Vector3 cylinderPos = (transformA.globalModel * Magnum::Vector4{cylinder.localCentroid, 1.0f}).xyz();
    Magnum::Vector3 cylinderAxis = (transformA.globalModel * Magnum::Vector4{cylinder.axis, 0.0f}).xyz().normalized();
    float cylinderRadius = cylinder.radius * transformA.scale.x();
    float cylinderHeight = cylinder.halfSize * Magnum::Math::min(transformA.scale.x(), Magnum::Math::min(transformA.scale.y(), transformA.scale.z()));

    WorldOBB wobb;
    getWorldOBB(obb, transformB, wobb);

    Magnum::Vector3 start = cylinderPos + cylinderAxis * cylinderHeight;
    Magnum::Vector3 end = cylinderPos - cylinderAxis * cylinderHeight;

    float minOverlap = std::numeric_limits<float>::max();
    Magnum::Vector3 minAxis;

    auto testOverlap = [&](const Magnum::Vector3& axis){
        float projOBB = std::abs(Magnum::Math::dot(axis, wobb.axes[0])) * wobb.halfSize.x() + std::abs(Magnum::Math::dot(axis, wobb.axes[1])) * wobb.halfSize.y() + std::abs(Magnum::Math::dot(axis, wobb.axes[2])) * wobb.halfSize.z();
        float d = Magnum::Math::dot(axis, cylinderAxis);
        float projHeight = cylinderHeight * std::abs(d);
        float projRadius = cylinderRadius * std::sqrt(std::max(0.0f, 1.0f - std::abs(d)*std::abs(d)));
        float projCylinder = projHeight + projRadius;
        float dist = std::abs(Magnum::Math::dot(wobb.globalCentroid - cylinderPos, axis));
        float overlap = projCylinder + projOBB - dist;
        if (overlap < 0) return false;
        if (overlap < minOverlap) {
            minOverlap = overlap;
            if (Magnum::Math::dot(wobb.globalCentroid - cylinderPos, axis) < 0) {
                minAxis = axis;
            } else {
                minAxis = -axis;
            }
        }
        return true;
    };

        if (!testOverlap(cylinderAxis)) {return;} //axe du cylindre   
        for (int i = 0; i < 3; ++i) {if (!testOverlap(wobb.axes[i])) return;} //les axes de l'obb

        //cross product de tous les axes precedents
        for (int i = 0; i < 3; ++i) {
            Magnum::Vector3 newAxis = Magnum::Math::cross(cylinderAxis, wobb.axes[i]).normalized();
            if (!testOverlap(newAxis)) return;
        }

        collisionInfo.isColliding = true;
        collisionInfo.normal = minAxis;
        collisionInfo.penetrationDepth = minOverlap;


}

void CollisionDetection::collision_cylinder_plane(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    //
}

void CollisionDetection::collision_aabb_aabb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    //
}

void CollisionDetection::collision_aabb_obb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    //
}

void CollisionDetection::collision_aabb_plane(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    //
}

void CollisionDetection::collision_obb_plane(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    collisionInfo.isColliding = false;
    const OBBCollider& obb = (const OBBCollider&) colliderA;
    const PlaneCollider& plane = (const PlaneCollider&) colliderB;

    WorldOBB wobb;
    getWorldOBB(obb, transformA, wobb);

    Magnum::Vector3 planePosition = (transformB.globalModel * Magnum::Vector4{plane.localCentroid, 1.0f}).xyz();
    Magnum::Vector3 planeNormal = (transformB.globalModel * Magnum::Vector4{plane.normal, 0.0f}).xyz().normalized();
    float distance = Magnum::Math::dot(wobb.globalCentroid - planePosition, planeNormal);
    float projection = 0.0f;
    for (int i = 0; i < 3; ++i) {
        projection += std::abs(Magnum::Math::dot(wobb.axes[i], planeNormal)) * wobb.halfSize[i];
    }


    if (distance <= projection) {
        collisionInfo.isColliding = true;
        collisionInfo.penetrationDepth = projection - abs(distance);
        collisionInfo.normal = planeNormal;

        Magnum::Vector3 closestPoint = wobb.globalCentroid - distance * planeNormal;
        collisionInfo.collisionPointA = closestPoint;
        collisionInfo.collisionPointB = closestPoint;
    }

}

void CollisionDetection::collision_plane_plane(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    //
}

void CollisionDetection::collision_ray_obb(const Entity entityA, const Ray& ray ,const TransformComponent& transformA, const Entity entityB, const OBBCollider& obb, const TransformComponent& transformB, CollisionInfo& collisionInfo){

    collisionInfo.isColliding = false;

    WorldOBB wobb;
    getWorldOBB(obb, transformB, wobb);

    Magnum::Matrix4 obbModel = Magnum::Matrix4{Magnum::Math::IdentityInit};
    obbModel[0] = Magnum::Vector4(wobb.axes[0], 0.0f);
    obbModel[1] = Magnum::Vector4(wobb.axes[1], 0.0f);
    obbModel[2] = Magnum::Vector4(wobb.axes[2], 0.0f);
    obbModel[3] = Magnum::Vector4(wobb.globalCentroid, 1.0f);

    Magnum::Matrix4 invObbModel = obbModel.inverted();

    Magnum::Vector3 localOrigin = (invObbModel * Magnum::Vector4{ray.origin, 1.0f}).xyz();
    Magnum::Vector3 localDir = (invObbModel * Magnum::Vector4{ray.direction, 0.0f}).xyz().normalized();

    Magnum::Vector3 minB = -wobb.halfSize;
    Magnum::Vector3 maxB = wobb.halfSize;

    float tmin = std::numeric_limits<float>::lowest();
    float tmax = std::numeric_limits<float>::max();

    for (int i = 0; i < 3; ++i) {
        if (abs(localDir[i]) < 1e-8f) {//ray parallele
            if (localOrigin[i] < minB[i] || localOrigin[i] > maxB[i])
                return;
        } else {
            float ood = 1.0f / localDir[i];
            float t1 = (minB[i] - localOrigin[i]) * ood;
            float t2 = (maxB[i] - localOrigin[i]) * ood;
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax)
                return;
        }
    }

    if (tmax < 0.0f) return;

    float tHit = tmin >= 0.0f ? tmin : tmax;
    Magnum::Vector3 localHit = localOrigin + localDir * tHit;
    Magnum::Vector3 globalHit = (obbModel * Magnum::Vector4{localHit, 1.0f}).xyz();

    if (tHit < 0.0f || tHit > ray.length) return; 

    collisionInfo.isColliding = true;
    collisionInfo.penetrationDepth = tHit;
    collisionInfo.collisionPointA = ray.origin + ray.direction * tHit;
    collisionInfo.collisionPointB = globalHit;

    Magnum::Vector3 normalLocal(0.0f);
    for (int i = 0; i < 3; ++i) {
        if (abs(localHit[i] - minB[i]) < 0.0001f)
            normalLocal[i] = -1.0f;
        else if (abs(localHit[i] - maxB[i]) < 0.0001f)
            normalLocal[i] = 1.0f;
    }

    collisionInfo.normal = (obbModel * Magnum::Vector4{normalLocal, 0.0f}).xyz().normalized();

}
void CollisionDetection::collision_ray_mesh(const Entity entityA, const Ray& ray, const TransformComponent& transformA, const Entity entityB, const MeshCollider& mesh, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    collisionInfo.isColliding = false;
    float closestT = std::numeric_limits<float>::max();
    
    Magnum::Matrix4 modelMatrix = transformB.globalModel;
    
    for(size_t i = 0; i < mesh.indices.size(); i += 3) {
        Magnum::Vector3 v0 = (modelMatrix * Magnum::Vector4{mesh.vertices[mesh.indices[i]], 1.0f}).xyz();
        Magnum::Vector3 v1 = (modelMatrix * Magnum::Vector4{mesh.vertices[mesh.indices[i + 1]], 1.0f}).xyz();
        Magnum::Vector3 v2 = (modelMatrix * Magnum::Vector4{mesh.vertices[mesh.indices[i + 2]], 1.0f}).xyz();
        Magnum::Vector3 e1 = v1 - v0;
        Magnum::Vector3 e2 = v2 - v0;
        Magnum::Vector3 normal = Magnum::Math::cross(e1, e2).normalized();
        Magnum::Vector3 h = Magnum::Math::cross(ray.direction, e2);
        float a = Magnum::Math::dot(e1, h);
        if(std::abs(a) < 1e-8f) continue;
        float f = 1.0f / a;
        Magnum::Vector3 s = ray.origin - v0;
        float u = f * Magnum::Math::dot(s, h);
        if(u < 0.0f || u > 1.0f) continue;
        Magnum::Vector3 q = Magnum::Math::cross(s, e1);
        float v = f * Magnum::Math::dot(ray.direction, q);
        if(v < 0.0f || u + v > 1.0f) continue;
        float t = f * Magnum::Math::dot(e2, q);
        if(t < 0.0f || t > ray.length) continue;
        if(t < closestT) {
            closestT = t;
            collisionInfo.isColliding = true;
            collisionInfo.penetrationDepth = t;
            collisionInfo.collisionPointA = ray.origin + ray.direction * t;
            collisionInfo.collisionPointB = collisionInfo.collisionPointA;
            collisionInfo.normal = normal;
        }
    }
}

   

void CollisionDetection::testCollision(const Entity entityA, const Collider &colliderA, const TransformComponent &transformA, const Entity entityB, const Collider &colliderB, const TransformComponent &transformB, CollisionInfo &collisionInfo){
    int typeA = static_cast<int>(colliderA.type);
    int typeB = static_cast<int>(colliderB.type);

    bool swap = false;
    const Collider* cA = &colliderA;
    const Collider* cB = &colliderB;
    const TransformComponent* tA = &transformA;
    const TransformComponent* tB = &transformB;
    Entity eA = entityA;
    Entity eB = entityB;
    
    //on trie selon le type de collider
    if (typeB < typeA) {
        std::swap(typeA, typeB);
        std::swap(cA, cB);
        std::swap(tA, tB);
        std::swap(eA, eB);
        swap = true;
    }

    CollisionFn fn = CollisionDetection::collisionDispatchTable[typeA][typeB];
    if (fn) {
        fn(eA, *cA, *tA, eB, *cB, *tB, collisionInfo);
        
        //on swap le resultat pour toujours garder la collision de A vers B
        if (swap) {
            std::swap(collisionInfo.collisionPointA, collisionInfo.collisionPointB);
            //std::swap(collisionInfo.localPointA, collisionInfo.localPointB);
            //collisionInfo.normal = - collisionInfo.normal; //deja fait dans les fonctions
            std::swap(collisionInfo.entityA, collisionInfo.entityB);
        }
    } else {
        collisionInfo.isColliding = false;
        //std::cout << "Collision non definie entre " << typeA << " et " << typeB << std::endl;
        //std::cout << "Pas de fonction de collision definit !!!" << std::endl;
    }
}                                        

vector<Magnum::Vector3> clipToPlane(const vector<Magnum::Vector3>& poly, const Magnum::Vector3& planePoint, const Magnum::Vector3& planeNormal) {
    vector<Magnum::Vector3> output;
    int n = poly.size();
    for (int i = 0; i < n; ++i) {
        Magnum::Vector3 A = poly[i];
        Magnum::Vector3 B = poly[(i + 1) % n];
        float dA = Magnum::Math::dot(A - planePoint, planeNormal);
        float dB = Magnum::Math::dot(B - planePoint, planeNormal);
        if (dA >= 0) output.push_back(A);
        if ((dA >= 0) ^ (dB >= 0)) {
            float t = dA / (dA - dB);
            output.push_back(A + t * (B - A));
        }
    }
    return output;
}

vector<Magnum::Vector3> clipToFace(const vector<Magnum::Vector3>& incidentVertices, const WorldOBB& referenceBox, int faceIndex){

    Magnum::Vector3 N = referenceBox.axes[faceIndex];
    Magnum::Vector3 R = referenceBox.axes[(faceIndex + 1) % 3];
    Magnum::Vector3 U = referenceBox.axes[(faceIndex + 2) % 3];
    Magnum::Vector3 C = referenceBox.globalCentroid + N * referenceBox.halfSize[faceIndex];
    float rExtent = referenceBox.halfSize[(faceIndex + 1) % 3];
    float uExtent = referenceBox.halfSize[(faceIndex + 2) % 3];

    vector<Magnum::Vector3> poly = clipToPlane(incidentVertices, C +  R * rExtent, -R);
    poly = clipToPlane(poly, C -  R * rExtent,  R);
    poly = clipToPlane(poly, C +  U * uExtent, -U);
    poly = clipToPlane(poly, C -  U * uExtent,  U);
    return poly;

}



//https://github.com/DallinClark/3d-physics-engine/blob/main/src/physics/collisions/collisions.cpp
//renvoie l'indice du point le plus eloigne dans la direction donne
int supportOBB(Magnum::Vector3 vertices[8], const Magnum::Vector3& direction) {
    float maxDot = std::numeric_limits<float>::lowest();
    int maxIndex = -1;
    for (int i = 0; i < 8; ++i) {
        float d = Magnum::Math::dot(vertices[i], direction);
        if (d > maxDot) {
            maxDot = d;
            maxIndex = i;
        }
    }
    return maxIndex;
}

//on cherche la face qui contient le vertex minIndex et qui a la normal la plus aligne avec axis
Face bestFace(Magnum::Vector3 vertices[8], vector<Face>& faces, int minIndex, Magnum::Vector3 axis){

    Face best;
    float bestAlignement = numeric_limits<float>::lowest();

    for(Face &f : faces) {

        bool containsTarget = false;
        vector<int> faceIndices;

        for (auto faceIndice : f.indices) {
            if (faceIndice == minIndex){ containsTarget = true;}
        }
        if(!containsTarget){continue;}

        float alignement = Magnum::Math::dot(f.normal, axis);
        if(alignement > bestAlignement){
            bestAlignement = alignement;
            best = f;
        }

    }
    return best;

}


//algo de Sutherland Hodgman pour le polygon clipping
void ContactPointDetection::contact_obb_obb(const Entity entityA, const WorldOBB& wobbA, const TransformComponent& transformA, const Entity entityB, const WorldOBB& wobbB, const TransformComponent& transformB, CollisionInfo& collisionInfo){

    //on determine la face de reference et la face incidente

    //collisionInfo.normal = normalize(-collisionInfo.normal);

    Magnum::Vector3 verticesA[8];
    Magnum::Vector3 verticesB[8];

    wobbA.getVertices(verticesA);
    wobbB.getVertices(verticesB);

    vector<Face> facesA;
    vector<Face> facesB;

    wobbA.getFaces(verticesA, facesA);
    wobbB.getFaces(verticesB, facesB);

    /* for (size_t i = 0; i < facesA.size(); ++i) {
        const Face& f = facesA[i];
        Console::getInstance().addLog("facesA[" + std::to_string(i) + "] normal: (" +
            std::to_string(f.normal.x) + ", " +
            std::to_string(f.normal.y) + ", " +
            std::to_string(f.normal.z) + ")");
    }
    for (size_t i = 0; i < facesB.size(); ++i) {
        const Face& f = facesB[i];
        Console::getInstance().addLog("facesB[" + std::to_string(i) + "] normal: (" +
            std::to_string(f.normal.x) + ", " +
            std::to_string(f.normal.y) + ", " +
            std::to_string(f.normal.z) + ")");
    } */


    int minIndexA = supportOBB(verticesA, -collisionInfo.normal);
    int minIndexB = supportOBB(verticesB, collisionInfo.normal);

    //Console::getInstance().addLog("minIndexA: " + std::to_string(minIndexA));
    //Console::getInstance().addLog("minIndexB: " + std::to_string(minIndexB));

    Face faceA = bestFace(verticesA, facesA, minIndexA, -collisionInfo.normal);
    Face faceB = bestFace(verticesB, facesB, minIndexB, collisionInfo.normal);

    //Console::getInstance().addLog("FaceA normal: (" + std::to_string(faceA.normal.x) + ", " + std::to_string(faceA.normal.y) + ", " + std::to_string(faceA.normal.z) + ")");
    //Console::getInstance().addLog("FaceB normal: (" + std::to_string(faceB.normal.x) + ", " + std::to_string(faceB.normal.y) + ", " + std::to_string(faceB.normal.z) + ")");

    float alignementA = dot(faceA.normal, -collisionInfo.normal);
    float alignementB = dot(faceB.normal, collisionInfo.normal);

    //Console::getInstance().addLog(("entityA: " + std::to_string(entityA) + ", entityB: " + std::to_string(entityB)).c_str());
    //Console::getInstance().addLog(("alignementA: " + std::to_string(alignementA) + ", alignementB: " + std::to_string(alignementB)).c_str());
    //Console::getInstance().addLog(("Collision normal: " + std::to_string(collisionInfo.normal.x) + ", " + std::to_string(collisionInfo.normal.y) + ", " + std::to_string(collisionInfo.normal.z)).c_str());

    Face referenceFace;
    Face incidentFace;

    vector<Face> referenceOBBFaces;
    vector<Magnum::Vector3> referenceOBBVertices;
    Magnum::Vector3 newClippingNormal;

 
    float volumeA = 8.0f * wobbA.halfSize.x() * wobbA.halfSize.y() * wobbA.halfSize.z();
    float volumeB = 8.0f * wobbB.halfSize.x() * wobbB.halfSize.y() * wobbB.halfSize.z();

    if (volumeA < volumeB) {
        referenceFace = faceA;
        incidentFace = faceB;
        referenceOBBVertices = vector<Magnum::Vector3>(verticesA, verticesA + 8);
        referenceOBBFaces = facesA;
        newClippingNormal = -collisionInfo.normal;
        //Console::getInstance().addLog("Reference: A (smaller), Incident: B");
    } else {
        referenceFace = faceB;
        incidentFace = faceA;
        referenceOBBVertices = vector<Magnum::Vector3>(verticesB, verticesB + 8);
        referenceOBBFaces = facesB;
        newClippingNormal = collisionInfo.normal;
        //Console::getInstance().addLog("Reference: B (smaller), Incident: A");
    }


    Magnum::Vector3 u = (referenceFace.vertices[1] - referenceFace.vertices[0]).normalized();
    Magnum::Vector3 v = (referenceFace.vertices[3] - referenceFace.vertices[0]).normalized();

    Magnum::Vector3 faceCenter = (referenceFace.vertices[0] + referenceFace.vertices[1] + referenceFace.vertices[2] + referenceFace.vertices[3]) * 0.25f;

    vector<pair<Magnum::Vector3, Magnum::Vector3>> clipPlanes = {
        {v, faceCenter},
        {-v, faceCenter},
        {u, faceCenter},
        {-u, faceCenter}
    };

    vector<Magnum::Vector3> clipped = incidentFace.vertices;
    //Console::getInstance().addLog(("start clipped " + std::to_string(clipped.size())).c_str());
 

    for(auto& [n,p] : clipPlanes){

        Magnum::Vector3 planeNormal = n;
        Magnum::Vector3 planePoint = p;

        if(Magnum::Math::dot(planeNormal, referenceFace.normal) < 0.0f){
            planeNormal = -planeNormal;
        }

        clipped = clipToPlane(clipped, planePoint, planeNormal);

        /* Console::getInstance().addLog(("clipped " + std::to_string(clipped.size())).c_str()); */
 

        if(clipped.empty()) break;
    }

    //clipping


    vector<Magnum::Vector3> manifold;
    float D = Magnum::Math::dot(referenceFace.normal, referenceFace.vertices[0]);
    for (auto& q : clipped) {
        float d = Magnum::Math::dot(referenceFace.normal, q) - D;
        Magnum::Vector3 pa = q - referenceFace.normal * d;
        manifold.push_back(pa);
    }

    collisionInfo.collisionPoints = manifold;

    //Console::getInstance().addLog(("final Contact points: " + std::to_string(collisionInfo.collisionPoints.size())).c_str());
 
}


void ContactPointDetection::contact_sphere_sphere(const Entity entityA, const SphereCollider& colliderA, const TransformComponent& transformA, const Entity entityB, const SphereCollider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    //pas besoin on le fait deja dans test sphere vs sphere
}

void ContactPointDetection::contact_sphere_obb(const Entity entityA, const SphereCollider& colliderA, const TransformComponent& transformA, const Entity entityB, const OBBCollider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    // Empty implementation
}

void ContactPointDetection::contact_obb_plane(const Entity entityA, const OBBCollider& colliderA, const TransformComponent& transformA, const Entity entityB, const PlaneCollider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    // Empty implementation
}

void ContactPointDetection::contact_sphere_plane(const Entity entityA, const SphereCollider& colliderA, const TransformComponent& transformA, const Entity entityB, const PlaneCollider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo) {
    // Empty implementation
}

} // namespace WaterSimulation