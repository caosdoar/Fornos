#pragma once

#include <glad/glad.h>
#include "compute.h"
#include "fornos.h"
#include "timing.h"
#include <memory>

struct CompressedMapUV;
class MeshMapping;

class PositionSolver
{
public:
	PositionSolver() {}

	void init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<MeshMapping> mesh);
	bool runStep();
	Vector3* getResults();

	inline size_t width() const { return _mapWidth; }
	inline size_t height() const { return _mapHeight; }
	inline float progress() const { return (float)_workOffset / (float)_workCount; }

	inline std::shared_ptr<const CompressedMapUV> uvMap() const { return _uvMap; }

private:
	size_t _workOffset;
	size_t _workCount;
	size_t _mapWidth;
	size_t _mapHeight;

	GLuint _positionProgram;
	std::unique_ptr<ComputeBuffer<Vector3> > _resultsCB;

	std::shared_ptr<const CompressedMapUV> _uvMap;
	std::shared_ptr<MeshMapping> _meshMapping;

	Timing _timing;
};

class PositionTask : public FornosTask
{
public:
	PositionTask(std::unique_ptr<PositionSolver> solver, const char *outputPath);
	~PositionTask();

	bool runStep();
	void finish();
	float progress() const;
	const char* name() const { return "Position"; }

private:
	std::unique_ptr<PositionSolver> _solver;
	std::string _outputPath;
};
