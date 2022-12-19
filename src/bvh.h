#pragma once
#include "common.h"

class BVH2 {
public:	
	BVH2(std::vector<Primitive>&);
	void Build();
	uint Depth( BVHNode2 );
	uint Count(BVHNode2 );
	BVHNode2 Root( ) { return nodes[rootNodeIdx_]; }
	BVHNode2 Left( BVHNode2 n ) { return nodes[n.left]; }
	BVHNode2 Right( BVHNode2 n ) { return nodes[n.left + 1]; }
	std::vector<BVHNode2> nodes;
	std::vector<uint> primIdx;

private:
	void UpdateNodeBounds(uint nodeIdx);
	void UpdateTriangleBounds( BVHNode2& node, Triangle& triangle );
	void UpdateSphereBounds( BVHNode2& node, Sphere& sphere );
	
	float FindBestSplitPlane( BVHNode2& node, int& axis, float& splitPos );
	float CalculateNodeCost( BVHNode2& node );		
	void Subdivide(uint nodeIdx);

	std::vector<Primitive>& primitives_;
	uint count_;
	uint subdivisions_ = 0;
	uint rootNodeIdx_, nodesUsed_;
	static const int bins__ = 8;
};

class BVH4
{
public:
	BVH4( BVH2& );
	std::vector<BVHNode4> nodes;
	std::vector<uint> primIdx;

private:
	BVH2& bvh2;
	void Convert( );
	void Collapse( int index ); // Collapse the binary tree to a qbvh

	inline int GetChildCount( const BVHNode4& node ) const
	{
		int result = 0;
		for ( int i = 0; i < 4; i++ )
		{
			if ( node.count[i] == INVALID ) break;
			result++;
		}
		return result;
	}
};
