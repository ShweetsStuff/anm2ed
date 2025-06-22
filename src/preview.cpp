#include "preview.h"

static void _preview_axis_set(Preview* self);
static void _preview_grid_set(Preview* self);

static void
_preview_axis_set(Preview* self)
{
    glBindVertexArray(self->axisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, self->axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(PREVIEW_AXIS_VERTICES), PREVIEW_AXIS_VERTICES, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

static void
_preview_grid_set(Preview* self)
{
    std::vector<f32> vertices;

    s32 verticalLineCount = PREVIEW_SIZE.x / self->gridSize.x;
    s32 horizontalLineCount = PREVIEW_SIZE.y / self->gridSize.y;

    /* Vertical */
    for (s32 i = 0; i <= verticalLineCount; i++)
    {
        s32 x = i * self->gridSize.x;
        f32 normX = (2.0f * x) / PREVIEW_SIZE.x - 1.0f;

        vertices.push_back(normX);
        vertices.push_back(-1.0f);
        vertices.push_back(normX);
        vertices.push_back(1.0f);
    }

    /* Horizontal */
    for (s32 i = 0; i <= horizontalLineCount; i++)
    {
        s32 y = i * self->gridSize.y;
        f32 normY = (2.0f * y) / PREVIEW_SIZE.y - 1.0f;

        vertices.push_back(-1.0f);
        vertices.push_back(normY);
        vertices.push_back(1.0f);
        vertices.push_back(normY);
    }

    self->gridVertexCount = (s32)vertices.size();
    
    glBindVertexArray(self->gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, self->gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);
}

void
preview_init(Preview* self, Resources* resources, Input* input)
{
    self->resources = resources;
    self->input = input;

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

    glGenVertexArrays(1, &self->axisVAO);
    glGenBuffers(1, &self->axisVBO);

    glGenVertexArrays(1, &self->gridVAO);
    glGenBuffers(1, &self->gridVBO);

    _preview_axis_set(self);
    _preview_grid_set(self);
}

void
preview_tick(Preview* self)
{
    self->zoom = CLAMP(self->zoom, PREVIEW_ZOOM_MIN, PREVIEW_ZOOM_MAX);
    self->oldGridSize = self->gridSize;
}

void
preview_draw(Preview* self)
{
    GLuint shader = self->resources->shaders[SHADER];
    float zoomFactor = self->zoom / 100.0f;

    /* Convert pan to pixels */
    glm::vec2 ndcPan = glm::vec2(
    self->pan.x / (PREVIEW_SIZE.x / 2.0f),
    -self->pan.y / (PREVIEW_SIZE.y / 2.0f)
    );

    /* Transformation matrix */
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(ndcPan, 0.0f));
    transform = glm::scale(transform, glm::vec3(zoomFactor, zoomFactor, 1.0f));
    
    glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);
    glViewport(0, 0, PREVIEW_SIZE.x, PREVIEW_SIZE.y);

    glClearColor
    (
        self->backgroundColor.r, 
        self->backgroundColor.g, 
        self->backgroundColor.b, 
        self->backgroundColor.a
    ); 
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, (f32*)value_ptr(transform));

    if (self->isGrid)
    {
        if (self->gridSize != self->oldGridSize)
            _preview_grid_set(self);

        glBindVertexArray(self->gridVAO);
        
        glUniform4f
        (
            glGetUniformLocation(shader, SHADER_UNIFORM_COLOR),
            self->gridColor.r, self->gridColor.g, self->gridColor.b, self->gridColor.a
        );

        glDrawArrays(GL_LINES, 0, self->gridVertexCount);
    
        glBindVertexArray(0);
    }

    if (self->isAxis)
    {
        glBindVertexArray(self->axisVAO);

        /* Axes */
        glUniform4f
        (
            glGetUniformLocation(shader, SHADER_UNIFORM_COLOR), 
            self->axisColor.r, self->axisColor.g, self->axisColor.b, self->axisColor.a
        );
        glDrawArrays(GL_LINES, 0, 2);

        glUniform4f
        (
            glGetUniformLocation(shader, SHADER_UNIFORM_COLOR), 
            self->axisColor.r, self->axisColor.g, self->axisColor.b, self->axisColor.a
        );
        glDrawArrays(GL_LINES, 2, 2);

        glBindVertexArray(0);
    }
    glUseProgram(0);
   
    glBindFramebuffer(GL_FRAMEBUFFER, 0); 

}

void
preview_free(Preview* self)
{
    glDeleteTextures(1, &self->texture);
    glDeleteFramebuffers(1, &self->fbo);
    glDeleteRenderbuffers(1, &self->rbo);

    memset(self, '\0', sizeof(Preview));
}