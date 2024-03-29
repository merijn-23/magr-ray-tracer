#pragma once

namespace Tmpl8
{

// Shading types
#define SHADING_SIMPLE "SHADING_SIMPLE"
#define SHADING_NEE "SHADING_NEE"
#define SHADING_NEEIS "SHADING_NEEIS"
#define SHADING_NEEMIS "SHADING_NEEMIS"

#define SAMPLING_HEMISPHERE "SAMPLING_HEMISPHERE"
#define SAMPLING_COSINE "SAMPLING_COSINE"

#define USE_BVH2 "USE_BVH2"
#define USE_BVH4 "USE_BVH4"

#define USE_RUSSIAN_ROULETTE "RUSSIAN_ROULETTE"
#define FILTER_FIREFLIES "FILTER_FIREFLIES"

typedef struct ImGuiData
{
	string shading_type = SHADING_NEE;
	string bvh_type = USE_BVH2;
	string sampling_type = SAMPLING_COSINE;
	bool use_russian_roulette = true;

	float vignet_strength = 0;
	float chromatic_strength = 0;
	float gamma_strength = .9f;
	float energy_total = 0;
	bool print_performance = false;
	bool show_energy_levels = false;
	bool reset_every_frame = false;
	bool focus_mode = true;
	bool filter_fireflies = true;

	int dummy_bvh_type = 1;
	int dummy_shading_type = 1;
	int dummy_sampling_type = 0;
	bool dummy_russian_roulette = true;
};

class Renderer : public TheApp
{
public:
	void Init( );
	void Tick( float deltaTime );
	void Shutdown( );
	// input handling
	void MouseUp( int button ) { /* implement if you want to detect mouse button presses */ }
	void MouseDown( int button );
	void MouseMove( int x, int y, bool mouse_active );
	void MouseWheel( float y );
	void KeyInput(std::map<int, int>);
	void Gui( );

	void InitWavefrontKernels();
	void InitPostProcKernels();
	void InitBuffers();
	void PostProc( );
	void RayTrace( );
	void ComputeEnergy();
	void FocusCamera( int x, int y );
	void SaveFrame( const char* file );

	// data members
	float deltaTime;
	int consecutiveFrames = 1;
	int2 mousePos;
	Scene scene;
	CameraManager camera;
	Settings* settings;
	TLAS* tlas;
	ImGuiData imgui;

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
	Kernel* connectKernel;
	Kernel* displayKernel;
	Kernel* focusKernel;

	Buffer* ray1Buffer;
	Buffer* ray2Buffer;
	Buffer* shadowRayBuffer;
	Buffer* settingsBuffer;
	Buffer* seedBuffer;
	Buffer* primIdxBuffer;
	Buffer* bvhTreeBuffer;

	// BVH buffers
	Buffer* bvhNodeBuffer;
	Buffer* bvhIdxBuffer;
	Buffer* tlasNodeBuffer;
	Buffer* blasNodeBuffer;
};

} // namespace Tmpl8