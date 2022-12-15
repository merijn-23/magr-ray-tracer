#pragma once

typedef struct
{
	float4 O, D, rD; // 1 / D
	float4 N, I, intensity;
	float t;
	// Index of primitive
	int primIdx, bounces, pixelIdx;
	bool inside;
	float u, v, w; // barycenter, is calculated upon intersection
} Ray;

typedef struct
{
	float4 color, absorption;
	float specular, n1, n2;
	bool isDieletric;
	int texIdx;
	int texW, texH;

	// Kajiya
	bool isLight;
	float4 emittance; 
} Material;

typedef struct
{
	float4 pos;
	float r, r2, invr;
} Sphere;

typedef struct
{
	float4 N;
	float d;
} Plane;

typedef struct
{
	float4 v0, v1, v2;
	float4 N, centroid;
	float2 uv0, uv1, uv2;
} Triangle;

typedef struct
{
	union
	{
		Triangle triangle;
		Sphere sphere;
		Plane plane;
	} objData;

	int objType;
	int matIdx;
} Primitive;

typedef struct
{
	// Whitted
	float4 pos, color;
	float strength;

	// Kajiya
	int primIdx;
} Light;

typedef struct
{
	int type;
	float fov, aperture, focalLength;
	float4 forward, right, up;
	float4 origin, horizontal, vertical, topLeft;
} Camera;

typedef struct
{
	int numPrimitives, numLights, tracerType, frames, antiAliasing;
	int numInRays, numOutRays;
} Settings;

typedef struct
{
	float4 aabbMin, aabbMax;
	uint first, count;
} BVHNode;