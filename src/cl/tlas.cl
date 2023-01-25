#ifndef __TLAS_CL
#define __TLAS_CL
int intersectTLAS(
	Ray* ray,
	TLASNode* tlasNodes,
	BLASNode* blasNodes,
#ifdef USE_BVH4
	BVHNode4* bvhNodes,
#endif
#ifdef USE_BVH2
	BVHNode2* bvhNodes,
#endif
	uint* primIdxs,
	bool occlusion
)
{
	TLASNode* node = &tlasNodes[0], * stack[32];
	uint stackPtr = 0;
	int steps = 0;
	while ( 1 ) {
		if ( node->leftRight == 0 ) {
			BLASNode* blas = &blasNodes[node->BLASidx];
#ifdef USE_BVH4
			steps += intersectBVH4( ray, bvhNodes, primIdxs, blas->bvhIdx, occlusion );
#endif
#ifdef USE_BVH2
			int value = intersectBVH2( ray, bvhNodes, primIdxs, blas->bvhIdx, occlusion );
			if(occlusion) if(value == -1) return -1;
			steps += value;
#endif
			if ( stackPtr == 0 ) break;
			else node = stack[--stackPtr];
			continue;
		}
		
		// current node is an interior node: visit child nodes, ordered
		TLASNode* child1 = &tlasNodes[node->leftRight & 0xffff];
		TLASNode* child2 = &tlasNodes[node->leftRight >> 16];
		float dist1 = intersectAABB( ray, child1->aabbMin, child1->aabbMax );
		float dist2 = intersectAABB( ray, child2->aabbMin, child2->aabbMax );
		if ( dist1 > dist2 ) {
			float d = dist1; dist1 = dist2; dist2 = d;
			TLASNode* c = child1; child1 = child2; child2 = c;
		}
		if ( dist1 == REALLYFAR ) {
			// missed both child nodes; pop a node from the stack
			if ( stackPtr == 0 ) break;
			else node = stack[--stackPtr];
		} else {
			node = child1;
			if ( dist2 != REALLYFAR )
				stack[stackPtr++] = child2;
		}
		//steps++;
	}
	return steps;
}
#endif // __TLAS_CL