#pragma once

#include "common.h"

class BVH {
public:	
	struct Bin { aabb bounds; int count = 0; };
	BVH() = default;
	void BuildBVH(std::vector<Primitive>& primitives);
	void Refit( std::vector<Primitive>& );
	Buffer* GetIdxBuffer();
	Buffer* GetBVHNodeBuffer();
	void CopyToDevice();

private:
	void UpdateNodeBounds(uint nodeIdx, std::vector<Primitive>& primitives);
	void UpdateTriangleBounds( BVHNode& node, Triangle& triangle );
	void UpdateSphereBounds( BVHNode& node, Sphere& sphere );
	
	float FindBestSplitPlane( BVHNode& node, int& axis, float& splitPos, vector<Primitive> primitives );
	float CalculateNodeCost( BVHNode& node );
		
	void Subdivide(uint nodeIdx, std::vector<Primitive>& primitives);

	std::vector<BVHNode> bvhNode_;
	std::vector<uint> primIdx_;
	Buffer* bvhNodeBuffer_;
	Buffer* primIdxBuffer_;

	uint rootNodeIdx_, nodesUsed_;
	uint N_;
	static const int bins__ = 8;
};

class TLAS
{
public:
	TLAS( ) = default;
	TLAS( std::vector<BVH>&& );
	void Build( );

private:
	std::vector<TLASNode> tlasNode_;
	std::vector<BVH> blas_;
	uint nodesUsed_;
};