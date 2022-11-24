#pragma once

// default screen resolution
#define SCRWIDTH	1280
#define SCRHEIGHT	720
// #define FULLSCREEN
// #define DOUBLESIZE

#define DEG_TO_RAD(deg) (deg * PI / 180.0)

namespace Tmpl8
{
	struct Camera
	{
		float4 origin, horizontal, vertical, topLeft;
	};

	enum class CamDir { Forward, Backwards, Left, Right };

	class CameraManager
	{
	private:
		float m_yaw;
		float m_pitch;
		float m_fov;
		float4 m_forward = float3( 0, 0, -1 );
		float3 m_right = float3( 1, 0, 0 );
		float3 m_up = float3( 0, 1, 0 );
		float4 m_screen_offset = float4(0, 0, focalLength, 0 );
		float m_speed = .5f;

	public:
		CameraManager( float vfov )
		{
			cam.origin = float3( 0, 0, 0 );
			Fov( vfov );
		}
		Camera cam;
		const float3 worldUp = float3( 0, 1, 0 );

		float viewportHeight;
		float viewportWidth;

		float aspect = (float)SCRWIDTH / (float)SCRHEIGHT;
		float focalLength = 1;
		float mouseSensivity = 0.5f;

		void Move( CamDir camdir, float deltaTime )
		{
			float velocity = m_speed;
			switch ( camdir )
			{
			case CamDir::Forward: {
				cam.origin -= m_forward * velocity;
			}break;
			case CamDir::Backwards: {
				cam.origin += m_forward * velocity;
			}break; 
			case CamDir::Left: {
				cam.origin -= m_right * velocity;
			}break; 
			case CamDir::Right: {
				cam.origin += m_right * velocity;
			}break;
			}
			UpdateCamVec( );
		}

		void MouseMove( float xOffset, float yOffset )
		{
			xOffset *= mouseSensivity;
			yOffset *= mouseSensivity;

			m_yaw = fmod( m_yaw + xOffset, 360.f );
			m_pitch += yOffset;
			m_pitch = clamp( m_pitch, -89.f, 89.f );

			float4 forward;
			float yaw = DEG_TO_RAD( m_yaw );
			float pitch = DEG_TO_RAD( m_pitch );
			forward.x = cos( yaw ) * cos( pitch );
			forward.y = sin( pitch );
			forward.z = sin( yaw ) * cos( pitch );
			m_forward = normalize( forward );
			UpdateCamVec( );
		}

		void Fov( float offset )
		{
			m_fov += offset;
			auto theta = m_fov * PI / 180;
			auto h = tan( theta / 2 );

			viewportHeight = 2 * h;
			viewportWidth = aspect * viewportHeight;

			UpdateCamVec( );
		}

		void UpdateCamVec( )
		{
			m_right = normalize( cross( worldUp, m_forward ) );
			m_up = normalize( cross( m_right, m_forward ) );

			cam.horizontal = viewportWidth * m_right;
			cam.vertical = viewportHeight * m_up;
			cam.topLeft = cam.origin - cam.horizontal / 2 - cam.vertical / 2 - m_forward * focalLength;
		}
	};
}