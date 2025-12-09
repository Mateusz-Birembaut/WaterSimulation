#include <WaterSimulation/Rendering/HeightmapReadback.h>

#include <Magnum/GL/OpenGL.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Vector2.h>

#include <cstring>

using namespace Magnum;
using namespace WaterSimulation;

namespace {
	constexpr GLenum kPixelFormat = GL_RGBA;
	constexpr GLenum kPixelType = GL_FLOAT;
	constexpr std::size_t kChannels = 4;
} // namespace

HeightmapReadback::~HeightmapReadback() {
	destroyBuffers();
}

void HeightmapReadback::init(const Vector2i& size) {
	m_size = size;
	allocateBuffers();
}

void HeightmapReadback::initTerrainHeightmapFromTexture(Magnum::GL::Texture2D& texture) {
	const Vector2i texSize = texture.imageSize(0);
	const auto totalTexels = std::size_t(texSize.x()) * std::size_t(texSize.y());

	m_terrainHeightmap.clear();
	m_terrainSize = texSize;
	if (totalTexels == 0) {
		m_terrainSize = {0, 0};
		return;
	}

	m_terrainHeightmap.resize(totalTexels);

	glBindTexture(GL_TEXTURE_2D, texture.id());
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, m_terrainHeightmap.data());
	glBindTexture(GL_TEXTURE_2D, 0);
}

void HeightmapReadback::resize(const Vector2i& size) {
	if (size == m_size)
		return;

	m_size = size;
	allocateBuffers();
}

void HeightmapReadback::allocateBuffers() {
	destroyBuffers();

	if (m_size.product() <= 0)
		return;

	m_texelCount = std::size_t(m_size.x()) * std::size_t(m_size.y());
	m_byteSize = m_texelCount * kChannels * sizeof(float);

	glGenBuffers(2, m_pbos.data());

	for (GLuint pbo : m_pbos) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
		glBufferData(GL_PIXEL_PACK_BUFFER, m_byteSize, nullptr, GL_STREAM_READ);
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	for (auto& cache : m_cpuCaches)
		cache.resize(m_texelCount * kChannels);

	m_writeIndex = 0;
	m_lastCpuIndex = 0;
	m_pending = {false, false};
	m_hasCpuData = false;
}

void HeightmapReadback::destroyBuffers() {
	if (m_pbos[0] == 0 && m_pbos[1] == 0)
		return;

	glDeleteBuffers(2, m_pbos.data());
	m_pbos = {0, 0};
}

void HeightmapReadback::enqueueReadback(GL::Texture2D& texture) {
	if (m_texelCount == 0 || m_pbos[0] == 0)
		return;

	const int currentWrite = m_writeIndex;

	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbos[currentWrite]);
	glBindTexture(GL_TEXTURE_2D, texture.id());
	glGetTexImage(GL_TEXTURE_2D, 0, kPixelFormat, kPixelType, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	m_pending[currentWrite] = true;
	m_writeIndex = (m_writeIndex + 1) % 2;
}

bool HeightmapReadback::fetchLatestCPUCopy() {
	if (m_texelCount == 0 || m_pbos[0] == 0)
		return false;

	const int readIndex = (m_writeIndex + 1) % 2;
	if (!m_pending[readIndex])
		return false;

	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbos[readIndex]);
	void* ptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

	if (!ptr) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		return false;
	}

	std::memcpy(m_cpuCaches[readIndex].data(), ptr, m_byteSize);

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	m_pending[readIndex] = false;
	m_lastCpuIndex = readIndex;
	m_hasCpuData = true;
	return true;
}

float HeightmapReadback::heightAt(int x, int y) const {
	if (!m_hasCpuData || x < 0 || y < 0 || x >= m_size.x() || y >= m_size.y())
		return 0.0f;

	const std::size_t texelIndex = std::size_t(y) * std::size_t(m_size.x()) + std::size_t(x);
	const std::size_t idx = texelIndex * kChannels;
	float waterHeight = m_cpuCaches[m_lastCpuIndex][idx];
	if (texelIndex < m_terrainHeightmap.size())
		waterHeight += m_terrainHeightmap[texelIndex];

	return waterHeight * 1.5f; // TODO : METTRE CA EN GLOBALE OU PENSER A LE MODIFIER
}

float HeightmapReadback::heightAtUV(const Vector2& uv) const {
	if (!m_hasCpuData)
		return 0.0f;

	const int x = Math::clamp<int>(int(uv.x() * float(m_size.x() - 1) + 0.5f), 0, m_size.x() - 1);
	const int y = Math::clamp<int>(int(uv.y() * float(m_size.y() - 1) + 0.5f), 0, m_size.y() - 1);

	return heightAt(x, y);
}

Magnum::Vector3 HeightmapReadback::stateAt(int x, int y) const {
	if (!m_hasCpuData || x < 0 || y < 0 || x >= m_size.x() || y >= m_size.y())
		return {0.0f, 0.0f, 0.0f};

	const std::size_t texelIndex = std::size_t(y) * std::size_t(m_size.x()) + std::size_t(x);
	const std::size_t idx = texelIndex * kChannels;
	const auto& cache = m_cpuCaches[m_lastCpuIndex];
	return {cache[idx + 0], cache[idx + 1], cache[idx + 2]};
}

Magnum::Vector2 HeightmapReadback::velocityAt(int x, int y) const {
	Magnum::Vector3 state = stateAt(x, y);
	float h = state.x();
	if (h < 1e-4f) return {0.0f, 0.0f};
	return {state.y() / h, state.z() / h};
}
