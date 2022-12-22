#include "precomp.h"

BVH2::BVH2( std::vector<Primitive>& primitives ) : primitives_( primitives )
{
	rootNodeIdx_ = 0;
	count_ = primitives_.size();
	nodes.resize( count_ * 2 );
	primIdx.resize( count_ );
	bbs_.resize( count_ );
	nodesUsed_ = 1;


	//float3 pos = float3( -1, -2, 1 );
	//float r = 2;
	//float d = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z - r * r;
	//auto points = sphereAAPlaneIntersection( pos, d, r, 2, -0.7f);
	
	//aabb bounds{ float3( 0 ), float3( 1 ) };
	//aabb outBounds;
	//clipTriangleToAABB( bounds, float3( -1, 0, .1f ), float3( -1, 1, .2f ), float3( 0.8f, 0.5f, 0.5f ), outBounds);

	aabb bounds{ float3( 0 ), float3( 1 ) };
	aabb outBounds;
	bool intersected = clipSphereToAABB( bounds, float3( 2, 0.5f, 0 ), 0.5f, outBounds );

	Build( true );
}

#pragma region bvh_statistics
float BVH2::TotalCost( uint nodeIdx )
{
	if (nodeIdx == -1) nodeIdx = rootNodeIdx_;
	BVHNode2 node = nodes[nodeIdx];

	// node is leaf, return SAH cost
	if (node.count > 0) return CalculateNodeCost( node );
	// node is interior node
	else
	{
		float lCost = TotalCost( node.left );
		float rCost = TotalCost( node.left + 1 );
		return lCost + rCost;
	}
}

uint BVH2::Depth( uint nodeIdx )
{
	if (nodeIdx == -1) nodeIdx = rootNodeIdx_;
	BVHNode2 node = nodes[nodeIdx];
	if (node.count > 0) return 0;
	else
	{
		uint lDepth = Depth( node.left );
		uint rDepth = Depth( node.left + 1 );
		if (lDepth > rDepth) return (lDepth + 1);
		else return (rDepth + 1);
	}
}

uint BVH2::Count( uint nodeIdx )
{
	if (nodeIdx == -1) nodeIdx = rootNodeIdx_;
	BVHNode2 node = nodes[nodeIdx];
	if (node.count > 0) return node.count;
	else
	{
		uint lcount = Count( node.left );
		uint rcount = Count( node.left + 1 );
		return lcount + rcount;
	}
}
#pragma endregion bvh_statistics

#pragma region bvh2
void BVH2::Build( bool statistics )
{
	// populate triangle index array
	CreateBoundingBoxes();
	BVHNode2& root = nodes[rootNodeIdx_];
	root.left = rootNodeIdx_, root.count = count_;
	UpdateNodeBounds( rootNodeIdx_ );
	// subdivide recursively
	Timer t;
	Subdivide( rootNodeIdx_ );

	if (statistics)
	{
		stat_build_time = t.elapsed() * 1000;
		stat_node_count = nodesUsed_;
		stat_depth = Depth();
		stat_sah_cost = TotalCost();
	}
}

