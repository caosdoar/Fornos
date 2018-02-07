#include "solver_normals.h"
#include "bvh.h"
#include "compute.h"
#include "math.h"
#include "mesh.h"
#include <cassert>

#pragma warning(disable:4996)
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static const size_t k_groupSize = 32;
static const size_t k_workPerFrame = 8192;

std::vector<Ray> computeRays(const CompressedMapUV *map, const float offset, bool inwards)
{
	const size_t count = map->positions.size();
	std::vector<Ray> rays(count);
	for (size_t i = 0; i < count; ++i)
	{
		const Vector3 n = map->normals[i];
		const Vector3 o = map->positions[i];
		rays[i].origin = o + n * offset;
		rays[i].direction = inwards ? -n : n;
	}

	return rays;
}

void fillMeshData(
	const Mesh *mesh,
	const BVH& bvh,
	std::vector<BVHGPUData> &bvhs,
	std::vector<Vector4> &positions,
	std::vector<Vector4> &normals)
{
	bvhs.emplace_back(BVHGPUData());
	BVHGPUData &d = bvhs.back();
	d.o = bvh.aabb.center;
	d.s = bvh.aabb.size;
	d.start = (uint32_t)positions.size();
	for (uint32_t tidx : bvh.triangles)
	{
		const auto &tri = mesh->triangles[tidx];
		const auto &v0 = mesh->vertices[tri.vertexIndex0];
		const auto &v1 = mesh->vertices[tri.vertexIndex1];
		const auto &v2 = mesh->vertices[tri.vertexIndex2];
		positions.push_back(mesh->positions[v0.positionIndex]);
		positions.push_back(mesh->positions[v1.positionIndex]);
		positions.push_back(mesh->positions[v2.positionIndex]);
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
	bvhs[index].right = (uint32_t)bvhs.size();
}

void ExportNormalMap(const Vector3 *normals, const CompressedMapUV *map, const size_t w, const size_t h, const char *path)
{
	uint8_t *rgb = new uint8_t[w * h * 3];
	memset(rgb, 0, sizeof(uint8_t) * w * h * 3);
	for (size_t i = 0; i < map->indices.size(); ++i)
	{
		const Vector3 normal = normals[i];
		const Vector3 n = normal * 0.5f + Vector3(0.5f);
		const size_t index = map->indices[i];
		const size_t x = index % w;
		const size_t y = index / w;
		const size_t pixidx = ((h - y - 1) * w + x) * 3;
		rgb[pixidx + 0] = (uint8_t)(n.x * 255.0f);
		rgb[pixidx + 1] = (uint8_t)(n.y * 255.0f);
		rgb[pixidx + 2] = (uint8_t)(n.z * 255.0f);
	}
	stbi_write_png(path, (int)w, (int)h, 3, rgb, (int)w * 3);
	delete[] rgb;
}

void NormalsSolver::init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<const Mesh> mesh, std::shared_ptr<const BVH> rootBVH)
{
	_normalsProgram = CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\normals.comp");

	// Copy uvmap data
	_uvMap = map;

	// Rays data
	auto rays = computeRays(map.get(), _params.rayOffset, _params.rayInwards);
	std::vector<RayGPUData> raysData(rays.begin(), rays.end());

	// Mesh data
	std::vector<BVHGPUData> bvhs;
	std::vector<Vector4> positions;
	std::vector<Vector4> normals;
	fillMeshData(mesh.get(), *rootBVH, bvhs, positions, normals);
	_bvhCount = bvhs.size();

	_workCount = ((map->positions.size() + k_groupSize - 1) / k_groupSize) * k_groupSize;

	_raysBO = CreateComputeBuffer(&raysData[0], raysData.size(), GL_STATIC_DRAW);
	_positionsBO = CreateComputeBuffer(&positions[0], positions.size(), GL_STATIC_DRAW);
	_normalsBO = CreateComputeBuffer(&normals[0], normals.size(), GL_STATIC_DRAW);
	_bvhBO = CreateComputeBuffer(&(bvhs[0]), bvhs.size(), GL_STATIC_DRAW);
	_resultsBO = CreateComputeBuffer(sizeof(float) * 3 * _workCount, GL_STATIC_READ);

	_workOffset = 0;
	//_workCount = map->positions.size();
	_mapWidth = map->width;
	_mapHeight = map->height;
}

bool NormalsSolver::runStep()
{
	assert(_workOffset < _workCount);
	const size_t workLeft = _workCount - _workOffset;
	const size_t work = workLeft < k_workPerFrame ? workLeft : k_workPerFrame;
	assert(work % k_groupSize == 0);

	glUseProgram(_normalsProgram);

	glUniform1ui(1, (GLuint)_workOffset);
	glUniform1ui(2, (GLuint)_bvhCount);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _raysBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, _positionsBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, _normalsBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, _bvhBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, _resultsBO);

	glDispatchCompute((GLuint)(work / k_groupSize), 1, 1);

	_workOffset += work;
	return _workOffset >= _workCount;
}

float* NormalsSolver::getResults()
{
	assert(_workOffset == _workCount);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	float *results = new float[_workCount * 3];
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _resultsBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * 3 * _workCount, results);
	return results;
}

NormalsTask::NormalsTask(std::unique_ptr<NormalsSolver> solver, const char *outputPath)
	: _solver(std::move(solver))
	, _outputPath(outputPath)
{
}

NormalsTask::~NormalsTask()
{
}

bool NormalsTask::runStep()
{
	assert(_solver);
	return _solver->runStep();
}

void NormalsTask::finish()
{
	assert(_solver);
	Vector3 *results = (Vector3*)_solver->getResults();
	auto map = _solver->uvMap();

	if (_solver->params().tangentSpace)
	{
		// Convert to tangent space
		const size_t count = map->indices.size();
		for (size_t i = 0; i < count; ++i)
		{
			const Vector3 on = results[i];
			const Vector3 n = map->normals[i];
			const Vector3 t = map->tangents[i];
			const Vector3 b = map->bitangents[i];
			//const float det = t.z*n.y*b.x - t.y*n.z*b.x - n.x*t.z*b.y + t.x*n.z*b.y + n.x*t.y*b.z - t.x*n.y*b.z;
			const Vector3 d0(n.z*b.y - n.y*b.z, n.x*b.z - n.z*b.x, n.y*b.x - n.x*b.y);
			const Vector3 d1(t.z*n.y - t.y*n.z, t.x*n.z - n.x*t.z, n.x*t.y - t.x*n.y);
			const Vector3 d2(t.y*b.z - t.z*b.y, t.z*b.x - t.x*b.z, t.x*b.y - t.y*b.x);
			const Vector3 r(dot(on, d0), dot(on, d1), dot(on, d2));
			results[i] = normalize(r);
		}
	}


	ExportNormalMap(&results[0], _solver->uvMap().get(), _solver->width(), _solver->height(), _outputPath.c_str());
	delete[] results;
}

float NormalsTask::progress() const
{
	assert(_solver);
	return _solver->progress();
}