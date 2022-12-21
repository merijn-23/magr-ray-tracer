#include "precomp.h"

BVH2::BVH2( std::vector<Primitive>& primitives ) : primitives_( primitives )
{
	rootNodeIdx_ = 0;
	count_ = primitives_.size( );
	nodes.resize( count_ * 2 );
	primIdx.resize( count_ );
	nodesUsed_ = 1;
	Build( );
	printf( "Nodes used: %i\n", nodesUsed_ );
	printf( "Depth: %i\n", Depth( Root( ) ) );
	printf( "Count after: %i\n", Count( Root( ) ) );
	printf( "Count left after: %i\n", Count( Left( Root( ) ) ) );
	printf( "Count right after: %i\n", Count( Right( Root( ) ) ) );
	BVHNode2 first = nodes[1];
	BVHNode2 right = nodes[2];
	printf( "left aabb: min %f %f %f, max %f %f %f\n", first.aabbMin.x, first.aabbMin.y, first.aabbMin.z, first.aabbMax.x, first.aabbMax.y, first.aabbMax.z );
	printf( "right aabb: min %f %f %f, max %f %f %f\n", right.aabbMin.x, right.aabbMin.y, right.aabbMin.z, right.aabbMax.x, right.aabbMax.y, right.aabbMax.z );
}

uint BVH2::Depth( const BVHNode2& node )
{
	if ( node.count > 0 ) return 0;
	else {
		uint lDepth = Depth( nodes[node.first] );
		uint rDepth = Depth( nodes[node.first + 1] );
		if ( lDepth > rDepth ) return ( lDepth + 1 );
		else return ( rDepth + 1 );
	}
}

uint BVH2::Count( const BVHNode2& node )
{
	if ( node.count > 0 ) return node.count;
	else {
		uint lcount = Count( nodes[node.first] );
		uint rcount = Count( nodes[node.first + 1] );
		return lcount + rcount;
	}
}

void BVH2::Build( )
{
	// populate triangle index array
	for ( int i = 0; i < count_; i++ ) primIdx[i] = i;
	printf( "Building BVH...\n" );
	BVHNode2& root = nodes[rootNodeIdx_];
	root.first = rootNodeIdx_, root.count = count_;
	UpdateNodeBounds( rootNodeIdx_ );
	// subdivide recursively
	Timer t;
	Subdivide( rootNodeIdx_ );
	printf( "BVH constructed in %.2fms.\n", t.elapsed( ) * 1000 );
}

void BVH2::UpdateNodeBounds( uint nodeIdx )
{
	BVHNode2& node = nodes[nodeIdx];
	node.aabbMin = float3( 1e30f );
	node.aabbMax = float3( -1e30f );
	for ( uint first = node.first, i = 0; i < node.count; i++ ) {
		Primitive& prim = primitives_[primIdx[first + i]];
		switch ( prim.objType ) {
			case TRIANGLE: UpdateTriangleBounds( node, prim.objData.triangle ); break;
			case SPHERE: UpdateSphereBounds( node, prim.objData.sphere ); break;
		}
	}
}

void BVH2::UpdateTriangleBounds( BVHNode2& node, Triangle& triangle )
{
	node.aabbMin = fminf( node.aabbMin, triangle.v0 );
	node.aabbMin = fminf( node.aabbMin, triangle.v1 );
	node.aabbMin = fminf( node.aabbMin, triangle.v2 );
	node.aabbMax = fmaxf( node.aabbMax, triangle.v0 );
	node.aabbMax = fmaxf( node.aabbMax, triangle.v1 );
	node.aabbMax = fmaxf( node.aabbMax, triangle.v2 );
}

void BVH2::UpdateSphereBounds( BVHNode2& node, Sphere& sphere )
{
	node.aabbMin = fminf( node.aabbMin, sphere.pos - sphere.r );
	node.aabbMax = fmaxf( node.aabbMax, sphere.pos + sphere.r );
}

