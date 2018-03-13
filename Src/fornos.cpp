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

#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

#include "fornos.h"
#include "fornosui.h"
#include "bvh.h"
#include "compute.h"
#include "mesh.h"
#include "timing.h"
#include "meshmapping.h"

#include "solver_ao.h"
#include "solver_bentnormals.h"
#include "solver_height.h"
#include "solver_position.h"
#include "solver_normals.h"
#include "solver_thickness.h"

static int windowWidth = 640;
static int windowHeight = 480;


static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error %d: %s\n", error, description);
}

bool FornosRunner::start(const FornosParameters &params, std::string &errors)
{
	// TODO: Several of this steps can take long and they will freeze the UI

	std::shared_ptr<Mesh> lowPolyMesh(Mesh::loadFile(params.shared.loPolyMeshPath.c_str()));
	if (lowPolyMesh)
	{
		switch (params.shared.loPolyMeshNormal)
		{
		case NormalImport::Import: break;
		case NormalImport::ComputePerFace: lowPolyMesh->computeFaceNormals(); break;
		case NormalImport::ComputePerVertex: lowPolyMesh->computeVertexNormals(); break;
		}
	}
	else
	{
		errors = "Missing low poly mesh";
		return false;
	}

	std::shared_ptr<Mesh> hiPolyMesh = 
		params.shared.hiPolyMeshPath.empty() ?
		lowPolyMesh : 
		std::shared_ptr<Mesh>(Mesh::loadFile(params.shared.hiPolyMeshPath.c_str()));
	if (hiPolyMesh && hiPolyMesh != lowPolyMesh)
	{
		switch (params.shared.hiPolyMeshNormal)
		{
		case NormalImport::Import: break;
		case NormalImport::ComputePerFace: hiPolyMesh->computeFaceNormals(); break;
		case NormalImport::ComputePerVertex: hiPolyMesh->computeVertexNormals(); break;
		}
	}

	const bool needsTangentSpace = 
		params.normals.enabled && params.normals.tangentSpace||
		params.bentNormals.enabled && params.bentNormals.tangentSpace;

	if (needsTangentSpace)
	{
		lowPolyMesh->computeTangentSpace();
	}

	std::shared_ptr<MapUV> map;
	
	switch (params.shared.mapping)
	{
	case MeshMappingMethod::Smooth:
	{
		std::shared_ptr<Mesh> lowPolyMeshForMapping;
		if (params.shared.loPolyMeshNormal != NormalImport::ComputePerVertex)
		{
			lowPolyMeshForMapping = std::shared_ptr<Mesh>(Mesh::createCopy(lowPolyMesh.get()));
			lowPolyMeshForMapping->computeVertexNormalsAggressive();
		}
		else
		{
			lowPolyMeshForMapping = lowPolyMesh;
		}

		map = std::shared_ptr<MapUV>(MapUV::fromMeshes(
			lowPolyMesh.get(),
			lowPolyMeshForMapping.get(),
			params.shared.texWidth,
			params.shared.texHeight));
	} break;

	case MeshMappingMethod::LowPolyNormals:
	{
		map = std::shared_ptr<MapUV>(MapUV::fromMesh(
			lowPolyMesh.get(),
			params.shared.texWidth,
			params.shared.texHeight));
	} break;

	case MeshMappingMethod::Hybrid:
	{
		std::shared_ptr<Mesh> lowPolyMeshForMapping;
		if (params.shared.loPolyMeshNormal != NormalImport::ComputePerVertex)
		{
			lowPolyMeshForMapping = std::shared_ptr<Mesh>(Mesh::createCopy(lowPolyMesh.get()));
			lowPolyMeshForMapping->computeVertexNormalsAggressive();
		}
		else
		{
			lowPolyMeshForMapping = lowPolyMesh;
		}

		map = std::shared_ptr<MapUV>(MapUV::fromMeshes_Hybrid(
			lowPolyMesh.get(),
			lowPolyMeshForMapping.get(),
			params.shared.texWidth,
			params.shared.texHeight,
			params.shared.mappingEdge));
	} break;
	}
	
	if (!map)
	{
		errors = "Low poly mesh is missing texture coordinates or normals information";
		return false;
	}
	std::shared_ptr<CompressedMapUV> compressedMap(new CompressedMapUV(map.get()));

	std::shared_ptr<BVH> rootBVH(BVH::createBinary(hiPolyMesh.get(), params.shared.bvhTrisPerNode, 8192));

	std::shared_ptr<MeshMapping> meshMapping(new MeshMapping());
	meshMapping->init(compressedMap, hiPolyMesh, rootBVH, params.shared.ignoreBackfaces);

	if (params.thickness.enabled)
	{
		ThicknessSolver::Params solverParams;
		solverParams.sampleCount = (uint32_t)params.thickness.sampleCount;
		solverParams.minDistance = params.thickness.minDistance;
		solverParams.maxDistance = params.thickness.maxDistance;
		std::unique_ptr<ThicknessSolver> solver(new ThicknessSolver(solverParams));
		solver->init(compressedMap, meshMapping);
		_tasks.emplace_back(
			new ThicknessTask(std::move(solver), params.thickness.outputPath.c_str(), params.shared.texDilation)
		);
	}

	if (params.bentNormals.enabled)
	{
		BentNormalsSolver::Params solverParams;
		solverParams.sampleCount = (uint32_t)params.bentNormals.sampleCount;
		solverParams.minDistance = params.bentNormals.minDistance;
		solverParams.maxDistance = params.bentNormals.maxDistance;
		solverParams.tangentSpace = params.bentNormals.tangentSpace;
		std::unique_ptr<BentNormalsSolver> solver(new BentNormalsSolver(solverParams));
		solver->init(compressedMap, meshMapping);
		_tasks.emplace_back(
			new BentNormalsTask(std::move(solver), params.bentNormals.outputPath.c_str(), params.shared.texDilation)
		);
	}

	if (params.ao.enabled)
	{
		AmbientOcclusionSolver::Params solverParams;
		solverParams.sampleCount = (uint32_t)params.ao.sampleCount;
		solverParams.minDistance = params.ao.minDistance;
		solverParams.maxDistance = params.ao.maxDistance;
		std::unique_ptr<AmbientOcclusionSolver> solver(new AmbientOcclusionSolver(solverParams));
		solver->init(compressedMap, meshMapping);
		_tasks.emplace_back(
			new AmbientOcclusionTask(std::move(solver), params.ao.outputPath.c_str(), params.shared.texDilation)
		);
	}

	if (params.normals.enabled)
	{
		NormalsSolver::Params solverParams;
		solverParams.tangentSpace = params.normals.tangentSpace;
		std::unique_ptr<NormalsSolver> normalsSolver(new NormalsSolver(solverParams));
		normalsSolver->init(compressedMap, meshMapping);
		_tasks.emplace_back(
			new NormalsTask(std::move(normalsSolver), params.normals.outputPath.c_str(), params.shared.texDilation)
		);
	}

	if (params.positions.enabled)
	{
		std::unique_ptr<PositionSolver> solver(new PositionSolver());
		solver->init(compressedMap, meshMapping);
		_tasks.emplace_back(
			new PositionTask(std::move(solver), params.positions.outputPath.c_str())
		);
	}

	if (params.height.enabled)
	{
		std::unique_ptr<HeightSolver> solver(new HeightSolver());
		solver->init(compressedMap, meshMapping);
		_tasks.emplace_back(
			new HeightTask(std::move(solver), params.height.outputPath.c_str(), params.shared.texDilation)
		);
	}

	_tasks.emplace_back(new MeshMappingTask(meshMapping));

	return true;
}

