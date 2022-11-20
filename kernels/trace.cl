#define SCRWIDTH 1280
#define SCRHEIGHT 720

int nPrimitives = 0;
int nLights = 0;
float epsilon = 0.0001f;

#include "kernels/ray.cl"
#include "kernels/primitives.cl"
#include "kernels/light.cl"
#include "kernels/camera.cl"

__kernel void trace(__global uint* pixels,
					__global read_only Sphere* spheres,
					__global read_only Plane* planes,
					//__global read_only Cube* cubes,
					__global read_only Material* materials,
					__global read_only Primitive* primitives,
					__global read_only Light* lights,
					read_only Camera cam,
					int numPrimitives,
					int numLights)
{
	int idx = get_global_id(0);
	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;

	nPrimitives = numPrimitives;
	nLights = numLights;

	Ray ray = initPrimaryRay(x, y, &cam);
	for(int i = 0; i < nPrimitives; i++)
	{
		intersect(i, primitives + i, &ray, spheres, planes);
	}

	if (ray.objIdx != -1)
	{
		float3 I = intersectionPoint(&ray);
		ray.N = getNormal(primitives + ray.objIdx, I, spheres, planes);

		float3 color = (float3)(0);
		for(int i = 0; i < nLights; i++)
		{
			color += handleShadowRay(&ray, lights + i, primitives, spheres, planes);//* materials[primitives[ray.objIdx].matIdx].colour;
		}
		//printf("%f, %f, %f\n", color.x, color.y, color.z);
		// color = (ray.N + (float3)(1)) * 127;
		color = min(color, (float3)(1));
		color *= 255;
		pixels[idx] = ((uint)color.x << 16) + ((uint)color.y << 8) + ((uint)color.z);
	}
	else
	{
		pixels[idx] = 0;
	}
	
	//if(idx == 0)
	//	printf("%i\n", sizeof(Ray));

}