#define DEG_TO_RAD M_PI_F / 180


Ray initPrimaryRay(int x, int y, Camera* cam, uint* seed)
{
	switch (cam->type) {
	case PROJECTION: {
		float u = (float)x * (1.0f / SCRWIDTH);
		float v = (float)y * (1.0f / SCRHEIGHT);

		u += randomFloat(seed) / (float)SCRWIDTH;
		v += randomFloat(seed) / (float)SCRHEIGHT;

		float4 P = cam->topLeft + u * cam->horizontal + v * cam->vertical;
		return initRay(cam->origin, normalize(P - cam->origin));
	}break;
	case FISHEYE: { // Kevin Suffern's book: "Ray Tracing from the Ground Up" p.188
		float u = (float)x * (2.f / SCRWIDTH);
		float v = (float)y * (2.f / SCRHEIGHT);

		float r2 = u * u + v * v;

		if (r2 > 1.0) return initRay((float4)(0), (float4)(0));

		float r = sqrt(r2);
		float psi = r * cam->fov * DEG_TO_RAD;
		float sinPsi = sin(psi);
		float cosPsi = cos(psi);
		float sinAlpha = u / r;
		float cosAlpha = v / r;

		float4 D = sinPsi * cosAlpha * cam->right + sinPsi * sinAlpha * cam->up - cosPsi * cam->forward;
		return initRay(cam->origin, D);
	}break;
	}
}