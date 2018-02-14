#include "solver_normals.h"
#include "bvh.h"
#include "compute.h"
#include "image.h"
#include "math.h"
#include "mesh.h"
#include "meshmapping.h"
#include <cassert>

static const size_t k_groupSize = 64;
static const size_t k_workPerFrame = 1024 * 128;

void NormalsSolver::init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<MeshMapping> meshMapping)
{
	_normalsProgram = CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\normals.comp");
	_tanspaceProgram = CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\tangentspace.comp");
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

	{
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
	}

	if (_params.tangentSpace)
	{
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glUseProgram(_tanspaceProgram);
		glUniform1ui(1, GLuint(_workOffset));
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _meshMapping->pixels()->bo());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, _meshMapping->pixelst()->bo());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, _resultsCB->bo());
		glDispatchCompute((GLuint)(work / k_groupSize), 1, 1);
	}

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
	exportNormalImage(results, _solver->uvMap().get(), _outputPath.c_str());
	delete[] results;
}

float NormalsTask::progress() const
{
	assert(_solver);
	return _solver->progress();
}