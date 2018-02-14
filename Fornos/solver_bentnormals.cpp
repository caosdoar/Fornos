#include "solver_bentnormals.h"
#include "compute.h"
#include "computeshaders.h"
#include "image.h"
#include "meshmapping.h"
#include <cassert>

static const size_t k_groupSize = 64;
static const size_t k_workPerFrame = 1024 * 128;
static const size_t k_samplePermCount = 64 * 64;

namespace
{
	std::vector<Vector3> computeSamples(size_t sampleCount, size_t permutationCount)
	{
		const size_t count = sampleCount * permutationCount;
		std::vector<Vector3> sampleDirs(count);
		computeSamplesImportanceCosDir(sampleCount, permutationCount, &sampleDirs[0]);
		return sampleDirs;
	}
}

void BentNormalsSolver::init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<MeshMapping> meshMapping)
{
	_rayProgram = LoadComputeShader_BN_GenData();
	_bentnormalsProgram = LoadComputeShader_BN_Sampling();
	_avgProgram = LoadComputeShader_BN_Aggregate();
	_tanspaceProgram = LoadComputeShader_ToTangentSpace();
	_uvMap = map;
	_meshMapping = meshMapping;
	_workCount = ((map->positions.size() + k_groupSize - 1) / k_groupSize) * k_groupSize;

	{
		ShaderParams params;
		params.sampleCount = (uint32_t)_params.sampleCount;
		params.minDistance = _params.minDistance;
		params.maxDistance = _params.maxDistance;
		_paramsCB = std::unique_ptr<ComputeBuffer<ShaderParams> >(
			new ComputeBuffer<ShaderParams>(params, GL_STATIC_DRAW));
	}

	auto samples = computeSamples(_params.sampleCount, k_samplePermCount);
	std::vector<Vector4> samplesData(samples.begin(), samples.end());
	_samplesCB = std::unique_ptr<ComputeBuffer<Vector4> >(
		new ComputeBuffer<Vector4>(&samplesData[0], samplesData.size(), GL_STATIC_DRAW));

	_rayDataCB = std::unique_ptr<ComputeBuffer<RayData> >(
		new ComputeBuffer<RayData>(k_workPerFrame / _params.sampleCount, GL_STATIC_READ));
	_resultsMiddleCB = std::unique_ptr<ComputeBuffer<Vector4> >(
		new ComputeBuffer<Vector4>(k_workPerFrame, GL_STATIC_READ)); // TODO: static read?
	_resultsFinalCB = std::unique_ptr<ComputeBuffer<Vector3> >(
		new ComputeBuffer<Vector3>(_workCount, GL_STATIC_READ));

	_workOffset = 0;
	_mapWidth = map->width;
	_mapHeight = map->height;
}

bool BentNormalsSolver::runStep()
{
	const size_t totalWork = _workCount * _params.sampleCount;
	assert(_workOffset < totalWork);
	const size_t workLeft = totalWork - _workOffset;
	const size_t work = workLeft < k_workPerFrame ? workLeft : k_workPerFrame;
	assert(work % k_groupSize == 0);

	if (_workOffset == 0) _timing.begin();

	glUseProgram(_rayProgram);
	glUniform1ui(1, GLuint(_workOffset / _params.sampleCount));
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _paramsCB->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, _meshMapping->meshPositions()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, _meshMapping->meshNormals()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, _meshMapping->coords()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, _meshMapping->coords_tidx()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, _rayDataCB->bo());
	glDispatchCompute((GLuint)(work / _params.sampleCount / k_groupSize), 1, 1);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glUseProgram(_bentnormalsProgram);
	glUniform1ui(1, GLuint(_workOffset / _params.sampleCount));
	glUniform1ui(2, (GLuint)_meshMapping->meshBVH()->size());
	glUniform1ui(14, (GLuint)k_samplePermCount);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _paramsCB->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _meshMapping->pixels()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, _meshMapping->meshPositions()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, _meshMapping->meshBVH()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, _samplesCB->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, _rayDataCB->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, _resultsMiddleCB->bo());
	glDispatchCompute((GLuint)(work / k_groupSize), 1, 1);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glUseProgram(_avgProgram);
	glUniform1ui(1, GLuint(_workOffset / _params.sampleCount));
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _paramsCB->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _resultsMiddleCB->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, _resultsFinalCB->bo());
	glDispatchCompute((GLuint)(work / _params.sampleCount / k_groupSize), 1, 1);

	if (_params.tangentSpace)
	{
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glUseProgram(_tanspaceProgram);
		glUniform1ui(1, GLuint(_workOffset / _params.sampleCount));
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _meshMapping->pixels()->bo());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, _meshMapping->pixelst()->bo());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, _resultsFinalCB->bo());
		glDispatchCompute((GLuint)(work / _params.sampleCount / k_groupSize), 1, 1);
	}

	_workOffset += work;

	if (_workOffset >= totalWork)
	{
		_timing.end();
		std::cout << "AO map took " << _timing.elapsedSeconds() << " seconds for " << _mapWidth << "x" << _mapHeight << std::endl;
	}

	return _workOffset >= totalWork;
}

Vector3* BentNormalsSolver::getResults()
{
	//assert(_sampleIndex >= _params.sampleCount);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	return _resultsFinalCB->readData();
}

BentNormalsTask::BentNormalsTask(std::unique_ptr<BentNormalsSolver> solver, const char *outputPath)
	: _solver(std::move(solver))
	, _outputPath(outputPath)
{
}

BentNormalsTask::~BentNormalsTask()
{
}

bool BentNormalsTask::runStep()
{
	assert(_solver);
	return _solver->runStep();
}

void BentNormalsTask::finish()
{
	assert(_solver);
	Vector3 *results = _solver->getResults();
	exportNormalImage(results, _solver->uvMap().get(), _outputPath.c_str());
	delete[] results;
}

float BentNormalsTask::progress() const
{
	return _solver->progress();
}