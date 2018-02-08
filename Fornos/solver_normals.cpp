#include "solver_normals.h"
#include "bvh.h"
#include "compute.h"
#include "math.h"
#include "mesh.h"
#include "meshmapping.h"
#include <cassert>

#pragma warning(disable:4996)
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static const size_t k_groupSize = 32;
static const size_t k_workPerFrame = 65536;

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

void NormalsSolver::init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<MeshMapping> meshMapping)
{
	_normalsProgram = CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\normals.comp");
	_uvMap = map;
	_meshMapping = meshMapping;
	_workCount = ((map->positions.size() + k_groupSize - 1) / k_groupSize) * k_groupSize;
	_resultsCB = std::unique_ptr<ComputeBuffer<float> >(
		new ComputeBuffer<float>(3 * _workCount, GL_STATIC_READ));
	_workOffset = 0;
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
	glUniform1ui(2, (GLuint)_meshMapping->meshBVH()->size());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _meshMapping->pixels()->bo());
	if (_meshMapping->pixelst()) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, _meshMapping->pixelst()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, _meshMapping->meshPositions()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, _meshMapping->meshNormals()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, _meshMapping->meshBVH()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, _meshMapping->coords()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, _meshMapping->coords_tidx()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, _resultsCB->bo());

	glDispatchCompute((GLuint)(work / k_groupSize), 1, 1);

	_workOffset += work;
	return _workOffset >= _workCount;
}

float* NormalsSolver::getResults()
{
	assert(_workOffset == _workCount);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	float *results = new float[_workCount * 3];
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _resultsCB->bo());
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
	ExportNormalMap(&results[0], _solver->uvMap().get(), _solver->width(), _solver->height(), _outputPath.c_str());
	delete[] results;
}

float NormalsTask::progress() const
{
	assert(_solver);
	return _solver->progress();
}