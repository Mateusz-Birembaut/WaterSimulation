#pragma once

#include <WaterSimulation/ECS.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Sampler.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/Trade.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/ImageView.h>
#include <Magnum/Math/Color.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/PluginManager/Manager.h>

#include <vector>
#include <string>
#include <memory>

namespace WaterSimulation {
    struct MaterialComponent {

        std::shared_ptr<Magnum::GL::Texture2D> albedo;
        std::shared_ptr<Magnum::GL::Texture2D> arm; // ao rough metal
        std::shared_ptr<Magnum::GL::Texture2D> normal;
        std::shared_ptr<Magnum::GL::Texture2D> heightmap;


        Magnum::Color4 baseColor{1.0f};


        void onAttach(Registry& registry[[maybe_unused]], Entity entity[[maybe_unused]]) {};
        void onDetach(Registry& registry[[maybe_unused]], Entity entity[[maybe_unused]]) {};
    };
} // namespace WaterSimulation