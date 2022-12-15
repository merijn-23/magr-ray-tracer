#pragma once

#include "common.h"

class BVH {
public:	
	BVH() = default;
	void BuildBvh(std::vector<Primitive>& primitives);
	Buffer* GetIdxBuffer();
	Buffer* GetBVHNodeBuffer();
	void CopyToDevice();

private:
	void UpdateNodeBounds(uint nodeIdx, std::vector<Primitive>& primitives);
	void UpdateTriangleBounds( BVHNode& node, Triangle& triangle );
	void UpdateSphereBounds( BVHNode& node, Sphere& sphere );
	float EvaluateSAH( BVHNode& node, int axis, float pos, std::vector<Primitive>& primitives );
	void Subdivide(uint nodeIdx, std::vector<Primitive>& primitives);

	std::vector<BVHNode> bvhNode_;
	std::vector<uint> primIdx_;
	Buffer* bvhNodeBuffer_;
	Buffer* primIdxBuffer_;

	uint rootNodeIdx_ = 0, nodesUsed_ = 1;
	uint N_;
};