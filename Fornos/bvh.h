#pragma once

#include "math.h"
#include <vector>
#include <cstdint>

class Mesh;

class BVH
{
public:
	AABB aabb;
	std::vector<BVH> children;
	std::vector<uint32_t> triangles;

	static BVH* createBinary(const Mesh *mesh, const size_t maxTriangleCount, const size_t maxTreeDepth);
};