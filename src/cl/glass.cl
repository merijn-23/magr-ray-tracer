#ifndef __GLASS_CL
#define __GLASS_CL

void beersLaw( Ray* ray, Material* mat )
{
	ray->intensity.x *= exp( -mat->absorption.x * ray->t );
	ray->intensity.y *= exp( -mat->absorption.y * ray->t );
	ray->intensity.z *= exp( -mat->absorption.z * ray->t );
}

float fresnel( Ray* ray, Material* mat, float4* outT )
{
	float costhetai = dot( ray->N, ray->D * -1 );

	float n1 = mat->n1;
	float n2 = mat->n2;
	if (ray->inside)
	{
		n1 = mat->n2;
		n2 = mat->n1;

		// give material absorption
		beersLaw( ray, mat );
	}

	float frac = n1 * (1 / n2);
	float k = 1 - frac * frac * (1 - costhetai * costhetai);

	// TIR
	if (k < 0) return 1.f;

	// use fresnel's law to find reflection and refraction factors
	(*outT) = normalize(
		frac * ray->D + ray->N * (frac * costhetai - sqrt( k ))
	);
	float costhetat = dot( -(ray->N), (*outT) );
	// precompute
	float n1costhetai = n1 * costhetai;
	float n2costhetai = n2 * costhetai;
	float n1costhetat = n1 * costhetat;
	float n2costhetat = n2 * costhetat;

	float frac1 = (n1costhetai - n2costhetat) / (n1costhetai + n2costhetat);
	float frac2 = (n1costhetat - n2costhetai) / (n1costhetat + n2costhetai);

	// calculate fresnel
	float Fr = 0.5f * (frac1 * frac1 + frac2 * frac2);
	return mat->specular + (1 - mat->specular) * Fr;
}

#endif // __GLASS_CL