//#include "kernels/primitives.cl"
struct Ray
{
	float3 O, D, rD;
	float t;
	int objIdx;
	bool inside;
} Ray;

__kernel void trace(__global uint* pixels, __global struct Ray* rays)
{
	int idx = get_global_id(0);
	struct Ray* ray = &rays[idx];
	if (ray->t == 128)
	{
		ray->t = 1;
		pixels[idx] = (uint)128 << 8;
	}
	else
	{
		pixels[idx] = 0;
	}
	
	//if(idx == 0)
	//	printf("%i\n", sizeof(Ray));

}