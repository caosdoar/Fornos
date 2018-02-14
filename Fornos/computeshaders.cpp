#include "computeshaders.h"
#include "computeshaders_content.h"
#include "compute.h"

#define COMPUTE_SHADER_FROM_FILES 0

GLuint LoadComputeShader_MeshMapping()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\meshmapping.comp");
#else
	return CreateComputeProgramFromMemory(meshmapping_comp);
#endif
}

GLuint LoadComputeShader_AO_GenData()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\ao_step0.comp");
#else
	return CreateComputeProgramFromMemory(ao_step0_comp);
#endif
}

GLuint LoadComputeShader_AO_Sampling()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\ao_step1.comp");
#else
	return CreateComputeProgramFromMemory(ao_step1_comp);
#endif
}

GLuint LoadComputeShader_AO_Aggregate()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\ao_step2.comp");
#else
	return CreateComputeProgramFromMemory(ao_step2_comp);
#endif
}

GLuint LoadComputeShader_BN_GenData()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\ao_step0.comp");
#else
	return CreateComputeProgramFromMemory(ao_step0_comp);
#endif
}

GLuint LoadComputeShader_BN_Sampling()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\bentnormals_step1.comp");
#else
	return CreateComputeProgramFromMemory(bentnormals_step1_comp);
#endif
}

GLuint LoadComputeShader_BN_Aggregate()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\bentnormals_step2.comp");
#else
	return CreateComputeProgramFromMemory(bentnormals_step2_comp);
#endif
}

GLuint LoadComputeShader_Thick_GenData()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\ao_step0.comp");
#else
	return CreateComputeProgramFromMemory(bentnormals_step2_comp);
#endif
}

GLuint LoadComputeShader_Thick_Sampling()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\thick_step1.comp");
#else
	return CreateComputeProgramFromMemory(ao_step0_comp);
#endif
}

GLuint LoadComputeShader_Thick_Aggregate()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\thick_step2.comp");
#else
	return CreateComputeProgramFromMemory(thick_step2_comp);
#endif
}

GLuint LoadComputeShader_Normal()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\normals.comp");
#else
	return CreateComputeProgramFromMemory(normals_comp);
#endif
}

GLuint LoadComputeShader_ToTangentSpace()
{
#if COMPUTE_SHADER_FROM_FILES
	return CreateComputeProgram("D:\\Code\\Fornos\\Fornos\\shaders\\tangentspace.comp");
#else
	return CreateComputeProgramFromMemory(tangentspace_comp);
#endif
}
