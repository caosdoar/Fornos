/*
Copyright 2018 Oscar Sebio Cajaraville

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#pragma once

#include <string>
#include <vector>

class FornosTask;

//
// Application parameters
//

enum NormalImport { Import = 0, ComputePerFace = 1, ComputePerVertex = 2 };
enum MeshMappingMethod { Smooth = 0, LowPolyNormals = 1, Hybrid = 2 };

struct FornosParameters_Shared
{
	std::string loPolyMeshPath;
	std::string hiPolyMeshPath;
	NormalImport loPolyMeshNormal = NormalImport::Import;
	NormalImport hiPolyMeshNormal = NormalImport::Import;
	int bvhTrisPerNode = 8;
	int texWidth = 2048;
	int texHeight = 2048;
	int texDilation = 16;
	bool ignoreBackfaces = true;
	MeshMappingMethod mapping = MeshMappingMethod::Smooth;
	float mappingEdge = 0.05f;
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
	void run();
	const FornosTask* currentTask() const { return _tasks.empty() ? nullptr : _tasks.back(); }

private:
	std::vector<FornosTask*> _tasks;
};