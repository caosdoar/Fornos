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

#include "computeshaders.h"
#include "computeshaders_content.h"
#include "compute.h"

#define COMPUTE_SHADER_FROM_FILES 0

GLuint LoadComputeShader_MeshMapping()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\meshmapping.comp");
#else
	return CreateComputeProgramFromMemory(meshmapping_comp);
#endif
}

GLuint LoadComputeShader_MeshMappingCullBackfaces()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\meshmapping_nobackfaces.comp");
#else
	return CreateComputeProgramFromMemory(meshmapping_nobackfaces_comp);
#endif
}

GLuint LoadComputeShader_AO_GenData()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\ao_step0.comp");
#else
	return CreateComputeProgramFromMemory(ao_step0_comp);
#endif
}

GLuint LoadComputeShader_AO_Sampling()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\ao_step1.comp");
#else
	return CreateComputeProgramFromMemory(ao_step1_comp);
#endif
}

GLuint LoadComputeShader_AO_Aggregate()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\ao_step2.comp");
#else
	return CreateComputeProgramFromMemory(ao_step2_comp);
#endif
}

GLuint LoadComputeShader_BN_GenData()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\ao_step0.comp");
#else
	return CreateComputeProgramFromMemory(ao_step0_comp);
#endif
}

GLuint LoadComputeShader_BN_Sampling()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\bentnormals_step1.comp");
#else
	return CreateComputeProgramFromMemory(bentnormals_step1_comp);
#endif
}

GLuint LoadComputeShader_BN_Aggregate()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\bentnormals_step2.comp");
#else
	return CreateComputeProgramFromMemory(bentnormals_step2_comp);
#endif
}

GLuint LoadComputeShader_Thick_GenData()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\ao_step0.comp");
#else
	return CreateComputeProgramFromMemory(ao_step0_comp);
#endif
}

GLuint LoadComputeShader_Thick_Sampling()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\thick_step1.comp");
#else
	return CreateComputeProgramFromMemory(thick_step1_comp);
#endif
}

GLuint LoadComputeShader_Thick_Aggregate()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\thick_step2.comp");
#else
	return CreateComputeProgramFromMemory(thick_step2_comp);
#endif
}

GLuint LoadComputeShader_Height()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\heights.comp");
#else
	return CreateComputeProgramFromMemory(heights_comp);
#endif
}

GLuint LoadComputeShader_Position()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\positions.comp");
#else
	return CreateComputeProgramFromMemory(positions_comp);
#endif
}

GLuint LoadComputeShader_Normal()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\normals.comp");
#else
	return CreateComputeProgramFromMemory(normals_comp);
#endif
}

GLuint LoadComputeShader_ToTangentSpace()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Shaders\\tangentspace.comp");
#else
	return CreateComputeProgramFromMemory(tangentspace_comp);
#endif
}
