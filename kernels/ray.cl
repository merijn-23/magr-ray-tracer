typedef struct Ray
{
	float3 O, D, rD, N, intensity;
	float t;
	// Index of primitive
	int primIdx, bounces;
	bool inside;
} Ray;

Ray initRay(float3 O, float3 D)
{
	float3 norm = normalize(D);
	Ray r = {O, norm, -norm, (float3)(0), (float3)(1), 1e34f, -1, 0, false};
	return r; 
}

Ray initRayNoNorm(float3 O, float3 D)
{
	Ray r = {O, D, -D, (float3)(0), (float3)(1), 1e34f, -1, 0, false};
	return r; 
}

Ray* reflect(Ray* ray, float3 I)
{
	float3 reflected = ray->D - 2.f * ray->N * dot(ray->N, ray->D);
	float3 origin = I + reflected * 2 * EPSILON;
	Ray reflectRay = initRayNoNorm(origin, reflected);
	reflectRay.intensity = ray->intensity;
	reflectRay.bounces = ray->bounces + 1;
	return &reflectRay;
}

Ray* transmission(Ray* ray, float3 I, float3 T)
{
	float3 origin = I + T * EPSILON;
	Ray tRay = initRayNoNorm(origin, T);
	tRay.intensity = ray->intensity;
	tRay.bounces = ray->bounces + 1;
	return &tRay;
}


float3 intersectionPoint(Ray* ray)
{
	return ray->O + ray->t * ray->D;
}

