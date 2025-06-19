#include "preview.h"


/* Initializes the preview */
void
preview_init(Preview* self)
{
    glGenFramebuffers(1, &self->fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);

    glGenTextures(1, &self->texture);
    glBindTexture(GL_TEXTURE_2D, self->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PREVIEW_SIZE.x, PREVIEW_SIZE.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->texture, 0);

    glGenRenderbuffers(1, &self->rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, self->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, PREVIEW_SIZE.x, PREVIEW_SIZE.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, self->rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
preview_draw(Preview* self)
{
    glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);
    glViewport(0, 0, PREVIEW_SIZE.x, PREVIEW_SIZE.y);

    glClearColor(self->color.r, self->color.g, self->color.b, self->color.a); 
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // unbind
}

void
preview_free(Preview* self)
{
    glDeleteTextures(1, &self->texture);
    glDeleteFramebuffers(1, &self->fbo);
    glDeleteRenderbuffers(1, &self->rbo);

    memset(self, '\0', sizeof(Preview));
}