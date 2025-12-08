#pragma once

#include <WaterSimulation/ECS.h>
#include <WaterSimulation/Mesh.h>

#include <Magnum/Math/Vector3.h>

#include <vector>

namespace WaterSimulation
{	
	struct BuoyancyComponent
	{
		
		std::vector<Magnum::Vector3> localTestPoints;
		float flotability{};
		float waterDrag = 1.0f; 
		float angularDrag = 0.5f;

        //float flowInfluence = 0.0f; influence de swe sur object ?

		void onAttach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
    	void onDetach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};


        void createTestPointsFromMesh(const Mesh& mesh)
        {
            localTestPoints.clear();

			auto [min, max] = WaterSimulation::Mesh::computeAABBFromMesh(mesh);
			
			localTestPoints.push_back({min.x(), min.y(), min.z()});
			localTestPoints.push_back({max.x(), min.y(), min.z()}); 
			localTestPoints.push_back({max.x(), min.y(), max.z()}); 
			localTestPoints.push_back({min.x(), min.y(), max.z()}); 
			localTestPoints.push_back({min.x(), max.y(), min.z()}); 
			localTestPoints.push_back({max.x(), max.y(), min.z()}); 
			localTestPoints.push_back({max.x(), max.y(), max.z()}); 
			localTestPoints.push_back({min.x(), max.y(), max.z()}); 

			Magnum::Vector3 center = (min + max) * 0.5f;
			localTestPoints.push_back(center);
        }

	};
	
} // namespace WaterSimulation
