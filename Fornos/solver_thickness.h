#pragma once

#include "math.h"
#include "compute.h"
#include "timing.h"
#include <glad/glad.h>
#include <memory>
#include <vector>

struct MapUV;
class MeshMapping;

class ThicknessSolver
{
public:
	struct Params
	{
		size_t sampleCount;
		float minDistance;
		float maxDistance;
	};

public:
	ThicknessSolver(const Params &params) : _params(params) {}

	void init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<MeshMapping> meshMapping);
	bool runStep();
	float* getResults();

	inline size_t width() const { return _mapWidth; }
	inline size_t height() const { return _mapHeight; }
	inline float progress() const 
	{ 
		return (float)(_workOffset + _workCount * _sampleIndex) / (float)(_workCount * _params.sampleCount); 
	}

	inline std::shared_ptr<const CompressedMapUV> uvMap() const { return _uvMap; }

private:
	Params _params;
	size_t _workOffset;
	size_t _workCount;
	size_t _sampleIndex;
	size_t _mapWidth;
	size_t _mapHeight;

	struct ThicknessShaderParams
	{
		uint32_t sampleCount;
		float minDistance;
		float maxDistance;
		float _pad0;
	};

	GLuint _thicknessProgram;
	std::unique_ptr<ComputeBuffer<ThicknessShaderParams> > _paramsCB;
	std::unique_ptr<ComputeBuffer<Vector4> > _samplesCB;
	std::unique_ptr<ComputeBuffer<float> > _resultsAccCB;
	std::unique_ptr<ComputeBuffer<float> > _resultsDivCB;

	std::shared_ptr<const CompressedMapUV> _uvMap;
	std::shared_ptr<MeshMapping> _meshMapping;

	Timing _timing;
};

class ThicknessTask : public FornosTask
{
public:
	ThicknessTask(std::unique_ptr<ThicknessSolver> solver, const char *outputPath);
	~ThicknessTask();

	bool runStep();
	void finish();
	float progress() const;
	const char* name() const { return "Thickness"; }

private:
	std::unique_ptr<ThicknessSolver> _solver;
	std::string _outputPath;
};