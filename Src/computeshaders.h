#pragma once

#include <glad/glad.h>

GLuint LoadComputeShader_MeshMapping();

GLuint LoadComputeShader_AO_GenData();
GLuint LoadComputeShader_AO_Sampling();
GLuint LoadComputeShader_AO_Aggregate();

GLuint LoadComputeShader_BN_GenData();
GLuint LoadComputeShader_BN_Sampling();
GLuint LoadComputeShader_BN_Aggregate();

GLuint LoadComputeShader_Thick_GenData();
GLuint LoadComputeShader_Thick_Sampling();
GLuint LoadComputeShader_Thick_Aggregate();

GLuint LoadComputeShader_Height();
GLuint LoadComputeShader_Position();
GLuint LoadComputeShader_Normal();

GLuint LoadComputeShader_ToTangentSpace();