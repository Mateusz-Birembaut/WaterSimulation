#pragma once

#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>


namespace WaterSimulation
{
	class Camera
	{

	public:

		Camera( const Magnum::Vector2i & windowSize, Magnum::Deg fov = Magnum::Deg(90.0f), float near = 0.1f, float far = 1000.0f, float speed = 1.0f , float rotationSpeed = 1.0f )
		:   m_near{near}, 
			m_far{far}, 
			m_speed{speed}, 
			m_rotationSpeed{rotationSpeed}, 
			m_fov {fov}, 
			m_windowSize{windowSize}
		{}

		Magnum::Matrix4 viewMatrix() {
			if(m_viewDirty){
				m_viewMatrix = Magnum::Matrix4::from(m_orientation.toMatrix(), m_position).inverted();
				m_viewDirty = false;
			}
			return m_viewMatrix;
		};

		Magnum::Matrix4 projectionMatrix() {
			if(m_projDirty){
				m_projectionMatrix = Magnum::Matrix4::perspectiveProjection(m_fov, aspectRatio(), m_near, m_far);
				m_projDirty = false;
			}
			return m_projectionMatrix;
		};

		void move(const Magnum::Vector3 & localDirection){
			//Magnum::Vector3 dir {localDirection};
			//if(!dir.isZero()) dir = dir.normalized();
			m_position += m_orientation.transformVector(localDirection);
			m_viewDirty = true;
		};

		void rotate(const Magnum::Vector2& delta) {
			if(delta.isZero()) return;

			Magnum::Quaternion yaw = Magnum::Quaternion::rotation(Magnum::Deg(delta.x()), Magnum::Vector3::yAxis());
			Magnum::Vector3 right = m_orientation.transformVector(Magnum::Vector3::xAxis());
			Magnum::Quaternion pitch = Magnum::Quaternion::rotation(Magnum::Deg(delta.y()), right);

			m_orientation = (yaw * pitch) * m_orientation;
			m_orientation = m_orientation.normalized();
			m_viewDirty = true;
		}

		float aspectRatio() const {
			return float(m_windowSize.x()) / float(m_windowSize.y());
		}

		void setWindowSize(const Magnum::Vector2i& windowSize) { 
			m_windowSize = windowSize; 
			m_projDirty = true;
		};

		void setFOV(const float fov){
			m_fov = Magnum::Deg(fov);
			m_projDirty = true;
		}
		Magnum::Deg FOV(){
			return m_fov;
		}

		void setNear(const float near){
			m_near = near;
			m_projDirty = true;
		}
		float near(){
			return m_near;
		}

		void setFar(const float far){
			m_far = far;
			m_projDirty = true;
		}
		float far(){
			return m_far;
		}

		Magnum::Vector3 position(){
			return m_position;
		}

		void setPos(Magnum::Vector3 pos){
			m_position = pos;
			m_viewDirty = true;
		}

		float speed(){
			return m_speed;
		}

		float rotSpeed(){
			return m_rotationSpeed;
		}

		Magnum::Vector3 direction() {
			return m_orientation.transformVector(Magnum::Vector3::zAxis(-1.0f)).normalized();
		}

	private:
		float m_near{0.1f};
		float m_far{1000.0f};

		float m_speed{1.0f};
		float m_rotationSpeed{1.0f};

        Magnum::Deg m_fov{90.0f};
        Magnum::Vector2i m_windowSize{};

        Magnum::Vector3 m_position{};
        Magnum::Quaternion m_orientation{};

        bool m_viewDirty{true};
        Magnum::Matrix4 m_viewMatrix{};

        bool m_projDirty{true};
        Magnum::Matrix4 m_projectionMatrix{};
	};

	
} // namespace Camera
