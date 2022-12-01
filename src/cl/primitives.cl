#include "src/common.h"

__global Primitive* primitives;
__global Material* materials;
__global Sphere* spheres;
__global Plane* planes;
__global Triangle* triangles;
__global Light* lights;
__global float4* textures;


// typedef struct Cube
// {
// 	float4 b[2];
// 	float4x4 M, invM;
// } Cube;

void intersectSphere( int primIdx, Sphere* sphere, Ray* ray )
{
	float4 oc = ray->O - sphere->pos;
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

void intersectPlane( int primIdx, Plane* plane, Ray* ray )
{
	float t = -( dot( ray->O, plane->N ) + plane->d ) / ( dot( ray->D, plane->N ) );
	if ( t > ray->t || t < 0 ) return;

	ray->t = t, ray->primIdx = primIdx;
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
float kEpsilon = 1e-8;
void intersectTriangle( int primIdx, Triangle* tri, Ray* ray )
{
	float4 v0v1 = tri->v1 - tri->v0;
	float4 v0v2 = tri->v2 - tri->v0;
	float4 pvec = cross( ray->D, v0v2 );
	float det = dot( v0v1, pvec );
	// ray and triangle are parallel if det is close to 0
#ifdef ONE_SIDED
	if ( det < kEpsilon ) return;
#else
	if ( fabs( det ) < kEpsilon ) return;
#endif
	float invDet = 1 / det;
	float4 tvec = ray->O - tri->v0;
	float u = dot( tvec, pvec ) * invDet;
	if ( u < 0 || u > 1 ) return;

	float4 qvec = cross( tvec, v0v1 );
	float v = dot( ray->D, qvec ) * invDet;
	if ( v < 0 || u + v > 1 ) return;

	float t = dot( v0v2, qvec ) * invDet;
	if ( t > ray->t || t < 0 ) return;
	ray->t = t;
	ray->primIdx = primIdx;
	ray->inside = true;
	// set barycenter
	tri->u = u;
	tri->v = v;
	tri->w = 1 - u - v;
}

void intersect( int primIdx, Primitive* prim, Ray* ray )
{
	switch ( prim->objType )
	{
		case SPHERE:
			intersectSphere( primIdx, spheres + prim->objIdx, ray );
			break;
		case PLANE:
			intersectPlane( primIdx, planes + prim->objIdx, ray );
			break;
		case TRIANGLE:
			intersectTriangle( primIdx, triangles + prim->objIdx, ray );
			break;
	}
}

float4 getNormal( Primitive* prim, float4 I )
{
	switch ( prim->objType )
	{
		case SPHERE:
		{
			Sphere sphere = spheres[prim->objIdx];
			return ( I - sphere.pos ) * sphere.invr;
		}
		case PLANE:
			return planes[prim->objIdx].N;
		case TRIANGLE:
			return triangles[prim->objIdx].N;
	}
}

float2 getSphereUV( Sphere* sphere, float4 N )
{
	float2 tex;
	tex.x = ( 1 + atan2pi( N.z, N.x ) ) * 0.5;
	tex.y = acospi( N.y );
	return tex;
}

float4 getAlbedo( Ray* ray, float4 I )
{
	Primitive prim = primitives[ray->primIdx];
	Material mat = materials[prim.matIdx];
	float4 albedo = mat.color;

	if ( mat.texIdx != -1 )
		switch ( prim.objType )
		{
			case SPHERE:
			{
				float2 uv = getSphereUV( spheres + prim.objIdx, ray->N );
				int x = (int)( uv.x * mat.texW );
				int y = (int)( uv.y * mat.texH );
				albedo = textures[x + y * mat.texW];
			}break;
			case TRIANGLE:
			{
				Triangle t = triangles[prim.objIdx];
				float2 uv0 = ( float2 )( t.uv0.x * mat.texW, t.uv0.y * mat.texH );
				float2 uv1 = ( float2 )( t.uv1.x * mat.texW, t.uv1.y * mat.texH );
				float2 uv2 = ( float2 )( t.uv2.x * mat.texW, t.uv2.y * mat.texH );

				float2 p = t.u * uv0 + t.v * uv1 + t.w * uv2;
				//albedo = textures[(int)p.x + (int)p.y * mat.texW];

				//albedo = t.u * c0 + t.v * c1 + t.w * c2;
				albedo = ( float4 )( t.u, t.v, t.w, 1 );
			}break;
		}
	return albedo;
}