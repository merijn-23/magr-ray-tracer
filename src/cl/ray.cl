
Ray initRay( float4 O, float4 D )
{
	float4 norm = normalize( D );
	return ( Ray ) { O, norm, -norm, ( float4 )( 0 ), ( float4 )( 1 ), 1e34f, -1, 0, false };
}

Ray initRayNoNorm( float4 O, float4 D )
{
	return ( Ray ) { O, D, -D, ( float4 )( 0 ), ( float4 )( 1 ), 1e34f, -1, 0, false };
}

Ray reflect( Ray* ray )
{
	float4 reflected = ray->D - 2.f * ray->N * dot( ray->N, ray->D );
	float4 origin = ray->I + reflected * 2 * EPSILON;
	Ray reflectRay = initRayNoNorm( origin, reflected );
	reflectRay.intensity = ray->intensity;
	reflectRay.bounces = ray->bounces + 1;
	return reflectRay;
}

Ray transmit( Ray* ray, float4 T )
{
	float4 origin = ray->I + T * EPSILON;
	Ray tRay = initRayNoNorm( origin, T );
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
	// 22% chance of failure, v is outside of unit circle
	while ( length(v) > 1) v = randomFloat3() * 2 - 1;
	if(dot(v, N) < 0) v = -v;

	return normalize(v);
}

