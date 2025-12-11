
#include <WaterSimulation/WaterSimulation.h>

#include <WaterSimulation/Systems/RenderSystem.h>
#include <WaterSimulation/Systems/TransformSystem.h>
#include <WaterSimulation/Systems/PhysicsSystem.h>

#include <WaterSimulation/Camera.h>
#include <WaterSimulation/Mesh.h>
#include <WaterSimulation/UIManager.h>
#include <WaterSimulation/ECS.h>

#include <WaterSimulation/Rendering/CustomShader/TerrainShader.h>
#include <WaterSimulation/Rendering/CustomShader/DebugShader.h>
#include <WaterSimulation/Rendering/CustomShader/PBRShader.h>

#include <WaterSimulation/Components/MeshComponent.h>
#include <WaterSimulation/Components/TransformComponent.h>
#include <WaterSimulation/Components/MaterialComponent.h>
#include <WaterSimulation/Components/ShaderComponent.h>
#include <WaterSimulation/Components/DirectionalLightComponent.h>
#include <WaterSimulation/Components/ShadowCasterComponent.h>
#include <WaterSimulation/Components/ShaderComponent.h>
#include <WaterSimulation/Components/WaterComponent.h>
#include <WaterSimulation/Components/TerrainComponent.h>
#include <WaterSimulation/Components/BuoyancyComponent.h>
#include <WaterSimulation/Components/RigidBodyComponent.h>

#include <Corrade/Containers/StringView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Debug.h>
#include <Corrade/Utility/Path.h>

#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/ImageView.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Time.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Shaders/VertexColorGL.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include <SDL2/SDL.h>

#include <memory>
#include <algorithm>
#include <limits>


static std::string resolveResourcesDir()
{
    namespace Path = Corrade::Utility::Path;
    const auto exeLoc = Path::executableLocation();
    const std::string exeDir = exeLoc ? std::string{Path::split(*exeLoc).first()} : std::string{"."};

    const std::string candidates[] = {
        Path::join(exeDir, "resources"),                           // alongside executable
        Path::join(Path::join(exeDir, ".."), "resources"),         // ../resources
        Path::join(Path::join(exeDir, "../.."), "resources"),
        Path::join(Path::join(exeDir, "../../.."), "resources"),      // ../../resources (common when exe in build/Debug/bin)
        std::string{"resources"}                                   // relative cwd fallback
    };

    for (const auto& candidate : candidates) {
        if (Path::isDirectory(candidate)) return candidate;
    }
    // Last resort: return alongside exe even if missing; caller will log
    return candidates[0];
}

const auto resDir = resolveResourcesDir();

using namespace Magnum;
using namespace Math::Literals;
using namespace Corrade::Utility;


