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

#include <map>

namespace Tmpl8
{

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
#define SPHERE		0
#define PLANE		1
#define CUBE		2
#define QUAD		3
#define TRIANGLE	4
// -----------------------------------------------------------
	typedef struct Primitive
	{
		int objType, objIdx, matIdx;
	};

	// -----------------------------------------------------------
	// Material struct
	// Keeps track of material properties of all primitives. Each 
	// primitive has a matIdx to keep track of its own material.
	// -----------------------------------------------------------
	typedef struct Material
	{
		float4 colour;
		float specular, n1, n2;
		bool isDieletric;
	};

	typedef struct Sphere
	{
		float4 pos;
		float r2, invr; // r * r, 1 / r
	};

	typedef struct Plane
	{
		float4 N;
		float d;
	};

	typedef struct Cube
	{
		float4 b[2];
		mat4 M, invM;
	};

	typedef struct Quad
	{
		float size;
		mat4 T, invT;
	};

	typedef struct Light
	{
		float4 pos, color;
		float strength;
		// Not used for point lights
		int primIdx;
	};

	typedef struct Triangle
	{
		float4 v0, v1, v2;
		float4 N;
	};


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
	private:
		template<class T>
		void Resize( T* arr, int currSize, int newSize )
		{
			T* newArray = new T[newSize];
			memcpy( newArray, arr, currSize * sizeof( T ) );
			currSize = newSize;
			delete[] arr;
			arr = newArray;
		}

	public:
		Scene( )
		{
		/*	primitives = new Primitive[9];
			_sizePrimitives = 9;
			spheres = new Sphere[2];
			_sizeSpheres = 2;
			planes = new Plane[6];
			_sizePlanes = 6;
			materials = new Material[8];
			_sizeMaterials = 8;
			triangles = new Triangle[1];
			_sizeTriangles = 1;
			lights = new Light[3];*/
			//Resize( primitives, 0, 9 );

			AddMaterial( Material{ float3( 1, 0, 0 ), 0, 0, 0, false }, "red" );
			AddMaterial( Material{ float3( 0, 1, 0 ), 0, 0, 0, false }, "green" );
			AddMaterial( Material{ float3( 0, 0, 1 ), 0, 0, 0, false }, "blue" );
			AddMaterial( Material{ float3( 1, 1, 1 ), 0, 0, 0, false }, "white" );
			AddMaterial( Material{ float3( 1, 1, 0 ), 0, 0, 0, false }, "yellow" );
			AddMaterial( Material{ float3( 1, 0, 1 ), 0, 0, 0, false }, "magenta" );
			AddMaterial( Material{ float3( 0, 1, 1 ), 0, 0, 0, false }, "cyan" );
			AddMaterial( Material{ float3( 1, 1, 1 ), 0, 1, 1.1f, true }, "glass" );

			AddSphere( float4( 0, 0.f, -1.5f, 0.f ), 0.5f, "red" );
			AddSphere( float4( 1.5f, -0.49f, 0.f, 0 ), 0.5f, "glass" );

			AddPlane( float3( 1, 0, 0 ), 5.f, "yellow" );
			AddPlane( float3( -1, 0, 0 ), 2.99f, "green" );
			AddPlane( float3( 0, 1, 0 ), 1.f, "blue" );
			AddPlane( float3( 0, -1, 0 ), 2.f, "white" );
			AddPlane( float3( 0, 0, 1 ), 9.f, "magenta" );
			AddPlane( float3( 0, 0, -1 ), 3.99f, "cyan" );

			AddTriangle( 
				float4( 0, 0.f, -1.5f, 0.f ),
				float4( 1, 0.f, -1.5f, 0.f ),
				float4( 0, 1.f, -1.5f, 0.f ), 
				"white" );
						
			lights[0] = Light{ float4( 2, 0, 3, 0 ), float4( 1,1,.8f,0 ), 2, -1 };
			lights[1] = Light{ float4( 1, 0, -5, 0 ), float4( 1,1,.8f,0 ), 2, -1 };
			lights[2] = Light{ float4( -2, 0, 3, 0 ), float4( 1,1,.8f,0 ), 1, -1 };

			SetTime( 0 );
			// Note: once we have triangle support we should get rid of the class
			// hierarchy: virtuals reduce performance somewhat.
		}

