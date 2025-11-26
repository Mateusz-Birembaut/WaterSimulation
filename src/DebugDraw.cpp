#include <WaterSimulation/DebugDraw.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Shaders/VertexColor.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Shaders/VertexColorGL.h>
#include <Magnum/Math/Matrix4.h>
#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/ArrayViewStl.h>

#include <vector>

using namespace Magnum;

DebugDraw::DebugDraw() {
    m_mesh = GL::Mesh{GL::MeshPrimitive::Lines};
}

void DebugDraw::addLine(const Vector3& a, const Vector3& b, const Color3& color) {
    m_lines.push_back({a, b, color});
}

void DebugDraw::clear() {
    m_lines.clear();
}

void DebugDraw::draw(const Matrix4& view, const Matrix4& proj) {
    if(m_lines.empty()) return;

    struct V { Vector3 pos; Color3 col; };
    std::vector<V> vertices;
    vertices.reserve(m_lines.size() * 2);

    for(const auto& l : m_lines) {
        vertices.push_back({l.a, l.color});
        vertices.push_back({l.b, l.color});
    }


	m_vertexBuffer.setData(Containers::arrayView(vertices));

    m_mesh.setCount(vertices.size());
    m_mesh.addVertexBuffer(m_vertexBuffer, 0,
        Shaders::VertexColor3D::Position{},
        Shaders::VertexColor3D::Color3{}
    );

    static Shaders::VertexColor3D shader{};
	shader.setTransformationProjectionMatrix(
		proj * view
	);

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);

    shader.draw(m_mesh);

    clear();
}

void DebugDraw::addGizmo(const Vector3& pos, float size) {
    // X axis (red)
    addLine(pos, pos + Vector3{size, 0.0f, 0.0f}, Color3{1.0f, 0.0f, 0.0f});

    // Y axis (green)
    addLine(pos, pos + Vector3{0.0f, size, 0.0f}, Color3{0.0f, 1.0f, 0.0f});

    // Z axis (blue)
    addLine(pos, pos + Vector3{0.0f, 0.0f, size}, Color3{0.0f, 0.0f, 1.0f});
}

void DebugDraw::addPoint(const Vector3& pos, const Color3& color, float size) {
    const float s = size; // Demi-taille visuelle
    
    // Une croix sur les 3 axes
    addLine(pos - Vector3{s, 0.0f, 0.0f}, pos + Vector3{s, 0.0f, 0.0f}, color);
    addLine(pos - Vector3{0.0f, s, 0.0f}, pos + Vector3{0.0f, s, 0.0f}, color);
    addLine(pos - Vector3{0.0f, 0.0f, s}, pos + Vector3{0.0f, 0.0f, s}, color);
}

void DebugDraw::addCube(const Vector3& center, float size, const Color3& color) {
    addBox(center, Vector3{size}, color);
}

void DebugDraw::addBox(const Vector3& center, const Vector3& size, const Color3& color) {
    // 'size' est la taille totale, donc on prend la moitié pour aller du centre aux bords
    const Vector3 half = size * 0.5f;

    // Les 8 coins du cube
    // Bas (Bottom) - Y négatif
    Vector3 p1 = center + Vector3{-half.x(), -half.y(), -half.z()};
    Vector3 p2 = center + Vector3{ half.x(), -half.y(), -half.z()};
    Vector3 p3 = center + Vector3{ half.x(), -half.y(),  half.z()};
    Vector3 p4 = center + Vector3{-half.x(), -half.y(),  half.z()};

    // Haut (Top) - Y positif
    Vector3 p5 = center + Vector3{-half.x(),  half.y(), -half.z()};
    Vector3 p6 = center + Vector3{ half.x(),  half.y(), -half.z()};
    Vector3 p7 = center + Vector3{ half.x(),  half.y(),  half.z()};
    Vector3 p8 = center + Vector3{-half.x(),  half.y(),  half.z()};

    // --- Connexions ---
    
    // Carré du bas
    addLine(p1, p2, color);
    addLine(p2, p3, color);
    addLine(p3, p4, color);
    addLine(p4, p1, color);

    // Carré du haut
    addLine(p5, p6, color);
    addLine(p6, p7, color);
    addLine(p7, p8, color);
    addLine(p8, p5, color);

    // Piliers verticaux (connexion bas -> haut)
    addLine(p1, p5, color);
    addLine(p2, p6, color);
    addLine(p3, p7, color);
    addLine(p4, p8, color);
}


// Implémentation de addRay
void DebugDraw::addRay(const Vector3& origin, const Vector3& direction, float length, const Color3& color) {
    // On calcule le point d'arrivée
    Vector3 end = origin + (direction.normalized() * length);
    
    // On réutilise ta fonction de base
    addLine(origin, end, color);
}