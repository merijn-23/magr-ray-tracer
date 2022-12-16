#ifndef __RAY_CL
#define __RAY_CL

Ray initRay( float4 O, float4 D, bool lastSpecular = false )
{
	Ray ray;
	ray.O = O;
	ray.D = D;
	ray.rD = 1 / D;
	ray.N = (float4)(0);
	ray.I = (float4)(0);
	ray.intensity = (float4)(1);
	ray.t = 1e34f;
	ray.primIdx = -1;
	ray.bounces = 0;
	ray.inside = false;
	ray.lastSpecular = lastSpecular;
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
	tRay.inside = !ray->inside;
	return tRay;
}

void intersectionPoint( Ray* ray )
{
	ray->I = ray->O + ray->t * ray->D;
}

float4 randomRayHemisphere( float4 N, uint* seed )
{
	float rand1 = 2.f * M_PI_F * randomFloat( seed );
	float rand2 = randomFloat( seed );
	float rand2s = sqrt( rand2 );

	float3 w = N.xyz;
	float3 axis = fabs( w.x ) > 0.1f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
	float3 u = normalize( cross( axis, w ) );
	float3 v = cross( w, u );
	return (float4)(normalize( u * cos( rand1 ) * rand2s + v * sin( rand1 ) * rand2s + w * sqrt( 1.0f - rand2 ) ), 0);
}

#endif // __RAY_CL