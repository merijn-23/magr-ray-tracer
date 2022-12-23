#include "precomp.h"
#include <stack>

static int spatialSplits = 0;
static int clips = 0;
BVH2::BVH2( std::vector<Primitive>& primitives ) : primitives_( primitives )
{
	rootNodeIdx_ = 0;
	nodes.resize( primitives_.size() * 5 );
	//primIdx.resize( count_ );
	//bbs_.resize( count_ );
	nodesUsed_ = 1;


	//float3 pos = float3( -1, -2, 1 );
	//float r = 2;
	//float d = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z - r * r;
	//auto points = sphereAAPlaneIntersection( pos, d, r, 2, -0.7f);

	//aabb bounds{ float3( 0 ), float3( 1 ) };
	//aabb outBounds;
	//clipTriangleToAABB( bounds, float3( -1, 0, .1f ), float3( -1, 1, .2f ), float3( 0.8f, 0.5f, 0.5f ), outBounds);

	//aabb bounds{ float3( 0 ), float3( 1 ) };
	//aabb outBounds;
	//bool intersected = clipSphereToAABB( bounds, float3( 2, 0.5f, 0 ), 0.5f, outBounds );
	printf( "primIdx size %i\n", primIdx.size() );
	Build( true );
	printf( "primIdx size %i\n", primIdx.size() );
	//printf( "tree bounding box check: %i\n", CheckBBs() );
	printf( "primitive count %i\n", primitives.size() );
	printf( "tree count %i\n", Count() );
	printf( "spatial splits done %i\n", spatialSplits );
	printf( "primitives clipped %i\n", clips );
}

#pragma region bvh_statistics
float BVH2::TotalCost( uint nodeIdx )
{
	if (nodeIdx == -1) nodeIdx = rootNodeIdx_;
	BVHNode2 node = nodes[nodeIdx];

	// node is leaf, return SAH cost
	if (node.count > 0) return CalculateNodeCost( node, node.count );
	// node is interior node
	else
	{
		float lCost = TotalCost( node.first );
		float rCost = TotalCost( node.first + 1 );
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
		uint lDepth = Depth( node.first );
		uint rDepth = Depth( node.first + 1 );
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
		uint lcount = Count( node.first );
		uint rcount = Count( node.first + 1 );
		return lcount + rcount;
	}
}

void BVH2::Build( bool statistics )
{
	// populate triangle index array
	auto data = CreateBVHPrimData();
	BVHNode2& root = nodes[rootNodeIdx_];
	UpdateNodeBounds( rootNodeIdx_, data );
	// subdivide recursively
	Timer t;
	BuildBVH( rootNodeIdx_, data );

	if (statistics)
	{
		stat_build_time = t.elapsed() * 1000;
		stat_node_count = nodesUsed_;
		stat_depth = Depth();
		stat_sah_cost = TotalCost();
	}
}

