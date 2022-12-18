#pragma once

#include "common.h"

class BVH {
public:	
	BVH(std::vector<Primitive>&);
	void Build();
	uint Depth(BVHNode );
	std::vector<BVHNode> bvhNode;
	std::vector<uint> bvhIdx;

private:
	void UpdateNodeBounds(uint nodeIdx);
	void UpdateTriangleBounds( BVHNode& node, Triangle& triangle );
	void UpdateSphereBounds( BVHNode& node, Sphere& sphere );
	
	float FindBestSplitPlane( BVHNode& node, int& axis, float& splitPos );
	float CalculateNodeCost( BVHNode& node );		
	void Subdivide(uint nodeIdx);

	std::vector<Primitive>& primitives_;

	uint rootNodeIdx_, nodesUsed_;
	uint count_;
	uint subdivisions_ = 0;
	static const int bins__ = 8;
};