WaterSimulation::Application::Application(const Arguments& arguments): 
    Platform::Application{arguments, Configuration{}
        .setTitle("Water Simulation App")
        .addWindowFlags(Configuration::WindowFlag::Resizable)
    } 
{
    // Détecte l'écran où se trouve la souris et redimensionne la fenêtre à la taille de cet écran
    int mouseX = 0, mouseY = 0;
    SDL_GetGlobalMouseState(&mouseX, &mouseY);
    int numDisplays = SDL_GetNumVideoDisplays();
    int targetDisplay = 0;
    for(int i = 0; i < numDisplays; ++i) {
        SDL_Rect bounds;
        if(SDL_GetDisplayBounds(i, &bounds) == 0) {
            if(mouseX >= bounds.x && mouseX < bounds.x + bounds.w &&
               mouseY >= bounds.y && mouseY < bounds.y + bounds.h) {
                targetDisplay = i;
                break;
            }
        }
    }
    SDL_Rect targetBounds;
    if(SDL_GetDisplayBounds(targetDisplay, &targetBounds) == 0) {
        SDL_Window* sdlWindow = static_cast<SDL_Window*>(window());
        // Positionner la fenêtre d'abord sur l'écran cible
        SDL_SetWindowPosition(sdlWindow, targetBounds.x, targetBounds.y);
        // Redimensionner la fenêtre via SDL pour s'assurer d'utiliser la résolution de l'écran cible
        SDL_SetWindowSize(sdlWindow, targetBounds.w, targetBounds.h);
        // Mettre aussi à jour Magnum au cas où
        setWindowSize({targetBounds.w, targetBounds.h});
    }
    Debug{} << "Creating application";
    
    m_timeline.start();
    
    // Plugins setup

    Corrade::PluginManager::Manager<Magnum::Trade::AbstractImporter> importerManager;
    Corrade::PluginManager::Manager<Magnum::Trade::AbstractImageConverter> converterManager;


    auto importer = importerManager.loadAndInstantiate("StbImageImporter");
    auto converter = converterManager.loadAndInstantiate("StbResizeImageConverter");

    /* auto importer = importerManager.loadAndInstantiate("StbImageImporter");

    if(!importer){
        Error{} << "Could not load STB Image Importer plugin, have you initialized all git submodules ?";
    }else{
        Debug{} << "Plugin STB Image Importer loaded ";
    }

    auto converter = converterManager.loadAndInstantiate("StbResizeImageConverter");

    if(!importer){
        Error{} << "Could not load STB Resize Image Converter plugin, have you initialized all git submodules ?";
    }else{
        Debug{} << "Plugin STB Image Resizer and Converter loaded ";
    }

    const auto heightmapFile = Corrade::Utility::Path::join(resDir, "heightmaps/unnamed.jpg");
    if(!importer->openFile(heightmapFile)) {
        Error{} << "Could not open heightmap" << heightmapFile;
        return;
    }
    auto image = importer->image2D(0);

    converter->configuration().setValue("size", "512 512");
    auto resized = converter->convert(*image);
    auto allo = resized->format();
    Debug{} << "FORMAT IS : " << allo;
    Debug{} << "SIZE IS : " << resized->size(); */
    
    // Shallow Water simulation setup
    m_shallowWaterSimulation = ShallowWater(511,511, .25f, 1.0f/60.0f);
    m_heightmapReadback.init({m_shallowWaterSimulation.getnx() + 1, m_shallowWaterSimulation.getny() + 1});

    //m_shallowWaterSimulation.loadTerrainHeightMap(&*resized, 20.0f, 3);

    m_shallowWaterSimulation.initDamBreak();

    debugShader = DisplayShader("debug.vs", "debug.fs");
    

    // ImGui setup
    m_imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(),windowSize(), framebufferSize());

    // OpenGL setup

    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);

    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    GL::Renderer::setClearColor(0xa5c9ea_rgbf);

    m_renderSystem.init(framebufferSize());

    m_pbrShader = std::make_shared<PBRShader>();

    Debug{} << "This application is running on"
            << GL::Context::current().version() << "using"
            << GL::Context::current().rendererString();

    m_UIManager = std::make_unique<UIManager>();
    m_camera = std::make_unique<Camera>(windowSize());
    m_camera.get()->setPos({0.0, 10.0, 0.0f});
    m_camera.get()->setSpeed(25.0f);
    m_camera.get()->setRotSpeed(5.0f);
    
    // test ECS et rendu avec shader de base
    // test sphere avec texture

    const auto grassFile = Corrade::Utility::Path::join(resDir, "textures/grass.png");
    if(!importer->openFile(grassFile)) {
        Error{} << "Could not open texture" << grassFile;
        return;
    }
    auto imageTest = importer->image2D(0);
    m_testAlbedo.setWrapping(Magnum::GL::SamplerWrapping::ClampToEdge)
                    .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
                    .setMagnificationFilter(Magnum::GL::SamplerFilter::Nearest)
                    .setStorage(1, Magnum::GL::TextureFormat::RGBA8, imageTest->size())
                    .setSubImage(0, {}, Magnum::ImageView2D{*imageTest});   

    auto albedoPtr = std::make_shared<Magnum::GL::Texture2D>(std::move(m_testAlbedo));

    
    const auto metalAlbedoPath = Corrade::Utility::Path::join(resDir, "textures/sphereMetal/s_metal_diff.png");
    const auto metalArmPath    = Corrade::Utility::Path::join(resDir, "textures/sphereMetal/s_metal_arm.png");
    const auto metalNormalPath = Corrade::Utility::Path::join(resDir, "textures/sphereMetal/s_metal_normal.png");
    m_metalAlbedo = loadTexCached(metalAlbedoPath);
    m_metalArm    = loadTexCached(metalArmPath);
    m_metalNormal = loadTexCached(metalNormalPath);

    float scale = 200.0f;
    
    createTerrain(scale);

    //visu eau rapide
    auto waterHeightTexPtr = std::shared_ptr<Magnum::GL::Texture2D>(&m_shallowWaterSimulation.getTerrainTexture(), [](Magnum::GL::Texture2D*){});
    auto waterAlbedoTexPtr = std::shared_ptr<Magnum::GL::Texture2D>(&m_shallowWaterSimulation.getStateTexture(), [](Magnum::GL::Texture2D*){});


    m_waterMesh = std::make_unique<Mesh>(Mesh::createGrid(512, 512, scale)); 
    Entity waterEntity = m_registry.create();
    m_registry.emplace<MeshComponent>(
        waterEntity,
        std::vector<std::pair<float, Mesh*>>{{0.0f, m_waterMesh.get()}}
    );
    m_registry.emplace<TransformComponent>(
        waterEntity,
        Magnum::Vector3{0.0f, -1.0f, -3.0f} 
    );
    m_registry.emplace<WaterComponent>(waterEntity, 512, 512, scale);

    auto waterShader = std::make_shared<DebugShader>();
    auto& waterMat = m_registry.emplace<MaterialComponent>(waterEntity);
    waterMat.heightmap = waterHeightTexPtr;
    waterMat.albedo = waterAlbedoTexPtr;

    m_registry.emplace<ShaderComponent>(
        waterEntity,
        waterShader
    ); 


    spawnSphereAt(Magnum::Vector3(0.0f, 15.0f, -35.0f));


    // sun light en cours
    auto sunEntity = m_registry.create();
    m_registry.emplace<TransformComponent>(sunEntity, Vector3{50.0f, 100.0f, 50.0f});
    m_registry.emplace<DirectionalLightComponent>(sunEntity, Color3{1.0f, 0.95f, 0.8f}, 5.0f);
    m_registry.emplace<ShadowCasterComponent>(sunEntity);  

    m_renderSystem.setHeightmapReadback(&m_heightmapReadback);
    m_physicSystem.setHeightmapReadback(&m_heightmapReadback);
}
    

