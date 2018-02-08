#pragma once

#include "math.h"
#include <vector>
#include <cstdint>

class Mesh;

class BVH
{
public:
	AABB aabb;
	size_t subtreeTriangleCount; // For optimizations
	std::vector<BVH> children;
	std::vector<uint32_t> triangles;

	BVH() : subtreeTriangleCount(0) {}

	static BVH* createBinary(const Mesh *mesh, const size_t maxTriangleCount, const size_t maxTreeDepth);
};