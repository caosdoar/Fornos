#pragma once

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <cassert>
#include <iostream>
#include <vector>
#include "math.h"

class Mesh;

inline GLuint CreateComputeProgram(const char *path)
{
	GLuint shader = glCreateShader(GL_COMPUTE_SHADER);

	std::ifstream ifs(path);
	std::string src((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

	const char *src_str = src.c_str();
	glShaderSource(shader, 1, &src_str, nullptr);
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

template <typename T>
class ComputeBuffer
{
public:
	ComputeBuffer(const T &data, GLenum usage)
	{
		_size = 1;
		glGenBuffers(1, &_bo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T), &data, usage);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}

	ComputeBuffer(const T *data, size_t count, GLenum usage)
	{
		_size = count;
		glGenBuffers(1, &_bo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T) * count, data, usage);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}

	ComputeBuffer(const size_t count, GLenum usage)
	{
		_size = count;
		glGenBuffers(1, &_bo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T) * count, nullptr, usage);
		GLuint zero = 0;
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8UI, GL_RED, GL_UNSIGNED_BYTE, &zero);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}

	~ComputeBuffer()
	{
		glDeleteBuffers(1, &_bo);
	}

	GLuint bo() const { return _bo; }

	size_t size() const { return _size; }

	T* readData() const
	{
		T *data = new T[_size];
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bo);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(T) * _size, data);
		return data;
	}

private:
	GLuint _bo;
	size_t _size;
};

template <typename T> inline GLuint CreateComputeBuffer(const T &data, GLenum usage)
{
	GLuint bo;
	glGenBuffers(1, &bo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T), &data, usage);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	return bo;
}

template <typename T> GLuint CreateComputeBuffer(const T *data, size_t count, GLenum usage)
{
	GLuint bo;
	glGenBuffers(1, &bo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T) * count, data, usage);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	return bo;
}

inline GLuint CreateComputeBuffer(size_t size, GLenum usage)
{
	GLuint bo;
	glGenBuffers(1, &bo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, usage);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	return bo;
}

struct MapUV
{
	std::vector<Vector3> positions;
	std::vector<Vector3> normals;
	std::vector<Vector3> tangents;
	std::vector<Vector3> bitangents;

	const uint32_t width;
	const uint32_t height;

	MapUV(uint32_t w, uint32_t h)
		: width(w)
		, height(h)
		, positions(w * h, Vector3())
		, normals(w * h, Vector3())
	{
	}

	static MapUV* fromMesh(const Mesh *mesh, uint32_t width, uint32_t height);
};

// MapUV without any pixels with no data
struct CompressedMapUV
{
	std::vector<Vector3> positions;
	std::vector<Vector3> normals;
	std::vector<Vector3> tangents;
	std::vector<Vector3> bitangents;
	std::vector<uint32_t> indices; // Actual index in the MapUV

	const uint32_t width;
	const uint32_t height;

	CompressedMapUV(const MapUV *map);
};

//Vector2 ExportFloatMap(const float *data, const size_t w, const size_t h, const char *path);

class FornosTask
{
public:
	virtual ~FornosTask() {}
	virtual bool runStep() = 0;
	virtual void finish() = 0;
	virtual float progress() const = 0;
	virtual const char* name() const = 0;
};

// Shared compute shader structures

struct RayGPUData
{
	Vector3 o;
	float _pad0;
	Vector3 d;
	float _pad1;
	RayGPUData() {}
	RayGPUData(const Ray &r) : o(r.origin), d(r.direction) {}
};

struct BVHGPUData
{
	Vector3 o; float _pad0;
	Vector3 s; float _pad1;
	uint32_t start, end;
	uint32_t left, right;
	BVHGPUData() : o(), s(), start(0), end(0), left(-1), right(-1) {}
};
