#include "solver_thickness.h"
#include "bvh.h"
#include "compute.h"
#include "mesh.h"
#include <cassert>

#pragma warning(disable:4996)
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static const size_t k_groupSize = 32;
static const size_t k_workPerFrame = 8192;

struct ThicknessShaderParams
{
	uint32_t sampleCount;
	float minDistance;
	float maxDistance;
	float _pad0;
};

std::vector<Vector3> computeSamples(size_t sampleCount)
{
	std::vector<Vector3> sampleDirs(sampleCount);
	computeSamplesImportanceCosDir(sampleCount, &sampleDirs[0]);
	return sampleDirs;
}

std::vector<Ray> computeRays(const CompressedMapUV *map, const float margin)
{
	const size_t count = map->positions.size();
	std::vector<Ray> rays(count);
	for (size_t i = 0; i < count; ++i)
	{
		const Vector3 n = map->normals[i];
		const Vector3 o = map->positions[i];
		rays[i].origin = o - n * margin;
		rays[i].direction = -n;
	}

	return rays;
}

void fillMeshData(
	const Mesh *mesh,
	const BVH& bvh,
	std::vector<BVHGPUData> &bvhs,
	std::vector<Vector4> &vertices)
{
	bvhs.emplace_back(BVHGPUData());
	BVHGPUData &d = bvhs.back();
	d.o = bvh.aabb.center;
	d.s = bvh.aabb.size;
	d.start = (uint32_t)vertices.size();
	for (uint32_t tidx : bvh.triangles)
	{
		const auto &tri = mesh->triangles[tidx];
		vertices.push_back(mesh->positions[mesh->vertices[tri.vertexIndex0].positionIndex]);
		vertices.push_back(mesh->positions[mesh->vertices[tri.vertexIndex1].positionIndex]);
		vertices.push_back(mesh->positions[mesh->vertices[tri.vertexIndex2].positionIndex]);
	}
	d.end = (uint32_t)vertices.size();

	const size_t index = bvhs.size() - 1; // Because d gets invalidated by fillMeshData!
	if (bvh.children.size() > 0)
	{
		d.left = (uint32_t)bvhs.size();
		fillMeshData(mesh, bvh.children[0], bvhs, vertices);
		fillMeshData(mesh, bvh.children[1], bvhs, vertices);
	}
	bvhs[index].right = (uint32_t)bvhs.size();
}

Vector2 ExportFloatMap(const float *data, const CompressedMapUV *map, const size_t w, const size_t h, const char *path)
{
	const size_t count = map->indices.size();

	Vector2 minmax(FLT_MAX, -FLT_MAX);
	for (size_t i = 0; i < count; ++i)
	{
		const float r = data[i];
		minmax.x = std::fminf(minmax.x, r);
		minmax.y = std::fmaxf(minmax.y, r);
	}

	const float scale = 1.0f / (minmax.y - minmax.x);
	const float bias = -minmax.x * scale;

	uint8_t *rgb = new uint8_t[w * h * 3];
	memset(rgb, 0, sizeof(uint8_t) * w * h * 3);

	for (size_t i = 0; i < count; ++i)
	{
		const float d = data[i];
		const float t = std::fminf(d * scale + bias, 1.0f);
		const uint8_t c = (uint8_t)(t * 255.0f);
		const size_t index = map->indices[i];
		const size_t x = index % w;
		const size_t y = index / w;
		const size_t pixidx = ((h - y - 1) * w + x) * 3;
		rgb[pixidx + 0] = c;
		rgb[pixidx + 1] = c;
		rgb[pixidx + 2] = c;
	}

	stbi_write_png(path, (int)w, (int)h, 3, rgb, (int)w * 3);
	delete[] rgb;

	return minmax;
}

void ThicknessSolver::init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<const Mesh> mesh, std::shared_ptr<const BVH> rootBVH)
{
	_thicknessProgram = CreateComputeProgram("D:\\Code\\Fornos\\x64\\Release\\thickness.comp");

	// Copy uvmap data
	_uvMap = map;

	// Rays data
	auto rays = computeRays(map.get(), 0.01f);
	auto samples = computeSamples(_params.sampleCount);
	std::vector<RayGPUData> raysData(rays.begin(), rays.end());
	std::vector<Vector4> samplesData(samples.begin(), samples.end());

	// Mesh data
	std::vector<BVHGPUData> bvhs;
	std::vector<Vector4> vertices;
	fillMeshData(mesh.get(), *rootBVH, bvhs, vertices);
	_bvhCount = bvhs.size();

	_workCount = ((map->positions.size() + k_groupSize - 1) / k_groupSize) * k_groupSize;

	{
		ThicknessShaderParams params;
		params.sampleCount = (uint32_t)_params.sampleCount;
		params.minDistance = _params.minDistance;
		params.maxDistance = _params.maxDistance;
		_paramsBO = CreateComputeBuffer(params, GL_STATIC_DRAW);
	}

	_raysBO = CreateComputeBuffer(&raysData[0], raysData.size(), GL_STATIC_DRAW);
	_samplesBO = CreateComputeBuffer(&samplesData[0], samplesData.size(), GL_STATIC_DRAW);
	_verticesBO = CreateComputeBuffer(&(vertices[0]), vertices.size(), GL_STATIC_DRAW);
	_divisionsBO = CreateComputeBuffer(&(bvhs[0]), bvhs.size(), GL_STATIC_DRAW);
	_resultsBO = CreateComputeBuffer(sizeof(float) * map->width * map->height, GL_STATIC_READ);

	_workOffset = 0;
	//_workCount = map->width * map->height;
	_mapWidth = map->width;
	_mapHeight = map->height;
}

bool ThicknessSolver::runStep()
{
	assert(_workOffset < _workCount);
	const size_t workLeft = _workCount - _workOffset;
	const size_t work = workLeft < k_workPerFrame ? workLeft : k_workPerFrame;
	assert(work % k_groupSize == 0);

	glUseProgram(_thicknessProgram);

	glUniform1ui(1, (GLuint)_workOffset);
	glUniform1ui(2, (GLuint)_bvhCount);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _paramsBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _raysBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, _samplesBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, _verticesBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, _divisionsBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, _resultsBO);

	glDispatchCompute((GLuint)(work / k_groupSize), 1, 1);

	_workOffset += work;
	return _workOffset >= _workCount;
}

float* ThicknessSolver::getResults()
{
	assert(_workOffset >= _workCount);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	float *results = new float[_workCount];
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _resultsBO);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * _workCount, results);
	return results;
}

ThicknessTask::ThicknessTask(std::unique_ptr<ThicknessSolver> solver, const char *outputPath)
	: _solver(std::move(solver))
	, _outputPath(outputPath)
{
}

ThicknessTask::~ThicknessTask()
{
}

bool ThicknessTask::runStep()
{
	assert(_solver);
	return _solver->runStep();
}

void ThicknessTask::finish()
{
	assert(_solver);
	float *results = _solver->getResults();
	ExportFloatMap(results, _solver->uvMap().get(), _solver->width(), _solver->height(), _outputPath.c_str());
	delete[] results;
}

float ThicknessTask::progress() const
{
	return _solver->progress();
}