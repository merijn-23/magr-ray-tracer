#pragma once

// -----------------------------------------------------------
// scene.h
// Simple test scene for ray tracing experiments. Goals:
// - Super-fast scene intersection
// - Easy interface: scene.FindNearest / IsOccluded
// - With normals and albedo: GetNormal / GetAlbedo
// - Area light source (animated), for light transport
// - Primitives can be hit from inside - for dielectrics
// - Can be extended with other primitives and/or a BVH
// - Optionally animated - for temporal experiments
// - Not everything is axis aligned - for cache experiments
// - Can be evaluated at arbitrary time - for motion blur
// - Has some high-frequency details - for filtering
// Some speed tricks that severely affect maintainability
// are enclosed in #ifdef SPEEDTRIX / #endif. Mind these
// if you plan to alter the scene in any way.
// -----------------------------------------------------------

// #define SPEEDTRIX

#define PLANE_X(o,i) {if((t=-(ray.O.x+o)*ray.rD.x)<ray.t)ray.t=t,ray.objIdx=i;}
#define PLANE_Y(o,i) {if((t=-(ray.O.y+o)*ray.rD.y)<ray.t)ray.t=t,ray.objIdx=i;}
#define PLANE_Z(o,i) {if((t=-(ray.O.z+o)*ray.rD.z)<ray.t)ray.t=t,ray.objIdx=i;}

