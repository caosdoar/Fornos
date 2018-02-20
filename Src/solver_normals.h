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

#include <glad/glad.h>
#include "compute.h"
#include "fornos.h"
#include "timing.h"
#include <memory>

struct CompressedMapUV;
class MeshMapping;

class NormalsSolver
{
public:
	struct Params
	{
		bool tangentSpace;
	};

	NormalsSolver(const Params &params) : _params(params) {}
	const Params& params() const { return _params; }

	void init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<MeshMapping> mesh);
	bool runStep();
	float* getResults();

	inline float progress() const { return (float)_workOffset / (float)_workCount; }

	inline std::shared_ptr<const CompressedMapUV> uvMap() const { return _uvMap; }

private:
	Params _params;
	size_t _workOffset;
	size_t _workCount;

	GLuint _normalsProgram;
	GLuint _tanspaceProgram;
	std::unique_ptr<ComputeBuffer<float> > _resultsCB;

	std::shared_ptr<const CompressedMapUV> _uvMap;
	std::shared_ptr<MeshMapping> _meshMapping;

	Timing _timing;
};

class NormalsTask : public FornosTask
{
public:
	NormalsTask(std::unique_ptr<NormalsSolver> solver, const char *outputPath, int dilation = 0);
	~NormalsTask();

	bool runStep();
	void finish();
	float progress() const;
	const char* name() const { return "Normals"; }

private:
	std::unique_ptr<NormalsSolver> _solver;
	std::string _outputPath;
	int _dilation;
};
