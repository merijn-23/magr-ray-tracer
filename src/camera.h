#pragma once

#define DEG_TO_RAD PI / 180.0

namespace Tmpl8
{
	enum class CamDir
	{
		Forward,
		Backwards,
		Left,
		Right,
		Up,
		Down
	};

	class CameraManager
	{
	private:
		float yaw_;
		float pitch_;

	public:
		CameraManager(float vfov = 110, int type = PROJECTION)
		{
			cam.type = type;
			cam.origin = float3(0, .1f, .2f);
			cam.forward = float3(0, 0, 1);
			cam.right = float3(1, 0, 0);
			cam.up = float3(0, 1, 0);
			cam.aperture = 0;
			cam.focalLength = 1;
			Fov(vfov);
		}
		Camera cam;
		const float3 worldUp = float3(0, 1, 0);

		float viewportHeight;
		float viewportWidth;

		float aspect = (float)SCRWIDTH / (float)SCRHEIGHT;
		float mouseSensivity = 0.5f;
		float speed = .05f;

		bool moved = true;

		void Move(CamDir camdir, float deltaTime)
		{
			moved = true;

			float velocity = speed; // *deltaTime;
			switch (camdir)
			{
			case CamDir::Forward:
				cam.origin -= cam.forward * velocity;
				break;
			case CamDir::Backwards:
				cam.origin += cam.forward * velocity;
				break;
			case CamDir::Left:
				cam.origin -= cam.right * velocity;
				break;
			case CamDir::Right:
				cam.origin += cam.right * velocity;
				break;
			case CamDir::Up:
				cam.origin += cam.up * velocity;
				break;
			case CamDir::Down:
				cam.origin -= cam.up * velocity;
				break;
			}
		}

		void MouseMove(float xOffset, float yOffset)
		{
			moved = true;

			xOffset *= mouseSensivity;
			yOffset *= mouseSensivity;

			yaw_ = fmod(yaw_ + xOffset, 360.f);
			pitch_ += yOffset;
			pitch_ = clamp(pitch_, -89.f, 89.f);

			float4 forward;
			float yaw = yaw_ * DEG_TO_RAD;
			float pitch = pitch_ * DEG_TO_RAD;
			forward.x = cos(yaw) * cos(pitch);
			forward.y = sin(pitch);
			forward.z = sin(yaw) * cos(pitch);
			cam.forward = normalize(forward);
		}

		void Zoom(float offset)
		{
			moved = true;
			Fov(cam.fov + offset);
		}

		void Fov(float vfov) 
		{
			cam.fov = vfov;
			auto theta = cam.fov * DEG_TO_RAD;
			auto h = tan(theta / 2);

			viewportHeight = 2 * h;
			viewportWidth = aspect * viewportHeight;
		}

		void UpdateCamVec()
		{
			Fov(cam.fov); // Needed for when fov is updated through imgui
			cam.right = normalize(cross(worldUp, cam.forward));
			cam.up = normalize(cross(cam.right, cam.forward));

			cam.horizontal = viewportWidth * cam.right;
			cam.vertical = viewportHeight * cam.up;
			cam.topLeft = cam.origin - cam.horizontal / 2 - cam.vertical / 2 -
				cam.forward;
		}
	};
} // namespace Tmpl8