namespace Tmpl8 {

//__declspec(align(64)) class Ray
//{
//public:
//	Ray() = default;
//	Ray( float3 origin, float3 direction, float distance = 1e34f )
//	{
//		O = origin, D = direction, t = distance;
//		// calculate reciprocal ray direction for triangles and AABBs
//		rD = float3( 1 / D.x, 1 / D.y, 1 / D.z );
//	#ifdef SPEEDTRIX
//		d0 = d1 = d2 = 0;
//	#endif
//	}
//	float3 IntersectionPoint() { return O + t * D; }
//	// ray data
//#ifndef SPEEDTRIX
//	float3 O, D, rD;
//#else
//	union { struct { float3 O; float d0; }; __m128 O4; };
//	union { struct { float3 D; float d1; }; __m128 D4; };
//	union { struct { float3 rD; float d2; }; __m128 rD4; };
//#endif
//	float t = 1e34f;
//	int objIdx = -1;
//	bool inside = false; // true when in medium
//};

// -----------------------------------------------------------
// Base primitive struct
// Keeps track of both the Material of a primitive as well as
// it's type and index in the correct array.
// Primitive types:
// 0 - Sphere
// 1 - Plane
// 2 - Cube
// 3 - Quad
// 4 - Triangle
// -----------------------------------------------------------
typedef struct Primitive_struct
{
	int objType, objIdx, matIdx;
} Primitive;

// -----------------------------------------------------------
// Material struct
// Keeps track of material properties of all primitives. Each 
// primitive has a matIdx to keep track of its own material.
// -----------------------------------------------------------
typedef struct Material_struct
{
	float4 colour;
	float reflect, refract;
} Material;

typedef struct Sphere_struct
{
	float4 pos;
	float r2, invr;
} Sphere;

typedef struct Plane_struct
{
	float4 N;
	float d;
} Plane;

typedef struct Cube_struct
{
	float4 b[2];
	mat4 M, invM;
} Cube;

typedef struct Quad_struct
{
	float size;
	mat4 T, invT;
} Quad;

typedef struct Light_struct
{
	float4 pos, color;
	float strength;
	// Not used for point lights
	int primIdx;
} Light;	


//// -----------------------------------------------------------
//// Cube primitive
//// Oriented cube. Unsure if this will also work for rays that
//// start inside it; maybe not the best candidate for testing
//// dielectrics.
//// -----------------------------------------------------------
//class Cube
//{
//public:
//	Cube() = default;
//	Cube( int idx, float3 pos, float3 size, mat4 transform = mat4::Identity() )
//	{
//		objIdx = idx;
//		b[0] = pos - 0.5f * size, b[1] = pos + 0.5f * size;
//		M = transform, invM = transform.FastInvertedTransformNoScale();
//	}
//	void Intersect( Ray& ray ) const
//	{
//		// 'rotate' the cube by transforming the ray into object space
//		// using the inverse of the cube transform.
//		float3 O = TransformPosition( ray.O, invM );
//		float3 D = TransformVector( ray.D, invM );
//		float rDx = 1 / D.x, rDy = 1 / D.y, rDz = 1 / D.z;
//		int signx = D.x < 0, signy = D.y < 0, signz = D.z < 0;
//		float tmin = (b[signx].x - O.x) * rDx;
//		float tmax = (b[1 - signx].x - O.x) * rDx;
//		float tymin = (b[signy].y - O.y) * rDy;
//		float tymax = (b[1 - signy].y - O.y) * rDy;
//		if (tmin > tymax || tymin > tmax) return;
//		tmin = max( tmin, tymin ), tmax = min( tmax, tymax );
//		float tzmin = (b[signz].z - O.z) * rDz;
//		float tzmax = (b[1 - signz].z - O.z) * rDz;
//		if (tmin > tzmax || tzmin > tmax) return;
//		tmin = max( tmin, tzmin ), tmax = min( tmax, tzmax );
//		if (tmin > 0)
//		{
//			if (tmin < ray.t) ray.t = tmin, ray.objIdx = objIdx;
//		}
//		else if (tmax > 0)
//		{
//			if (tmax < ray.t) ray.t = tmax, ray.objIdx = objIdx;
//		}
//	}
//	float3 GetNormal( const float3 I ) const
//	{
//		// transform intersection point to object space
//		float3 objI = TransformPosition( I, invM );
//		// determine normal in object space
//		float3 N = float3( -1, 0, 0 );
//		float d0 = fabs( objI.x - b[0].x ), d1 = fabs( objI.x - b[1].x );
//		float d2 = fabs( objI.y - b[0].y ), d3 = fabs( objI.y - b[1].y );
//		float d4 = fabs( objI.z - b[0].z ), d5 = fabs( objI.z - b[1].z );
//		float minDist = d0;
//		if (d1 < minDist) minDist = d1, N.x = 1;
//		if (d2 < minDist) minDist = d2, N = float3( 0, -1, 0 );
//		if (d3 < minDist) minDist = d3, N = float3( 0, 1, 0 );
//		if (d4 < minDist) minDist = d4, N = float3( 0, 0, -1 );
//		if (d5 < minDist) minDist = d5, N = float3( 0, 0, 1 );
//		// return normal in world space
//		return TransformVector( N, M );
//	}
//	float3 GetAlbedo( const float3 I ) const
//	{
//		return float3( 1, 1, 1 );
//	}
//	float3 b[2];
//	mat4 M, invM;
//	int objIdx = -1;
//	int matIdx = -1;
//};
//
//// -----------------------------------------------------------
//// Quad primitive
//// Oriented quad, intended to be used as a light source.
//// -----------------------------------------------------------
//class Quad
//{
//public:
//	Quad() = default;
//	Quad( int idx, float s, mat4 transform = mat4::Identity() )
//	{
//		objIdx = idx;
//		size = s * 0.5f;
//		T = transform, invT = transform.FastInvertedTransformNoScale();
//	}
//	void Intersect( Ray& ray ) const
//	{
//		const float3 O = TransformPosition( ray.O, invT );
//		const float3 D = TransformVector( ray.D, invT );
//		const float t = O.y / -D.y;
//		if (t < ray.t && t > 0)
//		{
//			float3 I = O + t * D;
//			if (I.x > -size && I.x < size && I.z > -size && I.z < size)
//				ray.t = t, ray.objIdx = objIdx;
//		}
//	}
//	float3 GetNormal( const float3 I ) const
//	{
//		// TransformVector( float3( 0, -1, 0 ), T ) 
//		return float3( -T.cell[1], -T.cell[5], -T.cell[9] );
//	}
//	float3 GetAlbedo( const float3 I ) const
//	{
//		return float3( 10 );
//	}
//	float size;
//	mat4 T, invT;
//	int objIdx = -1;
//	int matIdx = -1;
//};




// -----------------------------------------------------------
// Scene class
// We intersect this. The query is internally forwarded to the
// list of primitives, so that the nearest hit can be returned.
// For this hit (distance, obj id), we can query the normal and
// albedo.
// -----------------------------------------------------------
class Scene
{
public:
	Scene()
	{
		// we store all primitives in one continuous buffer
		//quad = Quad( 0, 1 );									// 0: light source
		//sphere = Sphere( 1, float3( 0 ), 0.5f );				// 1: bouncing ball
		//sphere2 = Sphere( 2, float3( 0, 2.5f, -3.07f ), 8 );	// 2: rounded corners
		//cube = Cube( 3, float3( 0 ), float3( 1.15f ) );		// 3: cube
		//plane[0] = Plane( 4, float3( 1, 0, 0 ), 3 );			// 4: left wall
		//plane[1] = Plane( 5, float3( -1, 0, 0 ), 2.99f );		// 5: right wall
		//plane[2] = Plane( 6, float3( 0, 1, 0 ), 1 );			// 6: floor
		//plane[3] = Plane( 7, float3( 0, -1, 0 ), 2 );			// 7: ceiling
		//plane[5] = Plane( 9, float3( 0, 0, -1 ), 3.99f );		// 9: back wall
		//plane[4] = Plane( 8, float3( 0, 0, 1 ), 3 );			// 8: front wall

		spheres[0] = Sphere{ float4(-.8f, 0.f, 0.f, 0), .25f, 2.f };			// 1: bouncing ball
		spheres[1] = Sphere{ float4(.3f, 0.f, 0.f, 0), .25f, 2.f };			// 1: bouncing ball
		mats[0] = Material{ float4(1, 0, 0, 0), .2f, 0 };
		mats[1] = Material{ float4(0, 1, 0, 0), .2f, 0 };
		mats[2] = Material{ float4(1, 1, 1, 0), 0, 0 };
		prims[0] = Primitive{ 0, 0, 0 };
		prims[1] = Primitive{ 0, 1, 1 };

		planes[0] = Plane{ float4(1, 0, 0, 0), 5.f };
		planes[1] = Plane{ float4(-1, 0, 0, 0), 2.99f };
		planes[2] = Plane{ float4(0, 1, 0, 0), 1.f };
		planes[3] = Plane{ float4(0, -1, 0, 0), 2.f };
		planes[4] = Plane{ float4(0, 0, 1, 0), 9.f };
		//planes[4] = Plane{ float4(0, 0, -1, 0), 3.99f };
		prims[2] = Primitive{ 1, 0, 2 };
		prims[3] = Primitive{ 1, 1, 2 };
		prims[4] = Primitive{ 1, 2, 2 };
		prims[5] = Primitive{ 1, 3, 2 };
		prims[6] = Primitive{ 1, 4, 2 };

		lights[0] = Light{ float4(2, 0, 3, 0), float4(1,1,.8f,0), 3, -1 };
		//prims[6] = Primitive{ 1, 4, 0 };

		SetTime( 0 );
		// Note: once we have triangle support we should get rid of the class
		// hierarchy: virtuals reduce performance somewhat.
	}

