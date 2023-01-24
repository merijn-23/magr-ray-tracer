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
#include "src/cl/bvh.cl"
#include "src/cl/tlas.cl"

__kernel void generate(
	__global Ray* rays,
	__global Settings* settings,
	__global uint* seeds,
	Camera _camera
)
{
	__local Camera camera;
	// first thread of a warp can retrieve settings and camera into local memory, all other threads must wait
	if (get_local_id( 0 ) == 0)
		camera = _camera;
	work_group_barrier( CLK_LOCAL_MEM_FENCE );

	int idx = get_global_id( 0 );
	uint* seed = seeds + idx;

	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;

	Ray r = initPrimaryRay( x, y, camera, settings, seed );
	r.lastSpecular = true;
	r.pixelIdx = idx;
	rays[idx] = r;

}

__kernel void extend(
	__global Ray* rays,
	__global Primitive* _primitives,
	__global TLASNode* tlasNodes,
	__global BLASNode* blasNodes,
#ifdef USE_BVH4
	__global BVHNode4* bvhNodes,
#endif
#ifdef USE_BVH2
	__global BVHNode2* bvhNodes,
#endif
	__global uint* primIdxs,
	__global float4* accum,
	__global Settings* settings
)
{
	// swap the atomics after an extend-shade cycle
	if (get_global_id( 0 ) == 0)
	{
		settings->numInRays = settings->numOutRays;
		settings->shadowRays = 0;
		primitives = _primitives;
	}
	work_group_barrier( CLK_GLOBAL_MEM_FENCE );
	
	// persistent thread
	while (true)
	{
		// stop when there are no more incoming extensionRays
		int idx = atomic_dec( &(settings->numInRays) ) - 1;
		if (idx < 0) break;

		Ray* ray = rays + idx;
		uint steps = intersectTLAS( ray, tlasNodes, blasNodes, bvhNodes, primIdxs );

		if (settings->renderBVH) accum[idx] = (float4)(steps / 32.f);
		if (ray->primIdx == -1) continue;
		intersectionPoint( ray );
		ray->N = getNormal( primitives + ray->primIdx, ray->I );
		if (ray->inside) ray->N = -ray->N;
	}
}

__kernel void shade(
	__global Ray* inputRays,
	__global Ray* extensionRays,
	__global ShadowRay* shadowRays,
	__global Primitive* _primitives,
	__global float4* _textures,
	__global Material* _materials,
	__global uint* _lights,
	__global Settings* settings,
	__global float4* accum,
	__global uint* seeds
)
{
	int global_idx = get_global_id( 0 );
	if (global_idx == 0)
	{
		primitives = _primitives;
		textures = _textures;
		materials = _materials;
		lights = _lights;

		settings->numInRays = settings->numOutRays;
		settings->numOutRays = 0;
	}
	work_group_barrier( CLK_GLOBAL_MEM_FENCE );
	uint* seed = seeds + global_idx;

	while (true)
	{
		int idx = atomic_dec( &(settings->numInRays) ) - 1;
		if (idx < 0) break;

		Ray* ray = inputRays + idx;
		// we did not hit anything, fall back to the skydome
		if (ray->primIdx == -1)
		{
			accum[ray->pixelIdx] += ray->intensity * readSkydome( ray->D );
			continue;
		}
		Ray extensionRay = initRay( (float4)(0), (float4)(0) );
		extensionRay.bounces = MAX_BOUNCES + 1;

		float4 color;
#ifdef SHADING_SIMPLE
		color = kajiyaShading( ray, &extensionRay, seed );
#endif
#ifdef SHADING_NEE
		ShadowRay shadowRay;
		shadowRay.pixelIdx = -1;
		color = neeShading( ray, &extensionRay, &shadowRay, settings, seed);
#endif
		accum[ray->pixelIdx] += color;
		if (extensionRay.bounces <= MAX_BOUNCES)
		{
			// get atomic inc in settings->numOutRays and set extensionRay in _extensionRays on that idx 
			int extensionIdx = atomic_inc( &(settings->numOutRays) ); // atomic_inc( &(settings->numOutRays) );
			extensionRays[extensionIdx] = extensionRay;
		}
#ifdef SHADING_NEE
		if(shadowRay.pixelIdx != -1)
		{
			int shadowIdx = atomic_inc(&(settings->shadowRays));
			//printf("raid shadow rays%i\n", shadowIdx);
			shadowRays[shadowIdx] = shadowRay;
		}
#endif
	}
}

__kernel void connect(
	__global ShadowRay* shadowRays,
	__global TLASNode* tlasNodes,
	__global BLASNode* blasNodes,
#ifdef USE_BVH4
	__global BVHNode4* bvhNodes,
#endif
#ifdef USE_BVH2
	__global BVHNode2* bvhNodes,
#endif
	__global uint* bvhIdxs,
	__global Primitive* _primitives,
	__global Material* _materials,
	__global Settings* settings,
	__global float4* accum
){
	if(get_global_id(0) == 0)
	{
		//printf("nr of shadow rays input %i\n", settings->shadowRays);
		primitives = _primitives;
		materials = _materials;
	}
	work_group_barrier( CLK_GLOBAL_MEM_FENCE );
	
	while (true)
	{
		int idx = atomic_dec( &(settings->shadowRays) ) - 1;
		if (idx < 0) break;
		ShadowRay shadowRay = shadowRays[idx];
		float4 dir = shadowRay.L - shadowRay.I;
		float dist = length(dir);
		float4 norm = dir / dist;

		Ray ray = initRay(shadowRay.I + norm * EPSILON, norm);
		ray.t = dist - 2 * EPSILON;

		intersectTLAS( &ray, tlasNodes, blasNodes, bvhNodes, bvhIdxs );		

		if(ray.t < dist - 2 * EPSILON) continue;
		//printf("ray %i connected\n", idx);

		// calculate 
		Primitive* prim = _primitives + shadowRay.lightIdx;
		float4 NL = getNormal(prim, shadowRay.L);
		float solidAngle = (fabs(dot(NL, - dir)) * getArea(prim)) * (1 / (dist * dist));

		float4 lightColor = _materials[prim->matIdx].emittance;
		float4 Ld = lightColor * solidAngle * shadowRay.BRDF * shadowRay.dotNL;
		accum[shadowRay.pixelIdx] += Ld * shadowRay.intensity;
	}
}

__kernel void reset( __global float4* pixels )
{
	pixels[get_global_id( 0 )] = (float4)(0);
}

#endif // __WAVEFRONT_CL