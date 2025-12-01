#include <WaterSimulation/FrustumVisualizer.h>
#include <WaterSimulation/DebugDraw.h>

using namespace Magnum;

static const Vector3 ndcCorners[8] = {
    {-1,-1,-1}, {1,-1,-1}, {-1,1,-1}, {1,1,-1},     // near
    {-1,-1, 1}, {1,-1, 1}, {-1,1, 1}, {1,1, 1}      // far
};

void FrustumVisualizer::draw(const Matrix4& view,
                             const Matrix4& proj,
                             const Color3& color)
{
    Matrix4 inv = (proj * view).inverted();

    Vector3 wc[8];
    for(int i = 0; i < 8; i++) {
        Vector4 p = inv * Vector4{ndcCorners[i], 1.0f};
        wc[i] = p.xyz() / p.w();
    }

    auto& dbg = DebugDraw::instance();

    // near rectangle
    dbg.addLine(wc[0], wc[1], color);
    dbg.addLine(wc[1], wc[3], color);
    dbg.addLine(wc[3], wc[2], color);
    dbg.addLine(wc[2], wc[0], color);

    // far rectangle
    dbg.addLine(wc[4], wc[5], color);
    dbg.addLine(wc[5], wc[7], color);
    dbg.addLine(wc[7], wc[6], color);
    dbg.addLine(wc[6], wc[4], color);

    // links
    dbg.addLine(wc[0], wc[4], color);
    dbg.addLine(wc[1], wc[5], color);
    dbg.addLine(wc[2], wc[6], color);
    dbg.addLine(wc[3], wc[7], color);
}
