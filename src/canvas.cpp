// Common rendering area

#include "canvas.h"

static void _canvas_texture_free(Canvas* self)
{
    if (self->fbo != 0)     glDeleteFramebuffers(1, &self->fbo);
    if (self->rbo != 0)     glDeleteRenderbuffers(1, &self->rbo);
    if (self->texture != 0) glDeleteTextures(1, &self->texture);
}

static void _canvas_texture_init(Canvas* self, vec2& size)
{
    _canvas_texture_free(self);

    self->size = size;
    
    glGenFramebuffers(1, &self->fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);

    glGenTextures(1, &self->texture);
    glBindTexture(GL_TEXTURE_2D, self->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (s32)self->size.x, (s32)self->size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->texture, 0);

    glGenRenderbuffers(1, &self->rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, self->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (s32)self->size.x, (s32)self->size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, self->rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void canvas_init(Canvas* self)
{
    // Axis
    glGenVertexArrays(1, &self->axisVAO);
    glGenBuffers(1, &self->axisVBO);

    glBindVertexArray(self->axisVAO);

    glBindBuffer(GL_ARRAY_BUFFER, self->axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CANVAS_AXIS_VERTICES), CANVAS_AXIS_VERTICES, GL_STATIC_DRAW);
 
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);
    
    // Grid
    glGenVertexArrays(1, &self->gridVAO);
    glGenBuffers(1, &self->gridVBO);
  
    glBindVertexArray(self->gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, self->gridVBO); 

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);

    // Rect
    glGenVertexArrays(1, &self->rectVAO);
    glGenBuffers(1, &self->rectVBO);

    glBindVertexArray(self->rectVAO);

    glBindBuffer(GL_ARRAY_BUFFER, self->rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GL_VERTICES), GL_VERTICES, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);

    // Texture
    glGenVertexArrays(1, &self->textureVAO);
    glGenBuffers(1, &self->textureVBO);
    glGenBuffers(1, &self->textureEBO);

    glBindVertexArray(self->textureVAO);

    glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 4 * 4, nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->textureEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GL_TEXTURE_INDICES), GL_TEXTURE_INDICES, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)0);

    glEnableVertexAttribArray(1); 
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)(2 * sizeof(f32)));
    
    glBindVertexArray(0);
}

mat4 canvas_transform_get(Canvas* self, vec2& pan, f32& zoom, OriginType origin)
{
    f32 zoomFactor = PERCENT_TO_UNIT(zoom);
    mat4 projection = glm::ortho(0.0f, self->size.x, 0.0f, self->size.y, -1.0f, 1.0f);
    mat4 view = mat4{1.0f};
    
    switch (origin)
    {
        case ORIGIN_TOP_LEFT:
            view = glm::translate(view, vec3(pan, 0.0f));
            break;
        default:
            view = glm::translate(view, vec3((self->size * 0.5f) + pan, 0.0f));
            break;
    }

    view = glm::scale(view, vec3(zoomFactor, zoomFactor, 1.0f));
    
    return projection * view;
}

