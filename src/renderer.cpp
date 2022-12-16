#include "precomp.h"

// ImGui variables
static float vignet_strength = 0.f;
static float chromatic = 0.f;
static float gamma_corr = 1.f;

// -----------------------------------------------------------
// Initialize the renderer
// -----------------------------------------------------------
void Renderer::Init()
{
	settings = new Settings();
	bvh.BuildBvh( scene.primitives );
	InitBuffers();
	InitWavefrontKernels();
	InitPostProcKernels();
}

void Renderer::Shutdown() {}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void Renderer::Tick( float _deltaTime )
{
	deltaTime = _deltaTime;
	// animation
	static float animTime = 0;
	scene.SetTime( animTime += deltaTime * 0.002f );
	// pixel loop
	Timer t;

	//UpdateBuffers();
	camera.UpdateCamVec();
	//CamToDevice();

	if (camera.moved)
	{
		resetKernel->Run(PIXELS);
		camera.moved = false;
		settings->frames = 1;
	}

	if (resetKernels)
	{
		InitWavefrontKernels();
		resetKernels = false;
	}

	RayTrace();

	if (showEnergy)
	{
		ComputeEnergy();
	}

	PostProc();

	settings->frames++;
	
	// performance report - running average - ms, MRays/s
	static float avg = 10, alpha = 1;
	avg = (1 - alpha) * avg + alpha * t.elapsed() * 1000;
	if (alpha > 0.05f)
		alpha *= 0.5f;
	float fps = 1000 / avg, rps = (SCRWIDTH * SCRHEIGHT) * fps;
	printf( "%5.2fms (%.1f fps) - %.1fMrays/s\n", avg, fps, rps / 1000000 );
}

void Renderer::RayTrace()
{
	for (int i = 0; i < SCRHEIGHT * SCRWIDTH; i++)
		seedBuffer->hostBuffer[i] = RandomUInt();
	seedBuffer->CopyToDevice();

	settings->numInRays = PIXELS;
	settings->numOutRays = 0;
	settingsBuffer->CopyToDevice();

	// generate initial primary rays
	generateKernel->SetArgument( 0, ray1Buffer );
	clSetKernelArg( generateKernel->kernel, 3, sizeof( Camera ), &camera.cam );
	generateKernel->Run( settings->numInRays );
	
	while (true)
	{
		extendKernel->SetArgument( 0, ray1Buffer );
		extendKernel->Run( settings->numInRays );

		shadeKernel->SetArgument( 0, ray1Buffer );
		shadeKernel->SetArgument( 1, ray2Buffer );
		shadeKernel->Run( settings->numInRays );

		settingsBuffer->CopyFromDevice();
		if (settings->numOutRays < MAX_RAYS) break;

		settings->numInRays = settings->numOutRays;
		settings->numOutRays = 0;
		settingsBuffer->CopyToDevice();
		std::swap( ray1Buffer, ray2Buffer );
	}
}

void Renderer::PostProc()
{
	// Post processing
	Buffer* src = swap1Buffer;
	Buffer* dst = swap2Buffer;

	// Kernel takes values from pixelbuffer and puts them in src
	post_prepKernel->SetArguments( accumBuffer, src, settingsBuffer );
	post_prepKernel->Run( PIXELS );

	if (vignet_strength > 0)
	{
		post_vignetKernel->SetArguments( src, dst, vignet_strength );
		post_vignetKernel->Run( PIXELS );
		std::swap( src, dst );
	}
	if (gamma_corr != 1)
	{
		post_gammaKernel->SetArguments( src, dst, gamma_corr );
		post_gammaKernel->Run( PIXELS );
		std::swap( src, dst );
	}
	if (chromatic > 0)
	{
		post_chromaticKernel->SetArguments( src, dst, chromatic );
		post_chromaticKernel->Run( PIXELS );
		std::swap( src, dst );
	}

	// Kernel takes values from src and writes them to screen texture
	displayKernel->SetArgument( 0, src );
	displayKernel->Run( PIXELS );
}

void Renderer::ComputeEnergy()
{
	accumBuffer->CopyFromDevice();
	float4* pixels = (float4*)(accumBuffer->hostBuffer);

	energy = 0;
	for (int i = 0; i < PIXELS; i++)
	{
		energy += pixels[i].x;
		energy += pixels[i].y;
		energy += pixels[i].z;
	}

	energy *= 1 / (float)(settings->frames);
}

