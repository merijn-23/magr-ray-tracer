#pragma once

// default screen resolution
#define SCRWIDTH	1280
#define SCRHEIGHT	720
// #define FULLSCREEN
// #define DOUBLESIZE

namespace Tmpl8 {

struct Camera 
{
	float4 pos, topLeft, topRight, botLeft;
};

class CameraManager
{
public:
	CameraManager()
	{
		// setup a basic view frustum
		cam = Camera{
		float3(0, 0, 3),
		float3(-aspect, 1, 0),
		float3(aspect, 1, 0),
		float3(-aspect, -1, 0) };
	}
	//Ray GetPrimaryRay( const int x, const int y )
	//{
	//	// calculate pixel position on virtual screen plane
		//const float u = (float)x * (1.0f / SCRWIDTH);
		//const float v = (float)y * (1.0f / SCRHEIGHT);
		//const float3 P = topLeft + u * (topRight - topLeft) + v * (bottomLeft - topLeft);
		//return Ray( camPos, normalize( P - camPos ) );
	//}
	float aspect = (float)SCRWIDTH / (float)SCRHEIGHT;
	Camera cam;
};
}