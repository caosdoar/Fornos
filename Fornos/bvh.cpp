#include "bvh.h"
#include "mesh.h"

void binaryDivisionBVH(const Mesh *mesh, const size_t maxTriangleCount, const size_t maxTreeDepth, BVH &parent, const size_t currentDepth)
{
	if (parent.triangles.size() <= maxTriangleCount ||
		currentDepth >= maxTreeDepth)
	{
		return;
	}

	Vector3 offset;

	const Vector3 parentSize = parent.aabb.size;
	if (parentSize.x >= parentSize.y)
	{
		if (parentSize.x >= parentSize.z)
		{
			offset = Vector3(parentSize.x * 0.5f, 0.0f, 0.0f);
		}
		else
		{
			offset = Vector3(0.0f, 0.0f, parentSize.z * 0.5f);
		}
	}
	else if (parentSize.y >= parentSize.z)
	{
		offset = Vector3(0.0f, parentSize.y * 0.5f, 0.0f);
	}
	else
	{
		offset = Vector3(0.0f, 0.0f, parentSize.z * 0.5f);
	}

	const Vector3 size = parent.aabb.size - offset;
	const AABB left(parent.aabb.center - offset, size);
	const AABB right(parent.aabb.center + offset, size);

	parent.children.resize(2);
	parent.children[0].aabb = left;
	parent.children[1].aabb = right;

	for (uint32_t tidx : parent.triangles)
	{
		const Mesh::Triangle &tri = mesh->triangles[tidx];
		const Vector3 p0 = mesh->positions[mesh->vertices[tri.vertexIndex0].positionIndex];
		const Vector3 p1 = mesh->positions[mesh->vertices[tri.vertexIndex1].positionIndex];
		const Vector3 p2 = mesh->positions[mesh->vertices[tri.vertexIndex2].positionIndex];
		const Triangle t(p0, p1, p2);
		if (TriangleAABB(t, left)) parent.children[0].triangles.push_back(tidx);
		if (TriangleAABB(t, right)) parent.children[1].triangles.push_back(tidx);
	}

	parent.triangles.clear();
	binaryDivisionBVH(mesh, maxTriangleCount, maxTreeDepth, parent.children[0], currentDepth+1);
	binaryDivisionBVH(mesh, maxTriangleCount, maxTreeDepth, parent.children[1], currentDepth+1);
}

void binaryDivisionBVH_v2(const Mesh *mesh, const size_t maxTriangleCount, const size_t maxTreeDepth, BVH &parent, const size_t currentDepth)
{
	if (parent.triangles.size() <= maxTriangleCount ||
		currentDepth >= maxTreeDepth)
	{
		return;
	}

	parent.children.resize(2);

	const Vector3 parentSize = parent.aabb.size;
	if (parentSize.x >= parentSize.y && parentSize.x >= parentSize.z)
	{
		// Divide X
		const float xTest = parent.aabb.center.x;
		for (uint32_t tidx : parent.triangles)
		{
			const Mesh::Triangle &tri = mesh->triangles[tidx];
			const Vector3 p0 = mesh->positions[mesh->vertices[tri.vertexIndex0].positionIndex];
			const Vector3 p1 = mesh->positions[mesh->vertices[tri.vertexIndex1].positionIndex];
			const Vector3 p2 = mesh->positions[mesh->vertices[tri.vertexIndex2].positionIndex];
			const bool left =  p0.x <= xTest || p1.x <= xTest || p2.x <= xTest;
			const bool right = p0.x >= xTest || p1.x >= xTest || p2.x >= xTest;
			if (left)  parent.children[0].triangles.push_back(tidx);
			if (right) parent.children[1].triangles.push_back(tidx);
		}

		const Vector3 offset(parentSize.x * 0.5f, 0.0f, 0.0f);
		const Vector3 size = parent.aabb.size - offset;
		parent.children[0].aabb = AABB(parent.aabb.center - offset, size);
		parent.children[1].aabb = AABB(parent.aabb.center + offset, size);
	}
	else if (parentSize.y >= parentSize.z)
	{
		// Divide Y
		const float yTest = parent.aabb.center.y;
		for (uint32_t tidx : parent.triangles)
		{
			const Mesh::Triangle &tri = mesh->triangles[tidx];
			const Vector3 p0 = mesh->positions[mesh->vertices[tri.vertexIndex0].positionIndex];
			const Vector3 p1 = mesh->positions[mesh->vertices[tri.vertexIndex1].positionIndex];
			const Vector3 p2 = mesh->positions[mesh->vertices[tri.vertexIndex2].positionIndex];
			const bool left =  p0.y <= yTest || p1.y <= yTest || p2.y <= yTest;
			const bool right = p0.y >= yTest || p1.y >= yTest || p2.y >= yTest;
			if (left)  parent.children[0].triangles.push_back(tidx);
			if (right) parent.children[1].triangles.push_back(tidx);
		}

		const Vector3 offset(0.0f, parentSize.y * 0.5f, 0.0f);
		const Vector3 size = parent.aabb.size - offset;
		parent.children[0].aabb = AABB(parent.aabb.center - offset, size);
		parent.children[1].aabb = AABB(parent.aabb.center + offset, size);
	}
	else
	{
		// Divide Z
		const float zTest = parent.aabb.center.z;
		for (uint32_t tidx : parent.triangles)
		{
			const Mesh::Triangle &tri = mesh->triangles[tidx];
			const Vector3 p0 = mesh->positions[mesh->vertices[tri.vertexIndex0].positionIndex];
			const Vector3 p1 = mesh->positions[mesh->vertices[tri.vertexIndex1].positionIndex];
			const Vector3 p2 = mesh->positions[mesh->vertices[tri.vertexIndex2].positionIndex];
			const bool left =  p0.z <= zTest || p1.z <= zTest || p2.z <= zTest;
			const bool right = p0.z >= zTest || p1.z >= zTest || p2.z >= zTest;
			if (left)  parent.children[0].triangles.push_back(tidx);
			if (right) parent.children[1].triangles.push_back(tidx);
		}

		const Vector3 offset(0.0f, 0.0f, parentSize.z * 0.5f);
		const Vector3 size = parent.aabb.size - offset;
		parent.children[0].aabb = AABB(parent.aabb.center - offset, size);
		parent.children[1].aabb = AABB(parent.aabb.center + offset, size);
	}

	parent.triangles.clear();
	binaryDivisionBVH_v2(mesh, maxTriangleCount, maxTreeDepth, parent.children[0], currentDepth + 1);
	binaryDivisionBVH_v2(mesh, maxTriangleCount, maxTreeDepth, parent.children[1], currentDepth + 1);
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

	binaryDivisionBVH_v2(mesh, maxTriangleCount, maxTreeDepth, *bvh, 0);

	return bvh;
}