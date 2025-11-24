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
        std::shared_ptr<Magnum::GL::Texture2D> normal;
        std::shared_ptr<Magnum::GL::Texture2D> metallicRoughness;
        std::shared_ptr<Magnum::GL::Texture2D> ao;
        std::shared_ptr<Magnum::GL::Texture2D> heightmap;

        Magnum::Color4 baseColor{1.0f};

        //TODO ajouter shader ici ? dans la opaque pass -> draw avec le shader approprié

        MaterialComponent() = default;

        MaterialComponent& setAlbedo(std::shared_ptr<Magnum::GL::Texture2D> tex) {
            albedo = tex;
            return *this;
        }

        MaterialComponent& setHeightMap(std::shared_ptr<Magnum::GL::Texture2D> tex) {
            heightmap = tex;
            return *this;
        }
        //TODO : ajouter les autres si on veut PBR

        void onAttach(Registry& registry[[maybe_unused]], Entity entity[[maybe_unused]]) {};
        void onDetach(Registry& registry[[maybe_unused]], Entity entity[[maybe_unused]]) {};
    };
} // namespace WaterSimulation