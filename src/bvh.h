#pragma once
#include "common.h"
struct BVHPrimData { aabb box; uint idx = 0; };
class BVH2
{
	friend class BVH4;
	friend class TLAS;
public:
	BVH2( std::vector<Primitive>&, std::vector<BVHInstance>& );
	void BuildBLAS( bool statistics, int startIdx);
	uint Depth( uint nodeIdx = -1 );
	uint Count( uint nodeIdx = -1 );
	float TotalCost( uint nodeIdx = -1 );
	BVHNode2 Root() { return bvhNodes[rootNodeIdx_]; }
	BVHNode2 Left( BVHNode2 n ) { return bvhNodes[n.first]; }
	BVHNode2 Right( BVHNode2 n ) { return bvhNodes[n.first + 1]; }
	std::vector<BVHNode2> bvhNodes;
	std::vector<uint> primIdx;
	float alpha = 1.f;
	// statistics
	uint stat_depth = 0, stat_node_count = 0, stat_spatial_splits = 0, stat_prims_clipped = 0, stat_prim_count = 0;
	float stat_sah_cost = 0, stat_build_time = 0;
private:
	std::vector<BVHInstance>& blasNodes;
	void BuildBVH( uint root, std::vector<BVHPrimData> data );
	void UpdateNodeBounds( uint nodeIdx, std::vector<BVHPrimData> prims );
	std::vector<BVHPrimData> CreateBVHPrimData( int startIdx );
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
	uint rootNodeIdx_ = 0, nodesUsed_ = 0;
};
class BVH4
{
public:
	BVH4( BVH2& );
	std::vector<BVHNode4>& Nodes( ) { return bvhNodes; }
	std::vector<uint>& Idx( ) { return bvh2.primIdx; }
	uint Depth( BVHNode4 );
	uint Count( BVHNode4 );
	//BVHNode4 Root( ) { return bvhNodes[rootNodeIdx_]; }
private:
	BVH2& bvh2;
	std::vector<BVHNode4> bvhNodes;
	void Convert( uint root);
	void Collapse( int index ); 
	int GetChildCount( const BVHNode4& node ) const;
};


