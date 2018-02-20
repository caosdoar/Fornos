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

#include "image.h"
#include "compute.h"
#include "logging.h"
#include "math.h"
#include "timing.h"
#include <cassert>

#pragma warning(disable:4996)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "tinyexr.h"

Vector2 getMinMax(const float *data, const size_t count)
{
	Vector2 minmax(FLT_MAX, -FLT_MAX);
	for (size_t i = 0; i < count; ++i)
	{
		const float r = data[i];
		minmax.x = std::fminf(minmax.x, r);
		minmax.y = std::fmaxf(minmax.y, r);
	}
	return minmax;
}

enum class Extension
{
	Unknown,
	Png,
	Tga,
	Exr
};

bool endsWith(const std::string &str, const std::string &ending)
{
	if (ending.size() > str.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), str.rbegin());
}

Extension getExtension(const char *path)
{
	std::string p(path);
	if (endsWith(p, ".png")) return Extension::Png;
	if (endsWith(p, ".tga")) return Extension::Tga;
	if (endsWith(p, ".exr")) return Extension::Exr;
	return Extension::Unknown;
}

Vector2 computeScaleBias(const Vector2 &minmax, bool normalize)
{
	if (normalize && minmax.x != FLT_MAX && minmax.y != -FLT_MAX)
	{
		const float scale = 1.0f / (minmax.y - minmax.x);
		return Vector2(scale, -minmax.x * scale);
	}
	return Vector2(1.0f, 0.0f);
}

struct PixPos
{
	int x; 
	int y;
	PixPos operator -() const { return PixPos{ -x, -y }; }
	PixPos operator +(const PixPos &o) const { return PixPos{ x + o.x, y + o.y }; }
	PixPos operator -(const PixPos &o) const { return PixPos{ x - o.x, y - o.y }; }
	PixPos operator *(const int k) const { return PixPos{ x * k, y * k }; }
	PixPos operator *(const PixPos &o) const { return PixPos{ x * o.x, y * o.y }; }
};

std::vector<bool> createValidPixelsTable(const CompressedMapUV *map)
{
	std::vector<bool> validPixels(map->width * map->height);
	for (const auto idx : map->indices)
	{
		validPixels[idx] = true;
	}
	return validPixels;
}

std::vector<bool> createValidPixelsTableRGB
(
	const CompressedMapUV *map, 
	const uint8_t *data, 
	uint8_t invalidR, 
	uint8_t invalidG, 
	uint8_t invalidB
)
{
	const size_t w = map->width;
	const size_t h = map->height;
	std::vector<bool> validPixels(w * h);
	for (const auto idx : map->indices)
	{
		const size_t x = idx % w;
		const size_t y = idx / h;
		const size_t pixIdx = ((h - y - 1) * w + x) * 3;
		const uint8_t r = data[pixIdx + 0];
		const uint8_t g = data[pixIdx + 1];
		const uint8_t b = data[pixIdx + 2];
		validPixels[idx] = (r != invalidR) | (g != invalidG) | (b != invalidB);
	}
	return validPixels;
}

#define DEBUG 1