WaterSimulation::Application::~Application() {
    m_registry.clear(); 
}

void WaterSimulation::Application::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    m_renderSystem.resize(event.framebufferSize());
    m_imgui.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
        event.windowSize(), event.framebufferSize());

    m_camera->setWindowSize(event.windowSize());
}

//main draw loop
void WaterSimulation::Application::drawEvent() {
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
    
    m_timeline.nextFrame();
    m_deltaTime = m_timeline.previousFrameDuration();

    handleCameraInputs();

    
    debugShader.bind(&m_shallowWaterSimulation.getStateTexture(), 0);
    debugShader.bind(&m_shallowWaterSimulation.getTerrainTexture(), 1);
    

    if(!simulationPaused) {
        for(int i = 0; i < step_number; ++i){
            m_transform_System.update(m_registry);
            m_physicSystem.update(m_registry, m_deltaTime);

            // appliquer les mouvments sur l'eau 
            const auto& disturbances = m_physicSystem.getDisturbances();
            if (!disturbances.empty()) {
                std::vector<ShallowWater::Disturbance> wakeDisturbances;
                wakeDisturbances.reserve(disturbances.size());
                for (const auto& d : disturbances) {
                    wakeDisturbances.push_back({d.px, d.py, d.strength, d._padding});
                }
                m_shallowWaterSimulation.applyDisturbances(wakeDisturbances);
            }

            m_shallowWaterSimulation.step();
            m_heightmapReadback.enqueueReadback(m_shallowWaterSimulation.getStateTexture());
        }
    }


    m_renderSystem.render(m_registry, *m_camera.get());

    m_UIManager->drawUI(*this);

    if(m_cursorLocked)
        setCursor(Platform::Sdl2Application::Cursor::HiddenLocked);

    swapBuffers();
    redraw();
}


