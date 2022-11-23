typedef struct Camera
{
	float3 origin, horizontal, vertical, topLeft;
} Camera;

Ray initPrimaryRay( int i, int j, Camera* cam )
{
	// u, v [0, 1]
	float u = (float)i * (1.0f / SCRWIDTH);
	float v = (float)j * (1.0f / SCRHEIGHT);
	float3 P = cam->topLeft + u * cam->horizontal + v * cam->vertical;
	// float3 P = cam->topLeft + u * (cam->topRight - cam->topLeft) + v * (cam->botLeft - cam->topLeft);
	return initRay( cam->origin, normalize( P - cam->origin ) );
}