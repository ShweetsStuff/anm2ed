#pragma once

#define GL_GLEXT_PROTOTYPES
#include <SDL3/SDL_opengl.h>

#define GL_FUNCTIONS(X)                                                                                                \
    X(ActiveTexture)                                                                                                   \
    X(AttachShader)                                                                                                    \
    X(BindAttribLocation)                                                                                              \
    X(BindBuffer)                                                                                                      \
    X(BindFramebuffer)                                                                                                 \
    X(BindRenderbuffer)                                                                                                \
    X(BindTexture)                                                                                                     \
    X(BindVertexArray)                                                                                                 \
    X(BlendFuncSeparate)                                                                                               \
    X(BufferData)                                                                                                      \
    X(Clear)                                                                                                           \
    X(ClearColor)                                                                                                      \
    X(CompileShader)                                                                                                   \
    X(CreateProgram)                                                                                                   \
    X(CreateShader)                                                                                                    \
    X(DeleteBuffers)                                                                                                   \
    X(DeleteFramebuffers)                                                                                              \
    X(DeleteProgram)                                                                                                   \
    X(DeleteRenderbuffers)                                                                                             \
    X(DeleteShader)                                                                                                    \
    X(DeleteTextures)                                                                                                  \
    X(DeleteVertexArrays)                                                                                              \
    X(Disable)                                                                                                         \
    X(DrawArrays)                                                                                                      \
    X(DrawElements)                                                                                                    \
    X(Enable)                                                                                                          \
    X(EnableVertexAttribArray)                                                                                         \
    X(FramebufferRenderbuffer)                                                                                         \
    X(FramebufferTexture2D)                                                                                            \
    X(GenBuffers)                                                                                                      \
    X(GenFramebuffers)                                                                                                 \
    X(GenRenderbuffers)                                                                                                \
    X(GenTextures)                                                                                                     \
    X(GenVertexArrays)                                                                                                 \
    X(GetActiveUniform)                                                                                                \
    X(GetProgramInfoLog)                                                                                               \
    X(GetProgramiv)                                                                                                    \
    X(GetShaderInfoLog)                                                                                                \
    X(GetShaderiv)                                                                                                     \
    X(GetString)                                                                                                       \
    X(GetUniformLocation)                                                                                              \
    X(LineWidth)                                                                                                       \
    X(LinkProgram)                                                                                                     \
    X(PixelStorei)                                                                                                     \
    X(ReadBuffer)                                                                                                      \
    X(ReadPixels)                                                                                                      \
    X(RenderbufferStorage)                                                                                             \
    X(ShaderSource)                                                                                                    \
    X(TexImage2D)                                                                                                      \
    X(TexParameteri)                                                                                                   \
    X(Uniform1f)                                                                                                       \
    X(Uniform1i)                                                                                                       \
    X(Uniform2f)                                                                                                       \
    X(Uniform2fv)                                                                                                      \
    X(Uniform3fv)                                                                                                      \
    X(Uniform4f)                                                                                                       \
    X(Uniform4fv)                                                                                                      \
    X(UniformMatrix4fv)                                                                                                \
    X(UseProgram)                                                                                                      \
    X(VertexAttrib4fv)                                                                                                 \
    X(VertexAttribPointer)                                                                                             \
    X(Viewport)

// The OpenGL functions the editor calls, through pointers loaded by gl_load once a context is current.
struct Gl
{
#define X(name) decltype(&gl##name) name = nullptr;
    GL_FUNCTIONS(X)
#undef X
};

extern Gl gl;

bool gl_load();
