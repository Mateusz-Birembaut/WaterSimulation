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
	};

} // namespace WaterSimulation

