#pragma once

#include <glad/glad.h>
#include "compute.h"
#include <memory>

struct CompressedMapUV;
class Mesh;
class BVH;

class NormalsSolver
{
public:
	struct Params
	{
		float rayOffset;
		bool rayInwards;
		bool tangentSpace;
	};

	NormalsSolver(const Params &params) : _params(params) {}
	const Params& params() const { return _params; }

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

	GLuint _normalsProgram;
	GLuint _raysBO;
	GLuint _positionsBO;
	GLuint _normalsBO;
	GLuint _bvhBO;
	GLuint _resultsBO;

	std::shared_ptr<const CompressedMapUV> _uvMap;
};

class NormalsTask : public FornosTask
{
public:
	NormalsTask(std::unique_ptr<NormalsSolver> solver, const char *outputPath);
	~NormalsTask();

	bool runStep();
	void finish();
	float progress() const;
	const char* name() const { return "Normals"; }

private:
	std::unique_ptr<NormalsSolver> _solver;
	std::string _outputPath;
};