void Renderer::InitBuffers()
{
	primBuffer = new Buffer( sizeof( Primitive ) * scene.primitives.size() );
	texBuffer = new Buffer( sizeof( float4 ) * scene.textures.size() );
	matBuffer = new Buffer( sizeof( Material ) * scene.materials.size() );

	ray1Buffer = new Buffer( 2 * PIXELS * sizeof( Ray ) );
	ray2Buffer = new Buffer( 2 * PIXELS * sizeof( Ray ) );
	shadowRayBuffer = new Buffer( 2 * PIXELS * sizeof( Ray ) );

	swap1Buffer = new Buffer( 4 * 4 * PIXELS );
	swap2Buffer = new Buffer( 4 * 4 * PIXELS );
	accumBuffer = new Buffer( 4 * 4 * PIXELS );
	screenBuffer = new Buffer( GetRenderTarget()->ID, 0, Buffer::TARGET );
	screen = 0;

	seedBuffer = new Buffer( sizeof( uint ) * PIXELS );
	settingsBuffer = new Buffer( sizeof( Settings ) );

	primBuffer->hostBuffer = (uint*)scene.primitives.data();
	matBuffer->hostBuffer = (uint*)scene.materials.data();
	texBuffer->hostBuffer = (uint*)scene.textures.data();
	settingsBuffer->hostBuffer = (uint*)settings;
	seedBuffer->hostBuffer = new uint[PIXELS];

	primBuffer->CopyToDevice();
	texBuffer->CopyToDevice();
	matBuffer->CopyToDevice();
}

void Renderer::InitWavefrontKernels()
{
	std::vector<std::string> defines = {};
	if (defineAntiAliasing) defines.push_back( "ANTIALIASING" );
	switch (defineShadingType)
	{
		case SHADING_SIMPLE:
			defines.push_back( "SHADING_SIMPLE" );
			break;
		case SHADING_NEE:
			defines.push_back( "SHADING_NEE" );
			break;
	}

	generateKernel = new Kernel( "src/cl/wavefront.cl", "generate", defines );
	extendKernel = new Kernel( "src/cl/wavefront.cl", "extend", defines );
	shadeKernel = new Kernel( "src/cl/wavefront.cl", "shade", defines );

	settings->numPrimitives = scene.primitives.size();
	settings->numLights = scene.lights.size();

	generateKernel->SetArgument( 1, settingsBuffer );
	generateKernel->SetArgument( 2, seedBuffer );

	extendKernel->SetArgument( 1, primBuffer );
	extendKernel->SetArgument( 2, bvh.GetBVHNodeBuffer() );
	extendKernel->SetArgument( 3, bvh.GetIdxBuffer() );
	extendKernel->SetArgument( 4, settingsBuffer );

	shadeKernel->SetArgument( 2, shadowRayBuffer );
	shadeKernel->SetArgument( 3, primBuffer );
	shadeKernel->SetArgument( 4, texBuffer );
	shadeKernel->SetArgument( 5, matBuffer );
	shadeKernel->SetArgument( 6, settingsBuffer );
	shadeKernel->SetArgument( 7, accumBuffer );
	shadeKernel->SetArgument( 8, seedBuffer );

	bvh.CopyToDevice();
}

void Renderer::InitPostProcKernels()
{
	post_prepKernel = new Kernel( "src/cl/postproc.cl", "prep" );
	post_vignetKernel = new Kernel( "src/cl/postproc.cl", "vignetting" );
	post_gammaKernel = new Kernel( "src/cl/postproc.cl", "gammaCorr" );
	post_chromaticKernel = new Kernel( "src/cl/postproc.cl", "chromatic" );
	displayKernel = new Kernel( "src/cl/postproc.cl", "display" );
	saveImageKernel = new Kernel( "src/cl/postproc.cl", "saveImage" );
	resetKernel = new Kernel( "src/cl/wavefront.cl", "reset" );

	resetKernel->SetArguments( accumBuffer );

	displayKernel->SetArguments( accumBuffer, screenBuffer );
}

void Tmpl8::Renderer::CamToDevice()
{
	//clSetKernelArg( traceKernel->kernel, 6, sizeof( Camera ), &camera.cam );
}

