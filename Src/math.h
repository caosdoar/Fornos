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

#include <cmath>
#include <cstdint>
#include <vector>
#include <random>

#define PI 3.14159265
#define PI_INV 0.318309886

struct Vector2
{
	float x;
	float y;

	Vector2() : x(0), y(0) {}
	Vector2(float x, float y) : x(x), y(y) {}
	Vector2(float v) : x(v), y(v) {}

	Vector2 operator -() const { return Vector2(-x, -y); }
	Vector2 operator +(const Vector2 &o) const { return Vector2(x + o.x, y + o.y); }
	Vector2 operator -(const Vector2 &o) const { return Vector2(x - o.x, y - o.y); }
	Vector2 operator *(const float k) const { return Vector2(x * k, y * k); }
	Vector2 operator *(const Vector2 &o) const { return Vector2(x * o.x, y * o.y); }
	Vector2 operator /(const float k) const { return Vector2(x / k, y / k); }
	Vector2 operator /(const Vector2 &o) const { return Vector2(x / o.x, y / o.y); }
};

inline float dot(const Vector2 &v0, const Vector2 &v1)
{
	return v0.x * v1.x + v0.y * v1.y;
}

inline float length(const Vector2 &v)
{
	return std::sqrtf(dot(v, v));
}

inline Vector2 normalize(const Vector2 &v)
{
	return v * (1.0f / length(v));
}

struct Vector3
{
	float x;
	float y;
	float z;

	Vector3() : x(0), y(0), z(0) {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
	Vector3(float v) : x(v), y(v), z(v) {}

	Vector3 operator -() const { return Vector3(-x, -y, -z); }
	Vector3 operator +(const Vector3 &o) const { return Vector3(x + o.x, y + o.y, z + o.z); }
	Vector3& operator +=(const Vector3 &o) { x += o.x; y += o.y; z += o.z;  return *this; }
	Vector3 operator -(const Vector3 &o) const { return Vector3(x - o.x, y - o.y, z - o.z); }
	Vector3& operator -=(const Vector3 &o) { x -= o.x; y -= o.y; z -= o.z;  return *this; }
	Vector3 operator *(const float k) const { return Vector3(x * k, y * k, z * k); }
	Vector3& operator *=(const float k) { x *= k; y *= k; z *= k; return *this; }
	Vector3 operator *(const Vector3 &o) const { return Vector3(x * o.x, y * o.y, z * o.z); }
	Vector3& operator *=(const Vector3 &o) { x *= o.x; y *= o.y; z *= o.z;  return *this; }
	Vector3 operator /(const float k) const { return Vector3(x / k, y / k, z / k); }
	Vector3& operator /=(const float k) { x /= k; y /= k; z /= k; return *this; }
	Vector3 operator /(const Vector3 &o) const { return Vector3(x / o.x, y / o.y, z / o.z); }
	Vector3& operator /=(const Vector3 &o) { x /= o.x; y /= o.y; z /= o.z;  return *this; }
};

inline float dot(const Vector3 &v0, const Vector3 &v1)
{
	return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}

inline Vector3 cross(const Vector3 &v0, const Vector3 &v1)
{
	return Vector3(
		v0.y * v1.z - v0.z * v1.y,
		v0.z * v1.x - v0.x * v1.z,
		v0.x * v1.y - v0.y * v1.x);
}

inline float length(const Vector3 &v)
{
	return std::sqrtf(dot(v, v));
}

inline Vector3 normalize(const Vector3 &v)
{
	return v * (1.0f / length(v));
}

inline Vector3 min(const Vector3 &v0, const Vector3 &v1)
{
	return Vector3(
		std::fminf(v0.x, v1.x),
		std::fminf(v0.y, v1.y),
		std::fminf(v0.z, v1.z));
}

inline Vector3 max(const Vector3 &v0, const Vector3 &v1)
{
	return Vector3(
		std::fmaxf(v0.x, v1.x),
		std::fmaxf(v0.y, v1.y),
		std::fmaxf(v0.z, v1.z));
}

struct Vector4
{
	float x, y, z, w;
	Vector4() {}
	Vector4(const Vector3 &v) : x(v.x), y(v.y), z(v.z), w(0) {}
};

struct Ray
{
	Vector3 origin;
	Vector3 direction;

