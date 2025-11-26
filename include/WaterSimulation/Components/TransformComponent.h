#pragma once
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/Math/Constants.h>

namespace WaterSimulation
{
	struct TransformComponent
	{
		Magnum::Matrix4 globalModel{Magnum::Math::IdentityInit};
		Magnum::Matrix4 inverseGlobalModel{Magnum::Math::IdentityInit};

		Magnum::Quaternion rotation{Magnum::Math::IdentityInit};
		Magnum::Vector3 position{0.0f, 0.0f, 0.0f};
		Magnum::Vector3 scale{1.0f, 1.0f, 1.0f};

		TransformComponent(
			Magnum::Vector3 _position = {0.0f, 0.0f, 0.0f}, 
			Magnum::Quaternion _rotation = Magnum::Quaternion(Magnum::Math::IdentityInit), 
			Magnum::Vector3 _scale = {1.0f,1.0f,1.0f})
			
		: rotation{_rotation}, position{_position}, scale{_scale}
		{
		}

		Magnum::Matrix4 model() const {
			const Magnum::Matrix3 rot = rotation.toMatrix();
			const Magnum::Matrix3 rotScale = rot * Magnum::Matrix3::fromDiagonal(scale);
			return Magnum::Matrix4::from(rotScale, position);
		}

		void computeGlobalModelMatrix(const Magnum::Matrix4& parentGlobal) {
			globalModel = parentGlobal * model();
		}

		Magnum::Vector3 forward() const {
			return rotation.transformVector(-Magnum::Vector3::zAxis());
		}

		Magnum::Vector3 target() const {
			return position + forward();
		}

		Magnum::Vector3 up() const {
			return rotation.transformVector(Magnum::Vector3::yAxis());
		}

		Magnum::Vector3 right() const {
			return rotation.transformVector(Magnum::Vector3::xAxis());
		}

		void lookAt(const Magnum::Vector3& target, const Magnum::Vector3& up = Magnum::Vector3::yAxis()) {
			Magnum::Vector3 direction = target - position;
			
			if(!direction.isZero()) {
				direction = direction.normalized();
				Magnum::Matrix4 lookAtMatrix = Magnum::Matrix4::lookAt(position, target, up);
				rotation = Magnum::Quaternion::fromMatrix(lookAtMatrix.rotationScaling().inverted());
			}
		}

					// Définit la rotation à partir de target, up et right
		void lookAt2(const Magnum::Vector3& target, const Magnum::Vector3& up, const Magnum::Vector3& right) {
			Magnum::Vector3 forward = (target - position).normalized();
			Magnum::Vector3 upOrtho = Magnum::Math::cross(right, forward).normalized();
			Magnum::Vector3 rightOrtho = Magnum::Math::cross(forward, upOrtho).normalized();
			Magnum::Matrix3 rotMat{
				rightOrtho,
				upOrtho,
				-forward
			};
			rotation = Magnum::Quaternion::fromMatrix(rotMat);
		}

		void onAttach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};
    	void onDetach(Registry & registry [[maybe_unused]], Entity entity [[maybe_unused]]){};

	};
		
} // namespace WaterSimulation
