#pragma once

#include "math.h"
#include "compute.h"
#include <glad/glad.h>
#include <memory>
#include <vector>

struct MapUV;
class Mesh;
class BVH;

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

	void init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<const Mesh> mesh, std::shared_ptr<const BVH> rootBVH);
	bool runStep();
	float* getResults();

	inline size_t width() const { return _mapWidth; }
	inline size_t height() const { return _mapHeight; }
	inline float progress() const { return (float)_workOffset / (float)_workCount; }

	inline std::shared_ptr<const CompressedMapUV> uvMap() const { return _uvMap; }

private:
	Params _params;
	size_t _workOffset;
	size_t _workCount;
	size_t _mapWidth;
	size_t _mapHeight;
	size_t _bvhCount;

	GLuint _thicknessProgram;
	GLuint _paramsBO;
	GLuint _raysBO;
	GLuint _samplesBO;
	GLuint _verticesBO;
	GLuint _divisionsBO;
	GLuint _resultsBO;

	std::shared_ptr<const CompressedMapUV> _uvMap;
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