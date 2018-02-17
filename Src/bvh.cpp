#include "bvh.h"
#include "mesh.h"
#include <cassert>

inline size_t bestSplitFromBuckets(const uint32_t buckets[16], const uint32_t tricount)
{
	int l = 0;
	int r = (int)tricount;
	int diff = INT_MAX;
	size_t best = 0;
	for (size_t i = 0; i < 16; ++i)
	{
		const uint32_t c = buckets[i];
		l += c;
		r -= c;
		int d = abs(l - r);
		if (d > diff)
		{
			break;
		}
		diff = d;
		best = i;
	}
	return best;
}

template <typename Axis>
float findSplit(const Mesh *mesh, const BVH &parent)
{
	const float minX = Axis::get(parent.aabb.center) - Axis::get(parent.aabb.size);
	const float maxX = Axis::get(parent.aabb.center) + Axis::get(parent.aabb.size);
	uint32_t buckets[16] = { 0 };
	for (uint32_t tidx : parent.triangles)
	{
		const Mesh::Triangle &tri = mesh->triangles[tidx];
		const Vector3 p0 = mesh->positions[mesh->vertices[tri.vertexIndex0].positionIndex];
		const Vector3 p1 = mesh->positions[mesh->vertices[tri.vertexIndex1].positionIndex];
		const Vector3 p2 = mesh->positions[mesh->vertices[tri.vertexIndex2].positionIndex];
		const Vector3 centroid = (p0 + p1 + p2) / 3.0f;
		const size_t i = size_t((Axis::get(centroid) - minX) / (maxX - minX) * 15.99f);
		assert(i < 16);
		++buckets[i];
	}

	size_t best = bestSplitFromBuckets(buckets, uint32_t(parent.triangles.size()));
	return minX + (maxX - minX) / 16.0f * float(best + 1);
}

struct AxisX { static float get(const Vector3 &v) { return v.x; } };
struct AxisY { static float get(const Vector3 &v) { return v.y; } };
struct AxisZ { static float get(const Vector3 &v) { return v.z; } };

