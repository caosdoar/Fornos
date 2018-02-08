#include "meshmapping.h"
#include "bvh.h"
#include "mesh.h"
#include <cassert>

static const size_t k_groupSize = 32;
static const size_t k_workPerFrame = 16384;

namespace
{
	std::vector<Pix_GPUData> computePixels(const CompressedMapUV *map)
	{
		const size_t count = map->positions.size();
		std::vector<Pix_GPUData> pixels(count);
		for (size_t i = 0; i < count; ++i)
		{
			auto &pix = pixels[i];
			pix.p = map->positions[i];
			pix.n = map->normals[i];
		}
		return pixels;
	}

	std::vector<PixT_GPUData> computePixelsT(const CompressedMapUV *map)
	{
		const size_t count = map->positions.size();
		std::vector<PixT_GPUData> pixels(count);
		for (size_t i = 0; i < count; ++i)
		{
			auto &pix = pixels[i];
			pix.t = map->tangents[i];
			pix.b = map->bitangents[i];
		}
		return pixels;
	}

	void fillMeshData(
		const Mesh *mesh,
		const BVH& bvh,
		std::vector<BVHGPUData> &bvhs,
		std::vector<Vector4> &positions,
		std::vector<Vector4> &normals)
	{
		if (bvh.children.empty() &&
			bvh.triangles.empty())
		{
			// Children node without triangles? Skip it
			return;
		}

#if 0
		if (!bvh.children.empty())
		{
			// If one of the children does not contain any triangles
			// we can skip this node completely as it is an extry AABB test
			// for nothing
			if (bvh.children[0].subtreeTriangleCount > 0 && bvh.children[1].subtreeTriangleCount == 0)
			{
				fillMeshData(mesh, bvh.children[0], bvhs, positions, normals);
				return;
			}
			if (bvh.children[1].subtreeTriangleCount > 0 && bvh.children[0].subtreeTriangleCount == 0)
			{
				fillMeshData(mesh, bvh.children[1], bvhs, positions, normals);
				return;
			}
		}
#endif

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

		std::vector<const char *> shaders;

		for (auto path : paths)
		{
			std::ifstream ifs(path);
			std::string src((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			char *shader = new char[src.size() + 1];
			strcpy_s(shader, src.size() + 1, src.c_str());
			shaders.emplace_back(shader);
		}

		glShaderSource(shader, 1, &shaders[0], nullptr);
		glCompileShader(shader);

		for (auto s : shaders) delete[] s;

#if 0
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
	// Pixels data
	{
		auto pixels = computePixels(map.get());
		_pixels = std::unique_ptr<ComputeBuffer<Pix_GPUData> >(
			new ComputeBuffer<Pix_GPUData>(&pixels[0], pixels.size(), GL_STATIC_DRAW));

		// Compute tangent data
		if (map->tangents.size() > 0)
		{
			auto pixelst = computePixelsT(map.get());
			_pixelst = std::unique_ptr<ComputeBuffer<PixT_GPUData> >(
				new ComputeBuffer<PixT_GPUData>(&pixelst[0], pixelst.size(), GL_STATIC_DRAW));
		}
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
			"D:\\Code\\Fornos\\Fornos\\shaders\\meshmapping_.comp"
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

	if (_workOffset == 0) _timing.begin();

	glUseProgram(_program);

	glUniform1ui(1, (GLuint)_workOffset);
	glUniform1ui(2, (GLuint)_coords->size());
	glUniform1ui(3, (GLuint)_bvh->size());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _pixels->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, _meshPositions->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, _bvh->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, _coords->bo());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, _tidx->bo());

	glDispatchCompute((GLuint)(work / k_groupSize), 1, 1);

	_workOffset += work;

	if (_workOffset == _workCount)
	{
		_timing.end();
		std::cout << "MeshMapping took " << _timing.elapsedSeconds() << " seconds." << std::endl;
	}

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
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

float MeshMappingTask::progress() const
{
	assert(_meshMapping);
	return _meshMapping->progress();
}
