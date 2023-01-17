#include "precomp.h"

TLAS::TLAS( BVH2& _bvh2 ) : bvh2_( _bvh2 )
{
	nodesUsed_ = 0;
	tlasNodes.resize( bvh2_.blasNodes.size( ) * 2 );
}
void TLAS::Build( )
{
	// assign a TLASleaf node to each BLAS
	int nodeIdx[256], nodeIndices = bvh2_.blasNodes.size();
	nodesUsed_ = 1;
	for ( uint i = 0; i < nodeIndices; i++ ) {
		nodeIdx[i] = nodesUsed_;
		BVHNode2 bvhNode = bvh2_.bvhNodes[bvh2_.blasNodes[i].bvhIdx];
		tlasNodes[nodesUsed_].aabbMin = bvhNode.aabbMin;
		tlasNodes[nodesUsed_].aabbMax = bvhNode.aabbMax;
		tlasNodes[nodesUsed_].BLASidx = i;
		tlasNodes[nodesUsed_++].leftRight = 0; // makes it a leaf
	}
	// use agglomerative clustering to build the TLAS
	int A = 0, B = FindBestMatch( nodeIdx, nodeIndices, A );
	while ( nodeIndices > 1 ) {
		int C = FindBestMatch( nodeIdx, nodeIndices, B );
		if ( A == C ) {
			int nodeIdxA = nodeIdx[A], nodeIdxB = nodeIdx[B];
			TLASNode& nodeA = tlasNodes[nodeIdxA];
			TLASNode& nodeB = tlasNodes[nodeIdxB];
			TLASNode& newNode = tlasNodes[nodesUsed_];
			newNode.leftRight = nodeIdxA + ( nodeIdxB << 16 );
			newNode.aabbMin = fminf( nodeA.aabbMin, nodeB.aabbMin );
			newNode.aabbMax = fmaxf( nodeA.aabbMax, nodeB.aabbMax );
			nodeIdx[A] = nodesUsed_++;
			nodeIdx[B] = nodeIdx[nodeIndices - 1];
			B = FindBestMatch( nodeIdx, --nodeIndices, A );
		} else A = B, B = C;
	}
	tlasNodes[0] = tlasNodes[nodeIdx[A]];
}
int TLAS::FindBestMatch( int* list, int N, int A )
{
	float smallest = REALLYFAR;
	int bestB = -1;
	for ( int B = 0; B < N; B++ ) if ( B != A ) {
		float3 bmin = fminf( tlasNodes[list[A]].aabbMin, tlasNodes[list[B]].aabbMin );
		float3 bmax = fmaxf( tlasNodes[list[A]].aabbMax, tlasNodes[list[B]].aabbMax );
		float3 e = bmax - bmin;
		float surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
		if ( surfaceArea < smallest ) smallest = surfaceArea, bestB = B;
	}
	return bestB;
}