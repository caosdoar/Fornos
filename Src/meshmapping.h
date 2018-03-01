#pragma once

#include <glad/glad.h>
#include "compute.h"
#include "fornos.h"
#include "math.h"
#include "timing.h"
#include <cstdint>
#include <memory>

struct CompressedMapUV;
class Mesh;
class BVH;

struct Pix_GPUData
{
	Vector3 p;
	float _pad0;
	Vector3 d;
	float _pad1;
};

struct PixT_GPUData
{
	Vector3 n;
	float _pad0;
	Vector3 t;
	float _pad1;
	Vector3 b;
	float _pad2;
};

struct BVHGPUData
{
	Vector3 aabbMin;
	Vector3 aabbMax;
	uint32_t start, end;
	uint32_t jump;
	BVHGPUData() : aabbMin(), aabbMax(), start(0), end(0), jump(0) {}
};

class MeshMapping
{
public:
	void init(std::shared_ptr<const CompressedMapUV> map, std::shared_ptr<const Mesh> mesh, std::shared_ptr<const BVH> rootBVH, bool cullBackfaces = false);
	bool runStep();

	inline float progress() const { return (float)_workOffset / (float)_workCount; }

	inline const ComputeBuffer<Vector4>* coords() const { return _coords.get(); }
	inline const ComputeBuffer<uint32_t>* coords_tidx() const { return _tidx.get(); }
	inline const ComputeBuffer<Pix_GPUData>* pixels() const { return _pixels.get(); }
	inline const ComputeBuffer<PixT_GPUData>* pixelst() const { return _pixelst.get(); }
	inline const ComputeBuffer<Vector4>* meshPositions() const { return _meshPositions.get(); }
	inline const ComputeBuffer<Vector4>* meshNormals() const { return _meshNormals.get(); }
	inline const ComputeBuffer<BVHGPUData>* meshBVH() const { return _bvh.get(); }


private:
	size_t _workOffset;
	size_t _workCount;
	bool _cullBackfaces = false;

	std::unique_ptr<ComputeBuffer<Vector4> > _coords;
	std::unique_ptr<ComputeBuffer<uint32_t> > _tidx;
	std::unique_ptr<ComputeBuffer<Pix_GPUData> > _pixels;
	std::unique_ptr<ComputeBuffer<PixT_GPUData> > _pixelst;
	std::unique_ptr<ComputeBuffer<Vector4> > _meshPositions;
	std::unique_ptr<ComputeBuffer<Vector4> > _meshNormals;
	std::unique_ptr<ComputeBuffer<BVHGPUData> > _bvh;
	GLuint _program;
	GLuint _programCullBackfaces;

	Timing _timing;
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