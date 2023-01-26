#ifndef __PRIMITIVES_CL
#define __PRIMITIVES_CL

#include "src/common.h"

__global Primitive* primitives;
__global Material* materials;
__global uint* lights;
__global float4* textures;

void intersectSphere( int primIdx, Sphere sphere, Ray* ray )
{
	float4 oc = ray->O - sphere.pos;
	float b = dot( oc, ray->D );
	float c = dot( oc, oc ) - sphere.r2;
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
		ray->t = t, ray->primIdx = primIdx;
		return;
	}
}

void intersectPlane( int primIdx, Plane plane, Ray* ray )
{
	float t = -( dot( ray->O, plane.N ) + plane.d ) / ( dot( ray->D, plane.N ) );
	if ( t > ray->t || t < 0 ) return;
	ray->t = t, ray->primIdx = primIdx;

	float4 uAxis = ( float4 )( plane.N.y, plane.N.z, -plane.N.x, 0 );
	float4 vAxis = cross( uAxis, plane.N );
	float4 I = ray->O + ray->t * ray->D;
	ray->u = dot( I, uAxis );
	ray->v = dot( I, vAxis );
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
float kEpsilon = 1e-8;
void intersectTriangle( int primIdx, Triangle tri, Ray* ray )
{
	float4 v0v1 = tri.v1 - tri.v0;
	float4 v0v2 = tri.v2 - tri.v0;
	float4 pvec = cross( ray->D, v0v2 );
	float det = dot( v0v1, pvec );
	// ray and triangle are parallel if det is close to 0
#ifdef ONE_SIDED_TRIANGLE
	if ( det < kEpsilon ) return;
#else
	if ( fabs( det ) < kEpsilon ) return;
#endif
	float invDet = 1 / det;
	float4 tvec = ray->O - tri.v0;
	float u = dot( tvec, pvec ) * invDet;
	if ( u < 0 || u > 1 ) return;

	float4 qvec = cross( tvec, v0v1 );
	float v = dot( ray->D, qvec ) * invDet;
	if ( v < 0 || u + v > 1 ) return;

	float t = dot( v0v2, qvec ) * invDet;
	if ( t > ray->t || t < 0 ) return;
	ray->t = t;
	ray->primIdx = primIdx;

	// set barycenter
	ray->u = u;
	ray->v = v;
}

void intersect( int primIdx, Primitive* prim, Ray* ray )
{
	switch ( prim->objType )
	{
		case SPHERE:
			intersectSphere( primIdx, prim->objData.sphere, ray ); break;
		case PLANE:
			intersectPlane( primIdx, prim->objData.plane, ray ); break;
		case TRIANGLE:
			intersectTriangle( primIdx, prim->objData.triangle, ray ); break;
	}
}

float4 getNormal( Primitive* prim, float4 I )
{
	switch ( prim->objType )
	{
		case SPHERE:
		{
			Sphere sphere = prim->objData.sphere;
			return ( I - sphere.pos ) * sphere.invr;
		}
		case PLANE:
			return prim->objData.plane.N;
		case TRIANGLE:
			return prim->objData.triangle.N;
	}
}

float4 getAlbedo( Ray* ray )
{
	Primitive prim = primitives[ray->primIdx];
	Material mat = materials[prim.matIdx];
	float4 albedo = mat.color;
	if ( mat.texIdx != -1 )
		switch ( prim.objType )
		{
			case TRIANGLE:
			{
				Triangle t = prim.objData.triangle;
				float2 uv = ray->u * t.uv1 + ray->v * t.uv0 + ( 1 - ray->u - ray->v) * t.uv2;
				uv = fmod( uv, ( float2 )( 1.f, 1.f ) );
				if ( uv.x < 0 ) uv.x = 1 + uv.x;
				if ( uv.y < 0 ) uv.y = 1 + uv.y;
				int x = (int)( uv.x * mat.texW );
				int y = (int)( uv.y * mat.texH );
				//printf( "x: %i, y: %i, max.texIdx: %i, mat.texW: %i, mat.texH: %i, u: %f, v: %f\n", x, y, mat.texIdx, mat.texW, mat.texH, uv.x, uv.y );
				albedo = textures[mat.texIdx + x + y * mat.texW];
			}break;
			case SPHERE:
			{
				float2 uv;
				uv.x = ( 1 + atan2pi( ray->N.z, ray->N.x ) ) * 0.5;
				uv.y = acospi( ray->N.y );
				int x = (int)( uv.x * mat.texW );
				int y = (int)( uv.y * mat.texH );
				albedo = textures[mat.texIdx + x + y * mat.texW];
			}break;
			case PLANE:
			{
				float u = fmod( ray->u, 1.f );
				float v = fmod( ray->v, 1.f );
				if ( u < 0 ) u = 1 - u;
				if ( v < 0 ) v = 1 - v;
				int x = (int)( u * mat.texW );
				int y = (int)( v * mat.texH );
				albedo = textures[mat.texIdx + ( x + y * mat.texW )];
			}break;
		}
	return albedo;
}

float getSurvivalProb( float4 albedo )
{
	return clamp( max(albedo.x, max(albedo.y, albedo.z)), 0.f, 1.f);
}

float4 getRandomPoint(Primitive* prim, uint* seed)
{
	switch(prim->objType)
	{
		case SPHERE:
		{
			Sphere sphere = prim->objData.sphere;
			// https://www.demonstrations.wolfram.com/RandomPointsOnASphere/
			float theta = random(seed) * 2 * M_PI_F;
			float u = random(seed) * 2 - 1;

			float precomp = sqrt(1 - u * u);
			float x = cos(theta) * precomp;
			float y = sin(theta) * precomp;
			float4 point = (float4)(x, y, u, 0) * sphere.r + sphere.pos;
			return point;
		}
		case TRIANGLE:
		{
			Triangle triangle = prim->objData.triangle;
			// https://blogs.sas.com/content/iml/2020/10/19/random-points-in-triangle.html
			float u1 = random(seed);
			//printf("u1 %f\n", u1);
			float u2 = random(seed);
			if(u1 + u2 > 1)
			{
				u1 = 1 - u1;
				u2 = 1 - u2;
			}
			float4 a = triangle.v1 - triangle.v0;
			float4 b = triangle.v2 - triangle.v0;
			return triangle.v0 + u1 * a + u2 * b;
		}
	}
}

//float getArea(Primitive* prim)
//{
//	switch(prim->objType)
//	{
//		case SPHERE:
//			return 4 * M_PI_F * prim->objData.sphere.r2;
//		case TRIANGLE:
//		{
//			Triangle t = prim->objData.triangle;
//			return 0.5f * (t.v0.x * (t.v1.y - t.v2.y) + t.v1.x * (t.v2.y - t.v0.y) + t.v2.x * (t.v0.y - t.v1.y));
//		}
//	}
//}

#endif // __PRIMITIVES_CL