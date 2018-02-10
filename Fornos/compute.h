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
			//assert(false);
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
	}

	ComputeBuffer(const T *data, size_t count, GLenum usage)
	{
		_size = count;
		glGenBuffers(1, &_bo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T) * count, data, usage);
	}

	ComputeBuffer(const size_t count, GLenum usage)
	{
		_size = count;
		glGenBuffers(1, &_bo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _bo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T) * count, nullptr, usage);
		GLuint zero = 0;
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8UI, GL_RED, GL_UNSIGNED_BYTE, &zero);
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

/*template <typename T> inline GLuint CreateComputeBuffer(const T &data, GLenum usage)
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
}*/

class ComputeTexture_Float
{
public:
	ComputeTexture_Float(size_t w, size_t h, bool halfPrecision = false) 
		: _w(w), _h(h), _hp(halfPrecision)
	{
		glGenTextures(1, &_tex);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, _hp ? GL_R16F : GL_R32F, GLsizei(_w), GLsizei(_h), 0, GL_RED, GL_FLOAT, NULL);
		const float zero = 0.0f;
		glClearTexImage(_tex, 0, GL_RED, GL_FLOAT, &zero);
	}

	~ComputeTexture_Float() { glDeleteTextures(1, &_tex); }

	size_t width() const { return _w; }
	size_t height() const { return _h; }

	void bind(GLuint unit, GLuint access)
	{
		glBindImageTexture(unit, _tex, 0, GL_FALSE, 0, access, _hp ? GL_R16F : GL_R32F);
	}

	float* readData() const
	{
		float *data = new float[_w * _h];
		glGetTextureImage(_tex, 0, GL_RED, GL_FLOAT, GLsizei(sizeof(float) * _w * _h), data);
		return data;
	}

private:
	GLuint _tex;
	size_t _w;
	size_t _h;
	bool _hp;
};

class ComputeTexture_Uint32
{
public:
	ComputeTexture_Uint32(size_t w, size_t h)
		: _w(w), _h(h)
	{
		glGenTextures(1, &_tex);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, GLsizei(_w), GLsizei(_h), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		const uint32_t zero = 0;
		glClearTexImage(_tex, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
	}

	~ComputeTexture_Uint32() { glDeleteTextures(1, &_tex); }

	size_t width() const { return _w; }
	size_t height() const { return _h; }

	void bind(GLuint unit, GLuint access)
	{
		glBindImageTexture(unit, _tex, 0, GL_FALSE, 0, access, GL_R32UI);
	}

	uint32_t* readData() const
	{
		uint32_t *data = new uint32_t[_w * _h];
		glGetTextureImage(_tex, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, GLsizei(sizeof(uint32_t) * _w * _h), data);
		return data;
	}

private:
	GLuint _tex;
	size_t _w;
	size_t _h;
};

class ComputeTexture_Uint16
{
public:
	ComputeTexture_Uint16(size_t w, size_t h)
		: _w(w), _h(h)
	{
		glGenTextures(1, &_tex);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, GLsizei(_w), GLsizei(_h), 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, NULL);
		const uint32_t zero = 0;
		glClearTexImage(_tex, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, &zero);
	}

	~ComputeTexture_Uint16() { glDeleteTextures(1, &_tex); }

	size_t width() const { return _w; }
	size_t height() const { return _h; }

	void bind(GLuint unit, GLuint access)
	{
		glBindImageTexture(unit, _tex, 0, GL_FALSE, 0, access, GL_R16UI);
	}

	uint16_t* readData() const
	{
		uint16_t *data = new uint16_t[_w * _h];
		glGetTextureImage(_tex, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, GLsizei(sizeof(uint16_t) * _w * _h), data);
		return data;
	}

private:
	GLuint _tex;
	size_t _w;
	size_t _h;
};

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
	Vector3 aabbMin;
	Vector3 aabbMax;
	uint32_t start, end;
	uint32_t jump;
	BVHGPUData() : aabbMin(), aabbMax(), start(0), end(0), jump(0) {}
};
