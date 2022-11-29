int nPrimitives = 0;
int nLights = 0;

#include "kernels/constants.cl""
#include "kernels/ray.cl"
#include "kernels/primitives.cl"
#include "kernels/light.cl"
#include "kernels/camera.cl"

const sampler_t SAMPLER = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

float3 shoot2( Ray* primaryRay )
{
	float3 color = (float3)(0);

	Ray stack[1024];
	stack[0] = *primaryRay;
	int n = 1;
	while ( n > 0 )
	{
		Ray ray = stack[--n];
		for ( int i = 0; i < nPrimitives; i++ )
			intersect( i, primitives + i, &ray );

		if ( ray.primIdx != -1 )
		{
			float3 I = intersectionPoint( &ray );
			ray.N = getNormal( primitives + ray.primIdx, I );
			if ( ray.inside )
				ray.N = -ray.N;

			// we hit an object
			Primitive prim = primitives[ray.primIdx];
			Material mat = materials[prim.matIdx];
			float3 albedo = mat.color;
			if ( prim.objType == SPHERE )
			{
				float2 uv = getSphereUV( spheres + prim.objIdx, ray.N );
				//albedo = read_imagef(textures, SAMPLER, uv);
				if(uv.y < 0 || uv.y > 1 && !mat.isDieletric)
					printf("%f\n", uv.y);
				albedo = (float3)(uv.x, uv.y, 1);
			}
			
			/*switch ( prim.objType )
			{
			case SPHERE:
				float2 uv = getSphereUV( spheres + prim.objIdx, ray.N );
				albedo = (float3)(1,1, 1);
				break;
			}*/

			if ( !mat.isDieletric )
			{
				if ( mat.specular < 1 )
				{
					// find the diffuse of this object
					for ( int i = 0; i < nLights; i++ )
					{
						color += handleShadowRay( &ray, lights + i ) * mat.color * ray.intensity;
					}
				}
				if ( mat.specular > 0 && ray.bounces < MAX_BOUNCE )
				{
					// shoot another ray into the scene from the point of impact
					Ray* reflectRay = reflect( &ray, I );
					reflectRay->intensity *= mat.specular;
					stack[n++] = *reflectRay;
				}
			}
			else if ( ray.bounces < MAX_BOUNCE )
			{
				float costhetai = dot( ray.N, ray.rD );

				float n1 = mat.n1;
				float n2 = mat.n2;
				if ( ray.inside )
				{
					n1 = mat.n2;
					n2 = mat.n1;

					// give material absorption
					float3 a = (float3)(1.f, 1.f, 1.f);
					ray.intensity.x *= exp(-a.x * ray.t);
					ray.intensity.y *= exp(-a.y * ray.t);
					ray.intensity.z *= exp(-a.z * ray.t);
				}
				float frac = n1 * (1 / n2);
				float k = 1 - frac * frac * (1 - costhetai * costhetai);

				if ( k < 0 )
				{
					// total internal reflection
					// shoot another ray into the scene from the point of impact
					Ray* reflectRay = reflect( &ray, I );
					stack[n++] = *reflectRay;
				}
				else
				{
					// use fresnel's law to find reflection and refraction factors
					float3 T = frac * ray.D + ray.N * (frac * costhetai - sqrt( k ));
					T = normalize( T );
					float costhetat = dot( -ray.N, T );
					// precompute
					float n1costhetai = n1 * costhetai;
					float n2costhetai = n2 * costhetai;
					float n1costhetat = n1 * costhetat;
					float n2costhetat = n2 * costhetat;

					float frac1 = (n1costhetai - n2costhetat) / (n1costhetai + n2costhetat);
					float frac2 = (n1costhetat - n2costhetai) / (n1costhetat + n2costhetai);

					// calculate fresnel
					float Fr = 0.5f * (frac1 * frac1 + frac2 * frac2);
					if(Fr < 0 || Fr > 1)
						printf("%f\n", Fr);
					// create reflection ray
					Ray* reflectRay = reflect( &ray, I );
					reflectRay->intensity *= Fr;
					stack[n++] = *reflectRay;

					// create transmission ray
					Ray* transmissionRay = transmission( &ray, I, T );
					transmissionRay->intensity *= (1 - Fr);
					stack[n++] = *transmissionRay;
				}

			}
		}
	}

	return color;
}

__kernel void trace( __global write_only float3* pixels,
	__global Sphere* _spheres,
	__global Plane* _planes,
	__global Triangle* _triangles,
	__global Material* _materials,
	__global Primitive* _primitives,
	__global Light* _lights,
	Camera cam,
	int numPrimitives,
	int numLights)
//	read_only image2d_t _texture)
{
	int idx = get_global_id( 0 );
	int x = idx % SCRWIDTH;
	int y = idx / SCRWIDTH;

	nPrimitives = numPrimitives;
	nLights = numLights;

	spheres = _spheres;
	planes = _planes;
	triangles = _triangles;
	materials = _materials;
	primitives = _primitives;
	lights = _lights;
	//textures = _texture;

	// create and shoot a ray into the scene
	Ray ray = initPrimaryRay( x, y, &cam );
	float3 color = shoot2( &ray );
	// prevent color overflow
	color = min( color, (float3)(1) );
	pixels[idx] = color;

	//color *= 255;
	//write_imagef( target, (int2)(x, y), (float4)(color, 1) );

}