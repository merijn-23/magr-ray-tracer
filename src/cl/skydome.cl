// https://www.cs.uu.nl/docs/vakken/magr/2021-2022/files/lecture%2011%20-%20various.pdf
float4 readSkydome( float4 dir )
{
	//return (float4)(0);
	float u = 1 + atan2( dir.x, -dir.z ) / M_PI_F;
	float v = acos( dir.y ) / M_PI_F;

	u /= 2;

	Material mat = materials[0];
	int x = (int)(u * mat.texW);
	int y = (int)(v * mat.texH);
	return textures[mat.texIdx + x + y * mat.texW];
}