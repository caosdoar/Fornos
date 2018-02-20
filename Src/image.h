/*
Copyright 2018 Oscar Sebio Cajaraville

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#pragma once

struct CompressedMapUV;
struct Vector2;
struct Vector3;

void exportFloatImage(const float *data, const CompressedMapUV *map, const char *path, bool normalize = false, int dilate = 0, Vector2 *o_minmax = nullptr);

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
void exportNormalImage(const Vector3 *data, const CompressedMapUV *map, const char *path, int dilate = 0);