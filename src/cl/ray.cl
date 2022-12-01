
Ray initRay( float4 O, float4 D )
{
	float4 norm = normalize( D );
	return ( Ray ) { O, norm, -norm, ( float4 )( 0 ), ( float4 )( 1 ), 1e34f, -1, 0, false };
}

Ray initRayNoNorm( float4 O, float4 D )
{
	return ( Ray ) { O, D, -D, ( float4 )( 0 ), ( float4 )( 1 ), 1e34f, -1, 0, false };
}

Ray reflect( Ray* ray, float4 I )
{
	float4 reflected = ray->D - 2.f * ray->N * dot( ray->N, ray->D );
	float4 origin = I + reflected * 2 * EPSILON;
	Ray reflectRay = initRayNoNorm( origin, reflected );
	reflectRay.intensity = ray->intensity;
	reflectRay.bounces = ray->bounces + 1;
	return reflectRay;
}

Ray transmission( Ray* ray, float4 I, float4 T )
{
	float4 origin = I + T * EPSILON;
	Ray tRay = initRayNoNorm( origin, T );
	tRay.intensity = ray->intensity;
	tRay.bounces = ray->bounces + 1;
	return tRay;
}

float4 intersectionPoint( Ray* ray )
{
	return ray->O + ray->t * ray->D;
}

