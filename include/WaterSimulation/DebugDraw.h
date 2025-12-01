#pragma once
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Vector3.h>
#include <vector>

class DebugDraw {
public:
    struct Line {
        Magnum::Vector3 a, b;
        Magnum::Color3 color;
    };

    static DebugDraw& instance() {
        static DebugDraw inst;
        return inst;
    }

    void addLine(const Magnum::Vector3& a,
                 const Magnum::Vector3& b,
                 const Magnum::Color3& color = Magnum::Color3{1.0f});
    
    void clear();
    void draw(const Magnum::Matrix4& view,
              const Magnum::Matrix4& proj);

	void addGizmo(const Magnum::Vector3& pos, float size = 1.0f);
    void addDirectionGizmo(const Magnum::Vector3& origin,
                        const Magnum::Vector3& direction,
                        float size = 1.0f);

    // Dessine une petite croix 3D pour marquer un point
    void addPoint(const Magnum::Vector3& pos, const Magnum::Color3& color, float size = 0.1f);

    // Dessine une boîte (cube) en fil de fer centrée sur 'center'
    void addCube(const Magnum::Vector3& center, float size, const Magnum::Color3& color);

    // Optionnel : Une boîte non cubique (AABB) définie par sa taille en X, Y, Z
    void addBox(const Magnum::Vector3& center, const Magnum::Vector3& size, const Magnum::Color3& color);



    // NOUVEAU : Dessine un rayon partant d'un point, vers une direction, sur une certaine longueur
    void addRay(const Magnum::Vector3& origin, const Magnum::Vector3& direction, float length, const Magnum::Color3& color);

private:
    DebugDraw();
    std::vector<Line> m_lines;
    Magnum::GL::Mesh m_mesh;
    Magnum::GL::Buffer m_vertexBuffer;
};
