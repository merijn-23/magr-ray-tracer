#ifndef __TLAS_CL
#define __TLAS_CL
void transformRay( Ray* ray, float* invT )
{
	ray->D = transformVector( &( ray->D ), invT );
	ray->O = transformPosition( &( ray->O ), invT );
	ray->rD = ( float4 )( 1.0f / ray->D.x, 1.0f / ray->D.y, 1.0f / ray->D.z, 1.0f );
}
int instanceIntersect( Ray* ray, BVHNode2* bvhNodes, uint* primIdxs, BVHInstance* bvhInstance, bool occlusion )
{
	// backup and transform ray using instance transform
	Ray backup = *ray;
	transformRay( ray, (float*)&bvhInstance->invT );
	// traverse the BLAS
#ifdef USE_BVH4
	int steps = intersectBVH4( ray, bvhNodes, primIdxs, bvhInstance->bvhIdx, occlusion );
#endif
#ifdef USE_BVH2
	int steps = intersectBVH2( ray, bvhNodes, primIdxs, bvhInstance->bvhIdx, occlusion );
#endif
	ray->D = backup.D;
	ray->O = backup.O;
	ray->rD = backup.rD;
	if ( occlusion ) if ( steps == -1 ) return -1;
	return steps;
}

int intersectTLAS(
	Ray* ray,
	TLASNode* tlasNodes,
	BVHInstance* blasNodes,
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
	float t_light = ray->t;
	while ( 1 ) {
		if ( node->leftRight == 0 ) {
			BVHInstance* bvhInstance = &blasNodes[node->BLASidx];
			int value = instanceIntersect( ray, bvhNodes, primIdxs, bvhInstance, occlusion );
			if ( occlusion ) if ( value == -1 ) return -1;
			steps += value;
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
		if ( dist1 >= t_light ) {
			// missed both child nodes; pop a node from the stack
			if ( stackPtr == 0 ) break;
			else node = stack[--stackPtr];
		} else {
			node = child1;
			if ( dist2 < t_light )
				stack[stackPtr++] = child2;
		}
		//steps++;
	}
	return steps;
}
#endif // __TLAS_CL