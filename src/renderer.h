#pragma once

namespace Tmpl8
{

// Shading types
#define SHADING_SIMPLE 0
#define SHADING_NEE 1
#define SHADING_NEEIS 2
#define SHADING_NEEMIS 3


class Renderer : public TheApp
{
public:
	void Init( );
	void Tick( float deltaTime );
	void Shutdown( );
	// input handling
	void MouseUp( int button ) { /* implement if you want to detect mouse button presses */ }
	void MouseDown( int button ) { /* implement if you want to detect mouse button presses */ }
	void MouseMove( int x, int y );
	void MouseWheel( float y );
	void KeyInput(std::map<int, int>);
	void Gui( );

	void InitWavefrontKernels();
	void InitPostProcKernels();
	void InitBuffers();
	void CamToDevice( );
	void PostProc( );
	void RayTrace( );
	void ComputeEnergy();
	void SaveFrame( const char* file );

	// data members
	float deltaTime;
	int2 mousePos;
	Scene scene;
	CameraManager camera;
	Settings* settings;
	BVH bvh;

	int consecutiveFrames = 1;
	float energy = 0;
	bool showEnergy = true;
	bool resetFrame = false;
	bool resetKernels = false;
	bool defineAntiAliasing = true;
	int defineShadingType = SHADING_SIMPLE;

	Kernel* resetKernel;
	Kernel* post_prepKernel;
	Kernel* post_vignetKernel;
	Kernel* post_gammaKernel;
	Kernel* post_chromaticKernel;
	Kernel* saveImageKernel;

	// Buffers
	Buffer* matBuffer;
	Buffer* primBuffer;

	// Used for post processing
	Buffer* swap1Buffer;
	Buffer* swap2Buffer;

	Buffer* accumBuffer;
	Buffer* screenBuffer;
	Buffer* texBuffer;

	Buffer* camBuffer;
	Buffer* lightBuffer;

	// Wavefront kernels
	Kernel* generateKernel;
	Kernel* extendKernel;
	Kernel* shadeKernel;
	Kernel* displayKernel;

	Buffer* ray1Buffer;
	Buffer* ray2Buffer;
	Buffer* shadowRayBuffer;
	Buffer* settingsBuffer;
	Buffer* seedBuffer;
	Buffer* primIdxBuffer;
	Buffer* bvhTreeBuffer;
};

} // namespace Tmpl8