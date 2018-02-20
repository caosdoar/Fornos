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

#include "bvh.h"
#include "logging.h"
#include "mesh.h"
#include "timing.h"
#include <cassert>

enum class Axis { X, Y, Z };

struct SplitResult
{
	Axis axis;
	float split;
};

struct BucketSplit
{
	size_t splitIdx;
	float cost;
};

struct BucketAABB
{
	Vector3 minv = Vector3(FLT_MAX);
	Vector3 maxv = Vector3(-FLT_MAX);

	void addPoint(const Vector3 &p)
	{
		minv = min(p, minv);
		maxv = max(p, maxv);
	}
};

BucketAABB combine(const BucketAABB &aabb0, const BucketAABB &aabb1)
{
	BucketAABB r = aabb0;
	r.minv = min(aabb1.minv, r.minv);
	r.maxv = max(aabb1.maxv, r.maxv);
	return r;
}

inline float computeCost(const BucketAABB &aabb, const int triangleCount)
{
	const Vector3 size = aabb.maxv - aabb.minv;
	const float surfaceArea = 2.0f * (size.x * size.y + size.x * size.z + size.x * size.y);
	return float(triangleCount) * surfaceArea;
}

inline BucketSplit selectSplitFromBuckets(const uint32_t buckets[16], const BucketAABB bucketsAABB[16], size_t triangleCount)
{
#if 0
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
	const float quality = 1.0f - float(diff) / float(tricount);
	return BucketSplit{ best, quality };
#else
	// Pass to compute AABB on the right side of the split
	BucketAABB aabbRightSide[15];
	aabbRightSide[14] = bucketsAABB[15];
	for (int i = 13; i >= 0; --i)
	{
		aabbRightSide[i] = combine(aabbRightSide[i + 1], bucketsAABB[i + 1]);
	}

	float bestCost = FLT_MAX;
	int bestSplitIdx = 0;

	BucketAABB aabbLeft;
	int countLeft = 0;
	int countRight = int(triangleCount);
	for (int i = 0; i < 16; ++i)
	{
		const auto c = buckets[i];
		countLeft += c;
		countRight -= c;
		aabbLeft = combine(aabbLeft, bucketsAABB[i]);
		BucketAABB aabbRight = aabbRightSide[i];
		const float costLeft = computeCost(aabbLeft, countLeft);
		const float costRight = computeCost(aabbRight, countRight);
		const float cost = costLeft + costRight;
		if (cost < bestCost)
		{
			bestCost = cost;
			bestSplitIdx = i;
		}
	}

	return BucketSplit{ size_t(bestSplitIdx), bestCost };
#endif
}