void canvas_clear(vec4& color)
{
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void canvas_viewport_set(Canvas* self)
{
    glViewport(0, 0, (s32)self->size.x, (s32)self->size.y);
}

void canvas_texture_set(Canvas* self)
{
    static vec2 previousSize = {-1, -1};

    if (previousSize != self->size)
    {
        _canvas_texture_init(self, self->size);
        previousSize = self->size;
    }
}

void canvas_grid_draw(Canvas* self, GLuint& shader, mat4& transform, f32& zoom, ivec2& size, ivec2& offset, vec4& color)
{
    if (size.x <= 0 || size.y <= 0)
        return; // avoid div-by-zero

    std::vector<f32> vertices;

    vec2 gridSize = self->size * (PERCENT_TO_UNIT(CANVAS_ZOOM_MAX - zoom));

    // First visible vertical line <= 0
    s32 startX = -(offset.x % size.x);
    if (startX > 0) startX -= size.x;

    for (s32 x = startX; x <= gridSize.x; x += size.x)
    {
        vertices.push_back((f32)x);
        vertices.push_back(0.0f);
        vertices.push_back((f32)x);
        vertices.push_back((f32)gridSize.y);
    }

    // First visible horizontal line <= 0
    s32 startY = -(offset.y % size.y);
    if (startY > 0) startY -= size.y;

    for (s32 y = startY; y <= gridSize.y; y += size.y)
    {
        vertices.push_back(0.0f);
        vertices.push_back((f32)y);
        vertices.push_back((f32)gridSize.x);
        vertices.push_back((f32)y);
    }

    s32 vertexCount = (s32)vertices.size() / 2;

    if (vertexCount == 0)
        return;

    glBindVertexArray(self->gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, self->gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(f32), vertices.data(), GL_DYNAMIC_DRAW);

    glUseProgram(shader);
    glBindVertexArray(self->gridVAO);
    glUniformMatrix4fv(glGetUniformLocation(shader, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(transform));
    glUniform4f(glGetUniformLocation(shader, SHADER_UNIFORM_COLOR), color.r, color.g, color.b, color.a);
    glDrawArrays(GL_LINES, 0, vertexCount);

    glBindVertexArray(0);
    glUseProgram(0);
}

void canvas_texture_draw(Canvas* self, GLuint& shader, GLuint& texture, mat4& transform, const f32* vertices, vec4 tint, vec3 colorOffset)
{
    glUseProgram(shader);
                
    glBindVertexArray(self->textureVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GL_UV_VERTICES), vertices, GL_DYNAMIC_DRAW);
    
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

void canvas_rect_draw(Canvas* self, const GLuint& shader, const mat4& transform, const vec4& color)
{
    glUseProgram(shader);

    glBindVertexArray(self->rectVAO);

    glUniformMatrix4fv(glGetUniformLocation(shader, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(transform));
    glUniform4fv(glGetUniformLocation(shader, SHADER_UNIFORM_COLOR), 1, value_ptr(color));

    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);
}

void canvas_rect_dotted_draw(Canvas* self, const GLuint& shader, const mat4& transform, const vec4& color)
{
    glUseProgram(shader);

    glBindVertexArray(self->rectVAO);

    glUniformMatrix4fv(glGetUniformLocation(shader, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(transform));
    glUniform4fv(glGetUniformLocation(shader, SHADER_UNIFORM_COLOR), 1, value_ptr(color));

    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);
}

void canvas_axes_draw(Canvas* self, GLuint& shader, mat4& transform, vec4& color)
{
    glUseProgram(shader);
    glBindVertexArray(self->axisVAO);
    glUniformMatrix4fv(glGetUniformLocation(shader, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(transform));
    glUniform4f(glGetUniformLocation(shader, SHADER_UNIFORM_COLOR), color.r, color.g, color.b, color.a);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);
}

void canvas_bind(Canvas* self)
{
    glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);
}

void canvas_unbind(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void canvas_free(Canvas* self)
{
    _canvas_texture_free(self);
}

mat4 canvas_mvp_get(mat4& transform, vec2 size, vec2 position, vec2 pivot, f32 rotation, vec2 scale, vec2 pivotAlt, f32 rotationAlt)
{
    vec2 scaleAbsolute  = glm::abs(scale);
    vec2 scaleSign = glm::sign(scale);
    f32 usedSign = (scaleSign.x * scaleSign.y) < 0.0f ? -1.0f : 1.0f;

    vec2 sizeScaled = size * scaleAbsolute;
    vec2 pivotScaled  = pivot * scaleAbsolute;
    vec2 pivotAltScaled = pivotAlt * scaleAbsolute;

    vec2 pivotAltMirrored = pivotScaled + (pivotAltScaled - pivotScaled) * scaleSign;

    mat4 model = glm::translate(mat4(1.0f), vec3(position - pivotScaled, 0.0f));
    model = glm::translate(model, vec3(pivotScaled, 0.0f));
    model = glm::scale(model, vec3(scaleSign, 1.0f));
    model = glm::rotate(model, glm::radians(rotation) * usedSign, vec3(0,0,1));
    model = glm::translate(model, vec3(-pivotScaled, 0.0f));
    model = glm::translate(model, vec3(pivotAltMirrored, 0.0f));
    model = glm::rotate(model, glm::radians(rotationAlt) * usedSign, vec3(0,0,1));
    model = glm::translate(model, vec3(-pivotAltMirrored, 0.0f));
    model = glm::scale(model, vec3(sizeScaled, 1.0f));

    return transform * model;
}

/*
mat4 canvas_mvp_get(mat4& transform, vec2 size, vec2 position, vec2 pivot, f32 rotation, vec2 scale, vec2 pivotAlt, f32 rotationAlt)
{
    vec2 scaleAbsolute = abs(scale);
    vec2 signScale  = glm::sign(scale); 
    vec2 pivotScaled = pivot * scaleAbsolute;
    vec2 pivotAltScaled = pivotAlt * scaleAbsolute;
    vec2 sizeScaled = size * scaleAbsolute;

    mat4 model = glm::translate(mat4(1.0f), vec3(position - pivotScaled, 0.0f));

    model = glm::translate(model, vec3(pivotScaled, 0.0f));
    model = glm::scale(model, vec3(signScale, 1.0f)); // Flip
    model = glm::rotate(model, radians(rotation), vec3(0,0,1));
    model = glm::translate(model, vec3(-pivotScaled, 0.0f));
    
    model = glm::translate(model, vec3(pivotAltScaled, 0.0f));
    model = glm::rotate(model, radians(rotationAlt), vec3(0,0,1));
    model = glm::translate(model, vec3(-pivotAltScaled, 0.0f));

    model = glm::scale(model, vec3(sizeScaled, 1.0f));

    return transform * model;
}
*/