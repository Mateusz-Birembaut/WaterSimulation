#pragma once

namespace WaterSimulation
{
	struct Collider {
    ColliderType type;
    float mass = 100.0f;
    Magnum::Matrix3 localInertiaTensor;
    Vector3 localCentroid = Vector3{0.0f};

    Vector3 support(const Vector3 & direction) const; //donne un point de support pour l'algo GJK implementation plus tard

    virtual ~Collider() = default;
};
} // namespace WaterSimulation

