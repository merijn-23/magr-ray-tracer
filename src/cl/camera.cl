typedef struct Camera
{
	float4 origin, horizontal, vertical, topLeft;
} Camera;

Ray initPrimaryRay( int x, int y, Camera* cam, uint* seed )
{
	// u, v [0, 1]
	float u = (float)x * (1.0f / SCRWIDTH);
	float v = (float)y * (1.0f / SCRHEIGHT);

	u += randomFloat( seed ) / (float)SCRWIDTH;
	v += randomFloat( seed ) / (float)SCRHEIGHT;

	float4 P = cam->topLeft + u * cam->horizontal + v * cam->vertical;
	return initRay( cam->origin, normalize( P - cam->origin ) );
}