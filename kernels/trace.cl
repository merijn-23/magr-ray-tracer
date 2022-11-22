#define SCRWIDTH 1280
#define SCRHEIGHT 720

#define MAX_BOUNCE 3

int nPrimitives = 0;
int nLights = 0;
float epsilon = 0.001f;

#include "kernels/ray.cl"
#include "kernels/primitives.cl"
#include "kernels/light.cl"
#include "kernels/camera.cl"

Ray stack[1024];

float3 shoot(Ray* ray)
{
	float3 color = (float3)(0);

	for(int i = 0; i < nPrimitives; i++)
		intersect(i, primitives + i, ray);
	
	if(ray->objIdx != -1)
	{
		float3 I = intersectionPoint(ray);
		ray->N = getNormal(primitives + ray->objIdx, I);

		// we hit an object
		Material mat = materials[primitives[ray->objIdx].matIdx];
		if(mat.reflect < 1)
		{
			// find the diffuse of this object
			for(int i = 0; i < nLights; i++)
			{
				color += handleShadowRay(ray, lights + i) * mat.colour;// * (1 - mat.reflect);
			}
		}
		if(mat.reflect > 0 && ray->bounces < MAX_BOUNCE)
		{
			// shoot another ray into the scene from the point of impact
			Ray* reflectRay = reflect(ray, I);
			color += shoot(reflectRay) * mat.reflect;
		}
	}
	return color;
}

__kernel void trace(__global uint* pixels,
					__global read_only Sphere* _spheres,
					__global read_only Plane* _planes,
					//__global read_only Cube* cubes,
					__global read_only Material* _materials,
					__global read_only Primitive* _primitives,
					__global read_only Light* _lights,
					read_only Camera cam,
					int numPrimitives,
					int numLights)
{
	int idx = get_global_id(0);
	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;

	nPrimitives = numPrimitives;
	nLights = numLights;

	spheres = _spheres;
	planes = _planes;
	materials = _materials;
	primitives = _primitives;
	lights = _lights;

	// create and shoot a ray into the scene
	Ray ray = initPrimaryRay(x, y, &cam);
	float3 color = shoot(&ray);

	// prevent overflow of the colors
	color = min(color, (float3)(1));
	color *= 255;
	pixels[idx] = ((uint)color.x << 16) + ((uint)color.y << 8) + ((uint)color.z);

}