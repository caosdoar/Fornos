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
	static Mesh* loadPly(const char *path);
	static Mesh* loadFile(const char *path);
	static Mesh* createCopy(const Mesh *mesh);

	void computeFaceNormals();
	void computeVertexNormals();
	void computeVertexNormalsAggressive();
	void computeTangentSpace();

	bool intersect(const Vector3 &o, const Vector3 &d, IntersectResult &o_result) const;
	void intersectAll(const Vector3 *origins, const Vector3 *directions, IntersectResult *o_results, size_t count) const;

private:
	bool intersect(const Vector3 &o, const Vector3 &d, const uint32_t tidx, IntersectResult &o_result) const;
};

