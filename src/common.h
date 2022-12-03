#pragma once

typedef struct Ray
{
	float4 O, D, rD, N, intensity;
	float t;
	// Index of primitive
	int primIdx, bounces;
	bool inside;
	float u, v, w; // barycenter, is calculated upon intersection
} Ray;

typedef struct Primitive
{
	int objType;
	// Index of object in respective list
	int objIdx;
	int matIdx;
} Primitive;

typedef struct Material
{
	float4 color;
	float specular, n1, n2;
	bool isDieletric;
	int texIdx;
	int texW, texH;

	// Kajiya
	bool isEmissive;
	float strength;
} Material;

typedef struct Sphere
{
	float4 pos;
	float r2, invr;
} Sphere;

typedef struct Plane
{
	float4 N;
	float d;
} Plane;

typedef struct Triangle
{
	float4 v0, v1, v2;
	float2 uv0, uv1, uv2;
	float4 N;
} Triangle;

typedef struct Light
{
	// Whitted
	float4 pos, color;
	float strength;

	// Kajiya
	int primIdx;
} Light;



// typedef struct Cube
// {
// 	float4 b[2];
// 	float4x4 M, invM;
// } Cube;