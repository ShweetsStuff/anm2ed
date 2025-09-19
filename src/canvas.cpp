#include "canvas.h"

static void _canvas_framebuffer_set(Canvas* self, const ivec2& size) {
  self->size = size;
  self->previousSize = size;

  glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);

  glBindTexture(GL_TEXTURE_2D, self->framebuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self->size.x, self->size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->framebuffer, 0);

  glBindRenderbuffer(GL_RENDERBUFFER, self->rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, self->size.x, self->size.y);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, self->rbo);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void canvas_init(Canvas* self, const ivec2& size) {
  // Axis
  glGenVertexArrays(1, &self->axisVAO);
  glGenBuffers(1, &self->axisVBO);

  glBindVertexArray(self->axisVAO);

  glBindBuffer(GL_ARRAY_BUFFER, self->axisVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(CANVAS_AXIS_VERTICES), CANVAS_AXIS_VERTICES, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);

  // Grid
  glGenVertexArrays(1, &self->gridVAO);
  glGenBuffers(1, &self->gridVBO);

  glBindVertexArray(self->gridVAO);
  glBindBuffer(GL_ARRAY_BUFFER, self->gridVBO);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

  // Rect
  glGenVertexArrays(1, &self->rectVAO);
  glGenBuffers(1, &self->rectVBO);

  glBindVertexArray(self->rectVAO);

  glBindBuffer(GL_ARRAY_BUFFER, self->rectVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(CANVAS_RECT_VERTICES), CANVAS_RECT_VERTICES, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

  // Grid
  glGenVertexArrays(1, &self->gridVAO);
  glBindVertexArray(self->gridVAO);

  glGenBuffers(1, &self->gridVBO);
  glBindBuffer(GL_ARRAY_BUFFER, self->gridVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(CANVAS_GRID_VERTICES), CANVAS_GRID_VERTICES, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

  glBindVertexArray(0);

  // Texture
  glGenVertexArrays(1, &self->textureVAO);
  glGenBuffers(1, &self->textureVBO);
  glGenBuffers(1, &self->textureEBO);

  glBindVertexArray(self->textureVAO);

  glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, nullptr, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->textureEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GL_TEXTURE_INDICES), GL_TEXTURE_INDICES, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

  glBindVertexArray(0);

  // Framebuffer
  glGenTextures(1, &self->framebuffer);
  glGenFramebuffers(1, &self->fbo);
  glGenRenderbuffers(1, &self->rbo);
  _canvas_framebuffer_set(self, size);

  self->isInit = true;
}

mat4 canvas_transform_get(Canvas* self, vec2 pan, float zoom, OriginType origin) {
  float zoomFactor = PERCENT_TO_UNIT(zoom);
  mat4 projection = glm::ortho(0.0f, (float)self->size.x, 0.0f, (float)self->size.y, -1.0f, 1.0f);
  mat4 view = mat4{1.0f};
  vec2 size = vec2(self->size.x, self->size.y);

  switch (origin) {
  case ORIGIN_TOP_LEFT:
    view = glm::translate(view, vec3(pan, 0.0f));
    break;
  default:
    view = glm::translate(view, vec3((size * 0.5f) + pan, 0.0f));
    break;
  }

  view = glm::scale(view, vec3(zoomFactor, zoomFactor, 1.0f));

  return projection * view;
}

void canvas_clear(vec4& color) {
  glClearColor(color.r, color.g, color.b, color.a);
  glClear(GL_COLOR_BUFFER_BIT);
}

void canvas_viewport_set(Canvas* self) { glViewport(0, 0, (int)self->size.x, (int)self->size.y); }

void canvas_framebuffer_resize_check(Canvas* self) {
  if (self->previousSize != self->size)
    _canvas_framebuffer_set(self, self->size);
}

void canvas_grid_draw(Canvas* self, GLuint& shader, mat4& transform, ivec2& size, ivec2& offset, vec4& color) {
  mat4 inverseTransform = glm::inverse(transform);

  glUseProgram(shader);

  glUniformMatrix4fv(glGetUniformLocation(shader, SHADER_UNIFORM_MODEL), 1, GL_FALSE, glm::value_ptr(inverseTransform));
  glUniform2f(glGetUniformLocation(shader, SHADER_UNIFORM_SIZE), size.x, size.y);
  glUniform2f(glGetUniformLocation(shader, SHADER_UNIFORM_OFFSET), offset.x, offset.y);
  glUniform4f(glGetUniformLocation(shader, SHADER_UNIFORM_COLOR), color.r, color.g, color.b, color.a);

  glBindVertexArray(self->gridVAO);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);

  glUseProgram(0);
}

