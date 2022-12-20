#ifndef __BVH_CL
#define __BVH_CL

#include "src/common.h"

float intersectAABB( Ray* ray, const float4 bmin, const float4 bmax )
{
	float tx1 = ( bmin.x - ray->O.x ) * ray->rD.x, tx2 = ( bmax.x - ray->O.x ) * ray->rD.x;
	float tmin = min( tx1, tx2 ), tmax = max( tx1, tx2 );
	float ty1 = ( bmin.y - ray->O.y ) * ray->rD.y, ty2 = ( bmax.y - ray->O.y ) * ray->rD.y;
	tmin = max( tmin, min( ty1, ty2 ) ), tmax = min( tmax, max( ty1, ty2 ) );
	float tz1 = ( bmin.z - ray->O.z ) * ray->rD.z, tz2 = ( bmax.z - ray->O.z ) * ray->rD.z;
	tmin = max( tmin, min( tz1, tz2 ) ), tmax = min( tmax, max( tz1, tz2 ) );
	if ( tmax >= tmin && tmin < ray->t && tmax > 0 ) return tmin; else return 1e30f;
}

uint intersectBVH4( Ray* ray, BVHNode4* bvhNode, uint* primIdx )
{
	BVHNode4* stack[128];
	BVHNode4* node = &bvhNode[0];
	uint stackPtr = 0;
	uint steps = 0;
	while ( 1 )
	{
		float dist[4];
		int order[4] = { 1, 2, 3, 4 };
		dist[0] = intersectAABB( ray, node->aabbMin0, node->aabbMax0 );
		dist[1] = intersectAABB( ray, node->aabbMin1, node->aabbMax1 );
		dist[2] = intersectAABB( ray, node->aabbMin2, node->aabbMax2 );
		dist[3] = intersectAABB( ray, node->aabbMin3, node->aabbMax3 );
		// sort
		for ( int i = 0; i < 4; i++ ) for ( int j = 0; j < 3; j++ )
			if ( dist[j] > dist[i] ) {	//swap them
				float d = dist[i]; dist[i] = dist[j]; dist[j] = d;
				int o = order[i]; order[i] = order[j]; order[j] = o;
			}

		for ( int i = 0; i < 4; i++ ) {
			int idx = order[i];

		}

		if ( node->count0 > 0 )
		{
			for ( uint i = 0; i < node->count0; i++ )
			{
				int index = primIdx[node->left0 + i];
				if ( index != INVALID ) intersect( index, &primitives[index], ray );
			}
			/*if ( stackPtr == 0 ) break;
			else node = stack[--stackPtr];
			continue;*/
		}
		if ( dist[0] == 1e30f )
		{
			if ( stackPtr == 0 ) break;
			else node = stack[--stackPtr];
		}
		else
		{
			steps++;
			node = child[0];
			if ( stackPtr > 64 ) printf( "%i\n", stackPtr );
			/*if ( dist[3] != 1e30f ) stack[stackPtr++] = child[3];
			if ( dist[2] != 1e30f ) stack[stackPtr++] = child[2];
			if ( dist[1] != 1e30f ) stack[stackPtr++] = child[1];*/
		}
	}
	return steps;
}

uint intersectBVH2( Ray* ray, BVHNode2* bvhNode, uint* primIdx )
{
	BVHNode2* stack[32];
	BVHNode2* node = &bvhNode[0];
	uint stackPtr = 0;
	uint steps = 0;
	while ( 1 )
	{
		if ( node->count > 0 ) // isLeaf?
		{
			for ( uint i = 0; i < node->count; i++ )
			{
				int index = primIdx[node->left + i];
				intersect( index, &primitives[index], ray );
			}
			if ( stackPtr == 0 ) break;
			else node = stack[--stackPtr];
			continue;
		}

		BVHNode2* child1 = &bvhNode[node->left];
		BVHNode2* child2 = &bvhNode[node->left + 1];
		float dist1 = intersectAABB( ray, child1->aabbMin, child1->aabbMax );
		float dist2 = intersectAABB( ray, child2->aabbMin, child2->aabbMax );

		if ( dist1 > dist2 )
		{
			// swap
			float d = dist1; dist1 = dist2; dist2 = d;
			BVHNode2* c = child1; child1 = child2; child2 = c;
		}

		if ( dist1 == 1e30f )
		{
			//printf( "decreasing stackptr" );
			if ( stackPtr == 0 ) break;
			else node = stack[--stackPtr];
		}
		else
		{
			steps++;
			node = child1;
			if ( stackPtr > 64 ) printf( "%i\n", stackPtr );
			if ( dist2 != 1e30f ) stack[stackPtr++] = child2;
		}

	}
	return steps;
}

#endif // __BVH_CL