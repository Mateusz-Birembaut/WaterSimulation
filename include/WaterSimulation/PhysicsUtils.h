#pragma once

#include <WaterSimulation/ECS.h>

#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Matrix3.h>

#include <iostream>
#include <vector>


using Magnum::Vector3;

namespace WaterSimulation
{
	


struct CollisionInfo{
    Entity entityA;
    Entity entityB;	

    bool isColliding = false;

    std::vector<Vector3> collisionPoints;
    std::vector<Vector3> collisionPointsA;
    std::vector<Vector3> collisionPointsB;
    Vector3 collisionPointA;
    Vector3 collisionPointB;
    Vector3 normal;
    float penetrationDepth;


    float restitution;

};

enum class ColliderType
{
    SPHERE,
    CYLINDER,
    AABB,
    OBB,
    PLANE,
    CONVEX,
    MESH,
};


struct Collider {
    ColliderType type;
    float mass = 100.0f;
    Magnum::Matrix3 localInertiaTensor;
    Vector3 localCentroid = Vector3{0.0f};

    Vector3 support(const Vector3 & direction) const; //donne un point de support pour l'algo GJK implementation plus tard

    virtual ~Collider() = default;
};

struct SphereCollider : public Collider {
    float radius;

    SphereCollider(float r) : radius(r) {
        type = ColliderType::SPHERE;
    }

    void computeInertiaTensor() {
        float I = (2.0f / 5.0f) * mass * radius * radius;
        localInertiaTensor = Magnum::Matrix3::fromDiagonal(Vector3{I, I, I});
    }
};

struct CylinderCollider : public Collider {
    float radius;
    float halfSize;
    Vector3 axis;

    CylinderCollider(float radius, float halfSize, Vector3 direction = Vector3{0.0f,1.0f,0.0f}) : radius(radius), halfSize(halfSize), axis(direction) {
        type = ColliderType::CYLINDER;
        computeInertiaTensor();
    }

    void computeInertiaTensor() {
        float Ix = (1.0f / 12.0f) * mass * (3 *radius*radius + 4 *halfSize * halfSize);
        float Iy = 0.5f * mass * radius * radius;
        float Iz = Ix;
        localInertiaTensor = Magnum::Matrix3{{
            {Ix, 0.0f, 0.0f},
            {0.0f, Iy, 0.0f},
            {0.0f, 0.0f, Iz}
        }};
    }
};

struct AABBCollider : public Collider {
    Vector3 min;
    Vector3 max;

    AABBCollider() = default;

    AABBCollider(Vector3 min, Vector3 max) : min(min), max(max) {
        type = ColliderType::AABB;
    }
};

struct OBBCollider : public Collider {
    Vector3 halfSize; 
    Vector3 rotation;
    
    OBBCollider(const Vector3& halfSize = Vector3{1.0f}) : halfSize(halfSize){
        type = ColliderType::OBB;
        rotation = Vector3{0.0f};
        computeInertiaTensor();
    }

    //un peu moche mais c'est le mieux
    void getVertices(const Vector3& center, const Vector3& right, const Vector3& up, const Vector3& front, Vector3 vertices[8]) const {
        vertices[0] = center + right * halfSize.x() + up * halfSize.y() + front * halfSize.z();
        vertices[1] = center + right * halfSize.x() + up * halfSize.y() - front * halfSize.z();
        vertices[2] = center + right * halfSize.x() - up * halfSize.y() + front * halfSize.z();
        vertices[3] = center + right * halfSize.x() - up * halfSize.y() - front * halfSize.z();
        vertices[4] = center - right * halfSize.x() + up * halfSize.y() + front * halfSize.z();
        vertices[5] = center - right * halfSize.x() + up * halfSize.y() - front * halfSize.z();
        vertices[6] = center - right * halfSize.x() - up * halfSize.y() + front * halfSize.z();
        vertices[7] = center - right * halfSize.x() - up * halfSize.y() - front * halfSize.z();
    }

    void computeInertiaTensor() {
        float w = 2.0f * halfSize.x();
        float h = 2.0f * halfSize.y();
        float d = 2.0f * halfSize.z();

        float Ix = (1.0f/12.0f) * mass * (h * h + d * d);
        float Iy = (1.0f/12.0f)* mass * (w * w + d * d);
        float Iz = (1.0f/12.0f) * mass * (w * w +h * h);

        localInertiaTensor = Magnum::Matrix3{{
            {Ix, 0.0f, 0.0f},
            {0.0f, Iy, 0.0f},
            {0.0f, 0.0f, Iz}
        }};
    }

};

struct MeshCollider : public Collider {
    std::vector<Vector3> vertices;
    std::vector<unsigned int> indices;
    
    MeshCollider() {
        type = ColliderType::MESH;
    }
};


struct Ray{
    Vector3 origin;
    Vector3 direction;
    float length;

    Ray(const Vector3& origin, const Vector3& direction, float length) : origin(origin), direction(direction), length(length) {}
};

struct Face {
    std::vector<Vector3> vertices;
    std::vector<int> indices;
    Vector3 normal;
};

//utilisé pour le test de collision uniquement
//nos obb sont definit en local, on veut en global
struct WorldOBB {
    Vector3 globalCentroid;
    Vector3 halfSize;
    Vector3 axes[3]; // la nouvelle base