void WaterSimulation::Application::keyPressEvent(KeyEvent& event) {
    if(m_imgui.handleKeyPressEvent(event)) return;

    using Key = Platform::Sdl2Application::Key;
    if(event.key() == Key::Enter) {
        setCursor(Platform::Sdl2Application::Cursor::HiddenLocked);
        m_cursorLocked = true;
        Debug{} << "locked";
        return;
    }
    if(event.key() == Key::Esc) {
        setCursor(Platform::Sdl2Application::Cursor::Arrow);
        m_cursorLocked = false;
        Debug{} << "unlocked";
        return;
    }

    if(event.key() == Key::N) {
        m_renderSystem.m_renderDepthOnly = !m_renderSystem.m_renderDepthOnly;
        Debug{} << "Depth Mode :" << m_renderSystem.m_renderDepthOnly;
        return;
    }
    if(event.key() == Key::B) {
        m_renderSystem.m_renderShadowMapOnly = !m_renderSystem.m_renderShadowMapOnly;
        Debug{} << "Draw shadow map only:" << m_renderSystem.m_renderShadowMapOnly;
        return;
    }
    if(event.key() == Key::V) {
        m_renderSystem.m_renderWaterMaskOnly = !m_renderSystem.m_renderWaterMaskOnly;
        Debug{} << "Draw shadow map only:" << m_renderSystem.m_renderWaterMaskOnly;
        return;
    }
    if(event.key() == Key::C) {
        m_renderSystem.m_renderCausticMapOnly = !m_renderSystem.m_renderCausticMapOnly;
        Debug{} << "Draw shadow map only:" << m_renderSystem.m_renderCausticMapOnly;
        return;
    }
    if(event.key() == Key::X) {
        m_renderSystem.m_renderGodRayMapOnly = !m_renderSystem.m_renderGodRayMapOnly;
        Debug{} << "Draw shadow map only:" << m_renderSystem.m_renderGodRayMapOnly;
        return;
    }

    if(event.key() == Key::A) {
        const Magnum::Vector3 camPos = m_camera->position();
        const Magnum::Vector3 dir = m_camera->direction();
        const float spawnOffset = 2.5f;
        const Magnum::Vector3 spawnPos = camPos + dir * spawnOffset;

        spawnSphereAt(spawnPos, dir);
        return;
    }

    m_keysPressed.insert(event.key());
}

void WaterSimulation::Application::keyReleaseEvent(KeyEvent& event) {
    if(m_imgui.handleKeyReleaseEvent(event)) return;
    m_keysPressed.erase(event.key());
}

void WaterSimulation::Application::pointerPressEvent(PointerEvent& event) {
    if(m_imgui.handlePointerPressEvent(event)) return;
}

void WaterSimulation::Application::pointerReleaseEvent(PointerEvent& event) {
    if(m_imgui.handlePointerReleaseEvent(event)) return;
}

void WaterSimulation::Application::pointerMoveEvent(PointerMoveEvent& event) {
    if(m_imgui.handlePointerMoveEvent(event)) return;

    if(!m_cursorLocked) return;

    Vector2 delta = event.relativePosition();
    if(delta.isZero()) return;

    delta *= Vector2{-1.0f, -1.0f};
    delta *= m_camera->rotSpeed() * m_deltaTime;
    m_camera->rotate(delta);

}

void WaterSimulation::Application::scrollEvent(ScrollEvent& event) {
    if(m_imgui.handleScrollEvent(event)) {
        /* Prevent scrolling the page */
        event.setAccepted();
        return;
    }
}