void dilateRGB(uint8_t *data, const CompressedMapUV *map, const std::vector<bool> &validPixels, const size_t maxDist)
{
	Timing timing;
	timing.begin();

	const PixPos offsets[] = { { 1,0 },{ -1,0 },{ 0,1 },{ 0,-1 },{ 1,1 },{ 1,-1 },{ -1,1 },{ -1,-1 } };

	const int w = int(map->width);
	const int h = int(map->height);

#pragma omp parallel for
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			if (!validPixels[x + y * w])
			{
				PixPos pos{ x, y };
				bool filled = false;

				for (size_t dist = 1; dist < maxDist && !filled; ++dist)
				{
					for (const PixPos offset : offsets)
					{
						PixPos centerPos = pos + offset * int(dist);
						if (centerPos.x > 0 && centerPos.y > 0 && centerPos.x < w - 1 && centerPos.y < h - 1)
						{
							bool valid = true;
							for (const PixPos o : offsets)
							{
								PixPos testPos = centerPos + o;
								if (!validPixels[testPos.x + testPos.y * w])
								{
									valid = false;
									break;
								}
							}

							if (valid)
							{
								const size_t pixIdxSrc = ((h - centerPos.y - 1) * w + centerPos.x) * 3;
								const size_t pixIdxDst = ((h - pos.y - 1) * w + pos.x) * 3;
								data[pixIdxDst + 0] = data[pixIdxSrc + 0];
								data[pixIdxDst + 1] = data[pixIdxSrc + 1];
								data[pixIdxDst + 2] = data[pixIdxSrc + 2];
								filled = true;
								break;
							}
						}
					}
				}
			}
		}
	}

	timing.end();
	logDebug("Image", "Image dilation took " + std::to_string(timing.elapsedSeconds()) + " seconds.");
}

void exportFloatImage(const float *data, const CompressedMapUV *map, const char *path, bool normalize, int dilate, Vector2 *o_minmax)
{
	assert(data);
	assert(map);
	assert(path);

	Extension ext = getExtension(path);
	if (ext == Extension::Unknown) return; // TODO: Error handling

	const size_t count = map->indices.size();
	const size_t w = map->width;
	const size_t h = map->height;

	const Vector2 minmax = getMinMax(data, count);
	if (o_minmax) *o_minmax = minmax;
	const Vector2 scaleBias = computeScaleBias(minmax, normalize);

	if (ext == Extension::Png || ext == Extension::Tga)
	{
		// TODO: Unnormalized quantized data is not a great option...
		//if (!normalize) return Vector2();

		uint8_t *rgb = new uint8_t[w * h * 3];
		memset(rgb, 0, sizeof(uint8_t) * w * h * 3);
		for (size_t i = 0; i < count; ++i)
		{
			const float d = data[i];
			const float t = d * scaleBias.x + scaleBias.y;
			const uint8_t c = (uint8_t)(std::min(std::max(t, 0.0f), 1.0f) * 255.0f);
			const size_t index = map->indices[i];
			const size_t x = index % w;
			const size_t y = index / w;
			const size_t pixidx = ((h - y - 1) * w + x) * 3;
			rgb[pixidx + 0] = c;
			rgb[pixidx + 1] = c;
			rgb[pixidx + 2] = c;
		}

		if (dilate > 0)
		{
			const auto validPixels = createValidPixelsTable(map);
			dilateRGB(rgb, map, validPixels, dilate);
		}

		if (ext == Extension::Png) stbi_write_png(path, (int)w, (int)h, 3, rgb, (int)w * 3);
		else stbi_write_tga(path, (int)w, (int)h, 3, rgb);

		delete[] rgb;
	}
	else if (ext == Extension::Exr)
	{
		EXRHeader header;
		InitEXRHeader(&header);
		EXRImage image;
		InitEXRImage(&image);
		image.num_channels = 1;

		float *f = new float[w * h];
		memset(f, 0, sizeof(float) * w * h);
		for (size_t i = 0; i < count; ++i)
		{
			const float d = data[i];
			const float t = d * scaleBias.x + scaleBias.y;
			const size_t index = map->indices[i];
			const size_t x = index % w;
			const size_t y = index / w;
			const size_t pixidx = ((h - y - 1) * w + x);
			f[pixidx] = t;
		}

		image.images = (unsigned char **)(&f);
		image.width = (int)w;
		image.height = (int)h;
		header.num_channels = 1;
		header.channels = new EXRChannelInfo[header.num_channels];
		strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
		header.pixel_types = new int[header.num_channels];
		header.requested_pixel_types = new int[header.num_channels];
		header.pixel_types[0] = TINYEXR_PIXELTYPE_FLOAT;
		header.requested_pixel_types[0] = TINYEXR_PIXELTYPE_HALF;
		const char *err;
		int ret = SaveEXRImageToFile(&image, &header, path, &err);
		if (ret != TINYEXR_SUCCESS)
		{
			// TODO: Error handling
		}

		delete[] f;
		delete[] header.channels;
		delete[] header.pixel_types;
		delete[] header.requested_pixel_types;
	}
}

