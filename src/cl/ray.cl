
Ray initRay( float4 O, float4 D )
{
	Ray ray;
	ray.O = O;
	ray.D = D;
	ray.rD = -D;
	ray.N = (float4)(0);
	ray.I = (float4)(0);
	ray.intensity = (float4)(1);
	ray.t = 1e34f;
	ray.primIdx = -1;
	ray.bounces = 0;
	ray.inside = false;
	return ray;
}


Ray reflect( Ray* ray )
{
	float4 reflected = ray->D - 2.f * ray->N * dot( ray->N, ray->D );
	float4 origin = ray->I + reflected * 2 * EPSILON;
	Ray reflectRay = initRay( origin, reflected );
	reflectRay.intensity = ray->intensity;
	reflectRay.bounces = ray->bounces + 1;
	return reflectRay;
}

Ray transmit( Ray* ray, float4 T )
{
	float4 origin = ray->I + T * EPSILON;
	Ray tRay = initRay( origin, T );
	tRay.intensity = ray->intensity;
	tRay.bounces = ray->bounces + 1;
	return tRay;
}

void intersectionPoint( Ray* ray )
{
	ray->I = ray->O + ray->t * ray->D;
}

float4 randomRayHemisphere( float4 N )
{
	float4 v = randomFloat3() * 2 - 1;
	//printf("%f, %f, %f\n", v.x, v.y, v.z);
	// 22% chance of failure, v is outside of unit circle
	while ( v.x * v.x + v.y * v.y + v.z * v.z > 1) v = randomFloat3() * 2 - 1;
	if(dot(v, N) < 0) v = -v;

	return normalize(v);
}

