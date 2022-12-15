#include "precomp.h"

void BVH::BuildBvh(std::vector<Primitive>& primitives)
{
	N_ = primitives.size();
	bvhNode_.resize(N_ * 2);
	primIdx_.resize(N_);
	// populate triangle index array
	for (int i = 0; i < N_; i++) primIdx_[i] = i;

	BVHNode& root = bvhNode_[rootNodeIdx_];
	root.first = 0, root.count = N_;
	UpdateNodeBounds(rootNodeIdx_, primitives);
	// subdivide recursively
	Subdivide(rootNodeIdx_, primitives);
}

void BVH::UpdateNodeBounds(uint nodeIdx, std::vector<Primitive>& primitives)
{
	BVHNode& node = bvhNode_[nodeIdx];
	node.aabbMin = float3(1e30f);
	node.aabbMax = float3(-1e30f);
	for (uint first = node.first, i = 0; i < node.count; i++)
	{
		uint leafPrimIdx = primIdx_[first + i];
		Triangle& leafPrim = primitives[leafPrimIdx].objData.triangle;
		node.aabbMin = fminf(node.aabbMin, leafPrim.v0),
		node.aabbMin = fminf(node.aabbMin, leafPrim.v1),
		node.aabbMin = fminf(node.aabbMin, leafPrim.v2),
		node.aabbMax = fmaxf(node.aabbMax, leafPrim.v0),
		node.aabbMax = fmaxf(node.aabbMax, leafPrim.v1),
		node.aabbMax = fmaxf(node.aabbMax, leafPrim.v2);
	}
}

void BVH::Subdivide(uint nodeIdx, std::vector<Primitive>& primitives)
{
	// terminate recursion
	BVHNode& node = bvhNode_[nodeIdx];
	if (node.count <= 2) return;
	// determine split axis and position
	float3 extent = node.aabbMax - node.aabbMin;
	int axis = 0;
	if (extent.y > extent.x) axis = 1;
	if (extent.z > extent[axis]) axis = 2;
	float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;
	// in-place partition
	int i = node.first;
	int j = i + node.count - 1;
	while (i <= j)
	{
		if (primitives[primIdx_[i]].objData.triangle.centroid[axis] < splitPos)
			i++;
		else
			swap(primIdx_[i], primIdx_[j--]);
	}
	// abort split if one of the sides is empty
	int leftCount = i - node.first;
	if (leftCount == 0 || leftCount == node.count) return;
	// create child nodes
	int leftChildIdx = nodesUsed_++;
	int rightChildIdx = nodesUsed_++;
	bvhNode_[leftChildIdx].first = node.first;
	bvhNode_[leftChildIdx].count = leftCount;
	bvhNode_[rightChildIdx].first = i;
	bvhNode_[rightChildIdx].count = node.count - leftCount;
	node.first = leftChildIdx;
	node.count = 0;
	UpdateNodeBounds(leftChildIdx, primitives);
	UpdateNodeBounds(rightChildIdx, primitives);
	// recurse
	Subdivide(leftChildIdx, primitives);
	Subdivide(rightChildIdx, primitives);
}


Buffer* BVH::GetBVHNodeBuffer()
{
	delete[] bvhNodeBuffer_;
	bvhNodeBuffer_ = new Buffer(sizeof(uint) * bvhNode_.size());
	bvhNodeBuffer_->hostBuffer = (uint*)bvhNode_.data();
	return bvhNodeBuffer_;
}

Buffer* BVH::GetIdxBuffer()
{
	delete[] primIdxBuffer_;
	primIdxBuffer_ = new Buffer(sizeof(uint) * primIdx_.size());
	primIdxBuffer_->hostBuffer = (uint*)primIdx_.data();
	return primIdxBuffer_;
}

void BVH::CopyToDevice()
{
	bvhNodeBuffer_->CopyToDevice();
	primIdxBuffer_->CopyToDevice();
}
