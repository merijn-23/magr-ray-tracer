#ifndef __POSTPROC_CL
#define __POSTPROC_CL

#include "src/constants.h"
#include "src/common.h"

__kernel void display(__global float3* source,
	write_only image2d_t target)
{
	int idx = get_global_id( 0 );
	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;

	float4 color = (float4)(source[idx], 1);
	write_imagef(target, (int2)(x, y), color);
}

__kernel void vignetting( __global float3* source,
	__global float3* dest,
	float strength)
{
	int idx = get_global_id( 0 );
	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;

	float3 color = source[idx]; //read_imagef(source, (x,y)).xyz;
	float2 pos = (float2)(x / (float)SCRWIDTH - 0.5, y / (float)SCRHEIGHT - 0.5);
	float vignette = 1 - smoothstep( 0, 1, length( pos ) );
	color = mix( color, color * vignette, strength );

	dest[idx] = color; //write_imagef(dest, (x,y), (float4)(color, 1));
	}

__kernel void gammaCorr( __global float3* source,
	__global float3* dest,
	float gamma)
{
	int idx = get_global_id( 0 );
	dest[idx] = pow(source[idx], gamma);
}

__kernel void chromatic( __global float3* source,
	__global float3* dest,
	float offset )
{
	int idx = get_global_id( 0 );
	
	// skip the first column
	if (idx % SCRWIDTH == 0) 
	{
		dest[idx] = source[idx];
		return; 
	}

	float3 cur = source[idx];
	float3 prev = source[idx - 1];
	// linear interpolation based on index
	float r = cur.x;
	float g = cur.y * (1 - offset) + prev.y * offset;
	float b = cur.z * (1 - 2 * offset) + prev.z * 2 * offset;

	dest[idx] = (float3)(r, g, b);
}

__kernel void prep(__global float3* pixels, __global float3* swap, __global Settings* settings)
{
	int idx = get_global_id(0);
	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;

	float3 color = pixels[get_global_id(0)] * (1 / (float)(settings->frames));
	color = min( color, (float3)(1) );

	swap[idx] = color;
}

__kernel void saveImage(read_only image2d_t src, __global float4* dst)
{
	int idx = get_global_id( 0 );
	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;

	float4 color = read_imagef(src, (int2)(x, y));
	color = min( color, (float4)(1) );
	dst[idx] = color;
}

#endif // __POSTPROC_CL