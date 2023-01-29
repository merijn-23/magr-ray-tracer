#ifndef __SKYDOME_CL
#define __SKYDOME_CL

// https://www.cs.uu.nl/docs/vakken/magr/2021-2022/files/lecture%2011%20-%20various.pdf
float4 readSkydome( float4 dir )
{
	return (float4)( 0.0784f, 0.0941f, 0.3215f, 0 );
	float u = (1 + atan2pi( dir.x, -dir.z )) * 0.5f;
	float v = acospi( dir.y );
	Material mat = materials[0];
	int x = (int)(u * mat.texW);
	int y = (int)(v * mat.texH);
	return textures[mat.texIdx + x + y * mat.texW];
}

#endif // __SKYDOME_CL