float BVH2::FindBestSplitPlane( BVHNode2& node, int& axis, float& splitPos )
{
	float bestCost = 1e30f;
	for ( int a = 0; a < 3; a++ ) {
		float boundsMin = 1e30f, boundsMax = -1e30f;
		for ( uint i = 0; i < node.count; i++ ) {
			Primitive& prim = primitives_[primIdx[node.first + i]];
			switch ( prim.objType ) {
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
		for ( uint i = 0; i < node.count; i++ ) {
			Primitive& prim = primitives_[primIdx[node.first + i]];
			switch ( prim.objType ) {
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
		for ( int i = 0; i < bins__ - 1; i++ ) {
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
		for ( int i = 0; i < bins__ - 1; i++ ) {
			float planeCost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
			if ( planeCost < bestCost )
				axis = a, splitPos = boundsMin + scale * ( i + 1 ), bestCost = planeCost;
		}
	}
	return bestCost;
}

float BVH2::CalculateNodeCost( BVHNode2& node )
{
	float3 e = node.aabbMax - node.aabbMin; // extent of the node
	float surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
	return node.count * surfaceArea;
}

void BVH2::Subdivide( uint nodeIdx )
{
#ifdef DEBUG_BVH
	printf( "Subdividing: %i\n", nodeIdx );
	printf( "Subdivisions: %i\n", subdivisions_++ );
#endif
	// terminate recursion
	BVHNode2& node = nodes[nodeIdx];
	// determine split axis using SAH
	int axis;
	float splitPos;
	float splitCost = FindBestSplitPlane( node, axis, splitPos );
	float nosplitCost = CalculateNodeCost( node );
	if ( splitCost >= nosplitCost ) return;
	// in-place partition
	int i = node.first;
	int j = i + node.count - 1;
	//cout << "SUBDIVIDE 195/ node.count = " << node.count << endl;
	while ( i <= j ) {
		float center = 0;
		Primitive& prim = primitives_[primIdx[i]];
		switch ( prim.objType ) {
			case TRIANGLE: center = prim.objData.triangle.centroid[axis]; break;
			case SPHERE: center = prim.objData.sphere.pos[axis]; break;
			default: continue;
		}
		if ( center < splitPos ) i++;
		else swap( primIdx[i], primIdx[j--] );
	}
	// abort split if one of the sides is empty
	int leftCount = i - node.first;
	if ( leftCount == 0 || leftCount == node.count ) return;
	// create child nodes
	int leftChildIdx = nodesUsed_++;
	int rightChildIdx = nodesUsed_++;
	nodes[leftChildIdx].first = node.first;
	nodes[leftChildIdx].count = leftCount;
	nodes[rightChildIdx].first = i;
	nodes[rightChildIdx].count = node.count - leftCount;
	node.first = leftChildIdx;
	//cout << "SUBDIVIDE 222/ node.count = " << node.count << endl;
	node.count = 0;
	UpdateNodeBounds( leftChildIdx );
	UpdateNodeBounds( rightChildIdx );
	// recurse
	Subdivide( leftChildIdx );
	Subdivide( rightChildIdx );
}

BVH4::BVH4( BVH2& _bvh2 ) : bvh2( _bvh2 )
{
	rootNodeIdx_ = 0;
#if 0 // debugging
	bvh2.nodes.clear( );
	bvh2.nodes.resize( 13 );

	int bb0 = 20;
	bvh2.nodes[0].aabbMin = float3( -bb0 );
	bvh2.nodes[0].aabbMax = float3( bb0 );
	bvh2.nodes[0].count = 0;
	bvh2.nodes[0].first = 1;

	int bb1 = 9;
	bvh2.nodes[1].aabbMin = float3( -bb1 );
	bvh2.nodes[1].aabbMax = float3( bb1 );
	bvh2.nodes[1].count = 0;
	bvh2.nodes[1].first = 3;

	int bb2 = 12;
	bvh2.nodes[2].aabbMin = float3( -bb2 );
	bvh2.nodes[2].aabbMax = float3( bb2 );
	bvh2.nodes[2].count = 0;
	bvh2.nodes[2].first = 5;

	int bb3 = 8;
	bvh2.nodes[3].aabbMin = float3( -bb3 );
	bvh2.nodes[3].aabbMax = float3( bb3 );
	bvh2.nodes[3].count = 0;
	bvh2.nodes[3].first = 7;

	bvh2.nodes[4].count = 4;
	bvh2.nodes[4].first = 4;

	bvh2.nodes[5].count = 5;
	bvh2.nodes[5].first = 5;

	int bb6 = 10;
	bvh2.nodes[6].aabbMin = float3( -bb6 );
	bvh2.nodes[6].aabbMax = float3( bb6 );
	bvh2.nodes[6].count = 0;
	bvh2.nodes[6].first = 9;

	bvh2.nodes[7].first = 7;
	bvh2.nodes[7].count = 7;
	bvh2.nodes[8].first = 8;
	bvh2.nodes[8].count = 8;

	int bb13 = 9;
	bvh2.nodes[9].aabbMin = float3( -bb13 );
	bvh2.nodes[9].aabbMax = float3( bb13 );
	bvh2.nodes[9].count = 0;
	bvh2.nodes[9].first = 11;

	bvh2.nodes[10].count = 10;
	bvh2.nodes[10].first = 10;

	bvh2.nodes[11].count = 11;
	bvh2.nodes[11].first = 11;

	bvh2.nodes[12].count = 12;
	bvh2.nodes[12].count = 12;
#endif
	Convert( );
	int i = 0;
}

uint BVH4::Depth( BVHNode4 node )
{
	return 0;
	if ( node.count > 0 ) return 0;
	else {
		uint depth[4];
		depth[0] = Depth( nodes[node.first[0]] );
		depth[1] = Depth( nodes[node.first[1]] );
		depth[2] = Depth( nodes[node.first[2]] );
		depth[3] = Depth( nodes[node.first[3]] );

		for ( int i = 0; i < 4; i++ ) for ( int j = 0; j < 3; j++ )
			if ( depth[j] < depth[i] ) {
				float d = depth[i]; depth[i] = depth[j]; depth[j] = d;
			}
		return depth[0];
	}
}

uint BVH4::Count( BVHNode4 node )
{
	int count = 0;
	for ( size_t i = 0; i < 4; i++ )
		if ( node.count[i] > 0 ) count += node.count[i];
		else count += Count( nodes[node.first[i]] );
	return count;
}

// https://github.com/jan-van-bergen/GPU-Raytracer/blob/master/Src/BVH/Converters/BVH4Converter.cpp
void BVH4::Convert( )
{
	nodes.resize( bvh2.nodes.size( ) );
	for ( size_t i = 0; i < nodes.size( ); i++ ) {
		int _acurr = i;
		int _childl = bvh2.nodes[i].first;
		int _childr = bvh2.nodes[i].first + 1;
		if ( bvh2.nodes[i].count > 0 )  continue;
		const BVHNode2& childLeft = bvh2.nodes[bvh2.nodes[i].first];
		const BVHNode2& childRight = bvh2.nodes[bvh2.nodes[i].first + 1];
		nodes[i].aabbMin[0] = childLeft.aabbMin;
		nodes[i].aabbMax[0] = childLeft.aabbMax;
		nodes[i].aabbMin[1] = childRight.aabbMin;
		nodes[i].aabbMax[1] = childRight.aabbMax;
		if ( childLeft.count > 0 ) {
			nodes[i].first[0] = childLeft.first;
			nodes[i].count[0] = childLeft.count;
		} else {
			nodes[i].first[0] = bvh2.nodes[i].first;
			nodes[i].count[0] = 0;
		}
		if ( childRight.count > 0 ) {
			nodes[i].first[1] = childRight.first;
			nodes[i].count[1] = childRight.count;
		} else {
			nodes[i].first[1] = bvh2.nodes[i].first + 1;
			nodes[i].count[1] = 0;
		}
		// fill the other two nodes
		for ( size_t j = 2; j < 4; j++ ) {
			nodes[i].first[j] = INVALID;
			nodes[i].count[j] = INVALID;
		}
	}
	// handle special case where the root is a leaf
	if ( bvh2.nodes[0].count > 0 ) {
		nodes[0].aabbMin[0] = bvh2.nodes[0].aabbMin;
		nodes[0].aabbMax[0] = bvh2.nodes[0].aabbMax;
		nodes[0].first[0] = bvh2.nodes[0].first;
		nodes[0].count[0] = bvh2.nodes[0].count;
		for ( size_t i = 0; i < 4; i++ ) {
			nodes[0].first[0] = INVALID;
			nodes[0].count[0] = INVALID;
		}
	} else {
		Collapse( 0 );
	}
}

// https://github.com/jan-van-bergen/GPU-Raytracer/blob/master/Src/BVH/Converters/BVH4Converter.cpp
void BVH4::Collapse( int index )
{
	BVHNode4& node = nodes[index];
	while ( true ) {
		int N_n = GetChildCount( node );
		float maxArea = -INFINITY;
		int maxIndex = INVALID;
		for ( size_t i = 0; i < N_n; i++ ) {
			if ( node.count[i] > 0 ) continue;
			int N_ci = GetChildCount( nodes[node.first[i]] );
			// check if the current can adopt the children of child node i
			if ( !( N_n - 1 + N_ci <= 4 ) ) continue;
			float diffX = node.aabbMax[i].x - node.aabbMin[i].x;
			float diffY = node.aabbMax[i].y - node.aabbMin[i].y;
			float diffZ = node.aabbMax[i].z - node.aabbMin[i].z;
			float halfArea = diffX * diffY + diffY * diffZ + diffZ * diffX;
			if ( halfArea > maxArea ) {
				maxArea = halfArea;
				maxIndex = i;
			}
		}
		if ( maxIndex == INVALID ) {
			break;
		}
		const BVHNode4& maxChild = nodes[node.first[maxIndex]];
		// replace max child with its first child
		node.aabbMin[maxIndex] = maxChild.aabbMin[0];
		node.aabbMax[maxIndex] = maxChild.aabbMax[0];
		node.first[maxIndex] = maxChild.first[0];
		node.count[maxIndex] = maxChild.count[0];
		// add the reset of maxChild children to current node children
		int maxChild_ChildCount = GetChildCount( maxChild );
		for ( size_t i = 1; i < maxChild_ChildCount; i++ ) {
			node.aabbMin[N_n - 1 + i] = maxChild.aabbMin[i];
			node.aabbMax[N_n - 1 + i] = maxChild.aabbMax[i];
			node.first[N_n - 1 + i] = maxChild.first[i];
			node.count[N_n - 1 + i] = maxChild.count[i];
		}
	}

	for ( size_t i = 0; i < 4; i++ ) {
		if ( node.count[i] == INVALID ) break;
		if ( node.count[i] == 0 ) Collapse( node.first[i] );
	}
}

int BVH4::GetChildCount( const BVHNode4& node ) const
{
	int result = 0;
	for ( int i = 0; i < 4; i++ ) {
		if ( node.count[i] == INVALID ) break;
		result++;
	}
	return result;
}
