# fornos

GPU Texture Baking Tool

A fast and simple tool to bake your high-poly mesh details to textures.

## Bakers

- [Height](#height-baker)
- [Position](#position-baker)
- [Normals](#normals-baker)
- [Ambient Occlusion](#ambient-occlusion-baker)
- [Bent Normals](#bent-normals-baker)
- [Thickness](#thickness-baker)

## Mesh formats

- Wavefront OBJ

## Image formats

- PNG
- TGA
- EXR

## Usage

### Common steps

#### 1. Select a low poly mesh file to bake to

This is the "destination" mesh. It must have UV coordinates and normal data.

The UV coordinates will be used to generate the baked meshes.

The normal data is used to map points from the low-poly mesh to the high-poly mesh. It is better to use "smooth" or per-vertex normals as per-face normals can produce mappings with gaps depending on the topology.

Normals can be computed by fornos if "compute per face" or "compute per vertex" is selected.

#### 2. Select a high poly mesh file to bake from

This is te "target" mesh. Your high resolution mesh where the details will be extracted from.

UV coordinates are not required but normals are necessary by some of the bakers (normals, ambient occlusion, bent normals and thickness)

#### 3. Select a target texture size

This is the size of all textures baked

#### 4. Enable any bakers

Check the box on the right of any of the bakers to enable them for the baking process.

It is also necessary to select a destination file for the result of the baker. Expand the baker options by clicking on the bar and select your file. Remember to enable the baker before.

#### 5. Configure the bakers options

Each baker is explained in detail below

#### 6. Bake!

Click the bake button.

This will go through different steps, from generating a map of your low-poly mesh to process each baker. After that you will have your shinning new textures.

### Height baker

Creates a height map with the differences between your low-poly and hi-poly meshes.

Supports 8-bit (PNG and TGA) and EXR file formats as outputs.

### Position baker

Creates an image with the mapped positions in the high-poly mesh.

This is useful for some processing tools like Unity3d's Delighting Tool.

Only EXR file format is supported as output.

### Normals baker

Creates a normal map.

If 8-bit format for the output is used the values will be stored as (value * 0.5 + 0.5). This is the format most engines will expect. If an EXR file is used the values will not be transformed. 

**Tangent space**: If checked the normals will be transformed to tangent space. The output will be ino bject space otherwise.

### Ambient Occlusion baker

Computes the ratio of occluders in a number of samples for a cosine weighted hemisphere. A.k.a. your usual ambient occlusion map.

**Sample count**: Number of samples used for each pixel. The greater the number, the better and the slower.

**Min distance**: Minimum distance to consider an occluder. In mesh units.

**Max distance**: Maximum distance to consider an occluder. In mesh units.

### Bent Normals baker

Bent normals are the average direction of the ambient light.

This is computed as an average of the direction of the non occluded rays, very similar as how ambient occlusion is computed.

For correct visualization engines usually requires matching parameters between ambient occlusion and bent normals. 

More information about bent normals in the [Unreal Engine documentation](https://docs.unrealengine.com/latest/INT/Engine/Rendering/LightingAndShadows/BentNormalMaps/).

**Sample count**: Number of samples used for each pixel.

**Min distance**: Minimum distance to consider an occluder. In mesh units.

**Max distance**: Maximum distance to consider an occluder. In mesh units.

### Thickness baker

Generates a map of how thick is in average the mesh for a point in the surface.

This map is useful for translucency and sub-surface scattering effects.

**Sample count**: Number of samples used for each pixel.

**Min distance**: Distance for the thickness value to be zero. In mesh units.

**Max distance**: Distance for the thickness value to be one. In mesh units.


## Known issues

The current project is at an usable state but as a very early preview. Crashes can be expected, memory consumption can be high for large textures or big meshes, and there is barely any error reporting to the user.

## Ackownledgements

This project is possible thanks to the following libraries:

- [GLFW](http://www.glfw.org)
- [glad](http://glad.dav1d.de)
- [imgui](https://github.com/ocornut/imgui)
- [tinyexr](https://github.com/syoyo/tinyexr)

Most of the intersection algorithms comes from [Game Physics Cookbook](https://github.com/gszauer/GamePhysicsCookbook)