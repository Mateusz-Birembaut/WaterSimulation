#pragma once

#include <vector>
#include <string>
#include <cstdio>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Vector2.h>

namespace objl { class Loader; }

namespace WaterSimulation
{
	void loadOBJ(
		const char* fileName, 
		std::vector<Magnum::Vector3>& vertices, 
		std::vector<Magnum::Vector3>& normals,
		std::vector<Magnum::Vector2>& uvs, 
		std::vector<unsigned int>& triangles
	);



	struct Mesh {
		std::vector<Magnum::Vector3> vertices;
		std::vector<Magnum::Vector3> normals;
		std::vector<Magnum::Vector2> uvs;
		std::vector<unsigned int> triangles;

		Mesh() = default;

		explicit Mesh(const std::string& objFile){
			loadOBJ(objFile.c_str(), vertices, normals, uvs, triangles);
		}

		static Mesh createGrid(int nx, int ny, float size) {
			Mesh mesh;
			float halfSize = size * 0.5f;
			for(int y = 0; y < ny; ++y) {
				for(int x = 0; x < nx; ++x) {
					float px = -halfSize + size * (float(x) / (nx-1));
					float py = -halfSize + size * (float(y) / (ny-1));
					mesh.vertices.emplace_back(px, 0.0f, py);
					mesh.uvs.emplace_back(float(x) / (nx-1), float(y) / (ny-1));
					mesh.normals.emplace_back(0.0f, 1.0f, 0.0f);
				}
			}
			for(int y = 0; y < ny-1; ++y) {
				for(int x = 0; x < nx-1; ++x) {
					int i0 = y * nx + x;
					int i1 = i0 + 1;
					int i2 = i0 + nx;
					int i3 = i2 + 1;
					mesh.triangles.push_back(i0); mesh.triangles.push_back(i2); mesh.triangles.push_back(i1);
					mesh.triangles.push_back(i1); mesh.triangles.push_back(i2); mesh.triangles.push_back(i3);
				}
			}
			return mesh;
		}

		static std::pair<Magnum::Vector3, Magnum::Vector3> computeAABBFromMesh(const Mesh& mesh)
		{
			if(mesh.vertices.empty()) return {Magnum::Vector3{0.0f}, Magnum::Vector3{0.0f}};
			Magnum::Vector3 min = mesh.vertices[0];
			Magnum::Vector3 max = mesh.vertices[0];
			for(const auto& v : mesh.vertices) {
				min = Magnum::Vector3{
					std::min(min.x(), v.x()),
					std::min(min.y(), v.y()),
					std::min(min.z(), v.z())
				};
				max = Magnum::Vector3{
					std::max(max.x(), v.x()),
					std::max(max.y(), v.y()),
					std::max(max.z(), v.z())
				};
			}
			return {min, max};
		}
		
	};

} // namespace WaterSimulation

