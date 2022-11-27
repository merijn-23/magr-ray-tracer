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
	int2 mousePos;
	float4* accumulator;
	Scene scene;
	CameraManager camera;

	Kernel* kernel;

	// Buffers
	Buffer* sphereBuffer;
	Buffer* planeBuffer;
	Buffer* triangleBuffer;
	Buffer* matBuffer;
	Buffer* primBuffer;

	Buffer* pixelBuffer;
	Buffer* texBuffer;

	Buffer* camBuffer;
	Buffer* lightBuffer;
};

} // namespace Tmpl8