#include "solver_position.h"
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

void PositionSolver::init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<MeshMapping> meshMapping)
{
	_positionProgram = LoadComputeShader_Position();
	_uvMap = map;
	_meshMapping = meshMapping;
	_workCount = ((map->positions.size() + k_groupSize - 1) / k_groupSize) * k_groupSize;
	_resultsCB = std::unique_ptr<ComputeBuffer<Vector3> >(
		new ComputeBuffer<Vector3>(_workCount, GL_STATIC_READ));
	_workOffset = 0;
	_mapWidth = map->width;
	_mapHeight = map->height;
}

bool PositionSolver::runStep()
{
	assert(_workOffset < _workCount);
	const size_t workLeft = _workCount - _workOffset;
	const size_t work = workLeft < k_workPerFrame ? workLeft : k_workPerFrame;
	assert(work % k_groupSize == 0);

	if (_workOffset == 0) _timing.begin();

	glUseProgram(_positionProgram);
	glUniform1ui(1, (GLuint)_workOffset);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _meshMapping->meshPositions()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _meshMapping->coords()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _meshMapping->coords_tidx()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, _resultsCB->bo());
	glDispatchCompute((GLuint)(work / k_groupSize), 1, 1);

	_workOffset += work;

	if (_workOffset >= _workCount)
	{
		_timing.end();
		std::cout << "Position map took " << _timing.elapsedSeconds() << " seconds for " << _mapWidth << "x" << _mapHeight << std::endl;
	}

	return _workOffset >= _workCount;
}

Vector3* PositionSolver::getResults()
{
	assert(_workOffset == _workCount);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	Vector3 *results = new Vector3[_workCount];
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _resultsCB->bo());
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Vector3) * _workCount, results);
	return results;
}

PositionTask::PositionTask(std::unique_ptr<PositionSolver> solver, const char *outputPath)
	: _solver(std::move(solver))
	, _outputPath(outputPath)
{
}

PositionTask::~PositionTask()
{
}

bool PositionTask::runStep()
{
	assert(_solver);
	return _solver->runStep();
}

void PositionTask::finish()
{
	assert(_solver);
	Vector3 *results = _solver->getResults();
	auto map = _solver->uvMap();
	exportVectorImage(results, _solver->uvMap().get(), _outputPath.c_str());
	delete[] results;
}

float PositionTask::progress() const
{
	assert(_solver);
	return _solver->progress();
}