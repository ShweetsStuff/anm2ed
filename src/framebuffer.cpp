#include "framebuffer.h"

#include "texture.h"

using namespace anm2ed::resource;
using namespace glm;

namespace anm2ed
{
  Framebuffer::Framebuffer()
  {
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glGenTextures(1, &texture);
    set();
  }

  Framebuffer::~Framebuffer()
  {
    if (!is_valid()) return;

    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &texture);
  }

  void Framebuffer::set()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.x, size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    auto count = size.x * size.y * texture::CHANNELS;
    std::vector<uint8_t> pixels(count);

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    return pixels;
  }

  void Framebuffer::clear(vec4 color) const
  {
    glEnable(GL_BLEND);
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (color.a == 0.0f) glDisable(GL_BLEND);
  }

  bool Framebuffer::is_valid() const { return fbo != 0; }
  void Framebuffer::viewport_set() const { glViewport(0, 0, size.x, size.y); }
  void Framebuffer::bind() const { glBindFramebuffer(GL_FRAMEBUFFER, fbo); }
  void Framebuffer::unbind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
}
