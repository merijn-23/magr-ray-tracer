#pragma once
#include "common.h"
struct BVHPrimData { aabb box; uint idx; };
class BVH2
{
public:
	BVH2( std::vector<Primitive>& );
	void Build( bool statistics );
	uint Depth( uint nodeIdx = -1 );
	uint Count( uint nodeIdx = -1 );
	//bool CheckBBs( uint nodeIdx = -1 );
	float TotalCost( uint nodeIdx = -1 );
	BVHNode2 Root() { return nodes[rootNodeIdx_]; }
	BVHNode2 Left( BVHNode2 n ) { return nodes[n.first]; }
	BVHNode2 Right( BVHNode2 n ) { return nodes[n.first + 1]; }
	std::vector<BVHNode2> nodes;
	std::vector<uint> primIdx;
	float alpha = .001f;
	// statistics
	uint stat_depth, stat_node_count, stat_spatial_splits, stat_prims_clipped, stat_prim_count;
	float stat_sah_cost, stat_build_time;
private:
	void BuildBVH( uint root, std::vector<BVHPrimData> data );
	void UpdateNodeBounds( uint nodeIdx, std::vector<BVHPrimData> prims );
	void UpdateTriangleBounds( BVHNode2& node, Triangle& triangle );
	void UpdateSphereBounds( BVHNode2& node, Sphere& sphere );
	std::vector<BVHPrimData> CreateBVHPrimData();
	void Subdivide( uint nodeIdx );
	float CalculateNodeCost( BVHNode2& node, uint count );
	float FindBestObjectSplitPlane( BVHNode2& node, int& axis, float& splitPos, float& overlap, std::vector<BVHPrimData> prims );
	std::pair<std::vector<BVHPrimData>, std::vector<BVHPrimData>> ObjectSplit( BVHNode2& node, int axis, float splitPos, std::vector<BVHPrimData> prims );
	float FindBestSpatialSplitPlane( BVHNode2& node, int& axis, float& splitPos, std::vector<BVHPrimData> prims );
	std::pair<std::vector<BVHPrimData>, std::vector<BVHPrimData>> SpatialSplit( BVHNode2& node, int axis, float splitPos, std::vector<BVHPrimData> prims );
	bool ClipTriangleToAABB( aabb bounds, float3 v0, float3 v1, float3 v2, aabb& outBounds );
	bool ClipSphereToAABB( aabb bounds, float3 pos, float r, aabb& outBounds );
	float3 LineAAPlaneIntersection( float3 v1, float3 v2, int axis, float plane );
	std::vector<float3> SphereAAPlaneIntersection( float3 pos, float d, int axis, float plane );
	std::vector<Primitive>& primitives_;
	uint subdivisions_ = 0;
	uint rootNodeIdx_, nodesUsed_;
};
class BVH4
{
public:
	BVH4( BVH2& );
	std::vector<BVHNode4>& Nodes( ) { return nodes; }
	std::vector<uint>& Idx( ) { return bvh2.primIdx; }
	uint Depth( BVHNode4 );
	uint Count( BVHNode4 );
	BVHNode4 Root( ) { return nodes[rootNodeIdx_]; }
private:
	BVH2& bvh2;
	std::vector<BVHNode4> nodes;
	void Convert( );
	void Collapse( int index ); 
	int GetChildCount( const BVHNode4& node ) const;
	uint rootNodeIdx_;
};