void WaterSimulation::Application::textInputEvent(TextInputEvent& event) {
    if(m_imgui.handleTextInputEvent(event)) return;
}

void WaterSimulation::Application::handleCameraInputs(){
    using Key = Platform::Sdl2Application::Key;
    const float distance = m_camera->speed() * m_deltaTime;
    if(m_keysPressed.count(Key::D)) m_camera->move({ distance, 0.0f,  0.0f});
    if(m_keysPressed.count(Key::Q)) m_camera->move({-distance, 0.0f,  0.0f});
    if(m_keysPressed.count(Key::S)) m_camera->move({ 0.0f, 0.0f,  distance});
    if(m_keysPressed.count(Key::Z)) m_camera->move({ 0.0f, 0.0f, -distance});
}

MAGNUM_APPLICATION_MAIN(WaterSimulation::Application)


uint32_t WaterSimulation::Application::createSphereEntity(const Magnum::Vector3& position, float radius, float mass, float flotability, float waterDrag, std::shared_ptr<Magnum::GL::Texture2D> albedo, std::shared_ptr<Magnum::GL::Texture2D> arm)
{
    Entity e = m_registry.create();

    auto & mat = m_registry.emplace<MaterialComponent>(e);


    const bool customTextures = (albedo || arm);
    mat.albedo = albedo ? albedo : m_metalAlbedo;
    mat.arm    = arm ? arm : m_metalArm;
    if (!customTextures && m_metalNormal) mat.normal = m_metalNormal;

    m_registry.emplace<TransformComponent>(e, position, Magnum::Quaternion(Magnum::Math::IdentityInit), Magnum::Vector3{radius});

    if(!m_testMesh) {
        try {
            auto spherePath = Corrade::Utility::Path::join(resDir, "assets/Meshes/sphereLOD1.obj");
            m_testMesh = std::make_unique<Mesh>(spherePath);
        } catch(...) {
            Debug{} << "Failed to ensure sphere mesh is loaded";
        }
    }

    if(m_testMesh) {
        m_registry.emplace<MeshComponent>(e, std::vector<std::pair<float, Mesh*>>{{0.0f, m_testMesh.get()}});
    } else {
        m_registry.emplace<MeshComponent>(e, std::vector<std::pair<float, Mesh*>>{});
    }

    if (m_pbrShader) {
        m_registry.emplace<ShaderComponent>(e, m_pbrShader);
    }

    auto& rb = m_registry.emplace<RigidBodyComponent>(e);
    rb.mass = mass;
    rb.inverseMass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
    rb.linearDamping = 0.4f;
    rb.angularDamping = 0.6f;
    rb.mesh = (m_testMesh) ? m_testMesh.get() : nullptr;

    auto* sphereCol = new SphereCollider(radius);
    sphereCol->mass = mass;
    sphereCol->computeInertiaTensor();
    rb.addCollider(sphereCol);

    auto& b = m_registry.emplace<BuoyancyComponent>(e);
    b.flotability = flotability;
    b.waterDrag = waterDrag;

    auto shaderPtr = std::make_shared<PBRShader>();
    m_registry.emplace<ShaderComponent>(
        e,
        shaderPtr
    ); 

    return e;
}

