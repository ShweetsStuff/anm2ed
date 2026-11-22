#pragma once

#include "gl.h"
#include "vec.h"
#include <vector>

class Framebuffer
{
public:
    enum class Type
    {
        UNDERLAY,
        LAYER,
        OVERLAY,
    };

    GLuint fbo = 0;
    GLuint rbo = 0;
    GLuint texture = 0;
    Vec2 size{};
    Vec2 previousSize{};
    GLint filter = GL_LINEAR; // how the texture is sampled when drawn scaled. Takes effect on the next resize

    Framebuffer();
    ~Framebuffer();

    void set();
    void resize_check();
    void size_set(Vec2);
    void viewport_set() const;
    void clear(Vec4 = Vec4(1.0f, 1.0f, 1.0f, 0.0f)) const;
    std::vector<uint8_t> pixels_get() const;
    bool is_valid() const;
    void bind() const;
    void unbind() const;
};
