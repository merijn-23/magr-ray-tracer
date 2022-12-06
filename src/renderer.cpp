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
	settings.tracerType = KAJIYA;
	settings.antiAliasing = true;
	InitKernel();
}

void Renderer::Shutdown() { }

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void Renderer::Tick(float _deltaTime)
{
	deltaTime = _deltaTime;
	// animation
	static float animTime = 0;
	scene.SetTime(animTime += deltaTime * 0.002f);
	// pixel loop
	Timer t;

	UpdateBuffers();
	camera.UpdateCamVec();
	CamToDevice();


	if (camera.moved)
	{
		resetKernel->Run(PIXELS);
		camera.moved = false;
		settings.frames = 1;
	}


	clSetKernelArg(traceKernel->kernel, 10, sizeof(Settings), &settings);
	traceKernel->Run(PIXELS);

	// Post processing
	Buffer* src = swap1Buffer;
	Buffer* dst = swap2Buffer;
	post_prepKernel->SetArguments(pixelBuffer, src);
	post_prepKernel->Run(PIXELS);
	if (vignet_strength > 0)
	{
		post_vignetKernel->SetArguments(src, dst, vignet_strength);
		post_vignetKernel->Run(PIXELS);
		std::swap(src, dst);
	}
	if (gamma_corr != 1)
	{
		post_gammaKernel->SetArguments(src, dst, gamma_corr);
		post_gammaKernel->Run(PIXELS);
		std::swap(src, dst);
	}
	if (chromatic > 0)
	{
		post_chromaticKernel->SetArguments(src, dst, chromatic);
		post_chromaticKernel->Run(PIXELS);
		std::swap(src, dst);
	}

	post_displayKernel->SetArgument(0, src);
	post_displayKernel->Run(PIXELS);

	settings.frames++;

	// performance report - running average - ms, MRays/s
	static float avg = 10, alpha = 1;
	avg = (1 - alpha) * avg + alpha * t.elapsed() * 1000;
	if (alpha > 0.05f)
		alpha *= 0.5f;
	float fps = 1000 / avg, rps = (SCRWIDTH * SCRHEIGHT) * fps;
	printf("%5.2fms (%.1f fps) - %.1fMrays/s\n", avg, fps, rps / 1000000);
}

void Renderer::InitKernel()
{
	traceKernel = new Kernel("src/cl/trace.cl", "render");
	resetKernel = new Kernel("src/cl/trace.cl", "reset");

	post_displayKernel = new Kernel("src/cl/postproc.cl", "display");
	post_vignetKernel = new Kernel("src/cl/postproc.cl", "vignetting");
	post_gammaKernel = new Kernel("src/cl/postproc.cl", "gamma_corr");
	post_chromaticKernel = new Kernel("src/cl/postproc.cl", "chromatic");
	post_prepKernel = new Kernel("src/cl/postproc.cl", "prep");

	primBuffer = new Buffer(sizeof(Primitive) * scene.primitives.size());
	matBuffer = new Buffer(sizeof(Material) * scene.materials.size());
	sphereBuffer = new Buffer(sizeof(Sphere) * scene.spheres.size());
	planeBuffer = new Buffer(sizeof(Plane) * scene.planes.size());
	triangleBuffer = new Buffer(sizeof(Triangle) * scene.triangles.size());
	lightBuffer = new Buffer(sizeof(Light) * scene.lights.size());
	texBuffer = new Buffer(sizeof(float4) * scene.textures.size());
	seedBuffer = new Buffer(sizeof(uint) * SCRHEIGHT * SCRWIDTH);

	// screen and temp buffer
	pixelBuffer = new Buffer(4 * 4 * SCRHEIGHT * SCRWIDTH);
	screenBuffer = new Buffer(GetRenderTarget()->ID, 0, Buffer::TARGET);

	swap1Buffer = new Buffer(4 * 4 * SCRHEIGHT * SCRWIDTH);
	swap2Buffer = new Buffer(4 * 4 * SCRHEIGHT * SCRWIDTH);

	screen = 0;

	//sphereBuffer->hostBuffer = (uint*)scene.spheres;
	primBuffer->hostBuffer = (uint*)scene.primitives.data();
	matBuffer->hostBuffer = (uint*)scene.materials.data();
	sphereBuffer->hostBuffer = (uint*)scene.spheres.data();
	planeBuffer->hostBuffer = (uint*)scene.planes.data();
	triangleBuffer->hostBuffer = (uint*)scene.triangles.data();
	lightBuffer->hostBuffer = (uint*)scene.lights.data();
	texBuffer->hostBuffer = (uint*)scene.textures.data();

	seedBuffer->hostBuffer = new uint[SCRHEIGHT * SCRWIDTH];

	// Set trace kernel arguments
	traceKernel->SetArguments(pixelBuffer, texBuffer, sphereBuffer, triangleBuffer, planeBuffer,
		matBuffer, primBuffer, lightBuffer, seedBuffer);

	// Set post processing kernels arguments
	post_displayKernel->SetArgument(1, screenBuffer);

	resetKernel->SetArguments(pixelBuffer);

	settings.numPrimitives = scene.primitives.size();
	settings.numLights = scene.lights.size();

	sphereBuffer->CopyToDevice();
	planeBuffer->CopyToDevice();
	triangleBuffer->CopyToDevice();
	matBuffer->CopyToDevice();
	primBuffer->CopyToDevice();
	lightBuffer->CopyToDevice();
	texBuffer->CopyToDevice();
	seedBuffer->CopyToDevice();
}

