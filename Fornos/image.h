#pragma once

struct CompressedMapUV;
struct Vector2;
struct Vector3;

void exportFloatImage(const float *data, const CompressedMapUV *map, const char *path, bool normalize = false, Vector2 *o_minmax = nullptr);

/// Export raw 3-channel-float data
/// Only EXR files supported here!
/// @param data Vector3 data
/// @param map How the data should be stored on the map
/// @param path Path to the file
void exportVectorImage(const Vector3 *data, const CompressedMapUV *map, const char *path);

/// Exports normals in a format sensitive way
/// For 8-bit-per-channel files it transforms components to the range 0 to 1
/// output = normal * 0.5 + 0.5
/// @param data Normals data
/// @param map How the data should be stored on the map
/// @param path Path to the file
void exportNormalImage(const Vector3 *data, const CompressedMapUV *map, const char *path);