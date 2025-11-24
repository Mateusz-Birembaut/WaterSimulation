#pragma once

#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>


namespace WaterSimulation
{
	class OrthoognalCamera
	{

		public:

		private:
			float m_near{0.1f};
			float m_far{1000.0f};

			Magnum::Vector3 m_position{};
			Magnum::Quaternion m_orientation{};
	}

}