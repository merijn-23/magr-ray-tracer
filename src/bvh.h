#pragma once
#include "common.h"

class BVH2 {
public:	
	BVH2(std::vector<Primitive>&);
	void Build();
	uint Depth( const BVHNode2& );
	uint Count(const BVHNode2& );
	const BVHNode2& Root( ) { return nodes[rootNodeIdx_]; }
	const BVHNode2& Left( const BVHNode2& n ) { return nodes[n.left]; }
	const BVHNode2& Right( const BVHNode2& n ) { return nodes[n.left + 1]; }
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

	uint Depth( BVHNode4 );
	uint Count( BVHNode4 );
	BVHNode4 Root( ) { return nodes[rootNodeIdx_]; }

private:
	BVH2& bvh2;
	std::vector<BVHNode4_Invalid> invalidNodes_;
	void Convert( );
	void Collapse( int index ); // Collapse the binary tree to a qbvh
	int GetChildCount( const BVHNode4_Invalid& node ) const;
	void MakeValidNodes( );

	uint rootNodeIdx_;
};
