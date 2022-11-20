typedef struct Camera
{
	float3 pos, topLeft, topRight, botLeft;
} Camera;

Ray initPrimaryRay(int x, int y, Camera* cam)
{
		float u = (float)x * (1.0f / SCRWIDTH);
		float v = (float)y * (1.0f / SCRHEIGHT);
		float3 P = cam->topLeft + u * (cam->topRight - cam->topLeft) + v * (cam->botLeft - cam->topLeft);
		return initRay( cam->pos, normalize( P - cam->pos ) );
}