#include "precomp.h"

// -----------------------------------------------------------
// Initialize the renderer
// -----------------------------------------------------------
void Renderer::Init()
{
    Scene scene = Scene();
    InitKernel();
}

void Renderer::Shutdown() {}

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
    camera.UpdateCamVec( );
    CamToDevice();

    kernel->Run(SCRWIDTH * SCRHEIGHT);
    //pixelBuffer->CopyFromDevice( );

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
    kernel = new Kernel("kernels/trace.cl", "trace");

    sphereBuffer = new Buffer(sizeof(scene.spheres));
    planeBuffer = new Buffer(sizeof(scene.planes));
    triangleBuffer = new Buffer(sizeof(scene.triangles));
    matBuffer = new Buffer(sizeof(scene.materials));
    primBuffer = new Buffer(sizeof(scene.primitives));
    lightBuffer = new Buffer(sizeof(scene.lights));

    // screen
    pixelBuffer = new Buffer(GetRenderTarget()->ID, 0, Buffer::TARGET);

    /*int id = scene.LoadTexture( "cash_money.png", "cash_money" );
    texBuffer = new Buffer( id, 0, Buffer::TEXTURE );*/
    screen = 0;

    //sphereBuffer->hostBuffer = (uint*)scene.spheres;
    sphereBuffer->hostBuffer = (uint*)scene.spheres;
    planeBuffer->hostBuffer = (uint*)scene.planes;
    triangleBuffer->hostBuffer = (uint*)scene.triangles;
    matBuffer->hostBuffer = (uint*)scene.materials;
    primBuffer->hostBuffer = (uint*)scene.primitives;
    lightBuffer->hostBuffer = (uint*)scene.lights;
    //pixelBuffer->hostBuffer = screen->pixels;

    kernel->SetArguments(pixelBuffer, sphereBuffer, planeBuffer, triangleBuffer,
                         matBuffer, primBuffer, lightBuffer);
    CamToDevice();
    kernel->SetArgument(8, (int)(sizeof(scene.primitives) / sizeof(Primitive)));
    kernel->SetArgument(9, (int)(sizeof(scene.lights) / sizeof(Light)));
    //clSetKernelArg( kernel->GetKernel(), 10, sizeof( cl_mem ), texBuffer );

    sphereBuffer->CopyToDevice();
    planeBuffer->CopyToDevice();
    triangleBuffer->CopyToDevice();
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
    lightBuffer->CopyToDevice();
    sphereBuffer->CopyToDevice();
    //clSetKernelArg(kernel->kernel, 6, sizeof(Camera), &camera.cam);
}

void Tmpl8::Renderer::CamToDevice()
{
    clSetKernelArg(kernel->kernel, 7, sizeof(Camera), &camera.cam);
}

void Renderer::MouseMove(int x, int y)
{
    camera.MouseMove(x - mousePos.x, y - mousePos.y);
    mousePos.x = x, mousePos.y = y;
}

void Renderer::MouseWheel(float y)
{
    camera.Fov(-y);
}
void Renderer::KeyRepeat(int key)
{
    switch (key)
    {
        case GLFW_KEY_W:
        {
            camera.Move(CamDir::Forward, deltaTime);
        }
        break;
        case GLFW_KEY_A:
        {
            camera.Move(CamDir::Left, deltaTime);
        }
        break;
        case GLFW_KEY_S:
        {
            camera.Move(CamDir::Backwards, deltaTime);
        }
        break;
        case GLFW_KEY_D:
        {
            camera.Move(CamDir::Right, deltaTime);
        }
        break;
    }
}

void Renderer::KeyDown(int key) {}

void Renderer::Gui()
{
    ImGui::ShowDemoWindow();
}