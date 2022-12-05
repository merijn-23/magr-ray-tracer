#include "src/constants.h"

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

__kernel void gamma_corr( __global float3* source,
	__global float3* dest,
	float gamma)
{
	int idx = get_global_id( 0 );
	dest[idx] = pow(source[idx], gamma);
}

__kernel void prep(__global float3* pixels, __global float3* swap)
{
	int idx = get_global_id(0);
	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;
	
	float3 color = pixels[get_global_id(0)];
	swap[idx] = pixels[get_global_id(0)];
}