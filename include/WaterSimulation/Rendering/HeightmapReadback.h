#pragma once

#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>

#include <array>
#include <cstddef>
#include <vector>

namespace WaterSimulation {

	class HeightmapReadback {
	      public:
		HeightmapReadback() = default;
		~HeightmapReadback();

		void init(const Magnum::Vector2i& size);
		void initTerrainHeightmapFromTexture(Magnum::GL::Texture2D& texture);
		void resize(const Magnum::Vector2i& size);

		void enqueueReadback(Magnum::GL::Texture2D& texture);
		bool fetchLatestCPUCopy();

		bool hasCpuData() const {
			return m_hasCpuData;
		}
		const std::vector<float>& latestCpuData() const {
			return m_cpuCaches[m_lastCpuIndex];
		}

		float heightAt(int x, int y) const;
		float heightAtUV(const Magnum::Vector2& uv) const;
		Magnum::Vector3 stateAt(int x, int y) const;
		Magnum::Vector2 velocityAt(int x, int y) const;

		Magnum::Vector2i size() const {
			return m_size;
		}

		const std::vector<float>& terrainHeightmap() const {
			return m_terrainHeightmap;
		}

		Magnum::Vector2i terrainSize() const {
			return m_terrainSize;
		}

	      private:
		void allocateBuffers();
		void destroyBuffers();

		std::array<GLuint, 2> m_pbos{0, 0};
		std::array<std::vector<float>, 2> m_cpuCaches{}; // pour la hauteur de l'eau seulement
		std::vector<float> m_terrainHeightmap{};
		Magnum::Vector2i m_terrainSize{0};

		Magnum::Vector2i m_size{0};
		std::size_t m_texelCount{0};
		std::size_t m_byteSize{0};

		int m_writeIndex{0};
		int m_lastCpuIndex{0};
		std::array<bool, 2> m_pending{false, false};
		bool m_hasCpuData{false};
	};

} // namespace WaterSimulation
