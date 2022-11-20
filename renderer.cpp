#include "precomp.h"

// -----------------------------------------------------------
// Initialize the renderer
// -----------------------------------------------------------
void Renderer::Init()
{
	// create fp32 rgb pixel buffer to render to
	//accumulator = (float4*)MALLOC64( SCRWIDTH * SCRHEIGHT * 16 );
	//memset( accumulator, 0, SCRWIDTH * SCRHEIGHT * 16 );
	//std::cout << sizeof(RayGPU);
	//printf("%i\n", sizeof(RayGPU));
	InitKernel();

}

// -----------------------------------------------------------
// Evaluate light transport
// -----------------------------------------------------------
//float3 Renderer::Trace( Ray& ray )
//{
//	scene.FindNearest( ray );
//	if (ray.objIdx == -1) return 0; // or a fancy sky color
//	float3 I = ray.O + ray.t * ray.D;
//	float3 N = scene.GetNormal( ray.objIdx, I, ray.D );
//	float3 albedo = scene.GetAlbedo( ray.objIdx, I );
//	/* visualize normal */ return (N + 1) * 0.5f;
//	/* visualize distance */ // return 0.1f * float3( ray.t, ray.t, ray.t );
//	/* visualize albedo */ // return albedo;
//}


// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void Renderer::Tick( float deltaTime )
{
	// animation
	static float animTime = 0;
	scene.SetTime( animTime += deltaTime * 0.002f );
	// pixel loop
	Timer t;

	//UpdateBuffers();

	kernel->Run(SCRWIDTH * SCRHEIGHT);
	pixelBuffer->CopyFromDevice();
	

#if 0
	// lines are executed as OpenMP parallel tasks (disabled in DEBUG)
	#pragma omp parallel for schedule(dynamic)
	for (int y = 0; y < SCRHEIGHT; y++)
	{
		// trace a primary ray for each pixel on the line
		for (int x = 0; x < SCRWIDTH; x++)
			accumulator[x + y * SCRWIDTH] =
				float4( Trace( camera.GetPrimaryRay( x, y ) ), 0 );
		// translate accumulator contents to rgb32 pixels
		for (int dest = y * SCRWIDTH, x = 0; x < SCRWIDTH; x++)
			screen->pixels[dest + x] = 
				RGBF32_to_RGB8( &accumulator[x + y * SCRWIDTH] );
	}
#endif
	// performance report - running average - ms, MRays/s
	static float avg = 10, alpha = 1;
	avg = (1 - alpha) * avg + alpha * t.elapsed() * 1000;
	if (alpha > 0.05f) alpha *= 0.5f;
	float fps = 1000 / avg, rps = (SCRWIDTH * SCRHEIGHT) * fps;
	printf( "%5.2fms (%.1f fps) - %.1fMrays/s\n", avg, fps, rps / 1000000 );
}

void Renderer::InitKernel()
{
	kernel = new Kernel("kernels/trace.cl", "trace");

	//sphereBuffer = new Buffer(sizeof(scene.spheres));
	sphereBuffer = new Buffer(sizeof(scene.spheres));
	planeBuffer = new Buffer(sizeof(scene.planes));
	cubeBuffer = new Buffer(sizeof(scene.cubes));
	matBuffer = new Buffer(sizeof(scene.mats));
	primBuffer = new Buffer(sizeof(scene.prims));
	lightBuffer = new Buffer(sizeof(scene.lights));
	pixelBuffer = new Buffer(4 * SCRHEIGHT * SCRWIDTH);

	//sphereBuffer->hostBuffer = (uint*)scene.spheres;
	sphereBuffer->hostBuffer = (uint*)scene.spheres;
	planeBuffer->hostBuffer = (uint*)scene.planes;
	cubeBuffer->hostBuffer = (uint*)scene.cubes;
	matBuffer->hostBuffer = (uint*)scene.mats;
	primBuffer->hostBuffer = (uint*)scene.prims;
	lightBuffer->hostBuffer = (uint*)scene.lights;
	pixelBuffer->hostBuffer = screen->pixels; 

	kernel->SetArguments(pixelBuffer, sphereBuffer, planeBuffer, matBuffer, primBuffer, lightBuffer);
	clSetKernelArg(kernel->kernel, 6, sizeof(Camera), &camera.cam);
	kernel->SetArgument(7, (int)(sizeof(scene.prims) / sizeof(Primitive)));
	kernel->SetArgument(8, (int)(sizeof(scene.lights)/sizeof(Light)));
	//clSetKernelArg(kernel->kernel, 5, sizeof(int), sizeof(scene.prims)/sizeof(scene.prims[0]));
	

	sphereBuffer->CopyToDevice();
	planeBuffer->CopyToDevice();
	cubeBuffer->CopyToDevice();
	matBuffer->CopyToDevice();
	primBuffer->CopyToDevice();
	lightBuffer->CopyToDevice();
}

void Renderer::UpdateBuffers()
{
	// todo: if buffer changed, update data
	//sphereBuffer->CopyToDevice();

	//std::cout << sizeof(RayGPU);
	
	//rayBuffer->CopyToDevice();
}