#include <WaterSimulation/Mesh.h>

#include <external/OBJ_Loader.h>

#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Vector2.h>
#include <Corrade/Utility/Debug.h>


void WaterSimulation::loadOBJ(const char* fileName, std::vector<Magnum::Vector3>& vertices, std::vector<Magnum::Vector3>& normals,
				std::vector<Magnum::Vector2>& uvs, std::vector<unsigned int>& triangles)
{
    vertices.clear();
    normals.clear();
    uvs.clear();
    triangles.clear();


    objl::Loader OBJLoader;
    if (OBJLoader.LoadFile(fileName))
    {


        int vertexOffset = 0;

        for( objl::Mesh& mesh : OBJLoader.LoadedMeshes){

            for(objl::Vertex& vertex : mesh.Vertices){

                vertices.emplace_back(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
                normals.emplace_back(vertex.Normal.X, vertex.Normal.Y, vertex.Normal.Z);
                uvs.emplace_back(vertex.TextureCoordinate.X, vertex.TextureCoordinate.Y);
            }
            int size {static_cast<int>(mesh.Indices.size())};
            for(int i = 0; i < size; i+= 3){

                triangles.emplace_back(mesh.Indices[i] + vertexOffset);
                triangles.emplace_back(mesh.Indices[i + 1] + vertexOffset);
                triangles.emplace_back(mesh.Indices[i + 2] + vertexOffset);

            }

            vertexOffset += mesh.Vertices.size();

            Corrade::Utility::Debug{} << "Successfully loaded OBJ file:" << fileName;   
        }
    }
    else
    {
        Corrade::Utility::Debug{} << "Failed to load OBJ file" << fileName;
    }
}


