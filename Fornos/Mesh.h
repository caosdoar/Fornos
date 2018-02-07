#pragma once

#include "math.h"
#include <cstdint>
#include <vector>

struct IntersectResult
{
	int tidx; // Index of the first vertex of the triangle
	float distance;
};

class Mesh
{
public:

	struct Vertex
	{
		uint32_t positionIndex;
		uint32_t texcoordIndex;
		uint32_t normalIndex;
	};

	struct Triangle
	{
		uint32_t vertexIndex0;
		uint32_t vertexIndex1;
		uint32_t vertexIndex2;
	};

	std::vector<Vector3> positions;
	std::vector<Vector2> texcoords;
	std::vector<Vector3> normals;
	std::vector<Vector3> tangents;
	std::vector<Vector3> bitangents;
	std::vector<Vertex> vertices;
	std::vector<Triangle> triangles;

public:
	static Mesh* loadWavefrontObj(const char *path);

	void computeFaceNormals();
	void computeVertexNormals();
	void computeTangentSpace();

	bool intersect(const Vector3 &o, const Vector3 &d, IntersectResult &o_result) const;
	void intersectAll(const Vector3 *origins, const Vector3 *directions, IntersectResult *o_results, size_t count) const;

private:
	bool intersect(const Vector3 &o, const Vector3 &d, const uint32_t tidx, IntersectResult &o_result) const;
};

