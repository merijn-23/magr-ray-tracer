#pragma once

namespace Tmpl8
{

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
	void KeyUp( int key ) { /* implement if you want to handle keys */ }
	void KeyDown( int key );
	void KeyRepeat( int key );
	void Gui( );

	void InitKernel( );
	void UpdateBuffers( );
	void CamToDevice( );

	// data members
	float deltaTime;
	int consecutiveFrames = 1;
	int2 mousePos;
	float4* accumulator;
	Scene scene;
	CameraManager camera;

	Kernel* traceKernel;
	Kernel* resetKernel;
	Kernel* post_prepKernel;
	Kernel* post_vignetKernel;
	Kernel* post_gammaKernel;
	Kernel* post_displayKernel;

	// Buffers
	Buffer* sphereBuffer;
	Buffer* planeBuffer;
	Buffer* triangleBuffer;
	Buffer* matBuffer;
	Buffer* primBuffer;

	// Used for post processing
	Buffer* swap1Buffer;
	Buffer* swap2Buffer;

	Buffer* pixelBuffer;
	Buffer* screenBuffer;
	Buffer* texBuffer;

	Buffer* camBuffer;
	Buffer* lightBuffer;

	Buffer* seedBuffer;
};

} // namespace Tmpl8