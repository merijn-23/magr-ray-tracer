#include "precomp.h"

// -----------------------------------------------------------
// Initialize the renderer
// -----------------------------------------------------------
void Renderer::Init( )
{
	Scene scene = Scene( );
	InitKernel( );
}

void Renderer::Shutdown( ) { }

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

	UpdateBuffers( );
	camera.UpdateCamVec( );
	CamToDevice( );

	traceKernel->Run( SCRWIDTH * SCRHEIGHT );
	vignetKernel->Run( SCRWIDTH * SCRHEIGHT );
	displayKernel->Run( SCRWIDTH * SCRHEIGHT );

	// performance report - running average - ms, MRays/s
	static float avg = 10, alpha = 1;
	avg = (1 - alpha) * avg + alpha * t.elapsed( ) * 1000;
	if ( alpha > 0.05f )
		alpha *= 0.5f;
	float fps = 1000 / avg, rps = (SCRWIDTH * SCRHEIGHT) * fps;
	printf( "%5.2fms (%.1f fps) - %.1fMrays/s\n", avg, fps, rps / 1000000 );
}

void Renderer::InitKernel( )
{
	traceKernel = new Kernel( "kernels/trace.cl", "trace" );
	displayKernel = new Kernel( "kernels/postproc.cl", "display" );
	vignetKernel = new Kernel( "kernels/postproc.cl", "vignetting" );

	primBuffer = new Buffer( sizeof( Primitive ) * scene.primitives.size( ) );
	matBuffer = new Buffer( sizeof( Material ) * scene.materials.size( ) );
	sphereBuffer = new Buffer( sizeof( Sphere ) * scene.spheres.size( ) );
	planeBuffer = new Buffer( sizeof( Plane ) * scene.planes.size( ) );
	triangleBuffer = new Buffer( sizeof( Triangle ) * scene.triangles.size( ) );
	lightBuffer = new Buffer( sizeof( Light ) * scene.lights.size( ) );
	texBuffer = new Buffer( sizeof( float4 ) * scene.textures.size( ) );

	// screen and temp buffer
	pixelBuffer = new Buffer( 4 * 4 * SCRHEIGHT * SCRWIDTH );
	screenBuffer = new Buffer( GetRenderTarget( )->ID, 0, Buffer::TARGET );

	screen = 0;

	//sphereBuffer->hostBuffer = (uint*)scene.spheres;
	primBuffer->hostBuffer = (uint*)scene.primitives.data( );
	matBuffer->hostBuffer = (uint*)scene.materials.data( );
	sphereBuffer->hostBuffer = (uint*)scene.spheres.data( );
	planeBuffer->hostBuffer = (uint*)scene.planes.data( );
	triangleBuffer->hostBuffer = (uint*)scene.triangles.data( );
	lightBuffer->hostBuffer = (uint*)scene.lights.data( );
	texBuffer->hostBuffer = (uint*)scene.textures.data( );


	// Set trace kernel arguments
	traceKernel->SetArguments( pixelBuffer, texBuffer, sphereBuffer, triangleBuffer, planeBuffer,
		matBuffer, primBuffer, lightBuffer );
	CamToDevice( );
	traceKernel->SetArgument( 9, (int)scene.primitives.size( ) );
	traceKernel->SetArgument( 10, (int)scene.lights.size( ) );

	// Set post processing kernels arguments
	displayKernel->SetArguments( pixelBuffer, screenBuffer );
	vignetKernel->SetArguments( pixelBuffer, .9f );

	sphereBuffer->CopyToDevice( );
	planeBuffer->CopyToDevice( );
	triangleBuffer->CopyToDevice( );
	matBuffer->CopyToDevice( );
	primBuffer->CopyToDevice( );
	lightBuffer->CopyToDevice( );
	texBuffer->CopyToDevice( );
}

void Renderer::UpdateBuffers( )
{
	lightBuffer->CopyToDevice( );
	sphereBuffer->CopyToDevice( );
}

void Tmpl8::Renderer::CamToDevice( )
{
	clSetKernelArg( traceKernel->kernel, 8, sizeof( Camera ), &camera.cam );
}

void Renderer::MouseMove( int x, int y )
{
	camera.MouseMove( x - mousePos.x, y - mousePos.y );
	mousePos.x = x, mousePos.y = y;
}

void Renderer::MouseWheel( float y )
{
	camera.Fov( -y );
}
void Renderer::KeyRepeat( int key )
{
	switch ( key )
	{
	case GLFW_KEY_W:
	{
		camera.Move( CamDir::Forward, deltaTime );
	}
	break;
	case GLFW_KEY_A:
	{
		camera.Move( CamDir::Left, deltaTime );
	}
	break;
	case GLFW_KEY_S:
	{
		camera.Move( CamDir::Backwards, deltaTime );
	}
	break;
	case GLFW_KEY_D:
	{
		camera.Move( CamDir::Right, deltaTime );
	}
	break;
	}
}

void Renderer::KeyDown( int key ) { }

void Renderer::Gui( )
{
	ImGui::ShowDemoWindow( );
}