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

#include "compute.h"
#include "fornos.h"
#include "math.h"
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

	inline float progress() const
	{
		return (float)(_workOffset) / (float)(_workCount * _params.sampleCount);
	}

	inline std::shared_ptr<const CompressedMapUV> uvMap() const { return _uvMap; }

private:
	Params _params;
	size_t _workOffset;
	size_t _workCount;

	size_t _sampleIndex;

	struct ShaderParams
	{
		uint32_t sampleCount;
		uint32_t samplePermCount;
		float minDistance;
		float maxDistance;
	};

	struct RayData
	{
		Vector3 o; float _pad0;
		Vector3 d; float _pad1;
		Vector3 tx; float _pad2;
		Vector3 ty; float _pad3;
	};

	GLuint _rayProgram;
	GLuint _thicknessProgram;
	GLuint _avgProgram;
	std::unique_ptr<ComputeBuffer<ShaderParams> > _paramsCB;
	std::unique_ptr<ComputeBuffer<Vector4> > _samplesCB;
	std::unique_ptr<ComputeBuffer<RayData> > _rayDataCB;
	std::unique_ptr<ComputeBuffer<float> > _resultsMiddleCB;
	std::unique_ptr<ComputeBuffer<float> > _resultsFinalCB;

	std::shared_ptr<const CompressedMapUV> _uvMap;
	std::shared_ptr<MeshMapping> _meshMapping;

	Timing _timing;
};

class ThicknessTask : public FornosTask
{
public:
	ThicknessTask(std::unique_ptr<ThicknessSolver> solver, const char *outputPath, int dilation = 0);
	~ThicknessTask();

	bool runStep();
	void finish();
	float progress() const;
	const char* name() const { return "Thickness"; }

private:
	std::unique_ptr<ThicknessSolver> _solver;
	std::string _outputPath;
	int _dilation;
};