uint32_t WaterSimulation::Application::spawnSphereAt(const Magnum::Vector3& position, const Magnum::Vector3& impulseDir)
{
    const bool heavy = m_nextHeavyShot;
    const float radius = 1.0f;
    const float mass = heavy ? 1600.0f : 350.0f;
    const float waterDrag = heavy ? 2.0f : 5.0f;
    const float flotability = heavy ? 80.0f : 300.0f;
    const float impulseStrength = heavy ? 9000.0f : 6000.0f;

    static const std::string metalAlbedoPath = Corrade::Utility::Path::join(resDir, "textures/sphereMetal/s_metal_diff.png");
    static const std::string metalArmPath    = Corrade::Utility::Path::join(resDir, "textures/sphereMetal/s_metal_arm.png");
    static const std::string metalNormalPath = Corrade::Utility::Path::join(resDir, "textures/sphereMetal/s_metal_norm.png");

    static const std::string woodAlbedoPath = Corrade::Utility::Path::join(resDir, "textures/sphereWood/moss_wood_diff_1k.png");
    static const std::string woodArmPath    = Corrade::Utility::Path::join(resDir, "textures/sphereWood/moss_wood_arm_1k.png");
    static const std::string woodNormalPath = Corrade::Utility::Path::join(resDir, "textures/sphereWood/moss_wood_nor_gl_1k.png");

    const std::string& shotNormal    = heavy ? metalNormalPath    : woodNormalPath;

    std::shared_ptr<Magnum::GL::Texture2D> shotAlbedoTex;
    std::shared_ptr<Magnum::GL::Texture2D> shotArmTex;
    if (heavy) {
        shotAlbedoTex = m_metalAlbedo ? m_metalAlbedo : loadTexCached(metalAlbedoPath);
        shotArmTex    = m_metalArm ? m_metalArm : loadTexCached(metalArmPath);
    } else {
        shotAlbedoTex = loadTexCached(woodAlbedoPath);
        shotArmTex    = loadTexCached(woodArmPath);
    }

    auto e = createSphereEntity(position, radius, mass, flotability, waterDrag, shotAlbedoTex, shotArmTex);

    if (m_registry.has<MaterialComponent>(e)) {
        auto& mat = m_registry.get<MaterialComponent>(e);
        std::shared_ptr<Magnum::GL::Texture2D> normalTex;
        if (heavy) {
            normalTex = m_metalNormal ? m_metalNormal : loadTexCached(shotNormal);
        } else {
            normalTex = loadTexCached(shotNormal);
        }
        if (normalTex) mat.normal = normalTex;
    }

    if (!impulseDir.isZero() && m_registry.has<RigidBodyComponent>(e) && m_registry.has<TransformComponent>(e)) {
        auto& rb = m_registry.get<RigidBodyComponent>(e);
        const Magnum::Vector3 dirNorm = impulseDir.normalized();
        rb.forceAccumulator += dirNorm * impulseStrength;
    }

    m_nextHeavyShot = !m_nextHeavyShot;

    return e;
}



