typedef struct Primitive
{
	int objType;
	// Index of object in respective list
	int objIdx;
	int matIdx;
} Primitive;

typedef struct Material
{
	float3 color;
	float specular, n1, n2;
	bool isDieletric;
	int texIdx;
	int texW, texH;
} Material;

typedef struct Sphere
{
	float3 pos;
	float r2, invr;
} Sphere;

typedef struct Plane
{
	float3 N;
	float d;
} Plane;

typedef struct Triangle
{
	float3 v0, v1, v2;
	float3 N;
} Triangle;

__global Primitive* primitives;
__global Material* materials;
__global Sphere* spheres;
__global Plane* planes;
__global Triangle* triangles;
__global float3* textures;

// typedef struct Cube
// {
// 	float4 b[2];
// 	float4x4 M, invM;
// } Cube;

void intersectSphere( int primIdx, Primitive* prim, Sphere* sphere, Ray* ray )
{
	float3 oc = ray->O - sphere->pos;
	float b = dot( oc, ray->D );
	float c = dot( oc, oc ) - sphere->r2;
	float t, d = b * b - c;
	if ( d <= 0 ) return;
	d = sqrt( d ), t = -b - d;
	if ( t < ray->t && t > 0 )
	{
		ray->t = t, ray->primIdx = primIdx;
		return;
	}
	t = d - b;
	if ( t < ray->t && t > 0 )
	{
		ray->t = t, ray->primIdx = primIdx, ray->inside = true;
		return;
	}
}

void intersectPlane( int primIdx, Primitive* prim, Plane* plane, Ray* ray )
{
	float t = -(dot( ray->O, plane->N ) + plane->d) / (dot( ray->D, plane->N ));
	if ( t > ray->t || t < 0 ) return;

	ray->t = t, ray->primIdx = primIdx;
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
float dotEpsilon = 0.01;
void intersectTriangle( int primIdx, Primitive* prim, Triangle* tri, Ray* ray )
{
	float3 v0v1 = tri->v1 - tri->v0;
	float3 v0v2 = tri->v2 - tri->v0;
	float3 pvec = cross( ray->D, v0v2 );
	float det = dot( v0v1, pvec );
	// ray and triangle are parallel if det is close to 0
#ifdef ONE_SIDED
	if ( det < dotEpsilon ) return;
#else
	if ( fabs( det ) < dotEpsilon ) return;
#endif
	float invDet = 1 / det;
	float3 tvec = ray->O - tri->v0;
	float u = dot( tvec, pvec ) * invDet;
	if ( u < 0 || u > 1 ) return;

	float3 qvec = cross( tvec, v0v1 );
	float v = dot( ray->D, qvec ) * invDet;
	if ( v < 0 || u + v > 1 ) return;

	float t = dot( v0v2, qvec ) * invDet;
	if ( t > ray->t || t < 0 ) return;
	ray->t = t;
	ray->primIdx = primIdx;
	ray->inside = false;
}

void intersect( int primIdx, Primitive* prim, Ray* ray )
{
	switch ( prim->objType )
	{
	case SPHERE:
		intersectSphere( primIdx, prim, spheres + prim->objIdx, ray );
		break;
	case PLANE:
		intersectPlane( primIdx, prim, planes + prim->objIdx, ray );
		break;
	case TRIANGLE:
		intersectTriangle( primIdx, prim, triangles + prim->objIdx, ray );
		break;
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
	switch ( prim->objType )
	{
	case SPHERE:
		return getSphereNormal( spheres + prim->objIdx, I );
	case PLANE:
		return getPlaneNormal( planes + prim->objIdx, I );
	}
}

float2 getSphereUV( Sphere* sphere, float3 N )
{
	float2 tex;
	tex.x = (1 + atan2pi( N.z, N.x )) * 0.5;
	tex.y = acospi( N.y );
	return tex;
}

float3 getAlbedo( Ray* ray, float3 I )
{
	Primitive prim = primitives[ray->primIdx];
	Material mat = materials[prim.matIdx];
	float3 albedo = mat.color;

	if ( mat.texIdx != -1 )
		switch ( prim.objType )
		{
		case SPHERE: {
			float2 uv = getSphereUV( spheres + prim.objIdx, ray->N );
			int x = (int)(uv.x * mat.texW);
			int y = (int)(uv.y * mat.texH);
			albedo = textures[x + y * mat.texW];
		}break;
		}

	return albedo;
}