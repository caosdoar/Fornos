#include "solver_ao.h"
#include "compute.h"
#include "meshmapping.h"
#include <cassert>

#pragma warning(disable:4996)
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

	void ExportAOMap(const float *data, const CompressedMapUV *map, const size_t w, const size_t h, const char *path)
	{
		const size_t count = map->indices.size();

		uint8_t *rgb = new uint8_t[w * h * 3];
		memset(rgb, 0, sizeof(uint8_t) * w * h * 3);

		for (size_t i = 0; i < count; ++i)
		{
			const float d = data[i];
			const float t = 1.0f - d;
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
	}
}

void AmbientOcclusionSolver::init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<MeshMapping> meshMapping)
{
	_aoProgram = CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\ambientocclusion.comp");
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
#if AO_USE_TEXTURES
	_resultsAccTex = std::unique_ptr<ComputeTexture_Uint32>(new ComputeTexture_Uint32(map->width, map->height));
#else
	_resultsAccCB = std::unique_ptr<ComputeBuffer<float> >(
		new ComputeBuffer<float>(_workCount, GL_STATIC_READ));
#endif

	_workOffset = 0;
	_sampleIndex = 0;
	_mapWidth = map->width;
	_mapHeight = map->height;
}

bool AmbientOcclusionSolver::runStep()
{
	assert(_workOffset < _workCount);
	const size_t workLeft = _workCount - _workOffset;
	const size_t work = workLeft < k_workPerFrame ? workLeft : k_workPerFrame;
	assert(work % k_groupSize == 0);

	if (_workOffset == 0 && _sampleIndex == 0) _timing.begin();

	glUseProgram(_aoProgram);

	glUniform1ui(1, (GLuint)_workOffset);
	glUniform1ui(2, (GLuint)_meshMapping->meshBVH()->size());
	glUniform1ui(16, (GLuint)_sampleIndex);
	glUniform1ui(14, (GLuint)k_samplePermCount);
#if AO_USE_TEXTURES
	glUniform1ui(15, (GLuint)_resultsAccTex->width());
#endif
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _paramsCB->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _meshMapping->pixels()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, _meshMapping->meshPositions()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, _meshMapping->meshNormals()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, _meshMapping->meshBVH()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, _meshMapping->coords()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, _meshMapping->coords_tidx()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, _samplesCB->bo());
#if AO_USE_TEXTURES
	_resultsAccTex->bind(0, GL_READ_WRITE);
#else
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, _resultsAccCB->bo());
#endif

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
		std::cout << "AO map took " << _timing.elapsedSeconds() << " seconds for " << _mapWidth << "x" << _mapHeight << std::endl;
	}

	return _sampleIndex >= _params.sampleCount;
}

float* AmbientOcclusionSolver::getResults()
{
	assert(_sampleIndex >= _params.sampleCount);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
#if AO_USE_TEXTURES
	//float *resultsAcc = _resultsAccCB->readData();
	uint32_t *resultsAcc = _resultsAccTex->readData();
	const float s = 1.0f / (float)(_params.sampleCount);
	const size_t count = _resultsAccTex->width() * _resultsAccTex->height();
	float *results = new float[count];
	for (size_t i = 0; i < count; ++i)
	{
		results[i] = float(resultsAcc[i]) * s;
	}
	delete[] resultsAcc;
	return results;
#else
	float *resultsAcc = _resultsAccCB->readData();
	const float s = 1.0f / (float)(_params.sampleCount);
	for (size_t i = 0; i < _resultsAccCB->size(); ++i)
	{
		resultsAcc[i] *= s;
	}
	return resultsAcc;
#endif
}

AmbientOcclusionTask::AmbientOcclusionTask(std::unique_ptr<AmbientOcclusionSolver> solver, const char *outputPath)
	: _solver(std::move(solver))
	, _outputPath(outputPath)
{
}

AmbientOcclusionTask::~AmbientOcclusionTask()
{
}

bool AmbientOcclusionTask::runStep()
{
	assert(_solver);
	return _solver->runStep();
}

void AmbientOcclusionTask::finish()
{
	assert(_solver);
	float *results = _solver->getResults();
	ExportAOMap(results, _solver->uvMap().get(), _solver->width(), _solver->height(), _outputPath.c_str());
	delete[] results;
}

float AmbientOcclusionTask::progress() const
{
	return _solver->progress();
}