SplitResult findBestSplit(const Mesh *mesh, const BVH &parent)
{
	SplitResult ret;

	BucketAABB centroiddsAABB;
	for (uint32_t tidx : parent.triangles)
	{
		const Mesh::Triangle &tri = mesh->triangles[tidx];
		const Vector3 p0 = mesh->positions[mesh->vertices[tri.vertexIndex0].positionIndex];
		const Vector3 p1 = mesh->positions[mesh->vertices[tri.vertexIndex1].positionIndex];
		const Vector3 p2 = mesh->positions[mesh->vertices[tri.vertexIndex2].positionIndex];
		const Vector3 centroid = (p0 + p1 + p2) / 3.0f;
		centroiddsAABB.addPoint(centroid);
	}

	uint32_t bucketsX[16] = { 0 };
	uint32_t bucketsY[16] = { 0 };
	uint32_t bucketsZ[16] = { 0 };
	BucketAABB bucketsAABBX[16];
	BucketAABB bucketsAABBY[16];
	BucketAABB bucketsAABBZ[16];

	for (uint32_t tidx : parent.triangles)
	{
		const Mesh::Triangle &tri = mesh->triangles[tidx];
		const Vector3 p0 = mesh->positions[mesh->vertices[tri.vertexIndex0].positionIndex];
		const Vector3 p1 = mesh->positions[mesh->vertices[tri.vertexIndex1].positionIndex];
		const Vector3 p2 = mesh->positions[mesh->vertices[tri.vertexIndex2].positionIndex];
		const Vector3 centroid = (p0 + p1 + p2) / 3.0f;

		const Vector3 ijk = (centroid - centroiddsAABB.minv) / (centroiddsAABB.maxv - centroiddsAABB.minv) * 15.99f;
		const size_t i = isnan(ijk.x) ? 0 : size_t(ijk.x);
		const size_t j = isnan(ijk.y) ? 0 : size_t(ijk.y);
		const size_t k = isnan(ijk.z) ? 0 : size_t(ijk.z);
		assert(i < 16 && j < 16 && k < 16);

		++bucketsX[i];
		++bucketsY[j];
		++bucketsZ[k];

		bucketsAABBX[i].addPoint(p0);
		bucketsAABBX[i].addPoint(p1);
		bucketsAABBX[i].addPoint(p2);
		bucketsAABBY[j].addPoint(p0);
		bucketsAABBY[j].addPoint(p1);
		bucketsAABBY[j].addPoint(p2);
		bucketsAABBZ[k].addPoint(p0);
		bucketsAABBZ[k].addPoint(p1);
		bucketsAABBZ[k].addPoint(p2);
	}

	const uint32_t tricount = uint32_t(parent.triangles.size());
	const BucketSplit splitX = selectSplitFromBuckets(bucketsX, bucketsAABBX, tricount);
	const BucketSplit splitY = selectSplitFromBuckets(bucketsY, bucketsAABBY, tricount);
	const BucketSplit splitZ = selectSplitFromBuckets(bucketsZ, bucketsAABBZ, tricount);

	if (splitX.cost <= splitY.cost && splitX.cost <= splitZ.cost)
	//if (parent.aabb.size.x >= parent.aabb.size.y && parent.aabb.size.x >= parent.aabb.size.z)
	{
		ret.axis = Axis::X;
		ret.split = centroiddsAABB.minv.x + (centroiddsAABB.maxv.x - centroiddsAABB.minv.x) / 16.0f * float(splitX.splitIdx + 1);
	}
	else if (splitY.cost <= splitX.cost && splitY.cost <= splitZ.cost)
	//else if (parent.aabb.size.y >= parent.aabb.size.x && parent.aabb.size.y >= parent.aabb.size.z)
	{
		ret.axis = Axis::Y;
		ret.split = centroiddsAABB.minv.y + (centroiddsAABB.maxv.y - centroiddsAABB.minv.y) / 16.0f * float(splitY.splitIdx + 1);
	}
	else
	{
		ret.axis = Axis::Z;
		ret.split = centroiddsAABB.minv.z + (centroiddsAABB.maxv.z - centroiddsAABB.minv.z) / 16.0f * float(splitZ.splitIdx + 1);
	}

	return ret;
}

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

	const SplitResult split = findBestSplit(mesh, parent);

	for (uint32_t tidx : parent.triangles)
	{
		const Mesh::Triangle &tri = mesh->triangles[tidx];
		const Vector3 p0 = mesh->positions[mesh->vertices[tri.vertexIndex0].positionIndex];
		const Vector3 p1 = mesh->positions[mesh->vertices[tri.vertexIndex1].positionIndex];
		const Vector3 p2 = mesh->positions[mesh->vertices[tri.vertexIndex2].positionIndex];
		const Vector3 c = (p0 + p1 + p2) * (1.0f / 3.0f);

		const bool left =
			((split.axis == Axis::X) & (c.x <= split.split)) |
			((split.axis == Axis::Y) & (c.y <= split.split)) |
			((split.axis == Axis::Z) & (c.z <= split.split));

		if (left)
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

	if (parent.children[0].triangles.empty() ||
		parent.children[1].triangles.empty())
	{
		// Unable to subdivide... We brake here
		// TODO: Check if there is a way to improve this
		parent.children.clear();
		parent.subtreeTriangleCount = parent.triangles.size();
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
	Timing timing;
	timing.begin();

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

	timing.end();
	logDebug("BVH", "BHV Creation took " + std::to_string(timing.elapsedSeconds()) + " seconds.");

	return bvh;
}