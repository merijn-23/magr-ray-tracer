#pragma once
#include "common.h"
namespace Tmpl8
{
	class Scene
	{
	public:
		Scene( );
		~Scene( );
		void SetTime( float t );
		Material& AddMaterial( std::string name );
		void AddSphere( float3 pos, float radius, std::string material );
		void AddPlane( float3 N, float d, std::string material );
		void AddQuad( float3 v0, float3 v1, float3 v2, float3 v3, const std::string material, bool flipNormal = false, float2 uv0 = { 0, 0 }, float2 uv1 = { 1,0 }, float2 uv2 = { 0, 1 }, float2 uv3 = { 1, 1 } );
		void AddTriangle( float3 v0, float3 v1, float3 v2, float2 uv0, float2 uv1, float2 uv2, const std::string material, bool flipNormal = false );
		void LoadModel( std::string filename, const std::string defaultMaterial, float3 pos = {0, 0, 0}, bool _forceDefaultMat = false );
		void LoadTexture( std::string filename, std::string name );

	public:
		__declspec( align( 64 ) ) // start a new cacheline here
			float animTime = 0;

		std::vector<Primitive> primitives;
		std::vector<Material> materials;
		std::vector<uint> lights;
		std::vector<float4> textures;
		std::vector<BVHInstance> blasNodes;
		BVH2* bvh2;
		BVH4* bvh4;

	private:
		std::map<std::string, int> matMap_;
		int matIdx_ = 0;
	};
} // namespace Tmpl8