void binaryDivisionBVH(const Mesh *mesh, const size_t maxTriangleCount, const size_t maxTreeDepth, BVH &parent, const size_t currentDepth)
{
	if (parent.triangles.size() <= maxTriangleCount ||
		currentDepth >= maxTreeDepth)
	{
		parent.subtreeTriangleCount = parent.triangles.size();
		return;
	}

	parent.children.resize(2);

	Vector3 aabbMinL(FLT_MAX); Vector3 aabbMaxL(-FLT_MAX);
	Vector3 aabbMinR(FLT_MAX); Vector3 aabbMaxR(-FLT_MAX);

	const Vector3 parentSize = parent.aabb.size;
	if (parentSize.x >= parentSize.y && parentSize.x >= parentSize.z)
	{
		// Divide X
		//const float xTest = findSplitX(mesh, parent);
		const float xTest = findSplit<AxisX>(mesh, parent);
		for (uint32_t tidx : parent.triangles)
		{
			const Mesh::Triangle &tri = mesh->triangles[tidx];
			const Vector3 p0 = mesh->positions[mesh->vertices[tri.vertexIndex0].positionIndex];
			const Vector3 p1 = mesh->positions[mesh->vertices[tri.vertexIndex1].positionIndex];
			const Vector3 p2 = mesh->positions[mesh->vertices[tri.vertexIndex2].positionIndex];
			const Vector3 c = (p0 + p1 + p2) * (1.0f / 3.0f);
			if (c.x <= xTest)
			{
				parent.children[0].triangles.push_back(tidx);
				aabbMinL = min(aabbMinL, min(p0, min(p1, p2)));
				aabbMaxL = max(aabbMaxL, max(p0, max(p1, p2)));
			}
			else
			{
				parent.children[1].triangles.push_back(tidx);
				aabbMinR = min(aabbMinR, min(p0, min(p1, p2)));
				aabbMaxR = max(aabbMaxR, max(p0, max(p1, p2)));
			}
		}
	}
	else if (parentSize.y >= parentSize.z)
	{
		// Divide Y
		const float yTest = findSplit<AxisY>(mesh, parent);
		for (uint32_t tidx : parent.triangles)
		{
			const Mesh::Triangle &tri = mesh->triangles[tidx];
			const Vector3 p0 = mesh->positions[mesh->vertices[tri.vertexIndex0].positionIndex];
			const Vector3 p1 = mesh->positions[mesh->vertices[tri.vertexIndex1].positionIndex];
			const Vector3 p2 = mesh->positions[mesh->vertices[tri.vertexIndex2].positionIndex];
			const Vector3 c = (p0 + p1 + p2) * (1.0f / 3.0f);
			if (c.y <= yTest)
			{
				parent.children[0].triangles.push_back(tidx);
				aabbMinL = min(aabbMinL, min(p0, min(p1, p2)));
				aabbMaxL = max(aabbMaxL, max(p0, max(p1, p2)));
			}
			else
			{
				parent.children[1].triangles.push_back(tidx);
				aabbMinR = min(aabbMinR, min(p0, min(p1, p2)));
				aabbMaxR = max(aabbMaxR, max(p0, max(p1, p2)));
			}
		}

	}
	else
	{
		// Divide Z
		const float zTest = findSplit<AxisZ>(mesh, parent);
		for (uint32_t tidx : parent.triangles)
		{
			const Mesh::Triangle &tri = mesh->triangles[tidx];
			const Vector3 p0 = mesh->positions[mesh->vertices[tri.vertexIndex0].positionIndex];
			const Vector3 p1 = mesh->positions[mesh->vertices[tri.vertexIndex1].positionIndex];
			const Vector3 p2 = mesh->positions[mesh->vertices[tri.vertexIndex2].positionIndex];
			const Vector3 c = (p0 + p1 + p2) * (1.0f / 3.0f);
			if (c.z <= zTest)
			{
				parent.children[0].triangles.push_back(tidx);
				aabbMinL = min(aabbMinL, min(p0, min(p1, p2)));
				aabbMaxL = max(aabbMaxL, max(p0, max(p1, p2)));
			}
			else
			{
				parent.children[1].triangles.push_back(tidx);
				aabbMinR = min(aabbMinR, min(p0, min(p1, p2)));
				aabbMaxR = max(aabbMaxR, max(p0, max(p1, p2)));
			}
		}
	}

	if (parent.children[0].triangles.empty() ||
		parent.children[1].triangles.empty())
	{
		// Unable to subdivide... We brake here
		// TODO: Check if there is a way to improve this
		parent.children.clear();
		return;
	}

	parent.triangles.clear();

	parent.children[0].aabb = AABB((aabbMinL + aabbMaxL) * 0.5f, (aabbMaxL - aabbMinL) * 0.5f);
	parent.children[1].aabb = AABB((aabbMinR + aabbMaxR) * 0.5f, (aabbMaxR - aabbMinR) * 0.5f);

	binaryDivisionBVH(mesh, maxTriangleCount, maxTreeDepth, parent.children[0], currentDepth + 1);
	binaryDivisionBVH(mesh, maxTriangleCount, maxTreeDepth, parent.children[1], currentDepth + 1);

	// Update AABB
	{
		aabbMinL = parent.children[0].aabb.center - parent.children[0].aabb.size;
		aabbMaxL = parent.children[0].aabb.center + parent.children[0].aabb.size;
		aabbMinR = parent.children[1].aabb.center - parent.children[1].aabb.size;
		aabbMaxR = parent.children[1].aabb.center + parent.children[1].aabb.size;
		const Vector3 aabbMin = min(aabbMinL, aabbMinR);
		const Vector3 aabbMax = max(aabbMaxL, aabbMaxR);
		parent.aabb.center = (aabbMax + aabbMin) * 0.5f;
		parent.aabb.size = (aabbMax - aabbMin) * 0.5f;
	}

	parent.subtreeTriangleCount =
		parent.children[0].subtreeTriangleCount +
		parent.children[1].subtreeTriangleCount;
}

BVH* BVH::createBinary(const Mesh *mesh, const size_t maxTriangleCount, const size_t maxTreeDepth)
{
	Vector3 mins(FLT_MAX);
	Vector3 maxs(-FLT_MAX);
	for (size_t i = 0; i < mesh->positions.size(); ++i)
	{
		const Vector3 p = mesh->positions[i];
		mins = min(mins, p);
		maxs = max(maxs, p);
	}

	BVH *bvh = new BVH();
	bvh->aabb.center = (maxs + mins) * 0.5f;
	bvh->aabb.size = (maxs - mins) * 0.5f;

	const size_t count = mesh->triangles.size();
	bvh->triangles.resize(count);
	for (size_t i = 0; i < count; ++i)
	{
		bvh->triangles[i] = (uint32_t)i;
	}

	binaryDivisionBVH(mesh, maxTriangleCount, maxTreeDepth, *bvh, 0);

	return bvh;
}