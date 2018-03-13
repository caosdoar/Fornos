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

#define DEBUG_EXPORT_DIRECTIONS_MAP 1

#include "compute.h"
#include "logging.h"
#include "math.h"
#include "mesh.h"
#include <fstream>

#if DEBUG_EXPORT_DIRECTIONS_MAP
#include "image.h"
#endif

GLuint CreateComputeProgram(const char *path)
{
	std::ifstream ifs(path);
	std::string src((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
	const char *src_str = src.c_str();
	return CreateComputeProgramFromMemory(src_str);
}

inline GLuint CreateComputeProgramFromMemory(const char *src)
{
	GLuint shader = glCreateShader(GL_COMPUTE_SHADER);

	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

#if 1
	{
		GLint compiled;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (compiled != GL_TRUE)
		{
			const size_t buffSize = 2048;
			char *buff = new char[buffSize];
			GLsizei length;
			glGetShaderInfoLog(shader, (GLsizei)buffSize, &length, buff);
			logError("Compute", buff);
			//assert(false);
		}
	}
#endif

	GLuint program = glCreateProgram();
	glAttachShader(program, shader);
	glLinkProgram(program);

	return program;
}

namespace
{
	bool rasterTriangle
	(
		const Mesh *mesh,
		const Mesh *meshForMapping,
		const Mesh::Triangle &tri, 
		const Vector2 &pixsize, 
		const Vector2 &halfpix, 
		const Vector2 &scale, 
		MapUV *map
	)
	{
		const auto &v0 = mesh->vertices[tri.vertexIndex0];
		const auto &v1 = mesh->vertices[tri.vertexIndex1];
		const auto &v2 = mesh->vertices[tri.vertexIndex2];

		if (v0.texcoordIndex == UINT32_MAX ||
			v1.texcoordIndex == UINT32_MAX ||
			v2.texcoordIndex == UINT32_MAX ||
			v0.normalIndex == UINT32_MAX ||
			v1.normalIndex == UINT32_MAX ||
			v2.normalIndex == UINT32_MAX)
		{
			return false;
		}

		const Vector3 p0 = mesh->positions[v0.positionIndex];
		const Vector3 p1 = mesh->positions[v1.positionIndex];
		const Vector3 p2 = mesh->positions[v2.positionIndex];

		const Vector2 u0 = mesh->texcoords[v0.texcoordIndex];
		const Vector2 u1 = mesh->texcoords[v1.texcoordIndex];
		const Vector2 u2 = mesh->texcoords[v2.texcoordIndex];

		const Vector3 n0 = mesh->normals[v0.normalIndex];
		const Vector3 n1 = mesh->normals[v1.normalIndex];
		const Vector3 n2 = mesh->normals[v2.normalIndex];

		Vector3 d0 = n0;
		Vector3 d1 = n1;
		Vector3 d2 = n2;
		if (meshForMapping)
		{
			const auto &mv0 = meshForMapping->vertices[tri.vertexIndex0];
			const auto &mv1 = meshForMapping->vertices[tri.vertexIndex1];
			const auto &mv2 = meshForMapping->vertices[tri.vertexIndex2];
			d0 = meshForMapping->normals[mv0.normalIndex];
			d1 = meshForMapping->normals[mv1.normalIndex];
			d2 = meshForMapping->normals[mv2.normalIndex];
		}

		const Vector3 t0 = mesh->tangents.empty() ? Vector3(0) : mesh->tangents[tri.vertexIndex0];
		const Vector3 t1 = mesh->tangents.empty() ? Vector3(0) : mesh->tangents[tri.vertexIndex1];
		const Vector3 t2 = mesh->tangents.empty() ? Vector3(0) : mesh->tangents[tri.vertexIndex2];
		const Vector3 b0 = mesh->tangents.empty() ? Vector3(0) : mesh->bitangents[tri.vertexIndex0];
		const Vector3 b1 = mesh->tangents.empty() ? Vector3(0) : mesh->bitangents[tri.vertexIndex1];
		const Vector3 b2 = mesh->tangents.empty() ? Vector3(0) : mesh->bitangents[tri.vertexIndex2];

		// Note: Surely not the fastest algorithm
		const Vector2 u01 = (u0 - halfpix) * scale;
		const Vector2 u11 = (u1 - halfpix) * scale;
		const Vector2 u21 = (u2 - halfpix) * scale;
		const uint32_t xMin = std::min(uint32_t(std::roundf(std::fminf(u01.x, std::fminf(u11.x, u21.x)))), map->width);
		const uint32_t yMin = std::min(uint32_t(std::roundf(std::fminf(u01.y, std::fminf(u11.y, u21.y)))), map->height);
		const uint32_t xMax = std::min(uint32_t(std::roundf(std::fmaxf(u01.x, std::fmaxf(u11.x, u21.x)))), map->width);
		const uint32_t yMax = std::min(uint32_t(std::roundf(std::fmaxf(u01.y, std::fmaxf(u11.y, u21.y)))), map->height);

		for (uint32_t y = yMin; y <= yMax; ++y)
		{
			for (uint32_t x = xMin; x <= xMax; ++x)
			{
				const Vector2 xy((float)x, (float)y);
				const Vector2 uv = xy * pixsize + halfpix;
				const Vector3 b = Barycentric(uv, u0, u1, u2);
				if (b.x >= -0.001f && b.x <= 1 && 
					b.y >= -0.001f && b.y <= 1 && 
					b.z >= -0.001f && b.z <= 1)
				{
					int i = y * map->width + x;
					map->positions[i] = p0 * b.x + p1 * b.y + p2 * b.z;
					map->directions[i] = normalize(d0 * b.x + d1 * b.y + d2 * b.z);
					map->normals[i] = normalize(n0 * b.x + n1 * b.y + n2 * b.z);
					map->tangents[i] = normalize(t0 * b.x + t1 * b.y + t2 * b.z);
					map->bitangents[i] = normalize(b0 * b.x + b1 * b.y + b2 * b.z);
				}
			}
		}

		return true;
	}

	MapUV* createMapUV(const Mesh *mesh, const Mesh *meshDirs, uint32_t width, uint32_t height)
	{
		assert(mesh);

		MapUV *map = new MapUV(width, height);

		const Vector2 scale((float)width, (float)height);
		const Vector2 pixsize = Vector2(1.0f) / scale;
		const Vector2 halfpix = pixsize * 0.5f;

		//if (computeTangentSpace)
		{
			const size_t size = map->normals.size();
			map->tangents.resize(size);
			map->bitangents.resize(size);
		}

		//for (int vindex = 0; vindex < mesh->positions.size(); vindex += 3)
		for (const auto &tri : mesh->triangles)
		{
			if (!rasterTriangle(mesh, meshDirs, tri, pixsize, halfpix, scale, map))
			{
				delete map;
				return nullptr;
			}
		}

		return map;
	}

	float edgeDistance(const Vector3 &e0, const Vector3 &e1, const Vector3 &p)
	{
		const Vector3 v = p - e0;
		const Vector3 e = e1 - e0;
		const float t = dot(v, e) / dot(e, e);
		const Vector3 p1 = e0 + e * t;
		return length(p1 - p);
	}

	float triangleDistance(const Vector3 &p0, const Vector3 &p1, const Vector3 &p2, const Vector3 &p)
	{
		const float d0 = edgeDistance(p0, p1, p);
		const float d1 = edgeDistance(p0, p2, p);
		const float d2 = edgeDistance(p1, p2, p);
		return std::fminf(d0, std::fminf(d1, d2));
	}

#pragma optimize( "", off )
	bool rasterTriangleEdge
	(
		const Mesh *mesh,
		const Mesh *meshForMapping,
		const Mesh::Triangle &tri,
		const Vector2 &pixsize,
		const Vector2 &halfpix,
		const Vector2 &scale,
		const float edge,
		MapUV *map
	)
	{
		const auto &v0 = mesh->vertices[tri.vertexIndex0];
		const auto &v1 = mesh->vertices[tri.vertexIndex1];
		const auto &v2 = mesh->vertices[tri.vertexIndex2];

		if (v0.texcoordIndex == UINT32_MAX ||
			v1.texcoordIndex == UINT32_MAX ||
			v2.texcoordIndex == UINT32_MAX ||
			v0.normalIndex == UINT32_MAX ||
			v1.normalIndex == UINT32_MAX ||
			v2.normalIndex == UINT32_MAX)
		{
			return false;
		}

		const Vector3 p0 = mesh->positions[v0.positionIndex];
		const Vector3 p1 = mesh->positions[v1.positionIndex];
		const Vector3 p2 = mesh->positions[v2.positionIndex];

		const Vector2 u0 = mesh->texcoords[v0.texcoordIndex];
		const Vector2 u1 = mesh->texcoords[v1.texcoordIndex];
		const Vector2 u2 = mesh->texcoords[v2.texcoordIndex];

		const Vector3 n0 = mesh->normals[v0.normalIndex];
		const Vector3 n1 = mesh->normals[v1.normalIndex];
		const Vector3 n2 = mesh->normals[v2.normalIndex];

		Vector3 d0 = n0;
		Vector3 d1 = n1;
		Vector3 d2 = n2;
		if (meshForMapping)
		{
			const auto &mv0 = meshForMapping->vertices[tri.vertexIndex0];
			const auto &mv1 = meshForMapping->vertices[tri.vertexIndex1];
			const auto &mv2 = meshForMapping->vertices[tri.vertexIndex2];
			d0 = meshForMapping->normals[mv0.normalIndex];
			d1 = meshForMapping->normals[mv1.normalIndex];
			d2 = meshForMapping->normals[mv2.normalIndex];
		}

		const Vector3 t0 = mesh->tangents.empty() ? Vector3(0) : mesh->tangents[tri.vertexIndex0];
		const Vector3 t1 = mesh->tangents.empty() ? Vector3(0) : mesh->tangents[tri.vertexIndex1];
		const Vector3 t2 = mesh->tangents.empty() ? Vector3(0) : mesh->tangents[tri.vertexIndex2];
		const Vector3 b0 = mesh->tangents.empty() ? Vector3(0) : mesh->bitangents[tri.vertexIndex0];
		const Vector3 b1 = mesh->tangents.empty() ? Vector3(0) : mesh->bitangents[tri.vertexIndex1];
		const Vector3 b2 = mesh->tangents.empty() ? Vector3(0) : mesh->bitangents[tri.vertexIndex2];

		// Note: Surely not the fastest algorithm
		const Vector2 u01 = (u0 - halfpix) * scale;
		const Vector2 u11 = (u1 - halfpix) * scale;
		const Vector2 u21 = (u2 - halfpix) * scale;
		const uint32_t xMin = (uint32_t)std::fmaxf(std::roundf(std::fminf(u01.x, std::fminf(u11.x, u21.x))), 0.0f);
		const uint32_t yMin = (uint32_t)std::fmaxf(std::roundf(std::fminf(u01.y, std::fminf(u11.y, u21.y))), 0.0f);
		const uint32_t xMax = (uint32_t)std::fmaxf(std::roundf(std::fmaxf(u01.x, std::fmaxf(u11.x, u21.x))), 0.0f);
		const uint32_t yMax = (uint32_t)std::fmaxf(std::roundf(std::fmaxf(u01.y, std::fmaxf(u11.y, u21.y))), 0.0f);

		for (uint32_t y = yMin; y <= yMax; ++y)
		{
			for (uint32_t x = xMin; x <= xMax; ++x)
			{
				const Vector2 xy((float)x, (float)y);
				const Vector2 uv = xy * pixsize + halfpix;
				const Vector3 b = Barycentric(uv, u0, u1, u2);
				if (b.x >= -0.001f && b.x <= 1 &&
					b.y >= -0.001f && b.y <= 1 &&
					b.z >= -0.001f && b.z <= 1)
				{
					const int i = y * map->width + x;
					const Vector3 p = p0 * b.x + p1 * b.y + p2 * b.z;
					const Vector3 n = normalize(n0 * b.x + n1 * b.y + n2 * b.z);

					/*const float e0 = std::fminf(edgeDistance(p0, p1, p) / edge, 1.0f);
					const float e1 = std::fminf(edgeDistance(p1, p2, p) / edge, 1.0f);
					const float e2 = std::fminf(edgeDistance(p2, p0, p) / edge, 1.0f);
					const float ev0 = std::fminf(e0, e2);
					const float ev1 = std::fminf(e0, e1);
					const float ev2 = std::fminf(e1, e2);
					const Vector3 d0h = normalize(d0 * ev0 + n0 * (1.0f - ev0));
					const Vector3 d1h = normalize(d1 * ev1 + n1 * (1.0f - ev1));
					const Vector3 d2h = normalize(d2 * ev2 + n2 * (1.0f - ev2));
					const Vector3 d = normalize(d0h * b.x + d1h * b.y + d2h * b.z);*/
					const float t = std::fminf(triangleDistance(p0, p1, p2, p) / edge, 1.0f);
					const Vector3 dsmooth = normalize(d0 * b.x + d1 * b.y + d2 * b.z);
					const Vector3 d = normalize(dsmooth * (1.0f - t) + n * t);
					
					map->positions[i] = p;
					map->directions[i] = d;
					map->normals[i] = n;
					map->tangents[i] = normalize(t0 * b.x + t1 * b.y + t2 * b.z);
					map->bitangents[i] = normalize(b0 * b.x + b1 * b.y + b2 * b.z);
				}
			}
		}

		return true;
	}
#pragma optimize( "", on ) 

	MapUV* createMapUVEdge(const Mesh *mesh, const Mesh *meshDirs, uint32_t width, uint32_t height, float edge)
	{
		assert(mesh);

		MapUV *map = new MapUV(width, height);

		const Vector2 scale((float)width, (float)height);
		const Vector2 pixsize = Vector2(1.0f) / scale;
		const Vector2 halfpix = pixsize * 0.5f;

		//if (computeTangentSpace)
		{
			const size_t size = map->normals.size();
			map->tangents.resize(size);
			map->bitangents.resize(size);
		}

		for (const auto &tri : mesh->triangles)
		{
			if (!rasterTriangleEdge(mesh, meshDirs, tri, pixsize, halfpix, scale, edge, map))
			{
				delete map;
				return nullptr;
			}
		}

		return map;
	}
}

MapUV* MapUV::fromMesh(const Mesh *mesh, uint32_t width, uint32_t height)
{
	assert(mesh);
	return createMapUV(mesh, nullptr, width, height);
}

MapUV* MapUV::fromMeshes(const Mesh *mesh, const Mesh *meshDirs, uint32_t width, uint32_t height)
{
	assert(mesh);
	assert(meshDirs);
	return createMapUV(mesh, meshDirs, width, height);
}

MapUV* MapUV::fromMeshes_Hybrid(const Mesh *mesh, const Mesh *meshDirs, uint32_t width, uint32_t height, float edge)
{
	assert(mesh);
	assert(meshDirs);
	return createMapUVEdge(mesh, meshDirs, width, height, edge);
}

CompressedMapUV::CompressedMapUV(const MapUV *map)
	: width(map->width)
	, height(map->height)
{
	assert(map);
	assert(map->normals.size() > 0);

	for (size_t i = 0; i < map->directions.size(); ++i)
	{
		const Vector3 n = map->directions[i];
		if (dot(n, n) > 0.5f) // With normal data
		{
			indices.emplace_back((uint32_t)i);
		}
	}

	positions.resize(indices.size());
	directions.resize(indices.size());
	for (size_t i = 0; i < indices.size(); ++i)
	{
		const size_t idx = indices[i];
		positions[i] = map->positions[idx];
		directions[i] = map->directions[idx];
	}

	if (map->tangents.size() > 0)
	{
		assert(map->bitangents.size() == map->tangents.size());

		normals.resize(indices.size());
		tangents.resize(indices.size());
		bitangents.resize(indices.size());
		for (size_t i = 0; i < indices.size(); ++i)
		{
			const size_t idx = indices[i];
			normals[i] = map->normals[idx];
			tangents[i] = map->tangents[idx];
			bitangents[i] = map->bitangents[idx];
		}
	}

#if DEBUG_EXPORT_DIRECTIONS_MAP
	exportNormalImage(&directions[0], this, "D:\\asdf.png");
#endif
}
