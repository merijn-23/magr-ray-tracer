#ifndef __WAVEFRONT_CL
#define __WAVEFRONT_CL

#include "src/constants.h"
#include "src/common.h"
#include "src/cl/util.cl"
#include "src/cl/primitives.cl"
#include "src/cl/ray.cl"
#include "src/cl/camera.cl"
#include "src/cl/glass.cl"
#include "src/cl/skydome.cl"
#include "src/cl/shading.cl"

__kernel void generate(
	__global Ray* rays,
	__global Settings* settings,
	__global uint* seeds,
	Camera _camera
)
{
	__local Camera camera;

	// first thread of a warp can retrieve settings and camera into local memory, all other threads must wait
	if ( get_local_id( 0 ) == 0 )
		camera = _camera;
	work_group_barrier( CLK_LOCAL_MEM_FENCE );

	int idx = get_global_id( 0 );
	uint* seed = seeds + idx;

	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;

	Ray r = initPrimaryRay( x, y, camera, settings, seed );
	r.pixelIdx = idx;
	rays[idx] = r;
}

__kernel void extend(
	__global Ray* rays,
	__global Primitive* _primitives,
	__global BVHNode* bvhNode,
	__global uint* bvhIdx,
	__global float4* accum,
	__global Settings* settings
)
{
	int idx = get_global_id( 0 );

	if ( idx == 0 ) primitives = _primitives;
	work_group_barrier( CLK_GLOBAL_MEM_FENCE );

	Ray* ray = rays + idx;
#if 0
	for ( int i = 0; i < settings->numPrimitives; i++ )
		intersect( i, primitives + i, ray );
#else
	uint steps = intersectBVH( ray, bvhNode, bvhIdx );
#endif
	//if (steps > 0) printf( "%i\n", steps );
	accum[idx] = (float4)(steps / 32.f);
	if ( ray->primIdx == -1 ) return;
	//accum[idx] = (float4)(1);

	intersectionPoint( ray );
	ray->N = getNormal( primitives + ray->primIdx, ray->I );
	if ( ray->inside ) ray->N = -ray->N;
}

__kernel void shade(
	__global Ray* inputRays,
	__global Ray* extensionRays,
	__global Ray* shadowRays,
	__global Primitive* _primitives,
	__global float4* _textures,
	__global Material* _materials,
	__global Settings* settings,
	__global float4* accum,
	__global uint* seeds
)
{
	int idx = get_global_id( 0 );
	if ( idx == 0 )
	{
		primitives = _primitives;
		textures = _textures;
		materials = _materials;
	}
	work_group_barrier( CLK_GLOBAL_MEM_FENCE );

	uint* seed = seeds + idx;

	Ray* ray = inputRays + idx;

	// we did not hit anything, fall back to the skydome
	if ( ray->primIdx == -1 )
	{
		accum[ray->pixelIdx] += ray->intensity * readSkydome( ray->D );
		return;
	}

	Ray extensionRay = initRay( ( float4 )( 0 ), ( float4 )( 0 ) );
	extensionRay.bounces = MAX_BOUNCES + 1;

	float4 color = kajiyaShading( ray, &extensionRay, seed );
	accum[ray->pixelIdx] += color;

	if ( extensionRay.bounces <= MAX_BOUNCES )
	{
		// get atomic inc in settings->numOutRays and set extensionRay in _extensionRays on that idx 
		int extensionIdx = atomic_inc( &( settings->numOutRays ) );
		extensionRays[extensionIdx] = extensionRay;
	}
}

__kernel void reset( __global float4* pixels )
{
	pixels[get_global_id( 0 )] = ( float4 )( 0 );
}

#endif // __WAVEFRONT_CL