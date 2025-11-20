#pragma once


#include <WaterSimulation/ECS.h>

#include <Magnum/Math/Matrix4.h>

namespace WaterSimulation {

class TransformSystem{
public:
    void update(WaterSimulation::Registry & registry);

private:
    void computeGlobalTransform(Entity entity, Registry & registry, const Magnum::Matrix4 & parentModel);
};

}