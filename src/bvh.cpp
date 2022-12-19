#include "precomp.h"

//#define DEBUG_BVH

BVH::BVH( std::vector<Primitive>& primitives ) : primitives_( primitives )
{
	rootNodeIdx_ = 0;
	count_ = primitives_.size( );
	printf( "Count before: %i\n", count_ );
	bvhNode.resize( count_ * 2 );
	bvhIdx.resize( count_ );
	nodesUsed_ = 1; // 0 is root, 1 is for cache alignment
	Build( );
	printf( "Nodes used: %i\n", nodesUsed_ );
	printf( "Depth: %i\n", Depth( Root()));
	printf( "Count after: %i\n", Count(Root()));
	BVHNode left = bvhNode[bvhNode[Root().left + 1].left];
	BVHNode right = bvhNode[bvhNode[Root().left + 1].left + 1];
	printf( "left aabb: min %f %f %f, max %f %f %f\n", left.aabbMin.x, left.aabbMin.y, left.aabbMin.z, left.aabbMax.x, left.aabbMax.y, left.aabbMax.z );
	printf( "right aabb: min %f %f %f, max %f %f %f\n", right.aabbMin.x, right.aabbMin.y, right.aabbMin.z, right.aabbMax.x, right.aabbMax.y, right.aabbMax.z );
	//for( auto& node : bvhNode ) if ( node.count > 0 ) cout << "node count " << node.count << endl;
}

uint BVH::Depth( BVHNode node )
{
	if ( node.count > 0 ) return 0;
	else
	{
		uint lDepth = Depth( bvhNode[node.left] );
		uint rDepth = Depth( bvhNode[node.left + 1] );
		if ( lDepth > rDepth ) return ( lDepth + 1 );
		else return ( rDepth + 1 );
	}
}

uint BVH::Count( BVHNode node )
{
	if (node.count > 0) return node.count;
	else
	{
		uint lcount = Count( bvhNode[node.left] );
		uint rcount = Count( bvhNode[node.left+1] );
		return lcount + rcount;
	}
}

void BVH::Build( )
{	
	// populate triangle index array
	for ( int i = 0; i < count_; i++ ) bvhIdx[i] = i;
	printf( "Building BVH...\n" );
	BVHNode& root = bvhNode[rootNodeIdx_];
	root.left = rootNodeIdx_, root.count = count_;
	UpdateNodeBounds( rootNodeIdx_ );
	// subdivide recursively
	Timer t;
	Subdivide( rootNodeIdx_ );
	printf( "BVH constructed in %.2fms.\n", t.elapsed( ) * 1000 );
}

void BVH::UpdateNodeBounds( uint nodeIdx )
{
	BVHNode& node = bvhNode[nodeIdx];
	node.aabbMin = float3( 1e30f );
	node.aabbMax = float3( -1e30f );
	for ( uint left = node.left, i = 0; i < node.count; i++ )
	{
		Primitive& prim = primitives_[bvhIdx[left + i]];
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
	node.aabbMin = fminf( node.aabbMin, sphere.pos - sphere.r );
	node.aabbMax = fmaxf( node.aabbMax, sphere.pos + sphere.r );
}

float BVH::FindBestSplitPlane( BVHNode& node, int& axis, float& splitPos)
{
	float bestCost = 1e30f;
	for ( int a = 0; a < 3; a++ )
	{
		float boundsMin = 1e30f, boundsMax = -1e30f;
		for ( uint i = 0; i < node.count; i++ )
		{
			Primitive& prim = primitives_[bvhIdx[node.left + i]];
			switch ( prim.objType )
			{
				case TRIANGLE:
				{
					boundsMin = min( boundsMin, prim.objData.triangle.centroid[a] );
					boundsMax = max( boundsMax, prim.objData.triangle.centroid[a] );
				} break;
				case SPHERE:
				{
					boundsMin = min( boundsMin, prim.objData.sphere.pos[a] );
					boundsMax = max( boundsMax, prim.objData.sphere.pos[a] );
				} break;
				default: continue;
			}
		}
		if ( boundsMin == boundsMax ) continue;
		// populate the bins
		struct Bin { aabb bounds; int count = 0; } bin[bins__];
		float scale = bins__ / ( boundsMax - boundsMin );
		for ( uint i = 0; i < node.count; i++ )
		{
			Primitive& prim = primitives_[bvhIdx[node.left + i]];
			switch ( prim.objType )
			{
				case TRIANGLE:
				{
					Triangle& tri = prim.objData.triangle;
					int binIdx = min( bins__ - 1, (int)( ( tri.centroid[a] - boundsMin ) * scale ) );
					bin[binIdx].count++;
					bin[binIdx].bounds.Grow( tri.v0 );
					bin[binIdx].bounds.Grow( tri.v1 );
					bin[binIdx].bounds.Grow( tri.v2 );
				} break;
				case SPHERE:
				{
					Sphere& sphere = prim.objData.sphere;
					int binIdx = min( bins__ - 1, (int)( ( sphere.pos[a] - boundsMin ) * scale ) );
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
				axis = a, splitPos = boundsMin + scale * ( i + 1 ), bestCost = planeCost;
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

void BVH::Subdivide( uint nodeIdx)
{
#ifdef DEBUG_BVH
	printf( "Subdividing: %i\n", nodeIdx );
	printf( "Subdivisions: %i\n", subdivisions_++ );
#endif
	// terminate recursion
	BVHNode& node = bvhNode[nodeIdx];
	// determine split axis using SAH
	int axis;
	float splitPos;
	float splitCost = FindBestSplitPlane( node, axis, splitPos);
	float nosplitCost = CalculateNodeCost( node );
	if ( splitCost >= nosplitCost ) return;
	// in-place partition
	int i = node.left;
	int j = i + node.count - 1;
	//cout << "SUBDIVIDE 195/ node.count = " << node.count << endl;
	while ( i <= j )
	{
		float center = 0;
		Primitive& prim = primitives_[bvhIdx[i]];
		switch ( prim.objType )
		{
			case TRIANGLE: center = prim.objData.triangle.centroid[axis]; break;
			case SPHERE: center = prim.objData.sphere.pos[axis]; break;
			default: continue;
		}
		if ( center < splitPos ) i++;
		else swap( bvhIdx[i], bvhIdx[j--] );
	}
	// abort split if one of the sides is empty
	int leftCount = i - node.left;
	if ( leftCount == 0 || leftCount == node.count ) return;
	// create child nodes
	int leftChildIdx = nodesUsed_++;
	int rightChildIdx = nodesUsed_++;
	bvhNode[leftChildIdx].left = node.left;
	bvhNode[leftChildIdx].count = leftCount;
	bvhNode[rightChildIdx].left = i;
	bvhNode[rightChildIdx].count = node.count - leftCount;
	node.left = leftChildIdx;
	//cout << "SUBDIVIDE 222/ node.count = " << node.count << endl;
	node.count = 0;
	UpdateNodeBounds( leftChildIdx);
	UpdateNodeBounds( rightChildIdx);
	// recurse
	Subdivide( leftChildIdx);
	Subdivide( rightChildIdx);
}
