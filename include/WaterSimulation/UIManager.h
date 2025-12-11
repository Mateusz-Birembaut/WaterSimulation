#pragma once

#include <WaterSimulation/Camera.h>
#include <WaterSimulation/ECS.h>

#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Containers/Pointer.h>

#include <WaterSimulation/ShallowWater.h>
#include <string.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Debug.h>

namespace WaterSimulation {
    class Application;

    class UIManager {
    public:
        void drawUI(Application& app);

        UIManager(){

            importer = importerManager.loadAndInstantiate("StbImageImporter");
            converter = converterManager.loadAndInstantiate("StbResizeImageConverter");

            //rs = Corrade::Utility::Resource{"WaterSimulationResources"};
           
        }

        void loadMap(const char * filename, int channels, float scaling, ShallowWater *simulation);
    private:

        Corrade::PluginManager::Manager<Magnum::Trade::AbstractImporter> importerManager;
        Corrade::PluginManager::Manager<Magnum::Trade::AbstractImageConverter> converterManager;

        Corrade::Containers::Pointer<Magnum::Trade::AbstractImporter> importer;
        Corrade::Containers::Pointer<Magnum::Trade::AbstractImageConverter> converter;

        //Corrade::Utility::Resource rs;

        
        void sunWindow(Registry & registry);
        void paramWindow(Magnum::Platform::Sdl2Application & app);
        void sceneGraph();
        void cameraWindow(Camera & cam);
        void visualWindow(Magnum::Platform::Sdl2Application & app);

        
    };
} // namespace WaterSimulationa