void Renderer::SaveFrame( const char* file )
{
	saveImageKernel->Run( PIXELS );
	swap1Buffer->CopyFromDevice();
	SaveImageF( file, SCRWIDTH, SCRHEIGHT, (float4*)swap1Buffer->hostBuffer );
}

void Renderer::MouseMove( int x, int y )
{
	camera.MouseMove( x - mousePos.x, y - mousePos.y );
	mousePos.x = x, mousePos.y = y;
}

void Renderer::MouseWheel( float y )
{
	camera.Zoom( -y );
}

void Renderer::KeyInput( std::map<int, int> keyMap )
{
	if (keyMap[GLFW_KEY_W] == GLFW_PRESS || keyMap[GLFW_KEY_W] == GLFW_REPEAT) camera.Move( CamDir::Forward, deltaTime );
	if (keyMap[GLFW_KEY_A] == GLFW_PRESS || keyMap[GLFW_KEY_A] == GLFW_REPEAT) camera.Move( CamDir::Left, deltaTime );
	if (keyMap[GLFW_KEY_S] == GLFW_PRESS || keyMap[GLFW_KEY_S] == GLFW_REPEAT) camera.Move( CamDir::Backwards, deltaTime );
	if (keyMap[GLFW_KEY_D] == GLFW_PRESS || keyMap[GLFW_KEY_D] == GLFW_REPEAT) camera.Move( CamDir::Right, deltaTime );
	if (keyMap[GLFW_KEY_Q] == GLFW_PRESS || keyMap[GLFW_KEY_Q] == GLFW_REPEAT) camera.Move( CamDir::Up, deltaTime );
	if (keyMap[GLFW_KEY_E] == GLFW_PRESS || keyMap[GLFW_KEY_E] == GLFW_REPEAT) camera.Move( CamDir::Down, deltaTime );
}

void Renderer::Gui()
{
	static char str0[128] = "";
	ImGui::InputText( "Filename", str0, IM_ARRAYSIZE( str0 ) ); ImGui::SameLine();
	if (ImGui::Button( "Save Image" ))
	{
		std::string input( str0 );
		SaveFrame( ("screenshots/" + input + ".png").c_str() );
	}

	if (ImGui::CollapsingHeader( "Camera" ))
	{
		ImGui::SliderFloat( "Speed", &camera.speed, .01f, 1, "%.2f" );
		if (ImGui::SliderFloat( "FOV", &camera.cam.fov, 30, 180, "%.2f" )) camera.moved = true;
		if (ImGui::SliderFloat( "Aperture", &camera.cam.aperture, 0, .2f, "%.2f" )) camera.moved = true;
		if (ImGui::SliderFloat( "Focal Length", &camera.cam.focalLength, .1f, 50, "%.2f" )) camera.moved = true;
		if (ImGui::RadioButton( "Projection", &camera.cam.type, PROJECTION )) camera.moved = true;
		if (ImGui::RadioButton( "FishEye", &camera.cam.type, FISHEYE )) camera.moved = true;
	}
	if (ImGui::CollapsingHeader( "Render" ))
	{
		if (ImGui::Checkbox( "Anti-Aliasing", &defineAntiAliasing )) { camera.moved = true; resetKernels = true; }
		if (ImGui::Checkbox( "Show Energy", &showEnergy )) { energy = 0; };
		if (showEnergy)
		{
			ImGui::Text( "Total Energy: %f", energy );
		}
		if (ImGui::RadioButton( "Simple", &(settings->tracerType), SHADING_SIMPLE )) { resetKernels = true; }
		if (ImGui::RadioButton( "NEE", &(settings->tracerType), SHADING_NEE )) { resetKernels = true; }
	}
	if (ImGui::CollapsingHeader( "Post Processing" ))
	{
		ImGui::SliderFloat( "Vignetting", &vignet_strength, 0.0f, 1.0f, "%.2f" );
		ImGui::SliderFloat( "Gamma Correction", &gamma_corr, 0.0f, 2.0f, "%.2f" );
		ImGui::SliderFloat( "Chromatic Abberation", &chromatic, 0.0f, 1.0f, "%.2f" );
	}

	//ImGui::ShowDemoWindow();
}