	void SetTime( float t )
	{
		// default time for the scene is simply 0. Updating/ the time per frame 
		// enables animation. Updating it per ray can be used for motion blur.
		animTime = t;
		lights[0].pos.x = sin(animTime) + 1;
		lights[0].pos.y = sin(animTime + PI * 0.5) * 0.5f;
		// light source animation: swing
		//mat4 M1base = mat4::Translate( float3( 0, 2.6f, 2 ) );
		//mat4 M1 = M1base * mat4::RotateZ( sinf( animTime * 0.6f ) * 0.1f ) * mat4::Translate( float3( 0, -0.9, 0 ) );
		//quad.T = M1, quad.invT = M1.FastInvertedTransformNoScale();
		//// cube animation: spin
		//mat4 M2base = mat4::RotateX( PI / 4 ) * mat4::RotateZ( PI / 4 );
		//mat4 M2 = mat4::Translate( float3( 1.4f, 0, 2 ) ) * mat4::RotateY( animTime * 0.5f ) * M2base;
		//cube.M = M2, cube.invM = M2.FastInvertedTransformNoScale();
		// sphere animation: bounce
		float tm = 1 - sqrf( fmodf( animTime, 2.0f ) - 1 );
		spheres[0].pos = float4( -0.75f, -0.5f + tm, 0, 0 );
	}

	__declspec(align(64)) // start a new cacheline here
	float animTime = 0;
	Primitive prims[7];
	Sphere spheres[2];
	Plane planes[6];
	Cube cubes[1];
	Material mats[3];
	Light lights[1];
	//Quad quad;
	//Sphere sphere;
	//Sphere sphere2;
	//Cube cube;
	//Plane plane[6];
	//
	//Sphere spheres[2];


};
}