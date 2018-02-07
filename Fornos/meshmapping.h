#pragma once

#include <glad/glad.h>
#include "compute.h"
#include "math.h"
#include <cstdint>
#include <memory>

struct CompressedMapUV;
class Mesh;
class BVH;

class MeshMapping
{
public:
	void init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<const Mesh> mesh, std::shared_ptr<const BVH> rootBVH);
	bool runStep();

	inline float progress() const { return (float)_workOffset / (float)_workCount; }

private:
	size_t _workOffset;
	size_t _workCount;

	std::unique_ptr<ComputeBuffer<Vector4> > _coords;
	std::unique_ptr<ComputeBuffer<uint32_t> > _tidx;
	std::unique_ptr<ComputeBuffer<RayGPUData> > _rays;
	std::unique_ptr<ComputeBuffer<Vector4> > _meshPositions;
	std::unique_ptr<ComputeBuffer<Vector4> > _meshNormals;
	std::unique_ptr<ComputeBuffer<BVHGPUData> > _bvh;
	GLuint _program;
};

class MeshMappingTask : public FornosTask
{
public:
	MeshMappingTask(std::shared_ptr<MeshMapping> meshmapping);
	~MeshMappingTask();

	bool runStep();
	void finish();
	float progress() const;
	const char* name() const { return "Mesh mapping"; }

private:
	std::shared_ptr<MeshMapping> _meshMapping;
};