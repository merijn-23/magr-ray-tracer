int nPrimitives = 0;
int nLights = 0;
//uint* seed;

uint randomUInt( uint* seed )
{
	*seed ^= *seed << 13;
	*seed ^= *seed >> 17;
	*seed ^= *seed << 5;
	return *seed;
}
float randomFloat( uint* seed) { return randomUInt( seed ) * 2.3283064365387e-10f; }
float4 randomFloat3( uint* seed ) { return ( float4 )( randomFloat( seed ), randomFloat( seed ), randomFloat( seed ), 0 ); };

#include "src/constants.h"
#include "src/common.h"
#include "src/cl/primitives.cl"
#include "src/cl/ray.cl"
#include "src/cl/light.cl"
#include "src/cl/camera.cl"

void beersLaw( Ray* ray )
{
	float4 a = ( float4 )( 1.f );
	ray->intensity.x *= exp( -a.x * ray->t );
	ray->intensity.y *= exp( -a.y * ray->t );
	ray->intensity.z *= exp( -a.z * ray->t );
}

bool fresnel( Ray* ray, Material* mat, float* outFr, float4* outT )
{
	float costhetai = dot( ray->N, ray->rD );

	float n1 = mat->n1;
	float n2 = mat->n2;
	if ( ray->inside )
	{
		n1 = mat->n2;
		n2 = mat->n1;

		// give material absorption
		beersLaw( ray );
	}

	float frac = n1 * ( 1 / n2 );
	float k = 1 - frac * frac * ( 1 - costhetai * costhetai );

	if ( k < 0 ) return false;

	// use fresnel's law to find reflection and refraction factors
	( *outT ) = normalize(
		frac * ray->D + ray->N * ( frac * costhetai - sqrt( k ) ) 
	);
	float costhetat = dot( -( ray->N ), ( *outT ) );
	// precompute
	float n1costhetai = n1 * costhetai;
	float n2costhetai = n2 * costhetai;
	float n1costhetat = n1 * costhetat;
	float n2costhetat = n2 * costhetat;

	float frac1 = ( n1costhetai - n2costhetat ) / ( n1costhetai + n2costhetat );
	float frac2 = ( n1costhetat - n2costhetai ) / ( n1costhetat + n2costhetai );

	// calculate fresnel
	( *outFr ) = 0.5f * ( frac1 * frac1 + frac2 * frac2 );
	return true;
}

bool trace(Ray* ray)
{
	for ( int i = 0; i < nPrimitives; i++ )
		intersect( i, primitives + i, ray );
	if ( ray->primIdx == -1 ) return false;
	intersectionPoint(ray);
	ray->N = getNormal( primitives + ray->primIdx, ray->I );
	if ( ray->inside ) ray->N = -ray->N;
	return true;
}

float4 shootWhitted( Ray* primaryRay )
{
	float4 color = ( float4 )( 0 );

	Ray stack[1024];
	stack[0] = *primaryRay;
	int n = 1;
	while ( n > 0 )
	{
		Ray ray = stack[--n];
		if ( !trace( &ray ) ) continue;

		Primitive prim = primitives[ray.primIdx];
		Material mat = materials[prim.matIdx];

		if ( !mat.isDieletric )
		{
			if ( mat.specular < 1 )
			{
				// find the diffuse of this object
				for ( int i = 0; i < nLights; i++ )
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
			float Fr = 0.f;
			float4 T = ( float4 )( 0 );
			if ( fresnel( &ray, &mat, &Fr, &T ) )
			{
				// total internal reflection
				// shoot another ray into the scene from the point of impact
				reflectRay.intensity *= Fr;
				Ray transmissionRay = transmit( &ray, T );
				transmissionRay.intensity *= ( 1 - Fr );
				stack[n++] = transmissionRay;
			}
			stack[n++] = reflectRay;
		}
	}
	return color;
}

float4 shootKajiya( Ray* ray, uint* seed )
{
	for(int i = 0; i < MAX_BOUNCE; i++)
	{
		if ( !trace( ray ) ) return (float4)(0);
		
		// we hit an object
		Primitive prim = primitives[ray->primIdx];
		Material mat = materials[prim.matIdx];

		if(mat.isLight) return mat.emittance * ray->intensity;
		
		float4 diffuseReflection = randomRayHemisphere(ray->N, seed);
		Ray r = initRay(ray->I + EPSILON * diffuseReflection, diffuseReflection);
		r.intensity = ray->intensity;

		// * pi / pi ???
		float4 brdf = getAlbedo(ray) * M_1_PI_F;
		r.intensity *= 2.0f * brdf * M_PI_F * dot(ray->N, diffuseReflection);
		ray = &r;
	}	
	return (float4)(0);
}

__kernel void render( __global float4* pixels,
	__global float4* _textures,
	__global Sphere* _spheres,
	__global Triangle* _triangles,
	__global Plane* _planes,
	__global Material* _materials,
	__global Primitive* _primitives,
	__global Light* _lights,
	__global uint* seeds,
	Camera cam,
	int numPrimitives,
	int numLights,
	int frames)
{
	int idx = get_global_id( 0 );
	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;

	nPrimitives = numPrimitives;
	nLights = numLights;

	spheres = _spheres;
	planes = _planes;
	triangles = _triangles;
	materials = _materials;
	primitives = _primitives;
	lights = _lights;
	textures = _textures;

	// create and shoot a ray into the scene
	Ray ray = initPrimaryRay( x, y, &cam );
	//float4 color = shootWhitted( &ray );
	float4 color = shootKajiya( &ray, seeds + idx );

	// prevent color overflow
	color = min( color, ( float4 )( 1 ) );
	//pixels[idx] = color;
	pixels[idx] = (pixels[idx] * (frames - 1) + color) * (1 / (float)frames);
}

__kernel void reset(__global float4* pixels)
{
	pixels[get_global_id(0)] = (float4)( 0 );
}