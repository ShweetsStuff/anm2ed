// Handles the rendering of the spritesheet editor

#include "editor.h"

static s32 _editor_grid_set(Editor* self)
{
    std::vector<f32> vertices;

    s32 verticalLineCount = (s32)(EDITOR_SIZE.x / MIN(self->settings->editorGridSizeX, EDITOR_GRID_MIN));
    s32 horizontalLineCount = (s32)(EDITOR_SIZE.y / MIN(self->settings->editorGridSizeY, EDITOR_GRID_MIN));

    // Vertical
    for (s32 i = 0; i <= verticalLineCount; i++)
    {
        s32 x = i * self->settings->editorGridSizeX - self->settings->editorGridOffsetX;
        f32 normX = (2.0f * x) / EDITOR_SIZE.x - 1.0f;

        vertices.push_back(normX);
        vertices.push_back(-1.0f);
        vertices.push_back(normX);
        vertices.push_back(1.0f);
    }

    // Horizontal
    for (s32 i = 0; i <= horizontalLineCount; i++)
    {
        s32 y = i * self->settings->editorGridSizeY - self->settings->editorGridOffsetY;
        f32 normY = (2.0f * y) / EDITOR_SIZE.y - 1.0f;

        vertices.push_back(-1.0f);
        vertices.push_back(normY);
        vertices.push_back(1.0f);
        vertices.push_back(normY);
    }

    glBindVertexArray(self->gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, self->gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(f32), vertices.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);

    return (s32)vertices.size();
}

void editor_init(Editor* self, Anm2* anm2, Anm2Reference* reference, Resources* resources, Settings* settings)
{
    self->anm2 = anm2;
    self->reference = reference;
    self->resources = resources;
    self->settings = settings;

    // Framebuffer + texture
    glGenFramebuffers(1, &self->fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);

    glGenTextures(1, &self->texture);
    glBindTexture(GL_TEXTURE_2D, self->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (s32)EDITOR_SIZE.x, (s32)EDITOR_SIZE.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->texture, 0);

    glGenRenderbuffers(1, &self->rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, self->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (s32)EDITOR_SIZE.x, (s32)EDITOR_SIZE.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, self->rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Grid
    glGenVertexArrays(1, &self->gridVAO);
    glGenBuffers(1, &self->gridVBO);

    // Border
    glGenVertexArrays(1, &self->borderVAO);
    glGenBuffers(1, &self->borderVBO);

    glBindVertexArray(self->borderVAO);

    glBindBuffer(GL_ARRAY_BUFFER, self->borderVBO);
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GL_TEXTURE_INDICES), GL_TEXTURE_INDICES, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)0);

    // UV position attribute
    glEnableVertexAttribArray(1); 
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)(2 * sizeof(f32)));

    glBindVertexArray(0);

    _editor_grid_set(self);
}

