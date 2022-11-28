#include "kernels/constants.cl"

__kernel void display(__global float3* pixels,
	write_only image2d_t target)
{
	int idx = get_global_id( 0 );
	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;

	float3 color = pixels[idx];
	write_imagef(target, (int2)(x, y), (float4)(color, 1));
}

__kernel void vignetting( __global float3* pixels, float strength)
{
	int idx = get_global_id( 0 );
	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;

	float3 color = pixels[idx];
	float2 pos = (float2)(x / (float)SCRWIDTH - 0.5, y / (float)SCRHEIGHT - 0.5);
	float vignette = 1 - smoothstep( 0, 1, length( pos ) );
	//printf( "%f\n", vignette );
	color = mix( color, color * vignette, strength );

	pixels[idx] = color;
}