void canvas_texture_draw(Canvas* self, GLuint& shader, GLuint& texture, mat4& transform, const float* vertices, vec4 tint, vec3 colorOffset) {
  glUseProgram(shader);

  glBindVertexArray(self->textureVAO);

  glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(CANVAS_TEXTURE_VERTICES), vertices, GL_DYNAMIC_DRAW);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);

  glUniform1i(glGetUniformLocation(shader, SHADER_UNIFORM_TEXTURE), 0);
  glUniform3fv(glGetUniformLocation(shader, SHADER_UNIFORM_COLOR_OFFSET), 1, value_ptr(colorOffset));
  glUniform4fv(glGetUniformLocation(shader, SHADER_UNIFORM_TINT), 1, value_ptr(tint));
  glUniformMatrix4fv(glGetUniformLocation(shader, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(transform));

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
}

void canvas_rect_draw(Canvas* self, const GLuint& shader, const mat4& transform, const vec4& color) {
  glUseProgram(shader);

  glBindVertexArray(self->rectVAO);

  glUniformMatrix4fv(glGetUniformLocation(shader, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(transform));
  glUniform4fv(glGetUniformLocation(shader, SHADER_UNIFORM_COLOR), 1, value_ptr(color));

  glDrawArrays(GL_LINE_LOOP, 0, 4);

  glBindVertexArray(0);
  glUseProgram(0);
}

void canvas_axes_draw(Canvas* self, GLuint& shader, mat4& transform, vec4& color) {
  vec4 originNDC = transform * vec4(0.0f, 0.0f, 0.0f, 1.0f);
  originNDC /= originNDC.w;

  glUseProgram(shader);
  glBindVertexArray(self->axisVAO);
  glUniform4fv(glGetUniformLocation(shader, SHADER_UNIFORM_COLOR), 1, value_ptr(color));
  glUniform2f(glGetUniformLocation(shader, SHADER_UNIFORM_ORIGIN_NDC), originNDC.x, originNDC.y);
  glUniform1i(glGetUniformLocation(shader, SHADER_UNIFORM_AXIS), 0);
  glDrawArrays(GL_LINES, 0, 2);
  glUniform1i(glGetUniformLocation(shader, SHADER_UNIFORM_AXIS), 1);
  glDrawArrays(GL_LINES, 0, 2);
  glBindVertexArray(0);
  glUseProgram(0);
}

void canvas_bind(Canvas* self) { glBindFramebuffer(GL_FRAMEBUFFER, self->fbo); }

void canvas_unbind(void) { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void canvas_free(Canvas* self) {
  if (!self->isInit)
    return;

  glDeleteFramebuffers(1, &self->fbo);
  glDeleteRenderbuffers(1, &self->rbo);
  glDeleteTextures(1, &self->framebuffer);
  glDeleteVertexArrays(1, &self->axisVAO);
  glDeleteVertexArrays(1, &self->rectVAO);
  glDeleteVertexArrays(1, &self->gridVAO);
  glDeleteVertexArrays(1, &self->textureVAO);
  glDeleteBuffers(1, &self->axisVBO);
  glDeleteBuffers(1, &self->rectVBO);
  glDeleteBuffers(1, &self->gridVBO);
  glDeleteBuffers(1, &self->textureVBO);
  glDeleteBuffers(1, &self->textureEBO);
}