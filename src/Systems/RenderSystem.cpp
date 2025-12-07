#include "Corrade/Utility/Debug.h"
#include <WaterSimulation/Systems/RenderSystem.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/DirectionalLightComponent.h>
#include <WaterSimulation/Components/ShadowCasterComponent.h>
#include <WaterSimulation/Components/TerrainComponent.h>
#include <WaterSimulation/Components/MaterialComponent.h>
#include <WaterSimulation/Components/WaterComponent.h>

#include <WaterSimulation/Camera.h>
#include <WaterSimulation/Mesh.h>

#include <WaterSimulation/FrustumVisualizer.h>
#include <WaterSimulation/DebugDraw.h>

#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/GL/Texture.h>

#include <utility>


using namespace Magnum;
using namespace Math::Literals;

namespace WaterSimulation {

void RenderSystem::render(Registry& registry,
                          Camera& cam) 
{
    auto viewMatrix = cam.viewMatrix();
    auto projectionMatrix = cam.projectionMatrix();

    auto lightViewProjPair = computeLightViewProj(registry, cam);
    Magnum::Vector3 lightPosition = lightViewProjPair.first;
    m_lightViewProjMatrix = lightViewProjPair.second;

    auto sunView = registry.view<DirectionalLightComponent, ShadowCasterComponent>();

    ShadowCasterComponent* shadowCastData = nullptr;

    if (sunView.begin() != sunView.end()) {

        Entity sunEntity = *sunView.begin();
        shadowCastData = &sunView.get<ShadowCasterComponent>(sunEntity);

    } else {
        Debug{} << "No sun entity found with DirectionalLightComponent and ShadowCasterComponent";
    }

    m_shadowMapPass.render(registry, cam, m_lightViewProjMatrix);
    
    if(m_renderShadowMapOnly){
        if (shadowCastData) {
            drawFullscreenTextureDebugDepth(m_shadowMapPass.getDepthTexture(), shadowCastData->near, shadowCastData->far, true);
            return;
        }
    }

    if(m_renderWaterMaskOnly){
        if (shadowCastData) {
            drawFullscreenTexture(m_shadowMapPass.getColorTexture(), shadowCastData->near, shadowCastData->far);
            return;
        }
    }

    m_opaquePass.render(registry, cam, m_lightViewProjMatrix, m_shadowMapPass.getDepthTexture());

    if(m_renderDepthOnly){
        drawFullscreenTextureDebugDepth(m_opaquePass.getDepthTexture(), cam.near(), cam.far(), false);
        return;
    }

    m_causticPass.render(registry, 
        cam, 
        m_shadowMapPass.getDepthTexture(), 
        m_shadowMapPass.getColorTexture(),
        m_opaquePass.getDepthTexture(),
        lightPosition,
        m_lightViewProjMatrix,
        cam.near(),
        cam.far(),
        shadowCastData->far
    );

    if(m_renderCausticMapOnly){
        drawFullscreenTexture(m_causticPass.getCausticTexture(), cam.near(), cam.far());
        return;
    }

    m_godrayPass.render(registry, 
        cam, 
        m_shadowMapPass.getDepthTexture(), 
        m_shadowMapPass.getColorTexture(),
        m_opaquePass.getDepthTexture(),
        lightPosition,
        m_lightViewProjMatrix,
        cam.near(),
        cam.far(),
        shadowCastData->far
    );

    if(m_renderGodRayMapOnly){
        drawFullscreenTexture(m_godrayPass.getGodRayTexture(), cam.near(), cam.far());
        return;
    }


    m_compositionPass.render(
        m_heightmapReadback,
        cam.position(),
        m_opaquePass.getColorTexture(),
        m_causticPass.getCausticTexture(),
        m_godrayPass.getGodRayTexture(),
        m_opaquePass.getDepthTexture(),
        registry,
        viewMatrix,
        projectionMatrix);

    // debug : afficher les données de la heightmap cpu
    //GL::defaultFramebuffer.bind();
    //visualizeHeightmap(registry, projectionMatrix * viewMatrix);

    //drawFullscreenTexture(m_opaquePass.getColorTexture(), cam.near(), cam.far());
    return;
}



// rend la profondeur avec shader adapté
void RenderSystem::drawFullscreenTextureDebugDepth(Magnum::GL::Texture2D& texture, float near, float far, bool isOrtho) {
    // TODO : si ortho modifier pour avoir un rendu 1:1 (pas obligé)
    GL::defaultFramebuffer.bind();
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    m_depthDebugShader
        .setMVP(Matrix4{})
        .setDepthTexture(texture)
        .setNear(near)
        .setFar(far)
        .setOrthographic(isOrtho)
        .setLinearize(m_linearizeRange)
        .draw(m_fullscreenTriangle);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
}



void RenderSystem::drawFullscreenTexture(Magnum::GL::Texture2D& texture, float near, float far) {
    GL::defaultFramebuffer.bind();
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    m_fullScreenTextureShader
        .bindDepthTexture(texture)
        .setNear(near)
        .setFar(far)
        .draw(m_fullscreenTriangle);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

}

std::pair<Magnum::Vector3, Magnum::Matrix4> RenderSystem::computeLightViewProj(WaterSimulation::Registry& registry, WaterSimulation::Camera& cam){
    auto sunView = registry.view<DirectionalLightComponent, ShadowCasterComponent>();
    if (sunView.begin() != sunView.end()) {
        Entity sunEntity = *sunView.begin();
        auto& sunDirection = sunView.get<DirectionalLightComponent>(sunEntity);
        auto& shadowCastData = sunView.get<ShadowCasterComponent>(sunEntity);

        float maxFarCam = 50.0f;
        float minFarCam = 1.0f;


        Magnum::Vector3 p_mid{cam.position() + (((cam.near() + std::clamp(cam.far(), minFarCam, maxFarCam))/2.0f) * cam.direction())};
        
        Magnum::Vector3 lightPos {p_mid - sunDirection.direction.normalized() * sunDirection.offset};

        Vector3 cameraUp = cam.up();

        Matrix4 lightTransform = Matrix4::lookAt(lightPos, p_mid, cameraUp);
        Matrix4 lightView = lightTransform.invertedRigid();
        
        // -----------------------

        const Magnum::Matrix4 lightProj = Magnum::Matrix4::orthographicProjection(
            shadowCastData.projectionSize, 
            shadowCastData.near,                    
            shadowCastData.far                       
        );

        return std::make_pair(lightPos, lightProj * lightView);
    } else {
        Debug {} << "pas de soleil, calcul shadow map impossible";
        return std::make_pair(Magnum::Vector3{}, Matrix4{Math::IdentityInit});
    }
}

GL::Texture2D& RenderSystem::terrainHeightMap(WaterSimulation::Registry& registry){ // TODO std optional a la place serai mieux
	auto terrainView = registry.view<TerrainComponent, MaterialComponent>();
	if (terrainView.begin() != terrainView.end()) {
        Entity terrainEntity = *terrainView.begin();
		auto& terrainMat = terrainView.get<MaterialComponent>(terrainEntity);

        return *terrainMat.heightmap.get();

    }else {
        Debug {} << "pas de terrain trouvé dans le registre";
        static GL::Texture2D dummyTexture;
        return dummyTexture;
    }
}

DisplayShader::DisplayShader(const char * vertex_shader_file, const char * fragment_shader_file ) {
    Corrade::Utility::Resource rs{"WaterSimulationResources"};

    GL::Shader vert{GL::Version::GL430, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL430, GL::Shader::Type::Fragment};

    vert.addSource(rs.getString(vertex_shader_file));
    frag.addSource(rs.getString(fragment_shader_file));

    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
    CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT_OUTPUT(link());
}

DisplayShader& DisplayShader::bind(Magnum::GL::Texture2D* tex, int location) {
    if (tex) {
        tex->bind(location);
    } else {
        Corrade::Utility::Error{} << "Could not bind texture to shader";
    }

    return *this;
}

DisplayShader& DisplayShader::setTransformationMatrix(const Matrix4& matrix) {
    setUniform(uniformLocation("mvp"), matrix);
    return *this;
}


void RenderSystem::visualizeHeightmap(Registry& registry, const Magnum::Matrix4& viewProj) {
    if (!m_heightmapReadback || !m_heightmapReadback->hasCpuData())
        return;

    auto waterView = registry.view<WaterComponent, TransformComponent>();
    if (waterView.begin() == waterView.end())
        return;

    auto waterEntity = *waterView.begin();
    TransformComponent& transformComp = registry.get<TransformComponent>(waterEntity);
    WaterComponent& wC = registry.get<WaterComponent>(waterEntity);

    const Magnum::Vector2i size = m_heightmapReadback->size();
    const float halfScale = wC.scale * 0.5f;

    // Créer les points à partir des hauteurs
    std::vector<Magnum::Vector3> points;
    const int step = 4; // Espacement entre les points pour ne pas surcharger

    for (int y = 0; y < size.y(); y += step) {
        for (int x = 0; x < size.x(); x += step) {
            const float u = float(x) / float(size.x() - 1);
            const float v = float(y) / float(size.y() - 1);

            const float localX = (u - 0.5f) * wC.scale;
            const float localZ = (v - 0.5f) * wC.scale;
            const float waterHeightLocal = m_heightmapReadback->heightAt(x, y);

            Magnum::Vector3 localPos{localX, waterHeightLocal, localZ};
            Magnum::Vector3 worldPos = transformComp.globalModel.transformPoint(localPos);
            points.push_back(worldPos);
        }
    }

    if (points.empty())
        return;

    // Buffer temporaire pour les points
    GL::Buffer pointBuffer;
    pointBuffer.setData(points);

    GL::Mesh pointMesh;
    pointMesh.setPrimitive(GL::MeshPrimitive::Points)
        .setCount(points.size())
        .addVertexBuffer(std::move(pointBuffer), 0, GL::Attribute<0, Magnum::Vector3>{});

    // Augmenter la taille des points
    GL::Renderer::setPointSize(5.0f);
    
    // Shader simple pour afficher les points (utilise le shader flat de base)
    Magnum::Shaders::FlatGL3D shader{Magnum::Shaders::FlatGL3D::Configuration{}.setFlags(Magnum::Shaders::FlatGL3D::Flag::VertexColor)};
    shader.setTransformationProjectionMatrix(viewProj)
        .setColor(0x00ff00_rgbf) // Vert
        .draw(pointMesh);
    
    GL::Renderer::setPointSize(1.0f); // Restaurer la taille par défaut
}

} // namespace WaterSimulation

