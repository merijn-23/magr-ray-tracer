#pragma once

#define SCRWIDTH		1280
#define SCRHEIGHT		720
#define PIXELS			SCRWIDTH * SCRHEIGHT

#define MAX_BOUNCES 7
#define MAX_RAYS 64
#define EPSILON 0.0001f

#define SPHERE			0
#define PLANE			1
#define TRIANGLE		2

#define PROJECTION		0
#define FISHEYE			1

#define WHITTED			0 
#define KAJIYA			1

#define INVALID			-1
//#define USE_BVH4
#define REALLYFAR		1e30f

#define BVH_BINS 8
#define MIN_LEAF_PRIMS 2

// gpu architecture
#define STREAMING_MULTIPROCESSORS 5 // GTX 1060

// create 8 full warps per SM of persistent threads
#define NR_OF_PERSISTENT_THREADS 256 * STREAMING_MULTIPROCESSORS