	Ray() {}
	Ray(const Vector3 &o, const Vector3 &d) : origin(o), direction(d) {}
};

struct AABB
{
	Vector3 center;
	Vector3 size; // Half size

	AABB() {}
	AABB(const Vector3 &center, const Vector3 &size) : center(center), size(size) {}
};

struct Triangle
{
	Vector3 a;
	Vector3 b;
	Vector3 c;

	Triangle() {}
	Triangle(const Vector3 &p0, const Vector3 &p1, const Vector3 &p2) : a(p0), b(p1), c(p2) {}
};

inline Vector2 GetInterval(const Triangle &t, const Vector3 &axis)
{
	const float a = dot(axis, t.a);
	const float b = dot(axis, t.b);
	const float c = dot(axis, t.c);
	return Vector2(
		std::fminf(a, std::fminf(b, c)),
		std::fmaxf(a, std::fmaxf(b, c)));
}

inline Vector2 GetInterval(const AABB &aabb, const Vector3 &axis)
{
	const Vector3 i = aabb.center - aabb.size;
	const Vector3 a = aabb.center + aabb.size;
	const Vector3 vertex[8] = {
		Vector3(i.x, a.y, a.z),
		Vector3(i.x, a.y, i.z),
		Vector3(i.x, i.y, a.z),
		Vector3(i.x, i.y, i.z),
		Vector3(a.x, a.y, a.z),
		Vector3(a.x, a.y, i.z),
		Vector3(a.x, i.y, a.z),
		Vector3(a.x, i.y, i.z) };

	Vector2 result(dot(axis, vertex[0]));
	for (int i = 1; i < 8; ++i)
	{
		const float v = dot(axis, vertex[i]);
		result.x = std::fminf(result.x, v);
		result.y = std::fmaxf(result.y, v);
	}
	return result;
}

inline bool OverlapOnAxis(const AABB &aabb, const Triangle &t, const Vector3 &axis)
{
	const Vector2 a = GetInterval(aabb, axis);
	const Vector2 b = GetInterval(t, axis);
	return (b.x <= a.y) && (a.x <= b.y);
}

inline bool TriangleAABB(const Triangle &t, const AABB &aabb)
{
	const Vector3 f0 = t.b - t.a;
	const Vector3 f1 = t.c - t.b;
	const Vector3 f2 = t.a - t.c;
	const Vector3 u0(1, 0, 0);
	const Vector3 u1(0, 1, 0);
	const Vector3 u2(0, 0, 1);

	Vector3 test[13] =
	{
		u0, u1, u2,
		cross(f0, f1),
		cross(u0, f0), cross(u0, f1), cross(u0, f2),
		cross(u1, f0), cross(u1, f1), cross(u1, f2),
		cross(u2, f0), cross(u2, f1), cross(u2, f2),
	};

	for (int i = 0; i < 13; ++i)
	{
		if (!OverlapOnAxis(aabb, t, test[i])) return false;
	}
	return true;
}

inline bool RayAABB(const Ray &ray, const AABB &aabb)
{
	const Vector3 mins = aabb.center - aabb.size;
	const Vector3 maxs = aabb.center + aabb.size;

	Vector3 d = ray.direction;
	if (std::fabsf(d.x) < FLT_EPSILON) d.x = 1e-5f;
	if (std::fabsf(d.y) < FLT_EPSILON) d.y = 1e-5f;
	if (std::fabsf(d.z) < FLT_EPSILON) d.z = 1e-5f;

	const Vector3 t1 = (mins - ray.origin) / d;
	const Vector3 t2 = (maxs - ray.origin) / d;
	const float tmin = std::fmaxf(std::fmaxf(std::fminf(t1.x, t2.x), std::fminf(t1.y, t2.y)), std::fminf(t1.z, t2.z));
	const float tmax = std::fminf(std::fminf(std::fmaxf(t1.x, t2.x), std::fmaxf(t1.y, t2.y)), std::fmaxf(t1.z, t2.z));

	return (tmax >= 0 && tmin <= tmax);
}

inline Vector3 Barycentric(const Vector2 p, const Vector2 p0, const Vector2 p1, const Vector2 p2)
{
	const Vector2 v0 = p1 - p0;
	const Vector2 v1 = p2 - p0;
	const Vector2 v2 = p - p0;
	const float s = 1.0f / (v0.x * v1.y - v1.x * v0.y);
	const float i = (v2.x * v1.y - v1.x * v2.y) * s;
	const float j = (v0.x * v2.y - v2.x * v0.y) * s;
	return Vector3(1.0f - i - j, i, j);
}

inline Vector3 Barycentric(const Vector3 &p, const Triangle &t)
{
	const Vector3 v0 = t.b - t.a;
	const Vector3 v1 = t.c - t.a;
	const Vector3 v2 = p - t.a;
	const float d00 = dot(v0, v0);
	const float d01 = dot(v0, v1);
	const float d11 = dot(v1, v1);
	const float d20 = dot(v2, v0);
	const float d21 = dot(v2, v1);
	const float denom = d00 * d11 - d01 * d01;
	if (std::fabsf(denom) < 0.0001) return Vector3(-1);
	const float y = (d11 * d20 - d01 * d21) / denom;
	const float z = (d00 * d21 - d01 * d20) / denom;
	return Vector3(1.0f - y - z, y, z);
}

inline float Raycast(const Ray &r, const Triangle &t)
{
	const Vector3 n = normalize(cross(t.b - t.a, t.c - t.a));
	const float nd = dot(r.direction, n);
	if (std::fabsf(nd) > 0.0001f)
	{
		const float np = dot(r.origin, n);
		const float l = (dot(t.a, n) - np) / nd;
		if (l > 0)
		{
			const Vector3 p = r.origin + r.direction * l;
			const Vector3 b = Barycentric(p, t);
			if (b.x >= 0 && b.x <= 1 &&
				b.y >= 0 && b.y <= 1 &&
				b.z >= 0 && b.z <= 1)
			{
				return l;
			}
		}
	}
	return -1;
}

inline float Raycast(const Ray &r, const std::vector<Vector3> &vertices)
{
	float mint = FLT_MAX;

	for (size_t tidx = 0; tidx < vertices.size(); tidx += 3)
	{
		const float t = Raycast(r, Triangle(vertices[tidx], vertices[tidx + 1], vertices[tidx + 2]));
		if (t > 0) mint = std::fminf(t, mint);
	}

	return (mint != FLT_MAX) ? mint : -1;
}

inline float radicalInverseVdC(uint32_t bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

inline uint32_t reverseBits(uint32_t bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return bits;
}

inline Vector2 hammersley(uint32_t i, uint32_t N)
{
	return Vector2((float)i / (float)N, radicalInverseVdC(i));
}

inline Vector2 hammersley(uint32_t i, uint32_t N, uint32_t randX, uint32_t randY)
{
	const float x = float(i) / float(N) + float(randX & 0xffff) / float(1 << 16);
	const float y = float(reverseBits(i) ^ randY) * 2.3283064365386963e-10f;
	return Vector2(x - long(x), y);
}

/**
Computes a list of samples by importance for dot(N,L)
Direction for each sample must be computed as:
o_basis[i].x * tangentX + o_basis[i].y * tangentY + o_basis[i].z * N
*/
inline void computeSamplesImportanceCosDir(const size_t count, Vector3 *o_basis)
{
	for (size_t i = 0; i < count; ++i)
	{
		const Vector2 u = hammersley((uint32_t)i, (uint32_t)count);
		const float r = std::sqrtf(u.x);
		const float phi = (float)(2.0 * PI) * u.y;
		const Vector3 v(r * std::cosf(phi), r * std::sinf(phi), std::sqrt(1.0f - u.x));
		o_basis[i] = v;
	}
}

inline void computeSamplesImportanceCosDir(const size_t sampleCount, const size_t permCount, Vector3 *o_basis)
{
	std::random_device rd;
	std::mt19937 re(rd());
	std::uniform_int_distribution<uint32_t> ru;

	for (size_t j = 0; j < permCount; ++j)
	{
		const uint32_t randX = ru(re);
		const uint32_t randY = ru(re);
		for (size_t i = 0; i < sampleCount; ++i)
		{
			const Vector2 u = hammersley((uint32_t)i, (uint32_t)sampleCount, randX, randY);
			const float r = std::sqrtf(u.x);
			const float phi = (float)(2.0 * PI) * u.y;
			const Vector3 v(r * std::cosf(phi), r * std::sinf(phi), std::sqrt(1.0f - u.x));
			o_basis[i + j * sampleCount] = v;
		}
	}
}