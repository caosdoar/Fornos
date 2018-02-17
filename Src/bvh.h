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

#include "math.h"
#include <vector>
#include <cstdint>

class Mesh;

/// Bounding Volume Hierarchy node
class BVH
{
public:
	AABB aabb;
	size_t subtreeTriangleCount; // For optimizations
	std::vector<BVH> children;
	std::vector<uint32_t> triangles;

	BVH() : subtreeTriangleCount(0) {}

	/// Builds a bounding volume hierarchy for a mesh
	/// @param mesh Mesh
	/// @param maxTriangleCount Maximum number of triangles in a leaf node
	/// @param maxTreeDepth Maximum depth of the tree (useful for stack based algorithms)
	static BVH* createBinary(const Mesh *mesh, const size_t maxTriangleCount, const size_t maxTreeDepth);
};