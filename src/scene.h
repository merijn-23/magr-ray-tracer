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

#define PLANE_X(o, i)                                \
    {                                                \
        if ((t = -(ray.O.x + o) * ray.rD.x) < ray.t) \
            ray.t = t, ray.objIdx = i;               \
    }
#define PLANE_Y(o, i)                                \
    {                                                \
        if ((t = -(ray.O.y + o) * ray.rD.y) < ray.t) \
            ray.t = t, ray.objIdx = i;               \
    }
#define PLANE_Z(o, i)                                \
    {                                                \
        if ((t = -(ray.O.z + o) * ray.rD.z) < ray.t) \
            ray.t = t, ray.objIdx = i;               \
    }

#include <map>


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
#define SPHERE 0
#define PLANE 1
#define CUBE 2
#define QUAD 3
#define TRIANGLE 4
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
    template <class T>
    void Resize(T* arr, int currSize, int newSize)
    {
        T* newArray = new T[newSize];
        memcpy(newArray, arr, currSize * sizeof(T));
        currSize = newSize;
        delete[] arr;
        arr = newArray;
    }

public:
    Scene();
    ~Scene();
    void SetTime(float t);
    void AddMaterial(Material material, std::string name);
    void AddSphere(float3 pos, float radius, std::string material);
    void AddPlane(float3 N, float d, std::string material);
    void AddTriangle(float3 v0, float3 v1, float3 v2, std::string material);
    void LoadModel(std::string filename);

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
    std::map<std::string, int> matMap_;
    int primIdx_ = 0;
    int sphereIdx_ = 0;
    int planeIdx_ = 0;
    int matIdx_ = 0;
    int lightIdx_ = 0;
    int triIdx_ = 0;

    int sizePrimitives_;
    int sizeSpheres_;
    int sizePlanes_;
    int sizeMaterials_;
    int sizeLights_;
    int sizeTriangles_;
};
} // namespace Tmpl8