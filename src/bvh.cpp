#include "precomp.h"

//#define DEBUG_BVH

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
	BVHNode2 left = nodes[1];
	BVHNode2 right = nodes[2];
	printf( "left aabb: min %f %f %f, max %f %f %f\n", left.aabbMin.x, left.aabbMin.y, left.aabbMin.z, left.aabbMax.x, left.aabbMax.y, left.aabbMax.z );
	printf( "right aabb: min %f %f %f, max %f %f %f\n", right.aabbMin.x, right.aabbMin.y, right.aabbMin.z, right.aabbMax.x, right.aabbMax.y, right.aabbMax.z );
}

uint BVH2::Depth( const BVHNode2& node )
{
	if ( node.count > 0 ) return 0;
	else {
		uint lDepth = Depth( nodes[node.left] );
		uint rDepth = Depth( nodes[node.left + 1] );
		if ( lDepth > rDepth ) return ( lDepth + 1 );
		else return ( rDepth + 1 );
	}
}

uint BVH2::Count( const BVHNode2& node )
{
	if ( node.count > 0 ) return node.count;
	else {
		uint lcount = Count( nodes[node.left] );
		uint rcount = Count( nodes[node.left + 1] );
		return lcount + rcount;
	}
}

void BVH2::Build( )
{
	// populate triangle index array
	for ( int i = 0; i < count_; i++ ) primIdx[i] = i;
	printf( "Building BVH...\n" );
	BVHNode2& root = nodes[rootNodeIdx_];
	root.left = rootNodeIdx_, root.count = count_;
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
	for ( uint left = node.left, i = 0; i < node.count; i++ ) {
		Primitive& prim = primitives_[primIdx[left + i]];
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
			Primitive& prim = primitives_[primIdx[node.left + i]];
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
			Primitive& prim = primitives_[primIdx[node.left + i]];
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
	int i = node.left;
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
	int leftCount = i - node.left;
	if ( leftCount == 0 || leftCount == node.count ) return;
	// create child nodes
	int leftChildIdx = nodesUsed_++;
	int rightChildIdx = nodesUsed_++;
	nodes[leftChildIdx].left = node.left;
	nodes[leftChildIdx].count = leftCount;
	nodes[rightChildIdx].left = i;
	nodes[rightChildIdx].count = node.count - leftCount;
	node.left = leftChildIdx;
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
	Convert( );
	MakeValidNodes( );
}

void BVH4::MakeValidNodes( )
{
	nodes.resize( invalidNodes_.size( ) );
	for ( int i = 0; i < invalidNodes_.size( ); i++ ) {
		nodes[i].aabbMin0 = invalidNodes_[i].aabbMin[0];
		nodes[i].aabbMax0 = invalidNodes_[i].aabbMax[0];
		nodes[i].aabbMin1 = invalidNodes_[i].aabbMin[1];
		nodes[i].aabbMax1 = invalidNodes_[i].aabbMax[1];
		nodes[i].aabbMin2 = invalidNodes_[i].aabbMin[2];
		nodes[i].aabbMax2 = invalidNodes_[i].aabbMax[2];
		nodes[i].aabbMin3 = invalidNodes_[i].aabbMin[3];
		nodes[i].aabbMax3 = invalidNodes_[i].aabbMax[3];
		nodes[i].left0 = invalidNodes_[i].left[0];
		nodes[i].left1 = invalidNodes_[i].left[1];
		nodes[i].left2 = invalidNodes_[i].left[2];
		nodes[i].left3 = invalidNodes_[i].left[3];
		nodes[i].count0 = invalidNodes_[i].count[0];
		nodes[i].count1 = invalidNodes_[i].count[1];
		nodes[i].count2 = invalidNodes_[i].count[2];
		nodes[i].count3 = invalidNodes_[i].count[3];
	}
}

uint BVH4::Depth( BVHNode4 node )
{
	return 0;
	/*if ( node.count > 0 ) return 0;
	else
	{
		uint lDepth = Depth( nodes[node.left] );
		uint rDepth = Depth( nodes[node.left + 1] );
		if ( lDepth > rDepth ) return ( lDepth + 1 );
		else return ( rDepth + 1 );
	}*/
}

uint BVH4::Count( BVHNode4 node )
{
	return 0;
	/*int count = 0;
	if ( node.count[0] > 0 ) return node.count[0];
	else
	{
		uint lcount = Count( nodes[node.left[0]] );
		uint rcount = Count( nodes[node.left[0] + 1] );
		count += lcount + rcount;
	}*/
}

// https://github.com/jan-van-bergen/GPU-Raytracer/blob/master/Src/BVH/Converters/BVH4Converter.cpp#L75
void BVH4::Convert( )
{
	invalidNodes_.resize( bvh2.nodes.size( ) );
	for ( size_t i = 0; i < nodes.size( ); i++ ) {
		// We use index 1 as a starting point, such that it points to the first child of the root
		if ( i == 1 ) {
			invalidNodes_[i].left[0] = 0;
			invalidNodes_[i].count[0] = 0;
			continue;
		}
		if ( !bvh2.nodes[i].count > 0 ) {
			const BVHNode2& child_left = bvh2.nodes[bvh2.nodes[i].left];
			const BVHNode2& child_right = bvh2.nodes[bvh2.nodes[i].left + 1];
			invalidNodes_[i].aabbMin[0] = child_left.aabbMin;
			invalidNodes_[i].aabbMax[0] = child_left.aabbMax;
			invalidNodes_[i].aabbMin[1] = child_right.aabbMin;
			invalidNodes_[i].aabbMax[1] = child_right.aabbMax;
			if ( child_left.count > 0 ) {
				invalidNodes_[i].left[0] = child_left.left;
				invalidNodes_[i].count[0] = child_left.count;
			} else {
				invalidNodes_[i].left[0] = bvh2.nodes[i].left;
				invalidNodes_[i].count[0] = 0;
			}
			if ( child_right.count > 0 ) {
				invalidNodes_[i].left[1] = child_right.left;
				invalidNodes_[i].count[1] = child_right.count;
			} else {
				invalidNodes_[i].left[1] = bvh2.nodes[i].left + 1;
				invalidNodes_[i].count[1] = 0;
			}
			// For now the tree is binary,
			// so make the rest of the indices invalid
			for ( int j = 2; j < 4; j++ ) {
				invalidNodes_[i].left[j] = INVALID;
				invalidNodes_[i].count[j] = INVALID;
			}
		}
	}
	// Handle the special case where the root is a leaf
	if ( bvh2.nodes[0].count > 0 ) {
		invalidNodes_[0].aabbMin[0] = bvh2.nodes[0].aabbMin;
		invalidNodes_[0].aabbMax[0] = bvh2.nodes[0].aabbMax;
		invalidNodes_[0].left[0] = bvh2.nodes[0].left;
		invalidNodes_[0].count[0] = bvh2.nodes[0].count;

		for ( int i = 1; i < 4; i++ ) {
			invalidNodes_[0].left[i] = INVALID;
			invalidNodes_[0].count[i] = INVALID;
		}
	} else {
		// Collapse tree top-down, starting from the root
		Collapse( rootNodeIdx_ );
	}
	primIdx = bvh2.primIdx;
}

void BVH4::Collapse( int index )
{
	BVHNode4_Invalid& node = invalidNodes_[index];
	while ( true ) {
		int child_count = GetChildCount( node );
		// Look for adoptable child with the largest surface area
		float max_area = -INFINITY;
		int   max_index = INVALID;
		for ( int i = 0; i < child_count; i++ ) if ( !( node.count[i] > 0 ) ) {
			int child_i_child_count = GetChildCount( invalidNodes_[node.left[i]] );
			// Check if the current Node can adopt the children of child Node i
			if ( child_count + child_i_child_count - 1 <= 4 ) {
				float diff_x = node.aabbMax[i].x - node.aabbMin[i].x;
				float diff_y = node.aabbMax[i].y - node.aabbMin[i].y;
				float diff_z = node.aabbMax[i].z - node.aabbMin[i].z;
				float half_area = diff_x * diff_y + diff_y * diff_z + diff_z * diff_x;
				if ( half_area > max_area ) {
					max_area = half_area;
					max_index = i;
				}
			}
		}
		// No merge possible anymore, stop trying
		if ( max_index == INVALID ) break;
		const BVHNode4_Invalid& max_child = invalidNodes_[node.left[max_index]];
		// Replace max child Node with its first child
		node.aabbMin[max_index] = max_child.aabbMin[0];
		node.aabbMax[max_index] = max_child.aabbMax[0];
		node.left[max_index] = max_child.left[0];
		node.count[max_index] = max_child.count[0];
		int max_child_child_count = GetChildCount( max_child );
		// Add the rest of max child Node's children after the current Node's own children
		for ( int i = 1; i < max_child_child_count; i++ ) {
			node.aabbMin[child_count + i - 1] = max_child.aabbMin[i];
			node.aabbMax[child_count + i - 1] = max_child.aabbMax[i];
			node.left[child_count + i - 1] = max_child.left[i];
			node.count[child_count + i - 1] = max_child.count[i];
		}
	};
	for ( int i = 0; i < 4; i++ ) {
		if ( node.count[i] == INVALID ) break;
		// If child Node i is an internal node, recurse
		if ( node.count[i] == 0 ) Collapse( node.left[i] );
	}
}

int BVH4::GetChildCount( const BVHNode4_Invalid& node ) const
{
	int result = 0;
	for ( int i = 0; i < 4; i++ ) {
		if ( node.count[i] == INVALID ) break;
		result++;
	}
	return result;
}
