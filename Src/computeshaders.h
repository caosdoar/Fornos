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

GLuint LoadComputeShader_MeshMapping();
GLuint LoadComputeShader_MeshMappingCullBackfaces();

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