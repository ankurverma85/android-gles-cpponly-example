//
// This file is used by the template to render a basic scene using GL.
//
#pragma warning(push, 1)

// Enable function definitions in the GL headers below
#ifndef EGL_EGL_PROTOTYPES
#define EGL_EGL_PROTOTYPES 1
#endif
// OpenGL ES includes
#define _USE_MATH_DEFINES

//#include <GLES3/gl3.h>
//#include <GLES3/gl32.h>

#include <GLES2/gl2.h>
#include <GLES/gl.h>

// EGL includes
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#include <cmath>
#include <math.h>

// OpenGL ES includes
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

// EGL includes
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#define EGL_PLATFORM_ANGLE_ANGLE 0x3202
#define EGL_PLATFORM_ANGLE_TYPE_ANGLE 0x3203
#define EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE 0x3207
#define EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE 0x3208
#define EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE 0x320D
#define EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE 0x320F
#define EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE 0x3204
#define EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE 0x3205
#pragma warning(pop)

#include "SimpleRenderer.h"
#include "MathHelper.h"

// These are used by the shader compilation methods.
#include <fstream>
#include <iostream>
#include <vector>

#define STRING(s) #s

GLuint CompileShader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);

    const char* sourceArray[1] = {source.c_str()};
    glShaderSource(shader, 1, sourceArray, NULL);
    glCompileShader(shader);

    GLint compileResult;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);

    if (compileResult == 0)
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<GLchar> infoLog(infoLogLength);
        glGetShaderInfoLog(shader, (GLsizei)infoLog.size(), NULL, infoLog.data());

        std::string errorMessage = std::string("Shader compilation failed: ");
        errorMessage += std::string(infoLog.begin(), infoLog.end());

        throw std::runtime_error(errorMessage.c_str());
    }

    return shader;
}

GLuint CompileProgram(const std::string& vsSource, const std::string& fsSource)
{
    GLuint program = glCreateProgram();

    if (program == 0)
    {
        throw std::runtime_error("Program creation failed");
    }

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSource);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSource);

    if (vs == 0 || fs == 0)
    {
        glDeleteShader(fs);
        glDeleteShader(vs);
        glDeleteProgram(program);
        return 0;
    }

    glAttachShader(program, vs);
    glDeleteShader(vs);

    glAttachShader(program, fs);
    glDeleteShader(fs);

    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    if (linkStatus == 0)
    {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<GLchar> infoLog(infoLogLength);
        glGetProgramInfoLog(program, (GLsizei)infoLog.size(), NULL, infoLog.data());

        std::string errorMessage = std::string("Program link failed: ");
        errorMessage += std::string(infoLog.begin(), infoLog.end());

        throw std::runtime_error(errorMessage.c_str());
    }

    return program;
}

SimpleRenderer::SimpleRenderer() : mWindowWidth(0), mWindowHeight(0), mDrawCount(0)
{
    // Vertex Shader source
    const std::string vs = STRING(uniform mat4 uModelMatrix; uniform mat4 uViewMatrix; uniform mat4 uProjMatrix; attribute vec4 aPosition;
                                  attribute vec4                                                                                aColor;
                                  varying vec4                                                                                  vColor;
                                  void                                                                                          main() {
                                      gl_Position = uProjMatrix * uViewMatrix * uModelMatrix * aPosition;
                                      vColor = aColor;
                                  });

    // Fragment Shader source
    const std::string fs = R"(
        #ifdef GL_ES
             precision mediump float;
        #endif
        varying vec4 vColor; void main() { gl_FragColor = vColor; })";

    // Set up the shader and its uniform/attribute locations.
    mProgram                = CompileProgram(vs, fs);
    mPositionAttribLocation = glGetAttribLocation(mProgram, "aPosition");
    mColorAttribLocation    = glGetAttribLocation(mProgram, "aColor");
    mModelUniformLocation   = glGetUniformLocation(mProgram, "uModelMatrix");
    mViewUniformLocation    = glGetUniformLocation(mProgram, "uViewMatrix");
    mProjUniformLocation    = glGetUniformLocation(mProgram, "uProjMatrix");

    // Then set up the cube geometry.
    GLfloat vertexPositions[] = {
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f,
        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f, 1.0f,  1.0f, -1.0f, 1.0f,  1.0f, 1.0f,
    };

    glGenBuffers(1, &mVertexPositionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexPositionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);

    float t = 1.0f;
    GLfloat vertexColors[] = {
        0.0f, 0.0f, 0.0f, t, 0.0f, 0.0f, 1.0f, t, 0.0f, 1.0f, 0.0f, t, 0.0f, 1.0f, 1.0f, t,
        1.0f, 0.0f, 0.0f, t, 1.0f, 0.0f, 1.0f, t, 1.0f, 1.0f, 0.0f, t, 1.0f, 1.0f, 1.0f, t,
    };

    glGenBuffers(1, &mVertexColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);

    short indices[] = {
        0, 1, 2,    // -x
        1, 3, 2,

        4, 6, 5,    // +x
        5, 6, 7,

        0, 5, 1,    // -y
        0, 4, 5,

        2, 7, 6,    // +y
        2, 3, 7,

        0, 6, 4,    // -z
        0, 2, 6,

        1, 7, 3,    // +z
        1, 5, 7,
    };

    glGenBuffers(1, &mIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

SimpleRenderer::~SimpleRenderer()
{
    if (mProgram != 0)
    {
        glDeleteProgram(mProgram);
        mProgram = 0;
    }

    if (mVertexPositionBuffer != 0)
    {
        glDeleteBuffers(1, &mVertexPositionBuffer);
        mVertexPositionBuffer = 0;
    }

    if (mVertexColorBuffer != 0)
    {
        glDeleteBuffers(1, &mVertexColorBuffer);
        mVertexColorBuffer = 0;
    }

    if (mIndexBuffer != 0)
    {
        glDeleteBuffers(1, &mIndexBuffer);
        mIndexBuffer = 0;
    }
}

void SimpleRenderer::Draw()
{
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (mProgram == 0) return;

    glUseProgram(mProgram);

    glBindBuffer(GL_ARRAY_BUFFER, mVertexPositionBuffer);
    glEnableVertexAttribArray(mPositionAttribLocation);
    glVertexAttribPointer(mPositionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, mVertexColorBuffer);
    glEnableVertexAttribArray(mColorAttribLocation);
    glVertexAttribPointer(mColorAttribLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);

    MathHelper::Matrix4 modelMatrix = MathHelper::SimpleModelMatrix((float)mDrawCount / 50.0f);
    glUniformMatrix4fv(mModelUniformLocation, 1, GL_FALSE, &(modelMatrix.m[0][0]));

    MathHelper::Matrix4 viewMatrix = MathHelper::SimpleViewMatrix();
    glUniformMatrix4fv(mViewUniformLocation, 1, GL_FALSE, &(viewMatrix.m[0][0]));

    MathHelper::Matrix4 projectionMatrix = MathHelper::SimpleProjectionMatrix(float(mWindowWidth) / float(mWindowHeight));
    glUniformMatrix4fv(mProjUniformLocation, 1, GL_FALSE, &(projectionMatrix.m[0][0]));

    // Draw 36 indices: six faces, two triangles per face, 3 indices per triangle
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    glDrawElements(GL_TRIANGLES, (6 * 2) * 3, GL_UNSIGNED_SHORT, 0);

    mDrawCount += 1;
}

void SimpleRenderer::UpdateWindowSize(GLsizei width, GLsizei height)
{
    glViewport(0, 0, width, height);
    mWindowWidth  = width;
    mWindowHeight = height;
}