    void getVertices(Vector3 vertices[8]) const {
        vertices[0] = globalCentroid + axes[0] * halfSize.x() + axes[1] * halfSize.y() + axes[2] * halfSize.z();
        vertices[1] = globalCentroid + axes[0] * halfSize.x() + axes[1] * halfSize.y() - axes[2] * halfSize.z();
        vertices[2] = globalCentroid + axes[0] * halfSize.x() - axes[1] * halfSize.y() + axes[2] * halfSize.z();
        vertices[3] = globalCentroid + axes[0] * halfSize.x() - axes[1] * halfSize.y() - axes[2] * halfSize.z();
        vertices[4] = globalCentroid - axes[0] * halfSize.x() + axes[1] * halfSize.y() + axes[2] * halfSize.z();
        vertices[5] = globalCentroid - axes[0] * halfSize.x() + axes[1] * halfSize.y() - axes[2] * halfSize.z();
        vertices[6] = globalCentroid - axes[0] * halfSize.x() - axes[1] * halfSize.y() + axes[2] * halfSize.z();
        vertices[7] = globalCentroid - axes[0] * halfSize.x() - axes[1] * halfSize.y() - axes[2] * halfSize.z();
    }

    void getFaces(Vector3 vertices[8],std::vector<Face>& facesOut) const {
        static const int faceDef[6][4] = {
            { 0, 1, 5, 4 },   // +Y
            { 2, 3, 7, 6 },   // -Y
            { 0, 2, 6, 4 },   // +X
            { 1, 3, 7, 5 },   // -X
            { 0, 1, 3, 2 },   // +Z
            { 4, 5, 7, 6 }    // -Z
        };

        static const Vector3 localNormals[6] = {
            Vector3{ 0,  1,  0},
            Vector3{ 0, -1,  0},
            Vector3{ 1,  0,  0},
            Vector3{-1,  0,  0},
            Vector3{ 0,  0,  1},
            Vector3{ 0,  0, -1}
        };


        facesOut.resize(6);
        for (int i = 0; i < 6; ++i) {
            Face& face = facesOut[i];
            face.indices = {faceDef[i][0], faceDef[i][1], faceDef[i][2], faceDef[i][3]};
            face.vertices.clear();
            for (int j = 0; j < 4; ++j) {
                face.vertices.push_back(vertices[faceDef[i][j]]);
            }

            const Vector3& local = localNormals[i];
            face.normal = (local.x() * axes[0] +
                           local.y() * axes[1] +
                           local.z() * axes[2]).normalized();
        }
    }
};


struct PlaneCollider : public Collider {
    Vector3 normal;

    PlaneCollider(const Vector3& normal) : normal(normal){
        type = ColliderType::PLANE;
    }
};


struct ConvexCollider : public Collider {
    std::vector<Vector3> points;
    std::vector<Magnum::Vector2i> edges; //aretes par indices
    std::vector<Magnum::Vector3i> faces; //faces par indices

    ConvexCollider(const std::vector<Vector3>& points, const std::vector<Magnum::Vector2i>& edges, const std::vector<Magnum::Vector3i>& faces): points(points), edges(edges), faces(faces) {
        type = ColliderType::CONVEX;
    }
};

//Pour le double dispatch
struct Collider;
struct CollisionInfo;
struct TransformComponent;
using CollisionFn = void(*)(const Entity, const Collider&, const TransformComponent&, const Entity,const Collider&, const TransformComponent&, CollisionInfo&);

class CollisionDetection {

    //attribue automatiquement la bonne fonction selon les types de colliders
    static constexpr int NUM_COLLIDER_TYPES = 7;
    static CollisionFn collisionDispatchTable[NUM_COLLIDER_TYPES][NUM_COLLIDER_TYPES];

public:

    static void getWorldOBB(const OBBCollider& collider, const TransformComponent& transform, WorldOBB& worldOBB);

    static void collision_sphere_sphere(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB ,const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void collision_sphere_cylinder(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void collision_sphere_aabb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void collision_sphere_obb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void collision_sphere_plane(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);

    static void collision_cylinder_cylinder(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void collision_cylinder_aabb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void collision_cylinder_obb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void collision_cylinder_plane(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);

    static void collision_aabb_aabb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void collision_aabb_obb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void collision_aabb_plane(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);

    static void collision_obb_obb(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void collision_obb_plane(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);

    static void collision_plane_plane(const Entity entityA, const Collider& colliderA, const TransformComponent& transformA, const Entity entityB, const Collider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void collision_ray_obb(const Entity entityA, const Ray& ray ,const TransformComponent& transformA, const Entity entityB, const OBBCollider& obb, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    
    static void collision_ray_mesh(const Entity entityA, const Ray& ray, const TransformComponent& transformA, const Entity entityB, const MeshCollider& mesh, const TransformComponent& transformB, CollisionInfo& collisionInfo);

    //dispatch automatiquement vers la bonne fonction
    static void testCollision(
        const Entity entityA, const Collider& colliderA, const TransformComponent& transformA,
        const Entity entityB, const Collider& colliderB, const TransformComponent& transformB,
        CollisionInfo& collisionInfo);
    
};

class ContactPointDetection{

public:

    static void contact_obb_obb(const Entity entityA, const WorldOBB& wobbA, const TransformComponent& transformA, const Entity entityB, const WorldOBB& wobbB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void contact_sphere_sphere(const Entity entityA, const SphereCollider& colliderA, const TransformComponent& transformA, const Entity entityB, const SphereCollider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void contact_sphere_obb(const Entity entityA, const SphereCollider& colliderA, const TransformComponent& transformA, const Entity entityB, const OBBCollider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void contact_obb_plane(const Entity entityA, const OBBCollider& colliderA, const TransformComponent& transformA, const Entity entityB, const PlaneCollider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
    static void contact_sphere_plane(const Entity entityA, const SphereCollider& colliderA, const TransformComponent& transformA, const Entity entityB, const PlaneCollider& colliderB, const TransformComponent& transformB, CollisionInfo& collisionInfo);
};

} // namespace WaterSimulation