#include "solver_height.h"
#include "bvh.h"
#include "compute.h"
#include "computeshaders.h"
#include "image.h"
#include "math.h"
#include "mesh.h"
#include "meshmapping.h"
#include <cassert>

static const size_t k_groupSize = 64;
static const size_t k_workPerFrame = 1024 * 128;

void HeightSolver::init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<MeshMapping> meshMapping)
{
	_heightProgram = LoadComputeShader_Height();
	_uvMap = map;
	_meshMapping = meshMapping;
	_workCount = ((map->positions.size() + k_groupSize - 1) / k_groupSize) * k_groupSize;
	_resultsCB = std::unique_ptr<ComputeBuffer<float> >(
		new ComputeBuffer<float>(3 * _workCount, GL_STATIC_READ));
	_workOffset = 0;
	_mapWidth = map->width;
	_mapHeight = map->height;
}

bool HeightSolver::runStep()
{
	assert(_workOffset < _workCount);
	const size_t workLeft = _workCount - _workOffset;
	const size_t work = workLeft < k_workPerFrame ? workLeft : k_workPerFrame;
	assert(work % k_groupSize == 0);

	if (_workOffset == 0) _timing.begin();

	glUseProgram(_heightProgram);
	glUniform1ui(1, (GLuint)_workOffset);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _meshMapping->coords()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _resultsCB->bo());
	glDispatchCompute((GLuint)(work / k_groupSize), 1, 1);

	_workOffset += work;

	if (_workOffset >= _workCount)
	{
		_timing.end();
		std::cout << "Height map took " << _timing.elapsedSeconds() << " seconds for " << _mapWidth << "x" << _mapHeight << std::endl;
	}

	return _workOffset >= _workCount;
}

float* HeightSolver::getResults()
{
	assert(_workOffset == _workCount);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	float *results = new float[_workCount];
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _resultsCB->bo());
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * _workCount, results);
	return results;
}

HeightTask::HeightTask(std::unique_ptr<HeightSolver> solver, const char *outputPath)
	: _solver(std::move(solver))
	, _outputPath(outputPath)
{
}

HeightTask::~HeightTask()
{
}

bool HeightTask::runStep()
{
	assert(_solver);
	return _solver->runStep();
}

void HeightTask::finish()
{
	assert(_solver);
	float *results = _solver->getResults();
	auto map = _solver->uvMap();
	Vector2 minmax;
	exportFloatImage(results, _solver->uvMap().get(), _outputPath.c_str(), true, &minmax);
	delete[] results;
	std::cout << "Height map range: " << minmax.x << " to " << minmax.y << std::endl;
}

float HeightTask::progress() const
{
	assert(_solver);
	return _solver->progress();
}