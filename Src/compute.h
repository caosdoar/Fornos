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
#include <string>
#include <cassert>
#include <vector>
#include "math.h"

class Mesh;

GLuint CreateComputeProgram(const char *path);

GLuint CreateComputeProgramFromMemory(const char *src);

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

/// Stores per-pixel data for the low-poly mesh
struct MapUV
{
	std::vector<Vector3> positions;
	std::vector<Vector3> directions;
	std::vector<Vector3> normals;
	std::vector<Vector3> tangents;
	std::vector<Vector3> bitangents;

	const uint32_t width;
	const uint32_t height;

	MapUV(uint32_t w, uint32_t h)
		: width(w)
		, height(h)
		, positions(w * h, Vector3())
		, directions(w * h, Vector3())
		, normals(w * h, Vector3())
	{
	}

	/// Builds a map from a mesh
	/// @param mesh Mesh
	/// @param width Map width
	/// @param height Map height
	static MapUV* fromMesh(const Mesh *mesh, uint32_t width, uint32_t height);
	static MapUV* fromMeshes(const Mesh *mesh, const Mesh *meshDirs, uint32_t width, uint32_t height);
	static MapUV* fromMeshes_Hybrid(const Mesh *mesh, const Mesh *meshDirs, uint32_t width, uint32_t height, float edge);
};

/// MapUV without any pixels with no data
/// This is for a more efficient processing in the GPU
struct CompressedMapUV
{
	std::vector<Vector3> positions;
	std::vector<Vector3> directions;
	std::vector<Vector3> normals;
	std::vector<Vector3> tangents;
	std::vector<Vector3> bitangents;
	std::vector<uint32_t> indices; // Actual index in the MapUV

	const uint32_t width;
	const uint32_t height;

	/// Creates a compressed map from a raw map
	CompressedMapUV(const MapUV *map);
};
