#include "precomp.h"

void BVH::BuildBvh( std::vector<Primitive>& primitives )
{
	N_ = primitives.size( );
	bvhNode_.resize( N_ * 2 );
	primIdx_.resize( N_ );
	// populate triangle index array
	for ( int i = 0; i < N_; i++ ) primIdx_[i] = i;

	BVHNode& root = bvhNode_[rootNodeIdx_];
	root.first = 0, root.count = N_;
	UpdateNodeBounds( rootNodeIdx_, primitives );
	// subdivide recursively
	Timer t;
	Subdivide( rootNodeIdx_, primitives );
	printf( "BVH constructed in %.2fms.\n", t.elapsed( ) * 1000 );
}

void BVH::UpdateNodeBounds( uint nodeIdx, std::vector<Primitive>& primitives )
{
	BVHNode& node = bvhNode_[nodeIdx];
	node.aabbMin = float3( 1e30f );
	node.aabbMax = float3( -1e30f );
	for ( uint first = node.first, i = 0; i < node.count; i++ )
	{
		uint leafPrimIdx = primIdx_[first + i];
		Primitive& prim = primitives[leafPrimIdx];
		switch ( prim.objType )
		{
			case TRIANGLE: UpdateTriangleBounds( node, prim.objData.triangle ); break;
			case SPHERE: UpdateSphereBounds( node, prim.objData.sphere ); break;
		}
	}
}

void BVH::UpdateTriangleBounds( BVHNode& node, Triangle& triangle )
{
	node.aabbMin = fminf( node.aabbMin, triangle.v0 );
	node.aabbMin = fminf( node.aabbMin, triangle.v1 );
	node.aabbMin = fminf( node.aabbMin, triangle.v2 );
	node.aabbMax = fmaxf( node.aabbMax, triangle.v0 );
	node.aabbMax = fmaxf( node.aabbMax, triangle.v1 );
	node.aabbMax = fmaxf( node.aabbMax, triangle.v2 );
}

void BVH::UpdateSphereBounds( BVHNode& node, Sphere& sphere )
{
	node.aabbMin = sphere.pos - sphere.r;
	node.aabbMax = sphere.pos + sphere.r;
}

float BVH::EvaluateSAH( BVHNode& node, int axis, float pos, std::vector<Primitive>& primitives )
{
	// determine triangle counts and bounds for this split candidate
	aabb leftBox, rightBox;
	int leftCount = 0, rightCount = 0;
	for ( uint i = 0; i < node.count; i++ )
	{
		Primitive& prim = primitives[primIdx_[node.first + i]];
		switch ( prim.objType )
		{
			case TRIANGLE: 
			{
				Triangle& tri = prim.objData.triangle;
				if ( tri.centroid[axis] < pos )
				{
					leftCount++;
					leftBox.Grow( tri.v0 );
					leftBox.Grow( tri.v1 );
					leftBox.Grow( tri.v2 );
				}
				else
				{
					rightCount++;
					rightBox.Grow( tri.v0 );
					rightBox.Grow( tri.v1 );
					rightBox.Grow( tri.v2 );
				}
			} break;
			case SPHERE:
			{
				Sphere& sphere = prim.objData.sphere;
				if ( sphere.pos[axis] < pos )
				{
					leftCount++;
					leftBox.Grow( sphere.pos - sphere.r );
				}
				else
				{
					rightCount++;
					rightBox.Grow( sphere.pos + sphere.r );
				}
			}break;
			default: continue;
		}

		
	}
	float cost = leftCount * leftBox.Area( ) + rightCount * rightBox.Area( );
	return cost > 0 ? cost : 1e30f;
}

void BVH::Subdivide( uint nodeIdx, std::vector<Primitive>& primitives )
{
	// terminate recursion
	BVHNode& node = bvhNode_[nodeIdx];
	if ( node.count <= 2 ) return;
	// determine split axis using SAH
	int bestAxis = -1;
	float bestPos = 0, bestCost = 1e30f;
	for ( int axis = 0; axis < 3; axis++ ) for ( uint i = 0; i < node.count; i++ )
	{
		Primitive& prim = primitives[primIdx_[node.first + i]];
		float candidatePos;
		switch ( prim.objType )
		{
			case TRIANGLE: candidatePos = prim.objData.triangle.centroid[axis]; break;
			case SPHERE: candidatePos = prim.objData.sphere.pos[axis]; break;
			default: continue;
		}
		
		float cost = EvaluateSAH( node, axis, candidatePos, primitives );
		if ( cost < bestCost )
			bestPos = candidatePos, bestAxis = axis, bestCost = cost;
	}
	int axis = bestAxis;
	float splitPos = bestPos;
	float3 e = node.aabbMax - node.aabbMin; // extent of parent
	float parentArea = e.x * e.y + e.y * e.z + e.z * e.x;
	float parentCost = node.count * parentArea;
	if ( bestCost >= parentCost ) return;
	// in-place partition
	int i = node.first;
	int j = i + node.count - 1;
	while ( i <= j )
	{
		float center = 0;
		Primitive& prim = primitives[primIdx_[i]];
		switch ( prim.objType )
		{
			case TRIANGLE: center = prim.objData.triangle.centroid[axis]; break;
			case SPHERE: center = prim.objData.sphere.pos[axis]; break;
			default: continue; 
		}

		if ( center < splitPos )
			i++;
		else
			swap( primIdx_[i], primIdx_[j--] );
	}
	// abort split if one of the sides is empty
	int leftCount = i - node.first;
	if ( leftCount == 0 || leftCount == node.count ) return;
	// create child nodes
	int leftChildIdx = nodesUsed_++;
	int rightChildIdx = nodesUsed_++;
	bvhNode_[leftChildIdx].first = node.first;
	bvhNode_[leftChildIdx].count = leftCount;
	bvhNode_[rightChildIdx].first = i;
	bvhNode_[rightChildIdx].count = node.count - leftCount;
	node.first = leftChildIdx;
	node.count = 0;
	UpdateNodeBounds( leftChildIdx, primitives );
	UpdateNodeBounds( rightChildIdx, primitives );
	// recurse
	Subdivide( leftChildIdx, primitives );
	Subdivide( rightChildIdx, primitives );
}

Buffer* BVH::GetBVHNodeBuffer( )
{
	delete[] bvhNodeBuffer_;
	bvhNodeBuffer_ = new Buffer( sizeof( uint ) * bvhNode_.size( ) );
	bvhNodeBuffer_->hostBuffer = (uint*)bvhNode_.data( );
	return bvhNodeBuffer_;
}

Buffer* BVH::GetIdxBuffer( )
{
	delete[] primIdxBuffer_;
	primIdxBuffer_ = new Buffer( sizeof( uint ) * primIdx_.size( ) );
	primIdxBuffer_->hostBuffer = (uint*)primIdx_.data( );
	return primIdxBuffer_;
}

void BVH::CopyToDevice( )
{
	bvhNodeBuffer_->CopyToDevice( );
	primIdxBuffer_->CopyToDevice( );
}
