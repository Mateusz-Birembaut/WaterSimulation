find_package(Magnum REQUIRED GL Sdl2Application)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

add_executable(WaterSimulation 
    WaterSimulation.cpp
    ShallowWater.cpp
    UIManager.cpp
)

# Don't forget to also add plugins here
target_link_libraries(WaterSimulation PRIVATE
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    MagnumIntegration::ImGui
    Corrade::PluginManager
    Magnum::Trade
    MagnumPlugins::StbImageImporter
    MagnumPlugins::StbResizeImageConverter
)


target_include_directories(WaterSimulation PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Make the executable a default target to build & run in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT WaterSimulation)
