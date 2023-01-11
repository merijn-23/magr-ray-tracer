#pragma once
class TLAS
{
public:
	TLAS( BVH2& bvh2 );
	void Build( );
	std::vector<TLASNode> tlasNodes;
private:
	int FindBestMatch( int* list, int N, int A );
	BVH2& bvh2_;
	uint nodesUsed_;
};