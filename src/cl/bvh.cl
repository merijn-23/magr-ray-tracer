#ifndef __BVH_CL
#define __BVH_CL
float intersectAABB( Ray* ray, const float4 bmin, const float4 bmax )
{
	float tx1 = ( bmin.x - ray->O.x ) * ray->rD.x, tx2 = ( bmax.x - ray->O.x ) * ray->rD.x;
	float tmin = min( tx1, tx2 ), tmax = max( tx1, tx2 );
	float ty1 = ( bmin.y - ray->O.y ) * ray->rD.y, ty2 = ( bmax.y - ray->O.y ) * ray->rD.y;
	tmin = max( tmin, min( ty1, ty2 ) ), tmax = min( tmax, max( ty1, ty2 ) );
	float tz1 = ( bmin.z - ray->O.z ) * ray->rD.z, tz2 = ( bmax.z - ray->O.z ) * ray->rD.z;
	tmin = max( tmin, min( tz1, tz2 ) ), tmax = min( tmax, max( tz1, tz2 ) );
	if ( tmax >= tmin && tmin < ray->t && tmax > 0 ) return tmin; else return REALLYFAR;
}
uint intersectBVH2( Ray* ray, BVHNode2* bvhNode, uint* primIdxs, uint bvhIdx )
{
	BVHNode2* stack[32];
	BVHNode2* node = bvhNode + bvhIdx;
	uint stackPtr = 0;
	uint steps = 0;
	while ( 1 ) {
		if ( node->count > 0 ) // isLeaf?
		{
			for ( uint i = 0; i < node->count; i++ ) {
				int index = primIdxs[node->first + i];
				intersect( index, &primitives[index], ray );
			}
			if ( stackPtr == 0 ) break;
			else node = stack[--stackPtr];
			continue;
		}
		BVHNode2* child1 = &bvhNode[node->first];
		BVHNode2* child2 = &bvhNode[node->first + 1];
		float dist1 = intersectAABB( ray, child1->aabbMin, child1->aabbMax );
		float dist2 = intersectAABB( ray, child2->aabbMin, child2->aabbMax );
		if ( dist1 > dist2 ) {
			// swap
			float d = dist1; dist1 = dist2; dist2 = d;
			BVHNode2* c = child1; child1 = child2; child2 = c;
		}
		if ( dist1 == REALLYFAR ) {
			if ( stackPtr == 0 ) break;
			else node = stack[--stackPtr];
		} else {
			steps++;
			node = child1;
			if ( dist2 != REALLYFAR ) {
				stack[stackPtr++] = child2;
				steps++;
			}
		}
	}
	return steps;
}
uint intersectBVH4( Ray* ray, BVHNode4* bvhNode, uint* primIdxs, uint bvhIdx )
{
	BVHNode4* stack[64];
	BVHNode4* node = bvhNode + bvhIdx;
	uint stackPtr = 0;
	uint steps = 0;
	while ( 1 ) {
		steps++;
		float dist[4];
		int order[4] = { 0, 1, 2, 3 };
		dist[0] = node->first[0] != INVALID ? intersectAABB( ray, node->aabbMin[0], node->aabbMax[0] ) : REALLYFAR;
		dist[1] = node->first[1] != INVALID ? intersectAABB( ray, node->aabbMin[1], node->aabbMax[1] ) : REALLYFAR;
		dist[2] = node->first[2] != INVALID ? intersectAABB( ray, node->aabbMin[2], node->aabbMax[2] ) : REALLYFAR;
		dist[3] = node->first[3] != INVALID ? intersectAABB( ray, node->aabbMin[3], node->aabbMax[3] ) : REALLYFAR;
		// sort
		//for ( int i = 0; i < 4; i++ ) for ( int j = 0; j < 3; j++ ) {
		//	if ( dist[j] > dist[i] ) {
		//		//swap them
		//		float d = dist[i]; dist[i] = dist[j]; dist[j] = d;
		//		int o = order[i]; order[i] = order[j]; order[j] = o;
		//	}
		//}
		for ( int i = 0; i < 4; i++ ) {
			int index = order[i];
			if ( node->first[index] == INVALID ) continue;
			if ( dist[index] == REALLYFAR ) continue;
			if ( node->count[index] > 0 ) {
				for ( uint j = 0; j < node->count[index]; j++ ) {
					int primIdx = primIdxs[node->first[index] + j];
					intersect( primIdx, primitives + primIdx, ray );
				}
			} else {
				stack[stackPtr++] = bvhNode + node->first[index];
			}
		}
		if ( stackPtr == 0 ) break;
		node = stack[--stackPtr];
	}
	return steps;
}

bool intersectBVH2Occlusion( Ray* ray, BVHNode2* bvhNode, uint* primIdx )
{
	float light_t = ray->t;
	BVHNode2* stack[32];
	BVHNode2* node = &bvhNode[0];
	uint stackPtr = 0;
	uint steps = 0;
	while ( 1 ) {
		if ( node->count > 0 ) // isLeaf?
		{
			for ( uint i = 0; i < node->count; i++ ) {
				int index = primIdx[node->first + i];
				intersect( index, &primitives[index], ray );
				if ( ray->t < light_t ) return true;
			}
			if ( stackPtr == 0 ) break;
			else node = stack[--stackPtr];
			continue;
		}
		BVHNode2* child1 = &bvhNode[node->first];
		BVHNode2* child2 = &bvhNode[node->first + 1];
		float dist1 = intersectAABB( ray, child1->aabbMin, child1->aabbMax );
		float dist2 = intersectAABB( ray, child2->aabbMin, child2->aabbMax );
		if ( dist1 > dist2 ) {
			// swap
			float d = dist1; dist1 = dist2; dist2 = d;
			BVHNode2* c = child1; child1 = child2; child2 = c;
		}
		if ( dist1 > light_t ) {
			if ( stackPtr == 0 ) break;
			else node = stack[--stackPtr];
		} else {
			steps++;
			node = child1;
			if ( dist2 < light_t ) {
				stack[stackPtr++] = child2;
				steps++;
			}
		}
	}
	return false;
}
#endif // __BVH_CL