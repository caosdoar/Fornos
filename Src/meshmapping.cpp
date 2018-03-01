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

#include "meshmapping.h"
#include "bvh.h"
#include "computeshaders.h"
#include "logging.h"
#include "mesh.h"
#include <cassert>

static const size_t k_groupSize = 64;
static const size_t k_workPerFrame = 1024 * 128;

namespace
{
	std::vector<Pix_GPUData> computePixels(const CompressedMapUV *map)
	{
		const size_t count = map->positions.size();
		std::vector<Pix_GPUData> pixels(count);
		for (size_t i = 0; i < count; ++i)
		{
			auto &pix = pixels[i];
			pix.p = map->positions[i];
			pix.d = map->directions[i];
		}
		return pixels;
	}

	std::vector<PixT_GPUData> computePixelsT(const CompressedMapUV *map)
	{
		const size_t count = map->positions.size();
		std::vector<PixT_GPUData> pixels(count);
		for (size_t i = 0; i < count; ++i)
		{
			auto &pix = pixels[i];
			pix.n = map->normals[i];
			pix.t = map->tangents[i];
			pix.b = map->bitangents[i];
		}
		return pixels;
	}

	void fillMeshData(
		const Mesh *mesh,
		const BVH& bvh,
		std::vector<BVHGPUData> &bvhs,
		std::vector<Vector4> &positions,
		std::vector<Vector4> &normals)
	{
		if (bvh.children.empty() &&
			bvh.triangles.empty())
		{
			// Children node without triangles? Skip it
			return;
		}

#if 1
		if (!bvh.children.empty())
		{
			// If one of the children does not contain any triangles
			// we can skip this node completely as it is an extry AABB test
			// for nothing
			if (bvh.children[0].subtreeTriangleCount > 0 && bvh.children[1].subtreeTriangleCount == 0)
			{
				fillMeshData(mesh, bvh.children[0], bvhs, positions, normals);
				return;
			}
			if (bvh.children[1].subtreeTriangleCount > 0 && bvh.children[0].subtreeTriangleCount == 0)
			{
				fillMeshData(mesh, bvh.children[1], bvhs, positions, normals);
				return;
			}
		}
#endif

		bvhs.emplace_back(BVHGPUData());
		BVHGPUData &d = bvhs.back();
		d.aabbMin = bvh.aabb.center - bvh.aabb.size;
		d.aabbMax = bvh.aabb.center + bvh.aabb.size;
		d.start = (uint32_t)positions.size();
		for (uint32_t tidx : bvh.triangles)
		{
			const auto &tri = mesh->triangles[tidx];
			const auto &v0 = mesh->vertices[tri.vertexIndex0];
			const auto &v1 = mesh->vertices[tri.vertexIndex1];
			const auto &v2 = mesh->vertices[tri.vertexIndex2];
			const auto p0 = mesh->positions[v0.positionIndex];
			const auto p1 = mesh->positions[v1.positionIndex];
			const auto p2 = mesh->positions[v2.positionIndex];
			positions.push_back(p0);
			positions.push_back(p1);
			positions.push_back(p2);
			normals.push_back(mesh->normals[v0.normalIndex]);
			normals.push_back(mesh->normals[v1.normalIndex]);
			normals.push_back(mesh->normals[v2.normalIndex]);
		}
		d.end = (uint32_t)positions.size();

		const size_t index = bvhs.size() - 1; // Because d gets invalidated by fillMeshData!
		if (bvh.children.size() > 0)
		{
			fillMeshData(mesh, bvh.children[0], bvhs, positions, normals);
			fillMeshData(mesh, bvh.children[1], bvhs, positions, normals);
		}
		bvhs[index].jump = (uint32_t)bvhs.size();
	}
}

void MeshMapping::init
(
	std::shared_ptr<const CompressedMapUV> map,
	std::shared_ptr<const Mesh> mesh,
	std::shared_ptr<const BVH> rootBVH,
	bool cullBackfaces
)
{
	// Pixels data
	{
		auto pixels = computePixels(map.get());
		_pixels = std::unique_ptr<ComputeBuffer<Pix_GPUData> >(
			new ComputeBuffer<Pix_GPUData>(&pixels[0], pixels.size(), GL_STATIC_DRAW));

		// Compute tangent data
		if (map->tangents.size() > 0)
		{
			auto pixelst = computePixelsT(map.get());
			_pixelst = std::unique_ptr<ComputeBuffer<PixT_GPUData> >(
				new ComputeBuffer<PixT_GPUData>(&pixelst[0], pixelst.size(), GL_STATIC_DRAW));
		}
	}

	// Mesh data
	{
		std::vector<BVHGPUData> bvhs;
		std::vector<Vector4> positions;
		std::vector<Vector4> normals;
		fillMeshData(mesh.get(), *rootBVH, bvhs, positions, normals);
		_meshPositions = std::unique_ptr<ComputeBuffer<Vector4> >(
			new ComputeBuffer<Vector4>(&positions[0], positions.size(), GL_STATIC_DRAW));
		_meshNormals = std::unique_ptr<ComputeBuffer<Vector4> >(
			new ComputeBuffer<Vector4>(&normals[0], normals.size(), GL_STATIC_DRAW));
		_bvh = std::unique_ptr<ComputeBuffer<BVHGPUData> >(
			new ComputeBuffer<BVHGPUData>(&bvhs[0], bvhs.size(), GL_STATIC_DRAW));
	}

	_workCount = ((map->positions.size() + k_groupSize - 1) / k_groupSize) * k_groupSize;

	// Results data
	{
		_coords = std::unique_ptr<ComputeBuffer<Vector4> >(
			new ComputeBuffer<Vector4>(_workCount, GL_STATIC_DRAW));
		_tidx = std::unique_ptr<ComputeBuffer<uint32_t> >(
			new ComputeBuffer<uint32_t>(_workCount, GL_STATIC_DRAW));
	}

	// Shader
	{
		_program = LoadComputeShader_MeshMapping();
		_programCullBackfaces = LoadComputeShader_MeshMappingCullBackfaces();
	}

	_cullBackfaces = cullBackfaces;

	_workOffset = 0;
}

bool MeshMapping::runStep()
{
	assert(_workOffset < _workCount);
	const size_t workLeft = _workCount - _workOffset;
	const size_t work = workLeft < k_workPerFrame ? workLeft : k_workPerFrame;
	assert(work % k_groupSize == 0);

	if (_workOffset == 0) _timing.begin();

	if (_cullBackfaces) glUseProgram(_programCullBackfaces);
	else glUseProgram(_program);

	glUniform1ui(1, (GLuint)_workOffset);
	glUniform1ui(2, (GLuint)_coords->size());
	glUniform1ui(3, (GLuint)_bvh->size());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _pixels->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, _meshPositions->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, _bvh->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, _coords->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, _tidx->bo());

	glDispatchCompute((GLuint)(work / k_groupSize), 1, 1);

	_workOffset += work;

	if (_workOffset == _workCount)
	{
		_timing.end();
		logDebug("MeshMap", "Mesh mapping took " + std::to_string(_timing.elapsedSeconds()) + " seconds.");
	}

	return _workOffset >= _workCount;
}

MeshMappingTask::MeshMappingTask(std::shared_ptr<MeshMapping> meshmapping)
	: _meshMapping(meshmapping)
{
}

MeshMappingTask::~MeshMappingTask()
{
}

bool MeshMappingTask::runStep()
{
	assert(_meshMapping);
	return _meshMapping->runStep();
}

void MeshMappingTask::finish()
{
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

float MeshMappingTask::progress() const
{
	assert(_meshMapping);
	return _meshMapping->progress();
}
