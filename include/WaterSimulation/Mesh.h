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

		int m_nx,m_ny;

		Mesh() = default;

		explicit Mesh(const std::string& objFile){
			loadOBJ(objFile.c_str(), vertices, normals, uvs, triangles);
		}

		static Mesh createGrid(int nx, int ny, float size) {
			Mesh mesh;
			mesh.m_nx = nx; mesh.m_ny = ny;
			float halfSize = size * 0.5f;
			for(int y = 0; y < ny; ++y) {
				for(int x = 0; x < nx; ++x) {
					float px = -halfSize + size * (float(x) / (nx-1));
					float py = -halfSize + size * (float(y) / (ny-1));

					mesh.vertices.emplace_back(px, 0.0f, py);
					mesh.uvs.emplace_back(float(x) / (nx-1), float(y) / (ny-1));
					mesh.normals.emplace_back(0.0f, 1.0f, 0.0f);

					mesh.vertices.emplace_back(px, 0.0f, py);
					mesh.uvs.emplace_back(float(x) / (nx-1), float(y) / (ny-1));
					mesh.normals.emplace_back(0.0f, 1.0f, 0.0f);

					mesh.vertices.emplace_back(px, 0.0f, py);
					mesh.uvs.emplace_back(float(x) / (nx-1), float(y) / (ny-1));
					mesh.normals.emplace_back(0.0f, 1.0f, 0.0f);

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
					//mesh.triangles.push_back(i0); mesh.triangles.push_back(i2); mesh.triangles.push_back(i1);
					//mesh.triangles.push_back(i1); mesh.triangles.push_back(i2); mesh.triangles.push_back(i3);
				}
			}
			return mesh;
		}
	};

} // namespace WaterSimulation

