#pragma once

typedef struct Ray
{
	float4 O, D, rD; // 1 / D
	float4 N, I, intensity;
	float t;
	// Index of primitive
	int primIdx, bounces, pixelIdx;
	bool inside;
	float u, v; // barycenter, is calculated upon intersection
} Ray;

typedef struct Material
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

typedef struct Sphere
{
	float4 pos;
	float r, r2, invr;
} Sphere;

typedef struct Plane
{
	float4 N;
	float d;
} Plane;

typedef struct Triangle
{
	float4 v0, v1, v2;
	float4 N, centroid;
	float2 uv0, uv1, uv2;
} Triangle;

typedef struct Primitive
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

typedef struct Light
{
	// Whitted
	float4 pos, color;
	float strength;

	// Kajiya
	int primIdx;
} Light;

typedef struct Camera
{
	int type;
	float fov, aperture, focalLength;
	float4 forward, right, up;
	float4 origin, horizontal, vertical, topLeft;
} Camera;

typedef struct Settings
{
	int numPrimitives, numLights, tracerType, frames, antiAliasing;
	int numInRays, numOutRays;
	int renderBVH;
} Settings;

typedef struct BVHNode2
{
	float4 aabbMin, aabbMax;
	uint left, count;
} BVHNode2;

typedef struct BVHNode4
{
	float4 aabbMin[4], aabbMax[4];
	uint left[4], count[4];
} BVHNode4;