#pragma once

#include <WaterSimulation/ECS.h>

#include <iostream>

namespace WaterSimulation {
	struct WaterComponent {
		int width{};
		int height{};
		float scale{75.0};

		GLuint pbo;

		std::vector<float> heightData{}; 


		explicit WaterComponent(int w = 0, int h = 0, float s = 75.0f)
		    : width(w), height(h), scale(s) {
			glGenBuffers(1, &pbo);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
			int nbElements { (w) * (h) * 4};
			heightData.resize(nbElements);
			glBufferData(GL_PIXEL_PACK_BUFFER,  nbElements * sizeof(float), NULL, GL_STREAM_READ);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		}

		WaterComponent(const WaterComponent&) = delete;
		WaterComponent& operator=(const WaterComponent&) = delete;

		WaterComponent(WaterComponent&& other) noexcept
			: width(other.width), 
			height(other.height), 
			scale(other.scale), 
			pbo(other.pbo),
			heightData(std::move(other.heightData)) 
		{
			other.pbo = 0; 
		}

		WaterComponent& operator=(WaterComponent&& other) noexcept {
			if (this != &other) {
				if (pbo) glDeleteBuffers(1, &pbo);

				width = other.width;
				height = other.height;
				scale = other.scale;
				pbo = other.pbo;
				
				heightData = std::move(other.heightData);

				other.pbo = 0; 
			}
			return *this;
		}

		~WaterComponent() {
			if (pbo) {
				glDeleteBuffers(1, &pbo);
				pbo = 0;
			}
		}

		void onAttach(Registry& registry [[maybe_unused]], Entity entity [[maybe_unused]]) {} // [[maybe_unused]] evite warnings
		void onDetach(Registry& registry [[maybe_unused]], Entity entity [[maybe_unused]]) {}
	};

} // namespace WaterSimulation
