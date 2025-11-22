find_package(Magnum REQUIRED GL Sdl2Application)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

# Compile resources
corrade_add_resource(WaterSimulation_RESOURCES ${PROJECT_SOURCE_DIR}/resources/resources.conf)

add_executable(WaterSimulation 
    WaterSimulation.cpp
    ShallowWater.cpp
    UIManager.cpp
    Mesh.cpp
    Components/MeshComponent.cpp
    Systems/RenderSystem.cpp
    Systems/TransformSystem.cpp
    Rendering/DepthPass.cpp
    ${WaterSimulation_RESOURCES}
)

# Don't forget to also add plugins here
target_link_libraries(WaterSimulation PRIVATE
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    MagnumIntegration::ImGui
    Corrade::PluginManager
    Corrade::Utility
    Magnum::Trade
    MagnumPlugins::StbImageImporter
    MagnumPlugins::StbResizeImageConverter
)


target_include_directories(WaterSimulation
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)

# Make the executable a default target to build & run in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT WaterSimulation)
