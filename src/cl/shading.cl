#ifndef __SHADING_CL
#define __SHADING_CL

#define BLACK ( float4 )(0)
#define WHITE ( float4 )(1)

float4 kajiyaShading( Ray* ray, Ray* extensionRay, uint* seed )
{
	// we hit an object
	Primitive prim = primitives[ray->primIdx];
	Material mat = materials[prim.matIdx];

	if (mat.isLight) return ray->intensity * mat.emittance;

	Ray r;
	float rand = randomFloat( seed );

	if (mat.isDieletric)
	{
		float4 T = (float4)(0);
		float Fr = fresnel( ray, &mat, &T );
		if (rand < Fr)
		{
			r = reflect( ray ); // reflection
		}
		else
		{
			r = transmit( ray, T ); // refraction
		}
	}
	else
	{
		if (rand < mat.specular)
		{
			r = reflect( ray );
		}
		else
		{
			// diffuse 
			float4 diffuseReflection = randomRayHemisphere( ray->N, seed );
			ray->intensity *= getAlbedo( ray ) * dot( diffuseReflection, ray->N );
			r = initRay( ray->I + EPSILON * diffuseReflection, diffuseReflection );
			r.intensity = ray->intensity;
			r.bounces = ray->bounces + 1;
		}
	}

	r.pixelIdx = ray->pixelIdx;
	*extensionRay = r;
	return BLACK;
}

#endif // __SHADING_CL