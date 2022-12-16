#ifndef __CAMERA_CL
#define __CAMERA_CL

#define DEG_TO_RAD M_PI_F / 180

Ray initPrimaryRay(int x, int y, Camera cam, Settings* settings, uint* seed)
{
	switch (cam.type) {
	case PROJECTION: {
		float u = (float)x * (1.0f / SCRWIDTH);
		float v = (float)y * (1.0f / SCRHEIGHT);

#ifdef ANTIALIASING
		u += randomFloat(seed) / (float)SCRWIDTH;
		v += randomFloat(seed) / (float)SCRHEIGHT;
#endif

		float4 P = cam.topLeft + u * cam.horizontal + v * cam.vertical;
		float4 dir = normalize(P - cam.origin);

		float4 focalPoint = cam.origin + dir * cam.focalLength;
		float4 O = cam.origin + (randomFloat3(seed) - 0.5f) * cam.aperture;
		dir = normalize(focalPoint - O);
		return initRay(O, dir, true);
	}break;
	case FISHEYE: { // Kevin Suffern's book: "Ray Tracing from the Ground Up" p.188
		float u = (float)(x - SCRWIDTH * .5f) * (2.f / SCRWIDTH);
		float v = (float)(y - SCRHEIGHT * .5f) * (2.f / SCRHEIGHT);

#ifdef ANTIALIASING
		u += randomFloat( seed ) / (float)SCRWIDTH;
		v += randomFloat( seed ) / (float)SCRHEIGHT;
#endif

		float r2 = u * u + v * v;
		if (r2 > 1.0) return initRay((float4)(0), (float4)(0));
		float r = sqrt(r2);
		float psi = r * cam.fov * DEG_TO_RAD;
		float sinPsi = sin(psi);
		float cosPsi = cos(psi);
		float sinAlpha = u / r;
		float cosAlpha = v / r;
		float4 D = sinPsi * cosAlpha * cam.up + sinPsi * sinAlpha * cam.right - cosPsi * cam.forward;
		return initRay(cam.origin, D, true);
	}break;
	}
}

#endif // __CAMERA_CL