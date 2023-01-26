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
			float4 albedo = getAlbedo( ray );
#ifdef RUSSIAN_ROULETTE
			float rr_p = getSurvivalProb( albedo );
			if ( rr_p < randomFloat( seed ) )
				return BLACK;
			else
				ray->intensity *= 1 / rr_p;
#endif
			// diffuse 
			float4 diffuseReflection = randomRayHemisphere( ray->N, seed );
			float4 BRDF = albedo * M_1_PI_F;
			float I_PDF = 2 * M_PI_F;
			ray->intensity *= BRDF * I_PDF * dot( diffuseReflection, ray->N );
			r = initRay( ray->I + EPSILON * diffuseReflection, diffuseReflection );
			r.intensity = ray->intensity;
			r.bounces = ray->bounces + 1;

		}
	}
	r.pixelIdx = ray->pixelIdx;
	*extensionRay = r;
	return BLACK;
}

float4 neeShading(Ray* ray, Ray* extensionRay, ShadowRay* shadowRay, Settings* settings, uint* seed)
{
	// we hit an object
	Primitive prim = primitives[ray->primIdx];
	Material mat = materials[prim.matIdx];
	if (mat.isLight){
		if(ray->lastSpecular) return ray->intensity * mat.emittance;
		else return BLACK;	
	}
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
		r.lastSpecular = true;
	}
	else
	{
		if (rand < mat.specular)
		{
			r = reflect( ray );
			r.lastSpecular = true;
		}
		else
		{
			// calculate BRDF
			float4 albedo = getAlbedo( ray );
			float4 BRDF = albedo * M_1_PI_F;
			
			// sample a random light source
			// output: a new shadow ray
			if (settings->numLights > 0)
			{
				uint lightIdx = lights[(uint)(floor( random( seed ) * settings->numLights ))];
				float4 pointOnLight = getRandomPoint( primitives + lightIdx, seed );
				//printf("Point on light: %f %f %f\n", pointOnLight.x, pointOnLight.y, pointOnLight.z);
				float4 dirToLight = pointOnLight - ray->I;
				float4 normalOnLight = getNormal( primitives + lightIdx, pointOnLight );
				//printf("normal on light: %f %f %f\n", normalOnLight.x, normalOnLight.y, normalOnLight.z);
				float dist = length( dirToLight );
				float dotNL = dot( ray->N, dirToLight * (1 / dist) );
				//printf("dotnl %f\n", dotNL);
				//printf("dotn-l %f\n", dot(normalOnLight, -dirToLight));
				/* no check to see if normalonlight dot -dirtolight > 0 because of infinitely thin light sources
				if  && dot(normalOnLight, -dirToLight) > 0*/
				if (dotNL > 0 && dot( normalOnLight, -dirToLight ) > 0)
				{
					//printf("hi\n");
					ShadowRay sr;
					sr.I = ray->I;
					sr.L = pointOnLight;
					sr.intensity = ray->intensity * settings->numLights;
					sr.BRDF = BRDF;
					sr.lightIdx = lightIdx;
					sr.pixelIdx = ray->pixelIdx;
					sr.dotNL = dotNL;
					sr.dist = dist;
					*shadowRay = sr;
				}
			}
			// russian roullete
#ifdef RUSSIAN_ROULETTE
			float rr_p = getSurvivalProb( albedo );
			if ( rr_p < randomFloat( seed ) )
				return BLACK;
			else
				ray->intensity *= 1 / rr_p;
#endif
			// diffuse 
			float I_PDF = 2 * M_PI_F;
			float4 diffuseReflection = randomRayHemisphere( ray->N, seed );
			r = initRay( ray->I + EPSILON * diffuseReflection, diffuseReflection );
			r.intensity = ray->intensity * BRDF * I_PDF * dot( diffuseReflection, ray->N );;
			r.bounces = ray->bounces + 1;
		}
	}
	r.pixelIdx = ray->pixelIdx;
	*extensionRay = r;
	return BLACK;
}
#endif // __SHADING_CL