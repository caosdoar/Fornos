#pragma once

#include <string>
#include <vector>

class FornosTask;

//
// Application parameters
//

enum NormalImport { Import = 0, ComputePerFace = 1, ComputePerVertex = 2 };

struct FornosParameters_Shared
{
	std::string loPolyMeshPath;
	std::string hiPolyMeshPath;
	NormalImport loPolyMeshNormal = NormalImport::Import;
	NormalImport hiPolyMeshNormal = NormalImport::Import;
	int bvhTrisPerNode = 8;
	int texWidth = 2048;
	int texHeight = 2048;
};

struct FornosParameters_SolverHeight
{
	bool enabled = false;
	std::string outputPath;

	bool ready() { return enabled && !outputPath.empty(); }
};

struct FornosParameters_SolverPositions
{
	bool enabled = false;
	std::string outputPath;

	bool ready() { return enabled && !outputPath.empty(); }
};

struct FornosParameters_SolverNormals
{
	bool enabled = false;
	bool tangentSpace = true;
	std::string outputPath;

	bool ready() { return enabled && !outputPath.empty(); }
};

struct FornosParameters_SolverAO
{
	bool enabled = false;
	int sampleCount = 256;
	float minDistance = 0.01f;
	float maxDistance = 10.0f;
	std::string outputPath;

	bool ready() { return enabled && !outputPath.empty(); }
};

struct FornosParameters_SolverBentNormals
{
	bool enabled = false;
	int sampleCount = 256;
	float minDistance = 0.01f;
	float maxDistance = 10.0f;
	bool tangentSpace = true;
	std::string outputPath;

	bool ready() { return enabled && !outputPath.empty(); }
};

struct FornosParameters_SolverThickness
{
	bool enabled = false;
	int sampleCount = 256;
	float minDistance = 0.01f;
	float maxDistance = 10.0f;
	std::string outputPath;

	bool ready() { return enabled && !outputPath.empty(); }
};

struct FornosParameters
{
	FornosParameters_Shared shared;
	FornosParameters_SolverHeight height;
	FornosParameters_SolverPositions positions;
	FornosParameters_SolverNormals normals;
	FornosParameters_SolverAO ao;
	FornosParameters_SolverBentNormals bentNormals;
	FornosParameters_SolverThickness thickness;
};

//
// Application tasks and execution
//

class FornosTask
{
public:
	virtual ~FornosTask() {}
	virtual bool runStep() = 0;
	virtual void finish() = 0;
	virtual float progress() const = 0;
	virtual const char* name() const = 0;
};

class FornosRunner
{
public:
	bool start(const FornosParameters &params, std::string &errors);
	bool pending() const { return !_tasks.empty(); }
	FornosTask* currentTask() const { return _tasks.empty() ? nullptr : _tasks.back(); }
	void moveToNextTask() { _tasks.pop_back(); }

private:
	std::vector<FornosTask*> _tasks;
};