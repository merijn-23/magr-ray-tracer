typedef struct Ray
{
	float3 O, D, rD, N;
	float t;
	// Index of primitive
	int objIdx;
	bool inside;
} Ray;

Ray initRay(float3 O, float3 D)
{
	float3 norm = normalize(D);
	Ray r = {O, norm, -norm, (float3)(0), 1e34f, -1, false};
	return r; 
}

float3 intersectionPoint(Ray* ray)
{
	return ray->O + ray->t * ray->D;
}