uint32_t WaterSimulation::Application::createTerrain(float scale)
{
    Entity testTerrain = m_registry.create();
    auto & matTerrain = m_registry.emplace<MaterialComponent>(testTerrain);
    m_registry.emplace<TerrainComponent>(testTerrain);

    auto& terrainTexture = m_shallowWaterSimulation.getTerrainTexture();
    m_heightmapReadback.initTerrainHeightmapFromTexture(terrainTexture);

    auto heightmapPtr = std::shared_ptr<Magnum::GL::Texture2D>(&terrainTexture, [](Magnum::GL::Texture2D*){});

    Corrade::PluginManager::Manager<Magnum::Trade::AbstractImporter> importerManager;
    auto importer = importerManager.loadAndInstantiate("StbImageImporter");
    if(!importer) Debug{} << "Could not load STB Image Importer plugin for terrain textures";

    if(importer) {
        {
            auto sandAlbedoFile = Corrade::Utility::Path::join(resDir, "textures/terrain/sand_diff.png");
            if(!importer->openFile(sandAlbedoFile)) {
                Error{} << "Could not open" << sandAlbedoFile;
            }
            auto sandAlbedoImage = importer->image2D(0);
            Magnum::GL::Texture2D sandAlbedoTex;
            sandAlbedoTex.setWrapping(Magnum::GL::SamplerWrapping::Repeat)
                .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setStorage(1, Magnum::GL::TextureFormat::RGBA8, sandAlbedoImage->size())
                .setSubImage(0, {}, Magnum::ImageView2D{*sandAlbedoImage});
            matTerrain.albedo = std::make_shared<Magnum::GL::Texture2D>(std::move(sandAlbedoTex));
        }

        {
            auto sandNormalFile = Corrade::Utility::Path::join(resDir, "textures/terrain/sand_normal.png");
            if(!importer->openFile(sandNormalFile)) {
                Error{} << "Could not open" << sandNormalFile;
            }
            auto sandNormalImage = importer->image2D(0);
            Magnum::GL::Texture2D sandNormalTex;
            sandNormalTex.setWrapping(Magnum::GL::SamplerWrapping::Repeat)
                .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setStorage(1, Magnum::GL::TextureFormat::RGBA8, sandNormalImage->size())
                .setSubImage(0, {}, Magnum::ImageView2D{*sandNormalImage});
            matTerrain.normal = std::make_shared<Magnum::GL::Texture2D>(std::move(sandNormalTex));
        }

        {
            auto sandArmFile = Corrade::Utility::Path::join(resDir, "textures/terrain/sand_arm.png");
            if(!importer->openFile(sandArmFile)) {
                Error{} << "Could not open" << sandArmFile;
            }
            auto sandArmImage = importer->image2D(0);
            Magnum::GL::Texture2D sandArmTex;
            sandArmTex.setWrapping(Magnum::GL::SamplerWrapping::Repeat)
                .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
                .setStorage(1, Magnum::GL::TextureFormat::RGBA8, sandArmImage->size())
                .setSubImage(0, {}, Magnum::ImageView2D{*sandArmImage});
            matTerrain.arm = std::make_shared<Magnum::GL::Texture2D>(std::move(sandArmTex));
        }
        
    }

    matTerrain.heightmap = heightmapPtr;
    m_terrainMesh = std::make_unique<Mesh>(Mesh::createGrid(512, 512, scale));
    m_registry.emplace<MeshComponent>(
        testTerrain,
        std::vector<std::pair<float, Mesh*>>{{0.0f, m_terrainMesh.get()}}
    );
    m_registry.emplace<TransformComponent>(
        testTerrain,
        Magnum::Vector3{0.0f, -1.0f, -3.0f}
    );
    auto& terrainRigidBody = m_registry.emplace<RigidBodyComponent>(testTerrain);
    terrainRigidBody.bodyType = PhysicsType::STATIC;
    terrainRigidBody.useGravity = false;
    terrainRigidBody.mesh = m_terrainMesh.get();
    terrainRigidBody.linearVelocity = Magnum::Vector3{0.0f};
    terrainRigidBody.angularVelocity = Magnum::Vector3{0.0f};

    auto* terrainCollider = new MeshCollider();
    terrainCollider->mass = 1.0f; 
    terrainCollider->localCentroid = Magnum::Vector3{0.0f};
    terrainCollider->localInertiaTensor = Magnum::Matrix3{Magnum::Math::ZeroInit};
    terrainCollider->vertices = m_terrainMesh->vertices;
    terrainCollider->indices = m_terrainMesh->triangles;

    const Magnum::Vector2i terrainSize = m_heightmapReadback.terrainSize();
    const std::size_t expectedCount = std::size_t(terrainSize.x()) * std::size_t(terrainSize.y());
    const auto& terrainHeights = m_heightmapReadback.terrainHeightmap();
    Magnum::Vector3 terrainMin{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    };
    Magnum::Vector3 terrainMax{
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()
    };
    if (terrainCollider->vertices.size() == expectedCount && terrainHeights.size() == expectedCount) {
        for (std::size_t i = 0; i < expectedCount; ++i) {
            terrainCollider->vertices[i].y() = terrainHeights[i] * 1.5f;
            const Magnum::Vector3& v = terrainCollider->vertices[i];
            terrainMin.x() = std::min(terrainMin.x(), v.x());
            terrainMin.y() = std::min(terrainMin.y(), v.y());
            terrainMin.z() = std::min(terrainMin.z(), v.z());
            terrainMax.x() = std::max(terrainMax.x(), v.x());
            terrainMax.y() = std::max(terrainMax.y(), v.y());
            terrainMax.z() = std::max(terrainMax.z(), v.z());
        }
    }
    terrainCollider->localMin = terrainMin;
    terrainCollider->localMax = terrainMax;
    terrainCollider->resolution = terrainSize;
    TransformComponent& terrainTransform = m_registry.get<TransformComponent>(testTerrain);
    Magnum::Matrix4 terrainModel = terrainTransform.model();
    terrainTransform.globalModel = terrainModel;
    terrainTransform.inverseGlobalModel = terrainModel.inverted();
    Magnum::Vector3 worldMin{std::numeric_limits<float>::max()};
    Magnum::Vector3 worldMax{std::numeric_limits<float>::lowest()};
    for (int ix = 0; ix < 2; ++ix) {
        for (int iy = 0; iy < 2; ++iy) {
            for (int iz = 0; iz < 2; ++iz) {
                Magnum::Vector3 corner{
                    ix ? terrainMax.x() : terrainMin.x(),
                    iy ? terrainMax.y() : terrainMin.y(),
                    iz ? terrainMax.z() : terrainMin.z()
                };
                Magnum::Vector3 worldCorner = terrainModel.transformPoint(corner);
                worldMin = Magnum::Vector3{
                    std::min(worldMin.x(), worldCorner.x()),
                    std::min(worldMin.y(), worldCorner.y()),
                    std::min(worldMin.z(), worldCorner.z())
                };
                worldMax = Magnum::Vector3{
                    std::max(worldMax.x(), worldCorner.x()),
                    std::max(worldMax.y(), worldCorner.y()),
                    std::max(worldMax.z(), worldCorner.z())
                };
            }
        }
    }
    terrainRigidBody.aabbCollider.min = worldMin;
    terrainRigidBody.aabbCollider.max = worldMax;

    terrainRigidBody.addCollider(terrainCollider);
    terrainRigidBody.bodyType = PhysicsType::STATIC;
    terrainRigidBody.mass = std::numeric_limits<float>::infinity();
    terrainRigidBody.inverseMass = 0.0f;
    terrainRigidBody.localInertiaTensor = Magnum::Matrix3{Magnum::Math::ZeroInit};
    terrainRigidBody.localInverseInertiaTensor = Magnum::Matrix3{Magnum::Math::ZeroInit};
    terrainRigidBody.globalInverseInertiaTensor = Magnum::Matrix3{Magnum::Math::ZeroInit};
    terrainRigidBody.forceAccumulator = Magnum::Vector3{0.0f};
    terrainRigidBody.torqueAccumulator = Magnum::Vector3{0.0f};
    
    auto shaderPtr = std::make_shared<TerrainShader>();
    m_registry.emplace<ShaderComponent>(
        testTerrain,
        shaderPtr
    ); 

    return testTerrain;
}