void FornosRunner::run()
{
	if (!_tasks.empty())
	{
		auto task = _tasks.back();
		if (task->runStep())
		{
			task->finish(); // Exports map or any other after-compute work
			delete task;
			_tasks.pop_back();
		}
	}
}

static void APIENTRY openglCallbackFunction(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam
) 
{
	if (severity == GL_DEBUG_SEVERITY_HIGH)
	{
		fprintf(stderr, "%s\n", message);
	}
}

#if defined(_WIN32) && !defined(_CONSOLE)
int CALLBACK WinMain
(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
#else
int main(int argc, char *argv[])
#endif
{
	// Setup window
	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) return 1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Fornos: Texture Baking", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSwapInterval(1);

#if 1
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openglCallbackFunction, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);
#endif

	FornosRunner runner;
	FornosUI ui;
	ui.init(&runner, window);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwGetWindowSize(window, &windowWidth, &windowHeight);
		glfwPollEvents();

		if (runner.pending())
		{
			glfwSwapInterval(0);
			runner.run();
		}
		else
		{
			glfwSwapInterval(1);
		}
		
		ui.process(windowWidth, windowHeight);

		// Rendering
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		ui.render();
		glfwSwapBuffers(window);
	}

	// Cleanup
	ui.shutdown();
	glfwTerminate();

	return 0;
}
