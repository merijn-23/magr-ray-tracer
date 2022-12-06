#pragma once

typedef struct
{
	float4 O, D, N, I, intensity;
	float t;
	// Index of primitive
	int primIdx, bounces;
	bool inside;
	float u, v, w; // barycenter, is calculated upon intersection
} Ray;

typedef struct
{
	int objType;
	// Index of object in respective list
	int objIdx;
	int matIdx;
} Primitive;

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
	float r2, invr;
} Sphere;

typedef struct
{
	float4 N;
	float d;
} Plane;

typedef struct
{
	float4 v0, v1, v2;
	float2 uv0, uv1, uv2;
	float4 N;
} Triangle;

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
	float fov;
	float4 forward, right, up;
	float4 origin, horizontal, vertical, topLeft;
} Camera;

typedef struct
{
	int numPrimitives, numLights, tracerType, frames, antiAliasing;
} Settings;