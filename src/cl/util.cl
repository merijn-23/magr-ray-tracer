#ifndef __UTIL_CL
#define __UTIL_CL

uint shiftBits( uint x )
{
	// x =                                 00000000000000001111111111111111
	x = ( x | ( x << 8 ) ) & 0x00FF00FF;	// 00000000111111110000000011111111
	x = ( x | ( x << 4 ) ) & 0x0F0F0F0F;	// 00001111000011110000111100001111
	x = ( x | ( x << 2 ) ) & 0x33333333;	// 00110011001100110011001100110011
	return ( x | ( x << 1 ) ) & 0x55555555; // 01010101010101010101010101010101
}

uint reverseShiftBits( uint x )
{
	// 01010101010101010101010101010101
	x = x & 0x55555555;
	x = ( x | ( x >> 1 ) ) & 0x33333333;
	x = ( x | ( x >> 2 ) ) & 0x0F0F0F0F;
	x = ( x | ( x >> 4 ) ) & 0x00FF00FF;
	return ( x | ( x >> 8 ) ) & 0x0000FFFF;
}

// transforms xyz to z-order curve according to https://stackoverflow.com/questions/12157685/z-order-curve-coordinates
uint zOrderCurve( uint x, uint y )
{
	return shiftBits( x ) | ( shiftBits( y ) << 1 );
}

uint2 reverseZOrderCurve( uint idx )
{
	// yxyxyxyxy
	uint x = reverseShiftBits( idx );
	uint y = reverseShiftBits( idx >> 1 );
	return ( uint2 )( x, y );
}

uint wangHash( uint s )
{
	s = ( s ^ 61 ) ^ ( s >> 16 );
	s *= 9, s = s ^ ( s >> 4 );
	s *= 0x27d4eb2d;
	s = s ^ ( s >> 15 );
	return s;
}
uint initSeed( uint seedBase )
{
	return wangHash( ( seedBase + 1 ) * 17 );
}

uint randomUInt( uint* seed )
{
	*seed ^= *seed << 13;
	*seed ^= *seed >> 17;
	*seed ^= *seed << 5;
	return *seed;
}
float randomFloat( uint* seed ) { return randomUInt( seed ) * 2.3283064365387e-10f; }
float random( uint* seed ) { return fabs( randomFloat( seed ) ); }
float4 randomFloat3( uint* seed ) { return ( float4 )( randomFloat( seed ), randomFloat( seed ), randomFloat( seed ), 0 ); };

float4 transformVector( float4* V, float* T )
{
	float4 tv = ( float4 )(
		//dot( T->s012, V->xyz ),
		//dot( T->s456, V->xyz ),
		//dot( T->s89a, V->xyz ),
		dot( ( float3 )( T[0], T[1], T[2] ), V->xyz ),
		dot( ( float3 )( T[4], T[5], T[6] ), V->xyz ),
		dot( ( float3 )( T[8], T[9], T[10] ), V->xyz ),
		0.0f );
	//printf( "Vector\nT \n%f, %f, %f, \n%f, %f, %f, \n%f, %f, %f\n V %f, %f, %f, %f\n TV %f, %f, %f, %f\n", T[0], T[1], T[2], T[4], T[5], T[6], T[8], T[9], T[10], V->x, V->y, V->z, V->w, tv.x, tv.y, tv.z, tv.w );
	return tv;
}

float4 transformPosition( float4* V, float* T )
{
	 float4 tv = ( float4 )(
		//dot( T->s012, V->xyz ) + T->s3,
		//dot( T->s456, V->xyz ) + T->s7,
		//dot( T->s89a, V->xyz ) + T->sb,
		dot( ( float3 )( T[0], T[1], T[2] ), V->xyz ) + T[3],
		dot( ( float3 )( T[4], T[5], T[6] ), V->xyz ) + T[7],
		dot( ( float3 )( T[8], T[9], T[10] ), V->xyz ) + T[11],
		0.0f );
	//printf( "Position\nT \n%f, %f, %f, \n%f, %f, %f, \n%f, %f, %f\n V %f, %f, %f, %f\n TV %f, %f, %f, %f\n", T[0], T[1], T[2], T[4], T[5], T[6], T[8], T[9], T[10], V->x, V->y, V->z, V->w, tv.x, tv.y, tv.z, tv.w ); 
	return tv;
}

#endif // __UTIL_CL