std::shared_ptr<Magnum::GL::Texture2D> WaterSimulation::Application::loadTex(const std::string& path) {
    Corrade::PluginManager::Manager<Trade::AbstractImporter> mgr;
    auto imp = mgr.loadAndInstantiate("StbImageImporter");
    if(!imp || !imp->openFile(path)) {
        Error{} << "tex fail" << Corrade::Containers::StringView{path};
        return {};
    }
    auto img = imp->image2D(0);
    Magnum::GL::Texture2D tex;
    tex.setWrapping(Magnum::GL::SamplerWrapping::Repeat)
       .setMinificationFilter(Magnum::GL::SamplerFilter::Linear)
       .setMagnificationFilter(Magnum::GL::SamplerFilter::Linear)
       .setStorage(1, Magnum::GL::TextureFormat::RGBA8, img->size())
       .setSubImage(0, {}, ImageView2D{*img});
    return std::make_shared<Magnum::GL::Texture2D>(std::move(tex));
}

std::shared_ptr<Magnum::GL::Texture2D> WaterSimulation::Application::loadTexCached(const std::string& path) {
    if (path.empty()) return {};
    if (auto it = m_textureCache.find(path); it != m_textureCache.end()) {
        if (auto cached = it->second.lock()) return cached;
    }
    auto tex = loadTex(path);
    if (tex) m_textureCache[path] = tex;
    return tex;
}