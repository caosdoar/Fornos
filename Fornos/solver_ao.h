#pragma once

#include "math.h"
#include "compute.h"
#include "timing.h"
#include <glad/glad.h>
#include <memory>
#include <vector>

#define AO_USE_TEXTURES 0

struct MapUV;
class MeshMapping;

class AmbientOcclusionSolver
{
public:
	struct Params
	{
		size_t sampleCount;
		float minDistance;
		float maxDistance;
	};

public:
	AmbientOcclusionSolver(const Params &params) : _params(params) {}

	void init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<MeshMapping> meshMapping);
	bool runStep();
	float* getResults();

	inline size_t width() const { return _mapWidth; }
	inline size_t height() const { return _mapHeight; }
	inline float progress() const
	{
		return (float)(_workOffset) / (float)(_workCount * _params.sampleCount);
	}

	inline std::shared_ptr<const CompressedMapUV> uvMap() const { return _uvMap; }

private:
	Params _params;
	size_t _workOffset;
	size_t _workCount;
	size_t _mapWidth;
	size_t _mapHeight;

	size_t _sampleIndex;

	struct ShaderParams
	{
		uint32_t sampleCount;
		float minDistance;
		float maxDistance;
		float _pad0;
	};

	struct RayData
	{
		Vector3 o; float _pad0;
		Vector3 d; float _pad1;
		Vector3 tx; float _pad2;
		Vector3 ty; float _pad3;
	};

	GLuint _rayProgram;
	GLuint _aoProgram;
	GLuint _avgProgram;
	std::unique_ptr<ComputeBuffer<ShaderParams> > _paramsCB;
	std::unique_ptr<ComputeBuffer<Vector4> > _samplesCB;
#if AO_USE_TEXTURES
	std::unique_ptr<ComputeTexture_Uint32> _resultsAccTex;
#else
	std::unique_ptr<ComputeBuffer<RayData> > _rayDataCB;
	std::unique_ptr<ComputeBuffer<float> > _resultsMiddleCB;
	std::unique_ptr<ComputeBuffer<float> > _resultsFinalCB;
#endif

	std::shared_ptr<const CompressedMapUV> _uvMap;
	std::shared_ptr<MeshMapping> _meshMapping;

	Timing _timing;
};

class AmbientOcclusionTask : public FornosTask
{
public:
	AmbientOcclusionTask(std::unique_ptr<AmbientOcclusionSolver> solver, const char *outputPath);
	~AmbientOcclusionTask();

	bool runStep();
	void finish();
	float progress() const;
	const char* name() const { return "Ambient Occlusion"; }

private:
	std::unique_ptr<AmbientOcclusionSolver> _solver;
	std::string _outputPath;
};