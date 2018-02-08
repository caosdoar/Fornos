#include "solver_thickness.h"
#include "bvh.h"
#include "compute.h"
#include "meshmapping.h"
#include <cassert>

#pragma warning(disable:4996)
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static const size_t k_groupSize = 32;
static const size_t k_workPerFrame = 8192;

namespace
{

	std::vector<Vector3> computeSamples(size_t sampleCount)
	{
		std::vector<Vector3> sampleDirs(sampleCount);
		computeSamplesImportanceCosDir(sampleCount, &sampleDirs[0]);
		return sampleDirs;
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
}

void ThicknessSolver::init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<MeshMapping> meshMapping)
{
	_thicknessProgram = CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\thickness.comp");
	_uvMap = map;
	_meshMapping = meshMapping;
	_workCount = ((map->positions.size() + k_groupSize - 1) / k_groupSize) * k_groupSize;

	{
		ThicknessShaderParams params;
		params.sampleCount = (uint32_t)_params.sampleCount;
		params.minDistance = _params.minDistance;
		params.maxDistance = _params.maxDistance;
		_paramsCB = std::unique_ptr<ComputeBuffer<ThicknessShaderParams> >(
			new ComputeBuffer<ThicknessShaderParams>(params, GL_STATIC_DRAW));
	}

	auto samples = computeSamples(_params.sampleCount);
	std::vector<Vector4> samplesData(samples.begin(), samples.end());
	_samplesCB = std::unique_ptr<ComputeBuffer<Vector4> >(
		new ComputeBuffer<Vector4>(&samplesData[0], samplesData.size(), GL_STATIC_DRAW));
	_resultsAccCB = std::unique_ptr<ComputeBuffer<float> >(
		new ComputeBuffer<float>(_workCount, GL_STATIC_READ));
	_resultsDivCB = std::unique_ptr<ComputeBuffer<float> >(
		new ComputeBuffer<float>(_workCount, GL_STATIC_READ));

	_workOffset = 0;
	_sampleIndex = 0;
	_mapWidth = map->width;
	_mapHeight = map->height;
}

bool ThicknessSolver::runStep()
{
	assert(_workOffset < _workCount);
	const size_t workLeft = _workCount - _workOffset;
	const size_t work = workLeft < k_workPerFrame ? workLeft : k_workPerFrame;
	assert(work % k_groupSize == 0);

	if (_workOffset == 0 && _sampleIndex == 0) _timing.begin();

	glUseProgram(_thicknessProgram);

	glUniform1ui(1, (GLuint)_workOffset);
	glUniform1ui(2, (GLuint)_meshMapping->meshBVH()->size());
	glUniform1ui(3, (GLuint)_sampleIndex);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _paramsCB->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _meshMapping->pixels()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, _meshMapping->meshPositions()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, _meshMapping->meshNormals()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, _meshMapping->meshBVH()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, _meshMapping->coords()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, _meshMapping->coords_tidx()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, _samplesCB->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, _resultsAccCB->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, _resultsDivCB->bo());

	glDispatchCompute((GLuint)(work / k_groupSize), 1, 1);

	_workOffset += work;
	if (_workOffset >= _workCount)
	{
		++_sampleIndex;
		_workOffset = 0;
	}

	if (_sampleIndex >= _params.sampleCount)
	{
		_timing.end();
		std::cout << "Thickness map took " << _timing.elapsedSeconds() << " seconds for " << _mapWidth << "x" << _mapHeight << std::endl;
	}

	return _sampleIndex >= _params.sampleCount;
}

float* ThicknessSolver::getResults()
{
	assert(_sampleIndex >= _params.sampleCount);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	float *resultsAcc = _resultsAccCB->readData();
	float *resultsDiv = _resultsDivCB->readData();
	for (size_t i = 0; i < _resultsAccCB->size(); ++i)
	{
		resultsAcc[i] /= resultsDiv[i];
	}
	delete[] resultsDiv;
	return resultsAcc;
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