void exportVectorImage(const Vector3 *data, const CompressedMapUV *map, const char *path)
{
	assert(data);
	assert(map);
	assert(path);

	Extension ext = getExtension(path);
	if (ext != Extension::Exr) return; // TODO: Error handling

	const size_t count = map->indices.size();
	const size_t w = map->width;
	const size_t h = map->height;

	EXRHeader header;
	InitEXRHeader(&header);
	EXRImage image;
	InitEXRImage(&image);
	image.num_channels = 1;

	std::vector<float> images[3];
	images[0].resize(w * h);
	images[1].resize(w * h);
	images[2].resize(w * h);

	for (size_t i = 0; i < count; ++i)
	{
		const size_t index = map->indices[i];
		const size_t x = index % w;
		const size_t y = index / w;
		const size_t pixidx = ((h - y - 1) * w + x);
		images[0][pixidx] = data[i].z;
		images[1][pixidx] = data[i].y;
		images[2][pixidx] = data[i].x;
	}

	float *image_ptr[3] = { &images[0][0], &images[1][0], &images[2][0] };
	image.images = (unsigned char **)(image_ptr);
	image.width = (int)w;
	image.height = (int)h;

	header.num_channels = 3;
	header.channels = new EXRChannelInfo[header.num_channels];
	strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
	strncpy(header.channels[1].name, "G", 255); header.channels[1].name[strlen("G")] = '\0';
	strncpy(header.channels[2].name, "R", 255); header.channels[2].name[strlen("R")] = '\0';

	header.pixel_types = new int[header.num_channels];
	header.requested_pixel_types = new int[header.num_channels];

	for (size_t i = 0; i < header.num_channels; ++i)
	{
		header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
		header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
	}

	const char *err;
	int ret = SaveEXRImageToFile(&image, &header, path, &err);
	if (ret != TINYEXR_SUCCESS)
	{
		// TODO: Error handling
	}

	delete[] header.channels;
	delete[] header.pixel_types;
	delete[] header.requested_pixel_types;
}

void exportNormalImage(const Vector3 *data, const CompressedMapUV *map, const char *path, int dilate)
{
	assert(data);
	assert(map);
	assert(path);

	Extension ext = getExtension(path);
	if (ext == Extension::Unknown) return; // TODO: Error handling

	const size_t count = map->indices.size();
	const size_t w = map->width;
	const size_t h = map->height;

	if (ext == Extension::Png || ext == Extension::Tga)
	{
		uint8_t *rgb = new uint8_t[w * h * 3];
		memset(rgb, 0, sizeof(uint8_t) * w * h * 3);
		for (size_t i = 0; i < count; ++i)
		{
			const Vector3 n = data[i] * 0.5f + Vector3(0.5f);
			const size_t index = map->indices[i];
			const size_t x = index % w;
			const size_t y = index / w;
			const size_t pixidx = ((h - y - 1) * w + x) * 3;
			rgb[pixidx + 0] = uint8_t(n.x * 255.0f);
			rgb[pixidx + 1] = uint8_t(n.y * 255.0f);
			rgb[pixidx + 2] = uint8_t(n.z * 255.0f);
		}

		if (dilate > 0)
		{
			const auto validPixels = createValidPixelsTableRGB(map, rgb, 0, 0, 0);
			dilateRGB(rgb, map, validPixels, dilate);
		}

		if (ext == Extension::Png) stbi_write_png(path, (int)w, (int)h, 3, rgb, (int)w * 3);
		else stbi_write_tga(path, (int)w, (int)h, 3, rgb);

		delete[] rgb;
	}
	else if (ext == Extension::Exr)
	{
		exportVectorImage(data, map, path);
	}
}