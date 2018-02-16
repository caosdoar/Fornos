#pragma once

#include <glad/glad.h>
#include "compute.h"
#include "timing.h"
#include <memory>

struct CompressedMapUV;
class MeshMapping;

class HeightSolver
{
public:
	HeightSolver() {}

	void init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<MeshMapping> mesh);
	bool runStep();
	float* getResults();

	inline size_t width() const { return _mapWidth; }
	inline size_t height() const { return _mapHeight; }
	inline float progress() const { return (float)_workOffset / (float)_workCount; }

	inline std::shared_ptr<const CompressedMapUV> uvMap() const { return _uvMap; }

private:
	size_t _workOffset;
	size_t _workCount;
	size_t _mapWidth;
	size_t _mapHeight;

	GLuint _heightProgram;
	std::unique_ptr<ComputeBuffer<float> > _resultsCB;

	std::shared_ptr<const CompressedMapUV> _uvMap;
	std::shared_ptr<MeshMapping> _meshMapping;

	Timing _timing;
};

class HeightTask : public FornosTask
{
public:
	HeightTask(std::unique_ptr<HeightSolver> solver, const char *outputPath);
	~HeightTask();

	bool runStep();
	void finish();
	float progress() const;
	const char* name() const { return "Height"; }

private:
	std::unique_ptr<HeightSolver> _solver;
	std::string _outputPath;
};