void BVH2::UpdateNodeBounds( uint nodeIdx, std::vector<BVHPrimData> prims )
{
	BVHNode2& node = nodes[nodeIdx];
	node.aabbMin = float3( 1e30f );
	node.aabbMax = float3( -1e30f );
	for (int i = 0; i < prims.size(); i++)
	{
		aabb box = prims[i].box;
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

float BVH2::CalculateNodeCost( BVHNode2& node, uint count )
{
	float3 e = node.aabbMax - node.aabbMin; // extent of the node
	float surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
	return count * surfaceArea;
}

void BVH2::BuildBVH( uint root, std::vector<BVHPrimData> data )
{
	std::stack<std::pair<uint, std::vector<BVHPrimData>>> stack;
	stack.push( { root, data } );

	while (!stack.empty())
	{
		std::pair<uint, std::vector<BVHPrimData>> pair = stack.top();
		stack.pop();

		BVHNode2& node = nodes[pair.first];
		// determine split axis using SAH
		int objectAxis;
		float objectSplitPos, overlap;
		float objectSplitCost = FindBestObjectSplitPlane( node, objectAxis, objectSplitPos, overlap, pair.second );

		float noSplitCost = CalculateNodeCost( node, pair.second.size() );

		int spatialAxis;
		float spatialSplitCost = 1e30f;
		float spatialSplitPos;

		float3 rootDims = Root().aabbMax - Root().aabbMin;
		float rootArea = max( 0.f, rootDims[0] * rootDims[1] + rootDims[0] * rootDims[2] + rootDims[1] * rootDims[2] );
		if (overlap / rootArea > alpha)
		{
			// perform step 2: attempt a spatial split
			spatialSplitCost = FindBestSpatialSplitPlane( node, spatialAxis, spatialSplitPos, pair.second );
		}

		if (pair.second.size() <= MIN_LEAF_PRIMS || (noSplitCost < objectSplitCost && noSplitCost < spatialSplitCost))
		{
			// make a leaf
			BVHNode2& node = nodes[pair.first];
			node.first = primIdx.size();
			node.count = pair.second.size();
			for (int i = 0; i < pair.second.size(); i++)
			{
				primIdx.push_back( pair.second[i].idx );
			}
			continue;
		}
		
		// either object split or spatial split
		std::pair<std::vector<BVHPrimData>, std::vector<BVHPrimData>> leftright;
		bool x = objectSplitCost < spatialSplitCost;
		if (objectSplitCost < spatialSplitCost)
		{
			leftright = ObjectSplit( node, objectAxis, objectSplitPos, pair.second );
		}
		else
		{
			spatialSplits++;
			leftright = SpatialSplit( node, spatialAxis, spatialSplitPos, pair.second );
		}

		uint leftId = nodesUsed_++;
		uint rightId = nodesUsed_++;
			
		// TODO
		// update node bounds
		UpdateNodeBounds( leftId, leftright.first );
		UpdateNodeBounds( rightId, leftright.second );

		node.first = leftId;
		node.count = 0;

		stack.push( { leftId, leftright.first } );
		stack.push( { rightId, leftright.second } );
	}
}

#pragma endregion bvh2

#pragma region object_splits
float BVH2::FindBestObjectSplitPlane( BVHNode2& node, int& axis, float& splitPos, float& overlap, std::vector<BVHPrimData> prims )
{
	float bestCost = 1e30f;
	for (int a = 0; a < 3; a++)
	{
		float boundsMin = 1e30f, boundsMax = -1e30f;
		for (uint i = 0; i < prims.size(); i++)
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
			aabb box = prims[i].box;
			boundsMin = min( boundsMin, box.Center( a ) );
			boundsMax = max( boundsMax, box.Center( a ) );
#endif
		}
		if (boundsMin == boundsMax) continue;
		// populate the bins
		struct Bin { aabb bounds; int count = 0; } bin[BVH_BINS];
		float scale = BVH_BINS / (boundsMax - boundsMin);
		for (uint i = 0; i < prims.size(); i++)
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
			aabb box = prims[i].box;
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

std::pair<std::vector<BVHPrimData>, std::vector<BVHPrimData>> BVH2::ObjectSplit( BVHNode2& node, int axis, float splitPos, std::vector<BVHPrimData> prims )
{
	// in-place partition
	std::vector<BVHPrimData> leftPrims, rightPrims;

	for (int i = 0; i < prims.size(); i++)
	{
		aabb& box = prims[i].box;
		float center = box.Center( axis );
		if (center < splitPos) leftPrims.push_back( prims[i] );
		else rightPrims.push_back( prims[i] );
	}

	return { leftPrims, rightPrims };
}
#pragma endregion object_splits

#pragma region spatial_splits
std::vector<BVHPrimData> BVH2::CreateBVHPrimData()
{
	std::vector<BVHPrimData> data;
	//for (int i = 0; i < count_; i++) primIdx[i] = i;

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
		data.push_back( { box, i } );
	}
	return data;
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
					stricterBounds.Grow( points[0] );
					stricterBounds.Grow( points[1] );
					stricterBounds.Grow( points[2] );
					stricterBounds.Grow( points[3] );
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


struct SpatialBin
{
	aabb bounds;
	int entries = 0;
	int exits = 0;
	float left = 1e30f;
	float right = -1e30f;

	SpatialBin operator+( const SpatialBin& other ) const
	{
		return { bounds.Union( other.bounds ), entries + other.entries, exits + other.exits, min( left, other.left ), max( right, other.right ) };
	}
};

float BVH2::FindBestSpatialSplitPlane( BVHNode2& node, int& axis, float& splitPos, std::vector<BVHPrimData> prims )
{
	float bestCost = 1e30f;
	for (int a = 0; a < 3; a++)
	{
		float boundsMin = 1e30f, boundsMax = -1e30f;
		for (uint i = 0; i < prims.size(); i++)
		{
			aabb box = prims[i].box;
			boundsMin = min( boundsMin, box.bmin[a] );
			boundsMax = max( boundsMax, box.bmax[a] );
		}
		if (boundsMin == boundsMax) continue;

		// populate the bins
		SpatialBin bins[BVH_BINS];
		float scale = BVH_BINS / (boundsMax - boundsMin);
		for (int b = 0; b < BVH_BINS; b++)
		{
			bins[b].left = boundsMin + b * (1 / scale);
			bins[b].right = (b == BVH_BINS - 1) ? boundsMax : boundsMin + (b + 1) * (1 / scale);
		}

		for (uint i = 0; i < prims.size(); i++)
		{
			aabb box = prims[i].box;
			int leftBin = min( int( scale * (box.bmin[a] - boundsMin) ), BVH_BINS - 1 );
			int rightBin = min( int( scale * (box.bmax[a] - boundsMin) ), BVH_BINS - 1 );

			while (box.bmin[a] <= bins[leftBin].left && leftBin > 0)
				leftBin--; 
			while (box.bmin[a] > bins[leftBin].right && leftBin != BVH_BINS - 1)
				leftBin++;
			while (box.bmax[a] < bins[rightBin].left && rightBin > 0)
				rightBin--;
			while (box.bmax[a] >= bins[rightBin].right && rightBin != BVH_BINS - 1)
				rightBin++;
			

			assert( leftBin <= rightBin );
			assert( leftBin >= 0 && rightBin >= 0 && leftBin < BVH_BINS&& rightBin < BVH_BINS );

			if (leftBin == rightBin)
			{
				bins[leftBin].entries++;
				bins[rightBin].exits++;
				bins[leftBin].bounds.Grow( box );
			}
			else
			{
				// Primitive spans at least 2 bins
				int left = BVH_BINS;
				int right = -1;

				Primitive prim = primitives_[prims[i].idx];

				for (int bin = leftBin; bin <= rightBin; bin++)
				{
					bool intersection;
					aabb outBounds;
					// create a quick bounding box for the bin
					aabb binBounds = box;
					binBounds.bmin[a] = bins[bin].left;
					binBounds.bmax[a] = bins[bin].right;

					// if we have planes in our bvh, we messed up somewhere
					assert( prim.objType == TRIANGLE || prim.objType == SPHERE );
					switch (prim.objType)
					{
					case TRIANGLE:
						Triangle t = prim.objData.triangle;
						intersection = clipTriangleToAABB( binBounds, t.v0, t.v1, t.v2, outBounds );
						break;
					case SPHERE:
						Sphere s = prim.objData.sphere;
						intersection = clipSphereToAABB( binBounds, s.pos, s.r, outBounds );
						break;
					}

					if (intersection)
					{
						left = min( left, bin );
						right = max( right, bin );
						// grow the bounding box of the bin which contains the so far seen primitives
						bins[bin].bounds.Grow( outBounds );
					}
				}

				if (left <= right)
				{
					bins[left].entries++;
					bins[right].exits++;
				}
			}
		}

		// now that we have the bins, we can figure out which split is the best for this axis
		SpatialBin leftBins[BVH_BINS];
		SpatialBin rightBins[BVH_BINS];
		SpatialBin leftBinSum, rightBinSum;
		for (int i = 0; i < BVH_BINS; i++)
		{
			leftBinSum = leftBinSum + bins[i];
			leftBins[i] = leftBinSum;

			rightBinSum = rightBinSum + bins[BVH_BINS - 1 - i];
			rightBins[BVH_BINS - 1 - i] = rightBinSum;
		}

		// loop over the split planes and check the left and right cumulative bins
		for (int i = 0; i < BVH_BINS - 1; i++)
		{
			SpatialBin leftBin = leftBins[i];
			SpatialBin rightBin = rightBins[i + 1];

			if (leftBin.entries == 0 || rightBin.exits == 0)
				continue;

			// calculate SAH
			float planeCost = leftBin.entries * leftBin.bounds.Area() + rightBin.exits * rightBin.bounds.Area();
			if (planeCost < bestCost)
			{
				bestCost = planeCost;
				axis = a;
				splitPos = leftBin.right;
			}
		}
	}
	return bestCost;
}

std::pair<std::vector<BVHPrimData>, std::vector<BVHPrimData>> BVH2::SpatialSplit( BVHNode2& node, int axis, float splitPos, std::vector<BVHPrimData> prims )
{
	std::vector<BVHPrimData> leftPrims, rightPrims;

	for (int i = 0; i < prims.size(); i++)
	{
		aabb& box = prims[i].box;
		float min = box.bmin[axis];
		float max = box.bmax[axis];

		if (min < splitPos && max > splitPos)
		{
			// split
			aabb leftClip = box;
			aabb rightClip = box;
			leftClip.bmax[axis] = splitPos;
			rightClip.bmin[axis] = splitPos;

			bool leftsuccess, rightsuccess;
			aabb leftClipped, rightClipped;

			uint index = prims[i].idx;
			Primitive prim = primitives_[index];
			switch (prim.objType)
			{
			case TRIANGLE:
				Triangle t = prim.objData.triangle;
				leftsuccess = clipTriangleToAABB( leftClip, t.v0, t.v1, t.v2, leftClipped );
				rightsuccess = clipTriangleToAABB( rightClip, t.v0, t.v1, t.v2, rightClipped );
				break;
			case SPHERE:
				Sphere s = prim.objData.sphere;
				leftsuccess = clipSphereToAABB( leftClip, s.pos, s.r, leftClipped );
				rightsuccess = clipSphereToAABB( rightClip, s.pos, s.r, rightClipped );
				break;
			}

			clips++;
			// update left split primitive
			if (leftsuccess)
				leftPrims.push_back( { leftClipped, index } );		
			if (rightsuccess)
				rightPrims.push_back( { rightClipped, index } );

		}
		else if (max < splitPos)
		{
			// fully left of split
			leftPrims.push_back( prims[i] );
		}
		else
		{
			rightPrims.push_back( prims[i] );
		}
	}

	return { leftPrims, rightPrims };
}
#pragma endregion spatial_splits

#pragma region bvh4
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

	cout << "Depth: " << Depth( nodes[0] ) << endl;
	cout << "Count: " << Count( nodes[0] ) << endl;
}

uint BVH4::Depth( BVHNode4 node )
{
	uint maxDepth = 0;
	if ( node.count[0] == 0 ) maxDepth = max( maxDepth, Depth( nodes[node.first[0]] ) + 1 );
	if ( node.count[1] == 0 ) maxDepth = max( maxDepth, Depth( nodes[node.first[1]] ) + 1 );
	if ( node.count[2] == 0 ) maxDepth = max( maxDepth, Depth( nodes[node.first[2]] ) + 1 );
	if ( node.count[3] == 0 ) maxDepth = max( maxDepth, Depth( nodes[node.first[3]] ) + 1 );
	return maxDepth;
}

uint BVH4::Count( BVHNode4 node )
{
	int count = 0;
	for ( size_t i = 0; i < 4; i++ )
		if ( node.count[i] > 0 ) count += node.count[i];
		else if ( node.count[i] == 0 ) count += Count( nodes[node.first[i]] );
	return count;
}

// https://github.com/jan-van-bergen/GPU-Raytracer/blob/master/Src/BVH/Converters/BVH4Converter.cpp
void BVH4::Convert( )
{
	nodes.resize( bvh2.nodes.size( ) );
	for ( size_t i = 0; i < nodes.size( ); i++ ) {
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
#pragma endregion bvh4
