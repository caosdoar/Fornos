# fornos

GPU Texture Baking Tool

A fast and simple tool to bake your high-poly mesh details to textures.

## Bakers

- Height
- Position
- Normals (Object & Tangent space)
- Ambient Occlusion
- Bent Normals
- Thickness

## Mesh formats

- Wavefront OBJ

## Image formats

- PNG
- TGA
- EXR

## Known issues

The current project is at an usable state but as a very early preview. Crashes can be expected, memory consumption can be high for large textures or big meshes, and there is barely any error reporting to the user.

## Akownledgements

This project is possible thanks to the following libraries:

- [GLFW](http://www.glfw.org)
- [glad](http://glad.dav1d.de)
- [imgui](https://github.com/ocornut/imgui)
- [tinyexr](https://github.com/syoyo/tinyexr)

Most of the intersection algorithms comes from [Game Physics Cookbook](https://github.com/gszauer/GamePhysicsCookbook)