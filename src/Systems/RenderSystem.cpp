#include "Corrade/Utility/Debug.h"
#include <WaterSimulation/Systems/RenderSystem.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/DirectionalLightComponent.h>
#include <WaterSimulation/Components/ShadowCasterComponent.h>
#include <WaterSimulation/Components/TerrainComponent.h>
#include <WaterSimulation/Components/MaterialComponent.h>

#include <WaterSimulation/Camera.h>
#include <WaterSimulation/Mesh.h>

#include <WaterSimulation/FrustumVisualizer.h>
#include <WaterSimulation/DebugDraw.h>

#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
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

    m_opaquePass.render(registry, viewMatrix, projectionMatrix, m_lightViewProjMatrix, m_shadowMapPass.getDepthTexture()); // TODO : calculer const Matrix4 viewProj = projMatrix * viewMatrix; pour la passer aux deux render

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

    drawFullscreenTexture(m_opaquePass.getColorTexture(), cam.near(), cam.far());
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


/*
    auto sunView = registry.view<DirectionalLightComponent, ShadowCasterComponent>();
    if(sunView.begin() != sunView.end()) {
        Entity sunEntity = *sunView.begin();
		auto& sunDirection = sunView.get<DirectionalLightComponent>(sunEntity);
		auto& shadowCastData = sunView.get<ShadowCasterComponent>(sunEntity);

        float maxFarCam = 50.0f;
        float minFarCam = 1.0f;

		Magnum::Vector3 p_mid{cam.position() + (((cam.near() + std::clamp(cam.far(), minFarCam, maxFarCam))/2.0f) * cam.direction())};
        Magnum::Vector3 lightPos {p_mid - sunDirection.direction.normalized() * sunDirection.offset};

        Vector3 forward = (p_mid - lightPos).normalized();

        Vector3 up = Magnum::Vector3::yAxis().normalized();
        Vector3 right = Magnum::Math::cross(forward, up).normalized();

        up = Magnum::Math::cross(right, forward).normalized();

        Matrix4 lightView = Matrix4::lookAt(lightPos, p_mid, up);
        lightView = lightView.invertedRigid();

		const Magnum::Matrix4 lightProj = Magnum::Matrix4::orthographicProjection(
			shadowCastData.projectionSize, 
			shadowCastData.near,                    
			shadowCastData.far                       
		);

        float len = 2.0f;
        DebugDraw::instance().addPoint(lightPos, Color3{0.0f, 0.0f, 0.0f}, 0.5f);
        DebugDraw::instance().addPoint(p_mid, Color3{1.0f, 1.0f, 0.0f}, 0.5f);
        DebugDraw::instance().addLine(p_mid, p_mid + (forward * len), Color3{0.0f, 0.0f, 1.0f});
        DebugDraw::instance().addLine(p_mid, p_mid + (up * len), Color3{0.0f, 1.0f, 0.0f});
        DebugDraw::instance().addLine(p_mid, p_mid + (right * len), Color3{1.0f, 0.0f, 0.0f});
        FrustumVisualizer::draw(lightView, lightProj, Color3{1.0f, 1.0f, 0.0f});
    }

    DebugDraw::instance().draw(viewMatrix, projectionMatrix);
    */

} // namespace WaterSimulation

