typedef struct Light
{
	float3 pos, colour;
	float strength;
    // primIdx is only used in path tracing
    int primIdx;
} Light;
__global Light* lights;


float3 getDiffuseShading( Light* light, float dot, float r )
{
    return dot * (1 / (r * r)) * light->strength * light->colour;
}

bool shootShadowRay( Ray* ray, float d )
{
    for(int i = 0; i < nPrimitives; i++)
    {
        intersect(i, primitives + i, ray);
        if(ray->t < d - epsilon)
            return false;
    }
    //printf("shoot");
    return true;
}

float3 handleShadowRay( Ray* ray, Light* light )
{
    float3 dir = intersectionPoint(ray) - light->pos;
    float dotP = dot(ray->N, -dir);
    if(dotP > 0)
    {
        //printf("handle");
        // Shoot a shadow ray into the scene and check if anything obstructs it
        Ray shadowRay = initRay(light->pos, dir);
        float dist = length(dir);
        if(shootShadowRay(&shadowRay, dist))
        {
            return getDiffuseShading(light, dotP, dist);
        }
    } 
    return (float3)(0);
}

