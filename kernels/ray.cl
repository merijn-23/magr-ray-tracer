typedef struct Ray
{
	float3 O, D, rD, N;
	float t;
	// Index of primitive
	int objIdx, bounces;
	bool inside;
} Ray;

Ray initRay(float3 O, float3 D)
{
	float3 norm = normalize(D);
	Ray r = {O, norm, -norm, (float3)(0), 1e34f, -1, 0, false};
	return r; 
}

void recycleRay(Ray* ray, float3 O, float3 D)
{
	float3 norm = normalize(D);
	ray->O = O;
	ray->D = norm;
	ray->rD = -norm;
	ray->N = (float3)(0);
	ray->t = 1e34f;
	ray->objIdx = -1;
	ray->bounces = 0;
	ray->inside = false;
}

float3 intersectionPoint(Ray* ray)
{
	return ray->O + ray->t * ray->D;
}

