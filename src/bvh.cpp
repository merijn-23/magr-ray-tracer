#include "precomp.h"

#define DEBUG_BVH

void BVH::BuildBVH( std::vector<Primitive>& primitives )
{
	rootNodeIdx_ = 0;
	nodesUsed_ = 2; // 0 is root, 1 is for cache alignment

	N_ = primitives.size( );
	bvhNode_.resize( N_ * 2 );
	primIdx_.resize( N_ );
	printf( "Idx size: %i\n", N_ );
	// populate triangle index array
	for ( int i = 0; i < N_; i++ ) primIdx_[i] = i;

	printf( "Building BVH...\n" );
	BVHNode& root = bvhNode_[rootNodeIdx_];
	root.first = 0, root.count = N_;
	UpdateNodeBounds( rootNodeIdx_, primitives );
	// subdivide recursively
	Timer t;
	Subdivide( rootNodeIdx_, primitives );
	printf( "BVH constructed in %.2fms.\n", t.elapsed( ) * 1000 );
}

void BVH::Refit( std::vector<Primitive>& primitives )
{
	Timer t;
	for ( int i = nodesUsed_ - 1; i >= 0; i-- ) if ( i != 1 )
	{
		BVHNode& node = bvhNode_[i];
		if ( node.count > 0 )
		{
			// leaf node: adjust bounds to contained triangles
			UpdateNodeBounds( i, primitives );
			continue;
		}
		// interior node: adjust bounds to child node bounds
		BVHNode& leftChild = bvhNode_[node.first];
		BVHNode& rightChild = bvhNode_[node.first + 1];
		node.aabbMin = fminf( leftChild.aabbMin, rightChild.aabbMin );
		node.aabbMax = fmaxf( leftChild.aabbMax, rightChild.aabbMax );
	}
	printf( "BVH refitted in %.2fms  ", t.elapsed( ) * 1000 );
}

void BVH::UpdateNodeBounds( uint nodeIdx, std::vector<Primitive>& primitives )
{
#ifdef DEBUG_BVH
	printf( "Updating node bounds: %i\n", nodeIdx );
#endif

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
#ifdef DEBUG_BVH
	printf( "Updating triangle bounds: %i\n", node.first );
#endif
	node.aabbMin = fminf( node.aabbMin, triangle.v0 );
	node.aabbMin = fminf( node.aabbMin, triangle.v1 );
	node.aabbMin = fminf( node.aabbMin, triangle.v2 );
	node.aabbMax = fmaxf( node.aabbMax, triangle.v0 );
	node.aabbMax = fmaxf( node.aabbMax, triangle.v1 );
	node.aabbMax = fmaxf( node.aabbMax, triangle.v2 );
}

void BVH::UpdateSphereBounds( BVHNode& node, Sphere& sphere )
{
#ifdef DEBUG_BVH
	printf( "Updating sphere bounds: %i\n", node.first );
#endif
	node.aabbMin = sphere.pos - sphere.r;
	node.aabbMax = sphere.pos + sphere.r;
}

float BVH::FindBestSplitPlane( BVHNode& node, int& axis, float& splitPos, vector<Primitive> primitives )
{
	float bestCost = 1e30f;
	for ( int axis = 0; axis < 3; axis++ )
	{
		float boundsMin = 1e30f, boundsMax = -1e30f;
		for ( uint i = 0; i < node.count; i++ )
		{
			Primitive& prim = primitives[primIdx_[node.first + i]];
			switch ( prim.objType )
			{
				case TRIANGLE:
				{
					boundsMin = min( boundsMin, prim.objData.triangle.centroid[axis] );
					boundsMax = max( boundsMax, prim.objData.triangle.centroid[axis] );
				} break;
				case SPHERE:
				{
					boundsMin = min( boundsMin, prim.objData.sphere.pos[axis] );
					boundsMax = max( boundsMax, prim.objData.sphere.pos[axis] );
				} break;
				default: continue;
			}
		}
		if ( boundsMin == boundsMax ) continue;
		// populate the bins
		Bin bin[bins__];
		float scale = bins__ / ( boundsMax - boundsMin );
		for ( uint i = 0; i < node.count; i++ )
		{
			Primitive& prim = primitives[primIdx_[node.first + i]];
			switch ( prim.objType )
			{
				case TRIANGLE:
				{
					Triangle& tri = prim.objData.triangle;
					int binIdx = min( bins__ - 1, (int)( ( tri.centroid[axis] - boundsMin ) * scale ) );
					bin[binIdx].count++;
					bin[binIdx].bounds.Grow( tri.v0 );
					bin[binIdx].bounds.Grow( tri.v1 );
					bin[binIdx].bounds.Grow( tri.v2 );
				} break;
				case SPHERE:
				{
					Sphere& sphere = prim.objData.sphere;
					int binIdx = min( bins__ - 1, (int)( ( sphere.pos[axis] - boundsMin ) * scale ) );
					bin[binIdx].count++;
					bin[binIdx].bounds.Grow( sphere.pos - sphere.r );
					bin[binIdx].bounds.Grow( sphere.pos + sphere.r );
				}break;
			}
		}
		// gather data for the 7 planes between the 8 bins
		float leftArea[bins__ - 1], rightArea[bins__ - 1];
		int leftCount[bins__ - 1], rightCount[bins__ - 1];
		aabb leftBox, rightBox;
		int leftSum = 0, rightSum = 0;
		for ( int i = 0; i < bins__ - 1; i++ )
		{
			leftSum += bin[i].count;
			leftCount[i] = leftSum;
			leftBox.Grow( bin[i].bounds );
			leftArea[i] = leftBox.Area( );
			rightSum += bin[bins__ - 1 - i].count;
			rightCount[bins__ - 2 - i] = rightSum;
			rightBox.Grow( bin[bins__ - 1 - i].bounds );
			rightArea[bins__ - 2 - i] = rightBox.Area( );
		}
		// calculate SAH cost for the 7 planes
		scale = ( boundsMax - boundsMin ) / bins__;
		for ( int i = 0; i < bins__ - 1; i++ )
		{
			float planeCost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
			if ( planeCost < bestCost )
				axis = axis, splitPos = boundsMin + scale * ( i + 1 ), bestCost = planeCost;
		}
	}
	return bestCost;
}

float BVH::CalculateNodeCost( BVHNode& node )
{
	float3 e = node.aabbMax - node.aabbMin; // extent of the node
	float surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
	return node.count * surfaceArea;
}

void BVH::Subdivide( uint nodeIdx, std::vector<Primitive>& primitives )
{
#ifdef DEBUG_BVH
	printf( "Subdividing: %i\n", nodeIdx );
#endif
	// terminate recursion
	BVHNode& node = bvhNode_[nodeIdx];
	// determine split axis using SAH
	int axis;
	float splitPos;
	float splitCost = FindBestSplitPlane( node, axis, splitPos, primitives );
	float nosplitCost = CalculateNodeCost( node );
	if ( splitCost >= nosplitCost ) return;
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

//----------------------------------------------------------------------------
// TLAS
//----------------------------------------------------------------------------

TLAS::TLAS( std::vector<BVH>&& bvhList) : blas_(std::move(bvhList) )
{
	tlasNode_.resize( blas_.size( ) * 2 );
	nodesUsed_ = 2;
}
