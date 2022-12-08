#ifndef __TRACE_CL
#define __TRACE_CL

#include "src/constants.h"
#include "src/common.h"

Settings settings;

#include "src/cl/util.cl"
#include "src/cl/primitives.cl"
#include "src/cl/ray.cl"
#include "src/cl/light.cl"
#include "src/cl/camera.cl"
#include "src/cl/skydome.cl"


bool trace( Ray* ray )
{
	for ( int i = 0; i < settings.numPrimitives; i++ )
		intersect( i, primitives + i, ray );
	if ( ray->primIdx == -1 ) return false;
	intersectionPoint( ray );
	ray->N = getNormal( primitives + ray->primIdx, ray->I );
	if ( ray->inside ) ray->N = -ray->N;
	return true;
}

float4 shootWhitted( Ray* primaryRay )
{
	float4 color = (float4)(0);

	Ray stack[1024];
	stack[0] = *primaryRay;
	int n = 1;
	while ( n > 0 )
	{
		Ray ray = stack[--n];
		if ( !trace( &ray ) ) color += ray.intensity * readSkydome(ray.D);

		Primitive prim = primitives[ray.primIdx];
		Material mat = materials[prim.matIdx];

		if ( !mat.isDieletric )
		{
			if ( mat.specular < 1 )
			{
				// find the diffuse of this object
				for ( int i = 0; i < settings.numLights; i++ )
					color += handleShadowRay( &ray, lights + i ) * getAlbedo( &ray ) * ray.intensity;
			}
			if ( mat.specular > 0 && ray.bounces < MAX_BOUNCE )
			{
				// shoot another ray into the scene from the point of impact
				Ray reflectRay = reflect( &ray );
				reflectRay.intensity *= mat.specular;
				stack[n++] = reflectRay;
			}
		}
		else if ( ray.bounces < MAX_BOUNCE )
		{
			Ray reflectRay = reflect( &ray );
			float4 T = (float4)(0);
			float Fr = fresnel( &ray, &mat, &T );
			if ( Fr < 1.f )
			{
				// total internal reflection
				// shoot another ray into the scene from the point of impact
				reflectRay.intensity *= Fr;
				Ray transmissionRay = transmit( &ray, T );
				transmissionRay.intensity *= (1 - Fr);
				stack[n++] = transmissionRay;
			}
			stack[n++] = reflectRay;
		}
	}
	return color;
}

float4 shootKajiya( Ray* camray, uint* seed )
{
	Ray* ray = camray;

	float4 mask = (float4)(1);

	for ( int i = -1; i < MAX_BOUNCE; i++ )
	{
		// We did not hit anything, fall back to the skydome
		if ( !trace( ray ) ) return mask * readSkydome( ray->D );

		// we hit an object
		Primitive prim = primitives[ray->primIdx];
		Material mat = materials[prim.matIdx];

		if ( mat.isLight ) return mask * mat.emittance; // return mat.emittance * ray->intensity;

		Ray r;
		float rand = randomFloat( seed );

		if ( mat.isDieletric )
		{
			float4 T = (float4)(0);
			float Fr = fresnel( ray, &mat, &T );
			if ( rand < Fr )
			{
				// reflection
				r = reflect( ray );
			}
			else
			{
				// refraction
				// multiply mask with ray intensity cuz of beers law
				mask *= ray->intensity;
				r = transmit( ray, T );
			}
		}
		else
		{
			if ( rand < mat.specular )
			{
				// reflection
				r = reflect( ray );
			}
			else
			{
				// diffuse 
				float4 diffuseReflection = randomRayHemisphere( ray->N, seed );
				mask *= getAlbedo( ray ) * dot( diffuseReflection, ray->N );
				r = initRay( ray->I + EPSILON * diffuseReflection, diffuseReflection );
			}
		}
		ray = &r;
	}
	return (float4)(0);
}

__kernel void render( __global float4* pixels,
	__global float4* _textures,
	__global Material* _materials,
	__global Primitive* _primitives,
	__global Light* _lights,
	__global uint* seeds,
	Camera cam,
	Settings _settings )
{
	int idx = get_global_id( 0 );
	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;

	settings = _settings;
	
	materials = _materials;
	primitives = _primitives;
	lights = _lights;
	textures = _textures;

	uint* seed = seeds + idx;

	// create and shoot a ray into the scene
	Ray ray = initPrimaryRay( x, y, &cam, seed );

	if (settings.tracerType == KAJIYA)
	{
		float4 color = shootKajiya(&ray, seed);
		pixels[idx] = (pixels[idx] * (settings.frames - 1) + color) * (1 / (float)settings.frames);
	} else if(settings.tracerType == WHITTED)
	{
		float4 color = shootWhitted( &ray );
		pixels[idx] = color;
	}
}

__kernel void reset( __global float4* pixels )
{
	pixels[get_global_id( 0 )] = (float4)(0);
}

#endif // __TRACE_CL