void editor_draw(Editor* self)
{
    GLuint shaderLine = self->resources->shaders[SHADER_LINE];
    GLuint shaderLineDotted = self->resources->shaders[SHADER_LINE_DOTTED];
    GLuint shaderTexture = self->resources->shaders[SHADER_TEXTURE];

    f32 zoomFactor = PERCENT_TO_UNIT(self->settings->editorZoom);
    
    // Get normalized panning
    glm::vec2 ndcPan = glm::vec2(-self->settings->editorPanX / (EDITOR_SIZE.x / 2.0f), -self->settings->editorPanY / (EDITOR_SIZE.y / 2.0f));

    glm::mat4 editorTransform = glm::translate(glm::mat4(1.0f), glm::vec3(ndcPan, 0.0f));
    editorTransform = glm::scale(editorTransform, glm::vec3(zoomFactor, zoomFactor, 1.0f));

    glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);
    glViewport(0, 0, EDITOR_SIZE.x, EDITOR_SIZE.y);

    glClearColor
    (
        self->settings->editorBackgroundColorR, 
        self->settings->editorBackgroundColorG, 
        self->settings->editorBackgroundColorB, 
        self->settings->editorBackgroundColorA
    ); 

    glClear(GL_COLOR_BUFFER_BIT);

    // Drawing the selected spritesheet
    if (self->spritesheetID > -1)
    {
        Texture* texture = &self->resources->textures[self->spritesheetID];

        glm::mat4 spritesheetTransform = editorTransform;
        glm::vec2 ndcScale = glm::vec2(texture->size.x, texture->size.y) / (EDITOR_SIZE * 0.5f);

        spritesheetTransform = glm::scale(spritesheetTransform, glm::vec3(ndcScale, 1.0f));

        glUseProgram(shaderTexture);

        glBindVertexArray(self->textureVAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GL_UV_VERTICES), GL_UV_VERTICES, GL_DYNAMIC_DRAW);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->id);
      
        glUniform1i(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TEXTURE), 0);
        glUniform4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TINT), 1, value_ptr(COLOR_OPAQUE));
        glUniform3fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_COLOR_OFFSET), 1, value_ptr(COLOR_OFFSET_NONE));
        glUniformMatrix4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(spritesheetTransform));
        
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindVertexArray(0);
        glUseProgram(0);

        // Border around the spritesheet
        if (self->settings->editorIsBorder)
        {
            glUseProgram(shaderLineDotted);

            glBindVertexArray(self->borderVAO);

            glUniformMatrix4fv(glGetUniformLocation(shaderLine, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, glm::value_ptr(spritesheetTransform));
            glUniform4fv(glGetUniformLocation(shaderLine, SHADER_UNIFORM_COLOR), 1, glm::value_ptr(EDITOR_BORDER_TINT));

            glDrawArrays(GL_LINE_LOOP, 0, 4);

            glBindVertexArray(0);
            glUseProgram(0);
        }

        Anm2Frame* frame = (Anm2Frame*)anm2_frame_from_reference(self->anm2, self->reference);

        // Drawing the frame's crop and pivot
        if (frame)
        {
            // Crop
            glm::mat4 rectTransform = editorTransform;  

            glm::vec2 rectNDCPos = frame->crop / (EDITOR_SIZE / 2.0f);  
            glm::vec2 rectNDCScale = frame->size / (EDITOR_SIZE / 2.0f);

            rectTransform = glm::translate(rectTransform, glm::vec3(rectNDCPos, 0.0f));  
            rectTransform = glm::scale(rectTransform, glm::vec3(rectNDCScale, 1.0f));

            glUseProgram(shaderLineDotted);

            glBindVertexArray(self->borderVAO);

            glUniformMatrix4fv(glGetUniformLocation(shaderLine, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, glm::value_ptr(rectTransform));
            glUniform4fv(glGetUniformLocation(shaderLine, SHADER_UNIFORM_COLOR), 1, glm::value_ptr(EDITOR_FRAME_TINT));

            glDrawArrays(GL_LINE_LOOP, 0, 4);

            glBindVertexArray(0);
            glUseProgram(0);

            // Pivot
            glm::mat4 pivotTransform = editorTransform;
            glm::vec2 pivotNDCPos = ((frame->crop + frame->pivot) - (EDITOR_PIVOT_SIZE / 2.0f)) / (EDITOR_SIZE / 2.0f);
            glm::vec2 pivotNDCScale = EDITOR_PIVOT_SIZE / (EDITOR_SIZE / 2.0f);

            pivotTransform = glm::translate(pivotTransform, glm::vec3(pivotNDCPos, 0.0f));
            pivotTransform = glm::scale(pivotTransform, glm::vec3(pivotNDCScale, 1.0f));

            glUseProgram(shaderTexture);
            
            glBindVertexArray(self->textureVAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);

            f32 vertices[] = ATLAS_UV_VERTICES(TEXTURE_PIVOT);

            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, self->resources->atlas.id);
            
            glUniform1i(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TEXTURE), 0);
            glUniform4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TINT), 1, value_ptr(EDITOR_FRAME_TINT));
            glUniform3fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_COLOR_OFFSET), 1, value_ptr(COLOR_OFFSET_NONE));
            glUniformMatrix4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(pivotTransform));
            
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glUseProgram(0); 
        }
    }

    // Grid
    if (self->settings->editorIsGrid)
    {
        static ivec2 previousGridSize = {-1, -1};
        static ivec2 previousGridOffset = {-1, -1};
        static s32 gridVertexCount = -1;

        glm::mat4 gridTransform = editorTransform;
        glm::vec2 gridNDCPos = (EDITOR_SIZE / 2.0f) / (EDITOR_SIZE / 2.0f);

        gridTransform = glm::translate(gridTransform, glm::vec3(gridNDCPos, 0.0f));

        ivec2 gridSize = ivec2(self->settings->editorGridSizeX, self->settings->editorGridSizeY);
        ivec2 gridOffset = ivec2(self->settings->editorGridOffsetX, self->settings->editorGridOffsetY);

        if (previousGridSize != gridSize || previousGridOffset != gridOffset)
        {
            gridVertexCount = _editor_grid_set(self);
            previousGridSize = gridSize;
            previousGridOffset = gridOffset;
        }

        glUseProgram(shaderLine);
        glBindVertexArray(self->gridVAO);
        glUniformMatrix4fv(glGetUniformLocation(shaderLine, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, (f32*)value_ptr(gridTransform));
        
        glUniform4f
        (
            glGetUniformLocation(shaderLine, SHADER_UNIFORM_COLOR),
            self->settings->editorGridColorR, self->settings->editorGridColorG, self->settings->editorGridColorB, self->settings->editorGridColorA
        );

        glDrawArrays(GL_LINES, 0, gridVertexCount);
    
        glBindVertexArray(0);
        glUseProgram(0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); 
}

void editor_tick(Editor* self)
{
    self->settings->editorZoom = CLAMP(self->settings->editorZoom, EDITOR_ZOOM_MIN, EDITOR_ZOOM_MAX);
}

void editor_free(Editor* self)
{
    glDeleteTextures(1, &self->texture);
    glDeleteFramebuffers(1, &self->fbo);
    glDeleteRenderbuffers(1, &self->rbo);
}