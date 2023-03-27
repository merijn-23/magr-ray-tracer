![futuristic](https://user-images.githubusercontent.com/22050862/227993934-1adf935c-0153-4995-8eac-296009f73cc1.png)

## About
GPU-based ray tracer for the course Advanced Graphics (INFOMAGR) at Utrecht University. </br>
By </br>
Merijn Schepers </br>
Tariq Bakhtali </br>

Original template by bikker.j@gmail.com.

## Features
### Rendering
- A Kajiya-style path tracing renderer, supporting diffuse interreflections and area lights.
- A Whitted-style ray tracing renderer, supporting shadows, reflections and refraction with absorption (Beer’s law).
- Wavefront Path Tracing on the GPU with persistent threads.
- Texturing on all supported primitives.
- A skydome, with a texture loaded from a HDR or JPG/PNG file format.

### Controls
- A ‘free camera’ with configurable position, orientation, FOV and aspect ratio.
- A basic user interface, or keys / mouse input handling to control the camera position and orientation at run-time.

### Camera
- Anti-aliasing, gamma correction, vignetting and chromatic aberration.
- Fish eye lens.
- Depth of Field.

### Space-Partitioning
- The BVH works for our Kajiya path tracer on the GPU, but is built on the CPU.
- The BVH handles spheres, triangles and planes, as well as diffuse and specular materials.
- We have separate functions for normal rays and occlusion rays. The occlusion rays only push nodes onto the stack if they are closer than the distance to the light, and it early-outs once it hits any object in between the origin and the light.
- BVH can be upgraded to QBVH, where each node has not two but four children
- BVH can be upgraded to SBVH using spatial splitting. This can be controlled with a variable $\alpha$, with the SBVH being a normal BVH at $\alpha = 1$, and a full SBVH when $\alpha = 0$.

## Sources
- [BVH](https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/)
- [QBVH](https://github.com/jan-van-bergen/GPU-Raytracer/blob/master/Src/BVH/Converters/BVH4Converter.cpp)
- [SBVH](https://www.nvidia.com/docs/IO/77714/sbvh.pdf)
- [Wavefront](https://research.nvidia.com/publication/2013-07_megakernels-considered-harmful-wavefront-path-tracing-gpus)

![dragon_traversal](https://user-images.githubusercontent.com/22050862/227994352-ea4effa7-12ae-4f9d-8b01-d5ca54613be2.png)
![imgui](https://user-images.githubusercontent.com/22050862/227994372-d0818453-3078-46b4-8f57-4b7950821bd9.png)


