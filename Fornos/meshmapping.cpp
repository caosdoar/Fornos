#include "meshmapping.h"
#include "bvh.h"
#include "mesh.h"
#include <cassert>

static const size_t k_groupSize = 32;
static const size_t k_workPerFrame = 8192;

namespace
{
	std::vector<Ray> computeRays(const CompressedMapUV *map)
	{
		const size_t count = map->positions.size();
		std::vector<Ray> rays(count);
		for (size_t i = 0; i < count; ++i)
		{
			const Vector3 n = map->normals[i];
			const Vector3 o = map->positions[i];
		}

		return rays;
	}

	void fillMeshData(
		const Mesh *mesh,
		const BVH& bvh,
		std::vector<BVHGPUData> &bvhs,
		std::vector<Vector4> &positions,
		std::vector<Vector4> &normals)
	{
		bvhs.emplace_back(BVHGPUData());
		BVHGPUData &d = bvhs.back();
		d.o = bvh.aabb.center;
		d.s = bvh.aabb.size;
		d.start = (uint32_t)positions.size();
		for (uint32_t tidx : bvh.triangles)
		{
			const auto &tri = mesh->triangles[tidx];
			const auto &v0 = mesh->vertices[tri.vertexIndex0];
			const auto &v1 = mesh->vertices[tri.vertexIndex1];
			const auto &v2 = mesh->vertices[tri.vertexIndex2];
			positions.push_back(mesh->positions[v0.positionIndex]);
			positions.push_back(mesh->positions[v1.positionIndex]);
			positions.push_back(mesh->positions[v2.positionIndex]);
			normals.push_back(mesh->normals[v0.normalIndex]);
			normals.push_back(mesh->normals[v1.normalIndex]);
			normals.push_back(mesh->normals[v2.normalIndex]);
		}
		d.end = (uint32_t)positions.size();

		const size_t index = bvhs.size() - 1; // Because d gets invalidated by fillMeshData!
		if (bvh.children.size() > 0)
		{
			fillMeshData(mesh, bvh.children[0], bvhs, positions, normals);
			fillMeshData(mesh, bvh.children[1], bvhs, positions, normals);
		}
		bvhs[index].right = (uint32_t)bvhs.size();
	}

	GLuint CreateComputeProgram(const std::vector<const char *> &paths)
	{
		GLuint shader = glCreateShader(GL_COMPUTE_SHADER);

		std::vector<std::string> shaders;

		for (auto path : paths)
		{
			std::ifstream ifs(path);
			std::string src((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			shaders.emplace_back(src);
		}

		std::vector<const char *> strs;
		for (auto s : shaders) strs.emplace_back(s.c_str());

		glShaderSource(shader, 1, &strs[0], nullptr);
		glCompileShader(shader);

#if 1
		{
			GLint compiled;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
			if (compiled != GL_TRUE)
			{
				const size_t buffSize = 2048;
				char *buff = new char[buffSize];
				GLsizei length;
				glGetShaderInfoLog(shader, (GLsizei)buffSize, &length, buff);
				std::cerr << buff << std::endl;
				assert(false);
			}
		}
#endif

		GLuint program = glCreateProgram();
		glAttachShader(program, shader);
		glLinkProgram(program);

		return program;
	}
}

void MeshMapping::init
(
	std::shared_ptr<const CompressedMapUV> map,
	std::shared_ptr<const Mesh> mesh,
	std::shared_ptr<const BVH> rootBVH
)
{
	// Rays data
	{
		auto rays = computeRays(map.get());
		std::vector<RayGPUData> raysData(rays.begin(), rays.end());
		_rays = std::unique_ptr<ComputeBuffer<RayGPUData> >(
			new ComputeBuffer<RayGPUData>(&raysData[0], raysData.size(), GL_STATIC_DRAW));
	}

	// Mesh data
	{
		std::vector<BVHGPUData> bvhs;
		std::vector<Vector4> positions;
		std::vector<Vector4> normals;
		fillMeshData(mesh.get(), *rootBVH, bvhs, positions, normals);
		_meshPositions = std::unique_ptr<ComputeBuffer<Vector4> >(
			new ComputeBuffer<Vector4>(&positions[0], positions.size(), GL_STATIC_DRAW));
		_meshNormals = std::unique_ptr<ComputeBuffer<Vector4> >(
			new ComputeBuffer<Vector4>(&normals[0], normals.size(), GL_STATIC_DRAW));
		_bvh = std::unique_ptr<ComputeBuffer<BVHGPUData> >(
			new ComputeBuffer<BVHGPUData>(&bvhs[0], bvhs.size(), GL_STATIC_DRAW));
	}

	_workCount = ((map->positions.size() + k_groupSize - 1) / k_groupSize) * k_groupSize;

	// Results data
	{
		_coords = std::unique_ptr<ComputeBuffer<Vector4> >(
			new ComputeBuffer<Vector4>(_workCount, GL_STATIC_DRAW));
		_tidx = std::unique_ptr<ComputeBuffer<uint32_t> >(
			new ComputeBuffer<uint32_t>(_workCount, GL_STATIC_DRAW));
	}

	// Shader
	{
		_program = CreateComputeProgram
		({
			"D:\\Code\\Fornos\\Fornos\\shaders\\raycast.comp",
			"D:\\Code\\Fornos\\Fornos\\shaders\\meshmapping.comp",
		});
	}

	_workOffset = 0;
}

bool MeshMapping::runStep()
{
	assert(_workOffset < _workCount);
	const size_t workLeft = _workCount - _workOffset;
	const size_t work = workLeft < k_workPerFrame ? workLeft : k_workPerFrame;
	assert(work % k_groupSize == 0);

	glUseProgram(_program);

	glUniform1ui(1, (GLuint)_workOffset);
	glUniform1ui(2, (GLuint)_bvh->size());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _rays->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, _meshPositions->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, _meshNormals->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, _coords->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, _tidx->bo());

	glDispatchCompute((GLuint)(work / k_groupSize), 1, 1);

	_workOffset += work;
	return _workOffset >= _workCount;
}

MeshMappingTask::MeshMappingTask(std::shared_ptr<MeshMapping> meshmapping)
	: _meshMapping(meshmapping)
{
}

MeshMappingTask::~MeshMappingTask()
{
}

bool MeshMappingTask::runStep()
{
	assert(_meshMapping);
	return _meshMapping->runStep();
}

void MeshMappingTask::finish()
{
}

float MeshMappingTask::progress() const
{
	assert(_meshMapping);
	return _meshMapping->progress();
}
