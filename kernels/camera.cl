typedef struct Camera
{
	float3 origin, horizontal, vertical, topLeft;
} Camera;

Ray initPrimaryRay( int x, int y, Camera* cam )
{
	// u, v [0, 1]
	float u = (float)x * (1.0f / SCRWIDTH);
	float v = (float)y * (1.0f / SCRHEIGHT);
	float3 P = cam->topLeft + u * cam->horizontal + v * cam->vertical;
	return initRay( cam->origin, normalize( P - cam->origin ) );
}