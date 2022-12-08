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
            ray.t = t, ray.primIdx = i;               \
    }
#define PLANE_Y(o, i)                                \
    {                                                \
        if ((t = -(ray.O.y + o) * ray.rD.y) < ray.t) \
            ray.t = t, ray.primIdx = i;               \
    }
#define PLANE_Z(o, i)                                \
    {                                                \
        if ((t = -(ray.O.z + o) * ray.rD.z) < ray.t) \
            ray.t = t, ray.primIdx = i;               \
    }

#include "common.h"

namespace Tmpl8
{

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
	Scene( );
	~Scene( );
	void SetTime( float t );
	Material& AddMaterial( std::string name );
	void AddSphere( float3 pos, float radius, std::string material );
	void AddPlane( float3 N, float d, std::string material );
	void AddTriangle( float3 v0, float3 v1, float3 v2, float2 uv0, float2 uv1, float2 uv2, std::string material );
	void LoadModel( std::string filename, std::string material, float3 pos );
	void LoadTexture( std::string filename, std::string name );

public:
	__declspec(align(64)) // start a new cacheline here
	float animTime = 0;

	std::vector<Primitive> primitives;
	std::vector<Material> materials;
	std::vector<Light> lights;
	std::vector<float4> textures;

private:
	std::map<std::string, int> matMap_;
	int matIdx_ = 0;
};
} // namespace Tmpl8