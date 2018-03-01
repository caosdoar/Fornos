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

#include "solver_normals.h"
#include "bvh.h"
#include "compute.h"
#include "computeshaders.h"
#include "image.h"
#include "logging.h"
#include "math.h"
#include "mesh.h"
#include "meshmapping.h"
#include <cassert>

static const size_t k_groupSize = 64;
static const size_t k_workPerFrame = 1024 * 128;

void NormalsSolver::init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<MeshMapping> meshMapping)
{
	_normalsProgram = LoadComputeShader_Normal();
	_tanspaceProgram = LoadComputeShader_ToTangentSpace();
	_uvMap = map;
	_meshMapping = meshMapping;
	_workCount = ((map->positions.size() + k_groupSize - 1) / k_groupSize) * k_groupSize;
	_resultsCB = std::unique_ptr<ComputeBuffer<float> >(
		new ComputeBuffer<float>(3 * _workCount, GL_STATIC_READ));
	_workOffset = 0;
}

bool NormalsSolver::runStep()
{
	assert(_workOffset < _workCount);
	const size_t workLeft = _workCount - _workOffset;
	const size_t work = workLeft < k_workPerFrame ? workLeft : k_workPerFrame;
	assert(work % k_groupSize == 0);

	if (_workOffset == 0) _timing.begin();

	glUseProgram(_normalsProgram);
	glUniform1ui(1, (GLuint)_workOffset);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _meshMapping->meshNormals()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _meshMapping->coords()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _meshMapping->coords_tidx()->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, _resultsCB->bo());
	glDispatchCompute((GLuint)(work / k_groupSize), 1, 1);

	if (_params.tangentSpace)
	{
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glUseProgram(_tanspaceProgram);
		glUniform1ui(1, GLuint(_workOffset));
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _meshMapping->pixelst()->bo());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _resultsCB->bo());
		glDispatchCompute((GLuint)(work / k_groupSize), 1, 1);
	}

	_workOffset += work;

	if (_workOffset >= _workCount)
	{
		_timing.end();
		logDebug("Norms",
			"Normal map took " + std::to_string(_timing.elapsedSeconds()) +
			" seconds for " + std::to_string(_uvMap->width) + "x" + std::to_string(_uvMap->height));
	}

	return _workOffset >= _workCount;
}

float* NormalsSolver::getResults()
{
	assert(_workOffset == _workCount);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	float *results = new float[_workCount * 3];
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _resultsCB->bo());
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * 3 * _workCount, results);
	return results;
}

NormalsTask::NormalsTask(std::unique_ptr<NormalsSolver> solver, const char *outputPath, int dilation)
	: _solver(std::move(solver))
	, _outputPath(outputPath)
	, _dilation(dilation)
{
}

NormalsTask::~NormalsTask()
{
}

bool NormalsTask::runStep()
{
	assert(_solver);
	return _solver->runStep();
}

void NormalsTask::finish()
{
	assert(_solver);
	Vector3 *results = (Vector3*)_solver->getResults();
	auto map = _solver->uvMap();
	exportNormalImage(results, _solver->uvMap().get(), _outputPath.c_str(), _dilation);
	delete[] results;
}

float NormalsTask::progress() const
{
	assert(_solver);
	return _solver->progress();
}