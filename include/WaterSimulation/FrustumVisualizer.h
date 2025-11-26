#pragma once
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Color.h>

class FrustumVisualizer {
public:
    static void draw(const Magnum::Matrix4& view,
                     const Magnum::Matrix4& proj,
                     const Magnum::Color3& color);
};