void Renderer::UpdateBuffers()
{
	lightBuffer->CopyToDevice();
	sphereBuffer->CopyToDevice();

	for (int i = 0; i < SCRHEIGHT * SCRWIDTH; i++)
	{
		seedBuffer->hostBuffer[i] = RandomUInt();
	}
	seedBuffer->CopyToDevice();
}

void Tmpl8::Renderer::CamToDevice()
{
	clSetKernelArg(traceKernel->kernel, 9, sizeof(Camera), &camera.cam);
}

void Renderer::MouseMove(int x, int y)
{

	camera.MouseMove(x - mousePos.x, y - mousePos.y);
	mousePos.x = x, mousePos.y = y;
}

void Renderer::MouseWheel(float y)
{
	camera.Zoom(-y);
}
void Renderer::KeyRepeat(int key)
{
	switch (key)
	{
	case GLFW_KEY_W:
		camera.Move(CamDir::Forward, deltaTime);
		break;
	case GLFW_KEY_A:
		camera.Move(CamDir::Left, deltaTime);
		break;
	case GLFW_KEY_S:
		camera.Move(CamDir::Backwards, deltaTime);
		break;
	case GLFW_KEY_D:
		camera.Move(CamDir::Right, deltaTime);
		break;
	case GLFW_KEY_Q:
		camera.Move(CamDir::Up, deltaTime);
		break;
	case GLFW_KEY_E:
		camera.Move(CamDir::Down, deltaTime);
		break;
	}
}

void Renderer::Gui()
{
	if (ImGui::CollapsingHeader("Camera")) {
		ImGui::SliderFloat("Speed", &camera.speed, .1f, 100);
		ImGui::SliderFloat("FOV", &camera.cam.fov, 90, 110);
		if (ImGui::RadioButton("Projection", &camera.cam.type, PROJECTION)) camera.moved = true;
		if (ImGui::RadioButton("FishEye", &camera.cam.type, FISHEYE)) camera.moved = true;
	}
	if (ImGui::CollapsingHeader("Render"))
	{
		if (ImGui::Checkbox("Anti-Aliasing", (bool*)(&settings.antiAliasing))) camera.moved = true;
		if (ImGui::RadioButton("Whitted", &settings.tracerType, WHITTED)) camera.moved = true;
		if (ImGui::RadioButton("Kajiya", &settings.tracerType, KAJIYA)) camera.moved = true;
	}
	if (ImGui::CollapsingHeader("Post Processing")) {
		ImGui::SliderFloat("Vignetting", &vignet_strength, 0.0f, 1.0f, "ratio = %.3f");
		ImGui::SliderFloat("Gamma Correction", &gamma_corr, 0.0f, 2.0f, "ratio = %.3f");
		ImGui::SliderFloat("Chromatic Abberation", &chromatic, 0.0f, 1.0f, "ratio = %.3f");
	}

	ImGui::ShowDemoWindow();
}