void BVH2::UpdateNodeBounds( uint nodeIdx )
{
	BVHNode2& node = nodes[nodeIdx];
	node.aabbMin = float3( 1e30f );
	node.aabbMax = float3( -1e30f );
	for (uint left = node.left, i = 0; i < node.count; i++)
	{
		aabb box = bbs_[left + i];
		node.aabbMin = fminf( node.aabbMin, box.bmin4f );
		node.aabbMax = fmaxf( node.aabbMax, box.bmax4f );
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
	float objectSplitPos, overlap;
	float objectSplitCost = FindBestObjectSplitPlane( node, axis, objectSplitPos, overlap );
	float nosplitCost = CalculateNodeCost( node );
	if (objectSplitCost >= nosplitCost) return;

	ObjectSplit( node, axis, objectSplitPos );
}
#pragma endregion bvh2

#pragma region object_splits
float BVH2::FindBestObjectSplitPlane( BVHNode2& node, int& axis, float& splitPos, float& overlap )
{
	float bestCost = 1e30f;
	for (int a = 0; a < 3; a++)
	{
		float boundsMin = 1e30f, boundsMax = -1e30f;
		for (uint i = 0; i < node.count; i++)
		{
#ifdef CENTROID

			Primitive& prim = primitives_[references_[node.left + i].primIdx];
			switch (prim.objType)
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
#else
			aabb box = bbs_[node.left + i];
			boundsMin = min( boundsMin, box.Center( a ) );
			boundsMax = max( boundsMax, box.Center( a ) );
#endif
		}
		if (boundsMin == boundsMax) continue;
		// populate the bins
		struct Bin { aabb bounds; int count = 0; } bin[BVH_BINS];
		float scale = BVH_BINS / (boundsMax - boundsMin); node.aabbMax[a] - node.aabbMin[a];
		for (uint i = 0; i < node.count; i++)
		{
#ifdef CENTROID
			Primitive& prim = primitives_[references_[node.left + i].primIdx];
			switch (prim.objType)
			{
			case TRIANGLE:
			{
				Triangle& tri = prim.objData.triangle;
				int binIdx = min( BVH_BINS - 1, (int)((tri.centroid[a] - boundsMin) * scale) );
				bin[binIdx].count++;
				bin[binIdx].bounds.Grow( tri.v0 );
				bin[binIdx].bounds.Grow( tri.v1 );
				bin[binIdx].bounds.Grow( tri.v2 );
			} break;
			case SPHERE:
			{
				Sphere& sphere = prim.objData.sphere;
				int binIdx = min( BVH_BINS - 1, (int)((sphere.pos[a] - boundsMin) * scale) );
				bin[binIdx].count++;
				bin[binIdx].bounds.Grow( sphere.pos - sphere.r );
				bin[binIdx].bounds.Grow( sphere.pos + sphere.r );
			}break;
			}
#else
			aabb box = bbs_[node.left + i];
			int binIdx = min( BVH_BINS - 1, (int)((box.Center( a ) - boundsMin) * scale) );
			bin[binIdx].count++;
			bin[binIdx].bounds.Grow( box );
#endif
		}
		// gather data for the 7 planes between the 8 bins
		float leftArea[BVH_BINS - 1], rightArea[BVH_BINS - 1];
		aabb leftBoxes[BVH_BINS - 1], rightBoxes[BVH_BINS - 1];
		int leftCount[BVH_BINS - 1], rightCount[BVH_BINS - 1];
		aabb leftBox, rightBox;
		int leftSum = 0, rightSum = 0;
		for (int i = 0; i < BVH_BINS - 1; i++)
		{
			leftSum += bin[i].count;
			leftCount[i] = leftSum;
			leftBox.Grow( bin[i].bounds );
			leftArea[i] = leftBox.Area();
			leftBoxes[i] = leftBox;
			rightSum += bin[BVH_BINS - 1 - i].count;
			rightCount[BVH_BINS - 2 - i] = rightSum;
			rightBox.Grow( bin[BVH_BINS - 1 - i].bounds );
			rightArea[BVH_BINS - 2 - i] = rightBox.Area();
			rightBoxes[BVH_BINS - 2 - i] = rightBox;
		}

		// calculate SAH cost for the 7 planes
		scale = (boundsMax - boundsMin) / BVH_BINS;
		for (int i = 0; i < BVH_BINS - 1; i++)
		{
			float planeCost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
			if (planeCost < bestCost)
			{
				axis = a;
				splitPos = boundsMin + scale * (i + 1);
				bestCost = planeCost;
				overlap = leftBoxes[i].Intersection( rightBoxes[i] ).Area();
			}
		}
	}
	return bestCost;
}

void BVH2::ObjectSplit( BVHNode2& node, int axis, float splitPos )
{
	// in-place partition
	int i = node.left;
	int j = i + node.count - 1;
	while (i <= j)
	{
		aabb& box = bbs_[i];
		float center = box.Center( axis );
		if (center < splitPos) i++;
		else
		{
			swap( bbs_[i], bbs_[j] );
			swap( primIdx[i], primIdx[j--] );
		}
	}
	// abort split if one of the sides is empty
	int leftCount = i - node.left;
	if (leftCount == 0 || leftCount == node.count) return;
	// create child nodes
	int leftChildIdx = nodesUsed_++;
	int rightChildIdx = nodesUsed_++;
	nodes[leftChildIdx].left = node.left;
	nodes[leftChildIdx].count = leftCount;
	nodes[rightChildIdx].left = i;
	nodes[rightChildIdx].count = node.count - leftCount;
	node.left = leftChildIdx;
	node.count = 0;
	UpdateNodeBounds( leftChildIdx );
	UpdateNodeBounds( rightChildIdx );
	// recurse
	Subdivide( leftChildIdx );
	Subdivide( rightChildIdx );
}
#pragma endregion object_splits

#pragma region spatial_splits
void BVH2::CreateBoundingBoxes()
{
	for (int i = 0; i < count_; i++) primIdx[i] = i;

	bbs_.clear();
	for (uint i = 0; i < primitives_.size(); i++)
	{
		aabb box;
		Primitive p = primitives_[i];
		switch (p.objType)
		{
		case TRIANGLE:
			box.Grow( p.objData.triangle.v0 );
			box.Grow( p.objData.triangle.v1 );
			box.Grow( p.objData.triangle.v2 );
			break;
		case SPHERE:
			box.Grow( p.objData.sphere.pos + p.objData.sphere.r );
			box.Grow( p.objData.sphere.pos - p.objData.sphere.r );
			break;
		}
		bbs_.push_back( box );
	}
}

float3 BVH2::lineAAPlaneIntersection( float3 v1, float3 v2, int axis, float plane )
{
	assert( v1[axis] != v2[axis] );

	float3 start, end;
	if (v1[axis] < v2[axis])
	{
		start = v1; end = v2;
	}
	else
	{
		start = v2; end = v1;
	}

	float3 edge = end - start;
	float intersect = (plane - start[axis]) / edge[axis];
	assert( intersect >= 0 && intersect <= 1 );

	return start + edge * intersect;
}

// https://www.wikiwand.com/en/Sutherland%E2%80%93Hodgman_algorithm
// Basically Sutherland Hodgman algorithm but extended to 3d
// So we check for each plane if the triangle clips the plane
// And then we add vertices if it does
// We check for clipping by keeping track on which side of the plane our current point and next point is
bool BVH2::clipTriangleToAABB( aabb bounds, float3 v0, float3 v1, float3 v2, aabb& outBounds )
{
	// Maximum number of vertices is 3 + 6 = 9 because each plane can replace one vertex by two vertices
	std::vector<float3> vertices = { v0, v1, v2 };

	// for each axis
	for (int a = 0; a < 3; a++)
	{
		// for each side of the box
		for (int p = 0; p < 2; p++)
		{
			float plane, normal;
			if (p == 0)
			{
				plane = bounds.bmin[a];
				normal = 1.0f;
			}
			else
			{
				plane = bounds.bmax[a];
				normal = -1.0f;
			}

			std::vector<float3> newVertices;

			// loop through all vertices, keeping track of which ones leave or enter the bounding box / plane
			for (int i = 0; i < vertices.size(); i++)
			{
				float3 curVertex = vertices[i];
				float3 nextVertex = vertices[(i + 1) % vertices.size()];

				bool containsCur = (curVertex[a] - plane) * normal >= 0;
				bool containsNext = (nextVertex[a] - plane) * normal >= 0;

				if (containsCur) newVertices.push_back( curVertex );

				// check if we are going in or out of the plane
				if (containsCur != containsNext)
				{
					float3 intersect = lineAAPlaneIntersection( curVertex, nextVertex, a, plane );
					newVertices.push_back( intersect );
				}
			}
			vertices = newVertices;
		}
	}

	if (vertices.size() < 3) return false;

	for (int i = 0; i < vertices.size(); i++)
		outBounds.Grow( vertices[i] );
	return true;
}

bool BVH2::clipSphereToAABB( aabb bounds, float3 pos, float r, aabb& outBounds )
{
	// initial bounds equal to sphere
	outBounds.Grow( pos + r );
	outBounds.Grow( pos - r );

	// for each axis
	for (int a = 0; a < 3; a++)
	{
		// for each side of the box
		for (int p = 0; p < 2; p++)
		{
			float plane, normal;
			if (p == 0)
			{
				plane = bounds.bmin[a];
				normal = 1.0f;
			}
			else
			{
				plane = bounds.bmax[a];
				normal = -1.0f;
			}

			float farPos = pos[a] + r * normal;
			if (farPos * normal > plane * normal)
			{
				float nearPos = pos[a] - r * normal;
				if (nearPos * normal < plane * normal)
				{
					// we have an intersection (line segment from center to boundary of sphere going through the plane)
					float d = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z - r * r;
					auto points = sphereAAPlaneIntersection( pos, d, a, plane );

					// construct tighter bounding box constrained by plane
					aabb stricterBounds;
					stricterBounds.Grow( points[0]);
					stricterBounds.Grow( points[1] );
					stricterBounds.Grow( points[2] );
					stricterBounds.Grow( points[3]);
					float3 farVec = pos;
					farVec[a] = farPos;
					stricterBounds.Grow( farVec );

					outBounds = outBounds.Intersection( stricterBounds );
				}
			}
			else
			{
				// sphere is fully outside of bounds
				return false;
			}
		}
	}
	return true;
}

std::vector<float3> BVH2::sphereAAPlaneIntersection( float3 pos, float d, int axis, float plane )
{
	// Sphere equation is:
	// x^2 + y^2 + z^2 + ax + by + cz + d = 0
	// with
	// a = -2x_c
	// b = -2y_c
	// c = -2z_c
	// d = x_c^2 + y_c^2 + z_c^2 - r^2.
	// We have an axis-aligned plane, so we know that one of our coordinates, namely 'axis', is 'plane',
	// so our intersection with the plane becomes a sphere.
	// We want to know two extreme points on this sphere,
	// which we find by setting our other coordinates to x_c, y_c or z_c, one by one.
	// We can then find the extreme point by solving the ABC-formula,
	// working under the assumption that we HAVE an intersection, and we only want to know one of the outcomes.
	// We can then return both intersection points.
	bool first = true;
	std::vector<float3> points;
	for (int axisL = 0; axisL < 3; axisL++)
	{
		if (axisL == axis) continue;

		// 'find' the axis that we are trying to solve
		int axisF = 0;
		while (axisF == axisL || axisF == axis) axisF++;
		
		float3 xyz;
		xyz[axis] = plane;
		xyz[axisL] = pos[axisL];
		float a = -2 * pos[axisF];
		float by = -2 * pos[axisL] * xyz[axisL];
		float cz = -2 * pos[axis] * xyz[axis];
		float y2 = xyz[axisL] * xyz[axisL];
		float z2 = xyz[axis] * xyz[axis];

		// w/o loss of generality this example works when solving for x, same applies when solving for y and z
		// Ax^2 + Bx + C = 0
		// x^2 + ax + ( y^2 + z^2 + by + cz + ( x+_c^2 + y_c^2 + z_c^2 - r^2 ))
		// so ABC formula with
		// A = 1 (one)
		// B = a
		// C = y^2 + z^2 + by + cz + ( x+_c^2 + y_c^2 + z_c^2 - r^2 )
		float D = a * a - 4 * (y2 + z2 + by + cz + d);

		// we assume that we HAVE an intersection, so there has to be a solution
		assert( D >= 0 );

		// final coordinate = -B + sqrt(D) / 2A
		float sqrtD = sqrtf( D );
		xyz[axisF] = (-a + sqrtD) * 0.5f;
		points.push_back( xyz );
		xyz[axisF] = (-a - sqrtD) * 0.5f;
		points.push_back( xyz );

	}
	return points;
}

float BVH2::FindBestSpatialSplitPlane( BVHNode2& node, int& axis, float& splitPos )
{
	float bestCost = 1e30f;
	for (int a = 0; a < 3; a++)
	{
		float boundsMin = 1e30f, boundsMax = -1e30f;
		for (uint i = 0; i < node.count; i++)
		{
			aabb box = bbs_[node.left + i];
			boundsMin = min( boundsMin, box.Center( a ) );
			boundsMax = max( boundsMax, box.Center( a ) );
		}
		if (boundsMin == boundsMax) continue;

		// populate the bins
		struct Bin { aabb bounds; int entries = 0; int exits = 0; } bin[BVH_BINS];
		float scale = BVH_BINS / (boundsMax - boundsMin);

		for (uint i = 0; i < node.count; i++)
		{
			aabb box = bbs_[node.left + i];
			int leftBin = min( int( scale * (box.bmin[a] - boundsMin) ), BVH_BINS - 1 );
			int rightBin = min( int( scale * (box.bmax[a] - boundsMin) ), BVH_BINS - 1 );

			assert( leftBin <= rightBin );
			assert( leftBin >= 0 && rightBin >= 0 );

			if (leftBin == rightBin)
			{
				// Primitive is completely contained in a single bin
				bin[leftBin].entries++;
				bin[rightBin].exits++;
				bin[leftBin].bounds.Grow( box );
			}
			else
			{
				// Primitive spans at least 2 bins
				int left = BVH_BINS;
				int right = -1;
				
			}
		}

	}
	return bestCost;
}

void BVH2::SpatialSplit( BVHNode2& node, int axis, float splitPos )
{

}
#pragma endregion spatial_splits

#pragma region bvh4
BVH4::BVH4( BVH2& _bvh2 ) : bvh2( _bvh2 )
{
	Convert();
}

// https://github.com/jan-van-bergen/GPU-Raytracer/blob/master/Src/BVH/Converters/BVH4Converter.cpp#L75
void BVH4::Convert()
{
	nodes.resize( bvh2.nodes.size() );
	for (size_t i = 0; i < nodes.size(); i++)
	{
		// We use index 1 as a starting point, such that it points to the first child of the root
		if (i == 1)
		{
			nodes[i].left[0] = 0;
			nodes[i].count[0] = 0;
			continue;
		}

		if (!nodes[i].count > 0)
		{
			const BVHNode2& child_left = bvh2.nodes[bvh2.nodes[i].left];
			const BVHNode2& child_right = bvh2.nodes[bvh2.nodes[i].left + 1];

			nodes[i].aabbMin[0] = child_left.aabbMin;
			nodes[i].aabbMax[0] = child_left.aabbMax;
			nodes[i].aabbMin[1] = child_right.aabbMin;
			nodes[i].aabbMax[1] = child_right.aabbMax;

			if (child_left.count > 0)
			{
				nodes[i].left[0] = child_left.left;
				nodes[i].count[0] = child_left.count;
			}
			else
			{
				nodes[i].left[0] = bvh2.nodes[i].left;
				nodes[i].count[0] = 0;
			}

			if (child_right.count > 0)
			{
				nodes[i].left[1] = child_right.left;
				nodes[i].count[1] = child_right.count;
			}
			else
			{
				nodes[i].left[1] = bvh2.nodes[i].left + 1;
				nodes[i].count[1] = 0;
			}

			// For now the tree is binary,
			// so make the rest of the indices invalid
			for (int j = 2; j < 4; j++)
			{
				nodes[i].left[j] = INVALID;
				nodes[i].count[j] = INVALID;
			}
		}
	}

	// Handle the special case where the root is a leaf
	if (nodes[0].count > 0)
	{
		nodes[0].aabbMin[0] = bvh2.nodes[0].aabbMin;
		nodes[0].aabbMax[0] = bvh2.nodes[0].aabbMax;
		nodes[0].left[0] = bvh2.nodes[0].left;
		nodes[0].count[0] = bvh2.nodes[0].count;

		for (int i = 1; i < 4; i++)
		{
			nodes[0].left[i] = INVALID;
			nodes[0].count[i] = INVALID;
		}
	}
	else
	{
		// Collapse tree top-down, starting from the root
		Collapse( 0 );
	}

	primIdx = primIdx; // NOTE: copy
}

void BVH4::Collapse( int index )
{
	BVHNode4& node = nodes[index];
	while (true)
	{
		int child_count = GetChildCount( node );

		// Look for adoptable child with the largest surface area
		float max_area = -INFINITY;
		int   max_index = INVALID;

		for (int i = 0; i < child_count; i++)
		{
			if (!node.count[i] > 0)
			{
				int child_i_child_count = GetChildCount( nodes[node.left[i]] );

				// Check if the current Node can adopt the children of child Node i
				if (child_count + child_i_child_count - 1 <= 4)
				{
					float diff_x = node.aabbMax[i].x - node.aabbMin[i].x;
					float diff_y = node.aabbMax[i].y - node.aabbMin[i].y;
					float diff_z = node.aabbMax[i].z - node.aabbMin[i].z;

					float half_area = diff_x * diff_y + diff_y * diff_z + diff_z * diff_x;

					if (half_area > max_area)
					{
						max_area = half_area;
						max_index = i;
					}
				}
			}
		}

		// No merge possible anymore, stop trying
		if (max_index == INVALID) break;

		const BVHNode4& max_child = nodes[node.left[max_index]];

		// Replace max child Node with its first child
		node.aabbMin[max_index] = max_child.aabbMin[0];
		node.aabbMax[max_index] = max_child.aabbMax[0];
		node.left[max_index] = max_child.left[0];
		node.count[max_index] = max_child.count[0];

		int max_child_child_count = GetChildCount( max_child );

		// Add the rest of max child Node's children after the current Node's own children
		for (int i = 1; i < max_child_child_count; i++)
		{
			node.aabbMin[child_count + i - 1] = max_child.aabbMin[i];
			node.aabbMax[child_count + i - 1] = max_child.aabbMax[i];
			node.left[child_count + i - 1] = max_child.left[i];
			node.count[child_count + i - 1] = max_child.count[i];
		}
	};

	for (int i = 0; i < 4; i++)
	{
		if (node.count[i] == INVALID) break;

		// If child Node i is an internal node, recurse
		if (node.count[i] == 0)
		{
			Collapse( node.left[i] );
		}
	}
}
#pragma endregion bvh4