		~Scene( )
		{
			/*delete[] primitives;
			delete[] spheres;
			delete[] planes;
			delete[] materials;
			delete[] lights;
			delete[] triangles;*/
		}

		void SetTime( float t )
		{
			// default time for the scene is simply 0. Updating/ the time per frame 
			// enables animation. Updating it per ray can be used for motion blur.
			animTime = t;
			//lights[0].pos.x = sin(animTime) + 1;
			//lights[0].pos.y = sin(animTime + PI * 0.5) * 0.5f;

			// sphere animation: bounce
			float tm = 1 - sqrf( fmodf( animTime, 2.0f ) - 1 );
			spheres[0].pos.y = -0.5f + tm;
		}

		void AddMaterial( Material material, std::string name )
		{
			/*if ( _matIdx >= _sizeMaterials )
				Resize( materials, _sizeMaterials );*/

			materials[_matIdx] = material;
			_matMap[name] = _matIdx;
			_matIdx++;
		}

		void AddSphere( float3 pos, float radius, std::string material )
		{
			/*if ( _sphereIdx >= _sizeSpheres || _primIdx >= _sizePrimitives )
			{
				Resize( spheres, _sizeSpheres );
				Resize( primitives, _sizePrimitives );
			}*/
			float r2 = radius * radius;
			float invr = 1 / radius;

			// create sphere
			Sphere sphere;
			sphere.pos = pos;
			sphere.r2 = r2;
			sphere.invr = invr;
			spheres[_sphereIdx] = sphere;

			// create primitive
			Primitive prim;
			prim.objType = SPHERE;
			prim.objIdx = _sphereIdx;
			prim.matIdx = _matMap[material];
			primitives[_primIdx] = prim;

			_sphereIdx++, _primIdx++;
		}

		void AddPlane( float3 N, float d, std::string material )
		{
			/*if ( _planeIdx >= _sizePlanes || _primIdx >= _sizePrimitives )
			{
				Resize( spheres, _sizeSpheres );
				Resize( primitives, _sizePrimitives );
			}*/
			Plane plane;
			plane.N = N;
			plane.d = d;
			planes[_planeIdx] = plane;

			// create primitive
			Primitive prim;
			prim.objType = PLANE;
			prim.objIdx = _planeIdx;
			prim.matIdx = _matMap[material];
			primitives[_primIdx] = prim;

			_planeIdx++, _primIdx++;
		}

		void AddTriangle( float3 v0, float3 v1, float3 v2, std::string material )
		{
			/*if ( _triIdx >= _sizeTriangles || _primIdx >= _sizePrimitives )
			{
				Resize( triangles, _sizeTriangles );
				Resize( primitives, _sizePrimitives );
			}*/
			float3 v0v1 = v1 - v0;
			float3 v0v2 = v2 - v0;
			float3 N = normalize( cross( v0v1, v0v2 ) );

			Triangle tri;
			tri.v0 = v0;
			tri.v1 = v1;
			tri.v2 = v2;
			tri.N = N;
			triangles[_triIdx] = tri;

			// create primitive
			Primitive prim;
			prim.objType = TRIANGLE;
			prim.objIdx = _triIdx;
			prim.matIdx = _matMap[material];
			primitives[_primIdx] = prim;

			_triIdx++, _primIdx++;
		}

	public:
		__declspec(align(64)) // start a new cacheline here
		float animTime = 0;

		Primitive primitives[9];
		Sphere spheres[2];
		Plane planes[6];
		Material materials[8];
		Light lights[3];
		Triangle triangles[1];

	private:
		std::map<std::string, int> _matMap;
		int _primIdx = 0;
		int _sphereIdx = 0;
		int _planeIdx = 0;
		int _matIdx = 0;
		int _lightIdx = 0;
		int _triIdx = 0;

		int _sizePrimitives;
		int _sizeSpheres;
		int _sizePlanes;
		int _sizeMaterials;
		int _sizeLights;
		int _sizeTriangles;
	};
}