#pragma once

// default screen resolution
#define SCRWIDTH 1280
#define SCRHEIGHT 720
// #define FULLSCREEN
// #define DOUBLESIZE

#define DEG_TO_RAD(deg) (deg * PI / 180.0)

namespace Tmpl8 {
struct Camera
{
    float4 origin, horizontal, vertical, topLeft;
};

enum class CamDir
{
    Forward,
    Backwards,
    Left,
    Right
};

class CameraManager
{
private:
    float yaw_;
    float pitch_;
    float fov_;
    float4 forward_ = float3(0, 0, -1);
    float3 right_ = float3(1, 0, 0);
    float3 up_ = float3(0, 1, 0);
    float speed_ = .1f;

public:
    CameraManager(float vfov = 110)
    {
        cam.origin = float3(0, 0, 0);
        Fov(vfov);
    }
    Camera cam;
    const float3 worldUp = float3(0, 1, 0);

    float viewportHeight;
    float viewportWidth;

    float aspect = (float)SCRWIDTH / (float)SCRHEIGHT;
    float focalLength = 1;
    float mouseSensivity = 0.5f;

    void Move(CamDir camdir, float deltaTime)
    {
        float velocity = speed_ * deltaTime;
        switch (camdir)
        {
            case CamDir::Forward:
                cam.origin -= forward_ * velocity;
            break;
            case CamDir::Backwards:
                cam.origin += forward_ * velocity;
            break;
            case CamDir::Left:
                cam.origin -= right_ * velocity;
            break;
            case CamDir::Right:
                cam.origin += right_ * velocity;
            break;
        }
    }

    void MouseMove(float xOffset, float yOffset)
    {
        xOffset *= mouseSensivity;
        yOffset *= mouseSensivity;

        yaw_ = fmod(yaw_ + xOffset, 360.f);
        pitch_ += yOffset;
        pitch_ = clamp(pitch_, -89.f, 89.f);

        float4 forward;
        float yaw = DEG_TO_RAD(yaw_);
        float pitch = DEG_TO_RAD(pitch_);
        forward.x = cos(yaw) * cos(pitch);
        forward.y = sin(pitch);
        forward.z = sin(yaw) * cos(pitch);
        forward_ = normalize(forward);
    }

    void Fov(float offset)
    {
        fov_ += offset;
        auto theta = fov_ * PI / 180;
        auto h = tan(theta / 2);

        viewportHeight = 2 * h;
        viewportWidth = aspect * viewportHeight;
    }

    void UpdateCamVec()
    {
        right_ = normalize(cross(worldUp, forward_));
        up_ = normalize(cross(right_, forward_));

        cam.horizontal = viewportWidth * right_;
        cam.vertical = viewportHeight * up_;
        cam.topLeft = cam.origin - cam.horizontal / 2 - cam.vertical / 2 -
                      forward_ * focalLength;
    }
};
} // namespace Tmpl8