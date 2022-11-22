typedef struct Primitive
{
	int objType;
	// Index of object in correct list
	int objIdx;
	int matIdx;
} Primitive;
__global Primitive* primitives;


typedef struct Material
{
	float3 colour;
	float reflect, refract;
} Material;
__global Material* materials;


typedef struct Sphere
{
	float3 pos;
	float r2, invr;
} Sphere;
__global Sphere* spheres;


typedef struct Plane
{
	float3 N;
	float d;
} Plane;
__global Plane* planes;


// typedef struct Cube
// {
// 	float4 b[2];
// 	float4x4 M, invM;
// } Cube;

void intersectSphere( int primIdx, Primitive* prim, Sphere* sphere, Ray* ray )
{
	float3 oc = ray->O - sphere->pos;
	float b = dot(oc, ray->D);
	float c = dot(oc, oc) - sphere->r2;
	float t, d = b * b - c;
	if (d <= 0) return;
	d = sqrt(d), t = -b - d;
	if (t < ray->t && t > 0)
	{
		ray->t = t, ray->objIdx = primIdx;
		return;
	}
	t = d - b;
	if (t < ray->t && t > 0)
	{
		ray->t = t, ray->objIdx = primIdx;
		return;
	}
}

void intersectPlane( int primIdx, Primitive* prim, Plane* plane, Ray* ray )
{
		float t = -(dot( ray->O, plane->N ) + plane->d) / (dot( ray->D, plane->N ));
		if (t < ray->t && t > 0)
		{
			ray->t = t, ray->objIdx = primIdx;
		} 
}

void intersect( int primIdx, Primitive* prim, Ray* ray )
{
	switch(prim->objType){
		case 0:
			intersectSphere(primIdx, prim, spheres + prim->objIdx, ray);
			break;
		case 1:
			intersectPlane(primIdx, prim, planes + prim->objIdx, ray);
	}
}


float3 getSphereNormal( Sphere* sphere, float3 I ) 
{
	return (I - sphere->pos) * sphere->invr;
}

float3 getPlaneNormal( Plane* plane, float3 I )
{
	return plane->N;
}

float3 getNormal( Primitive* prim, float3 I )
{
	switch(prim->objType)
	{
		case 0:
			return getSphereNormal(spheres + prim->objIdx, I);
		case 1:
			return getPlaneNormal(planes + prim->objIdx, I);
	}
}

