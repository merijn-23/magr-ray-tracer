// 7 bytes
typedef struct __attribute__ ((packed)) Ray
{
	float3 O, D, rD;
	float t;
	int objIdx;
	bool inside;
};

typedef struct __attribute__ ((packed)) Sphere
{
	float3 pos = 0;
	float r2 = 0, invr = 0;
	int objIdx = -1;
	int matIdx = -1;
};

void Intersect(Sphere* sphere, Ray* ray)
{
	float3 oc = ray->O - sphere->pos;
	float b = dot(oc, ray->D);
	float c = dot(oc, oc) - sphere->r2;
	float t, d = b * b - c;
	if (d <= 0) return;
	d = sqrtf(d), t = -b - d;
	if (t < ray->t && t > 0)
	{
		ray->t = t, ray->objIdx = sphere->objIdx;
		return;
	}
	t = d - b;
	if (t < ray->t && t > 0)
	{
		ray->t = t, ray->objIdx = sphere->objIdx;
		return;
	}
}

float3 GetNormal( Sphere* sphere, float3 I ) 
{
	return (I - sphere->pos) * sphere->invr;
}

float3 GetAlbedo( float3 I )
{
	return float3( 0.93f );
}

