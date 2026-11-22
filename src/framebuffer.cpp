#include "framebuffer.h"

#include "texture.h"

using namespace glm;

Framebuffer::Framebuffer()
{
    gl.GenFramebuffers(1, &fbo);
    gl.GenRenderbuffers(1, &rbo);
    gl.GenTextures(1, &texture);
    set();
}

Framebuffer::~Framebuffer()
{
    if (!is_valid())
        return;

    gl.DeleteFramebuffers(1, &fbo);
    gl.DeleteRenderbuffers(1, &rbo);
    gl.DeleteTextures(1, &texture);
}

void Framebuffer::set()
{
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);

    gl.BindTexture(GL_TEXTURE_2D, texture);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    gl.BindRenderbuffer(GL_RENDERBUFFER, rbo);
    gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.x, size.y);
    gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resize_check()
{
    if (size != previousSize)
    {
        set();
        previousSize = size;
    }
}

void Framebuffer::size_set(vec2 size)
{
    previousSize = this->size;
    this->size = size;
    resize_check();
}

std::vector<uint8_t> Framebuffer::pixels_get() const
{
    auto count = size.x * size.y * IMAGE_CHANNELS;
    std::vector<uint8_t> pixels(count);

    gl.ReadBuffer(GL_COLOR_ATTACHMENT0);
    gl.PixelStorei(GL_PACK_ALIGNMENT, 1);
    gl.PixelStorei(GL_PACK_ROW_LENGTH, 0);
    gl.ReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    gl.BindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    return pixels;
}

void Framebuffer::clear(vec4 color) const
{
    gl.Enable(GL_BLEND);
    gl.ClearColor(color.r, color.g, color.b, color.a);
    gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (color.a <= 0.0f)
        gl.Disable(GL_BLEND);
}

bool Framebuffer::is_valid() const
{
    return fbo != 0;
}

void Framebuffer::viewport_set() const
{
    gl.Viewport(0, 0, size.x, size.y);
}

void Framebuffer::bind() const
{
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void Framebuffer::unbind() const
{
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
}
