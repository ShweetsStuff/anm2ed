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
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);

    glBindVertexArray(0);
}

static void
_preview_grid_set(Preview* self)
{
    std::vector<f32> vertices;

    s32 verticalLineCount = PREVIEW_SIZE.x / MIN(self->settings->gridSizeX, PREVIEW_GRID_MIN);
    s32 horizontalLineCount = PREVIEW_SIZE.y / MIN(self->settings->gridSizeY, PREVIEW_GRID_MIN);

    /* Vertical */
    for (s32 i = 0; i <= verticalLineCount; i++)
    {
        s32 x = i * self->settings->gridSizeX - self->settings->gridOffsetX;
        f32 normX = (2.0f * x) / PREVIEW_SIZE.x - 1.0f;

        vertices.push_back(normX);
        vertices.push_back(-1.0f);
        vertices.push_back(normX);
        vertices.push_back(1.0f);
    }

    /* Horizontal */
    for (s32 i = 0; i <= horizontalLineCount; i++)
    {
        s32 y = i * self->settings->gridSizeY - self->settings->gridOffsetY;
        f32 normY = (2.0f * y) / PREVIEW_SIZE.y - 1.0f;

        vertices.push_back(-1.0f);
        vertices.push_back(normY);
        vertices.push_back(1.0f);
        vertices.push_back(normY);
    }

    self->gridVertexCount = (s32)vertices.size();
    
    glBindVertexArray(self->gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, self->gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(f32), vertices.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);
}

void
preview_init(Preview* self, Anm2* anm2, Resources* resources, Input* input, Settings* settings)
{
    self->anm2 = anm2;
    self->resources = resources;
    self->input = input;
    self->settings = settings;

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

    glGenVertexArrays(1, &self->textureVAO);
    glGenBuffers(1, &self->textureVBO);
    glGenBuffers(1, &self->textureEBO);

    glBindVertexArray(self->textureVAO);

    glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 4 * 4, NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->textureEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(PREVIEW_TEXTURE_INDICES), PREVIEW_TEXTURE_INDICES, GL_STATIC_DRAW);

    /* Position */
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)0);

    /* UV */
    glEnableVertexAttribArray(1); 
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)(2 * sizeof(f32)));

    glBindVertexArray(0);

    _preview_axis_set(self);
    _preview_grid_set(self);
}

void
preview_tick(Preview* self)
{
    self->settings->zoom = CLAMP(self->settings->zoom, PREVIEW_ZOOM_MIN, PREVIEW_ZOOM_MAX);
    self->oldGridSize = glm::vec2(self->settings->gridSizeX, self->settings->gridSizeY);
    self->oldGridOffset = glm::vec2(self->settings->gridOffsetX, self->settings->gridOffsetY);

    if (self->animationID > -1)
    {
        Anm2Animation* animation = &self->anm2->animations[self->animationID];     

        if (self->isPlaying)
        {  
            self->time += (f32)self->anm2->fps / TICK_RATE;

            if (self->time >= (f32)animation->frameNum - 1)
                self->time = 0.0f;
        }
        else
            self->time = CLAMP(self->time, 0.0f, (f32)animation->frameNum);
    }
}

void
preview_draw(Preview* self)
{
    GLuint shaderLine = self->resources->shaders[SHADER_LINE];
    GLuint shaderTexture = self->resources->shaders[SHADER_TEXTURE];
    
    f32 zoomFactor = self->settings->zoom / 100.0f;

    /* Convert pan to pixels */
    glm::vec2 ndcPan = glm::vec2(
    self->settings->panX / (PREVIEW_SIZE.x / 2.0f),
    -self->settings->panY / (PREVIEW_SIZE.y / 2.0f)
    );

    /* Transformation matrix */
    glm::mat4 previewTransform = glm::translate(glm::mat4(1.0f), glm::vec3(ndcPan, 0.0f));
    previewTransform = glm::scale(previewTransform, glm::vec3(zoomFactor, zoomFactor, 1.0f));

    glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);
    glViewport(0, 0, PREVIEW_SIZE.x, PREVIEW_SIZE.y);

    glClearColor
    (
        self->settings->backgroundColorR, 
        self->settings->backgroundColorG, 
        self->settings->backgroundColorB, 
        self->settings->backgroundColorA
    ); 
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderLine);
    glUniformMatrix4fv(glGetUniformLocation(shaderLine, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, (f32*)value_ptr(previewTransform));

    if (self->settings->isGrid)
    {
        if 
        (
            (ivec2(self->settings->gridSizeX, self->settings->gridSizeY) != self->oldGridSize) ||
            (ivec2(self->settings->gridOffsetX, self->settings->gridOffsetY) != self->oldGridOffset)
        )
            _preview_grid_set(self);

        glBindVertexArray(self->gridVAO);
        
        glUniform4f
        (
            glGetUniformLocation(shaderLine, SHADER_UNIFORM_COLOR),
            self->settings->gridColorR, self->settings->gridColorG, self->settings->gridColorB, self->settings->gridColorA
        );

        glDrawArrays(GL_LINES, 0, self->gridVertexCount);
    
        glBindVertexArray(0);
    }

    if (self->settings->isAxis)
    {
        glBindVertexArray(self->axisVAO);

        /* Axes */
        glUniform4f
        (
            glGetUniformLocation(shaderLine, SHADER_UNIFORM_COLOR), 
            self->settings->axisColorR, self->settings->axisColorG, self->settings->axisColorB, self->settings->axisColorA
        );
        glDrawArrays(GL_LINES, 0, 2);
        glUniform4f
        (
            glGetUniformLocation(shaderLine, SHADER_UNIFORM_COLOR), 
            self->settings->axisColorR, self->settings->axisColorG, self->settings->axisColorB, self->settings->axisColorA
        );
        glDrawArrays(GL_LINES, 2, 2);

        
        glBindVertexArray(0);
    }

    glUseProgram(0);
 
    glUseProgram(shaderTexture);
    
    if (self->animationID > -1)
    {
        Anm2Frame rootFrame = Anm2Frame{};
        Anm2Animation* animation = &self->anm2->animations[self->animationID];     
        bool isRootFrame = anm2_frame_from_time(self->anm2, animation, &rootFrame, ANM2_ROOT_ANIMATION, 0, self->time);

        /* Layers (Reversed) */
        for (auto & [id, layerAnimation] : animation->layerAnimations)
        {
            Anm2Layer* layer = &self->anm2->layers[id];
            Anm2Frame frame;

            Texture* texture = &self->resources->loadedTextures[layer->spritesheetID];

            if (texture->isInvalid)
                continue;

            if (!layerAnimation.isVisible || layerAnimation.frames.size() <= 0)
                continue;

            if (!anm2_frame_from_time(self->anm2, animation, &frame, ANM2_LAYER_ANIMATION, id, self->time))
                continue;

            if (!frame.isVisible)
                continue;

            glm::mat4 layerTransform = previewTransform;
            glm::vec2 previewSize = glm::vec2(PREVIEW_SIZE);

            glm::vec2 scale = self->settings->isRootTransform ?
                (frame.scale / 100.0f) * (rootFrame.scale / 100.0f) :
                (frame.scale / 100.0f);

            glm::vec2 position = self->settings->isRootTransform ?
                (frame.position + rootFrame.position) :
                frame.position;

            glm::vec2 scaledSize = frame.size * scale;
            glm::vec2 scaledPivot = frame.pivot * scale;

            glm::vec2 ndcPos = position / (previewSize / 2.0f);
            glm::vec2 ndcPivotOffset = scaledPivot / (previewSize * 0.5f);
            glm::vec2 ndcScale = scaledSize / (previewSize * 0.5f);

            layerTransform = glm::translate(layerTransform, glm::vec3(ndcPos - ndcPivotOffset, 0.0f));
            layerTransform = glm::translate(layerTransform, glm::vec3(ndcPivotOffset, 0.0f));
            layerTransform = glm::rotate(layerTransform, glm::radians(frame.rotation), glm::vec3(0, 0, 1));
            layerTransform = glm::translate(layerTransform, glm::vec3(-ndcPivotOffset, 0.0f));
            layerTransform = glm::scale(layerTransform, glm::vec3(ndcScale, 1.0f));
                        
            glm::vec2 uvMin = frame.crop / glm::vec2(texture->size);
            glm::vec2 uvMax = (frame.crop + frame.size) / glm::vec2(texture->size);

            f32 vertices[] = {
                0, 0, uvMin.x, uvMin.y,
                1, 0, uvMax.x, uvMin.y,
                1, 1, uvMax.x, uvMax.y,
                0, 1, uvMin.x, uvMax.y
            };
                        
            glBindVertexArray(self->textureVAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture->handle);
            glUniform1i(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TEXTURE), 0);
            glUniform4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TINT), 1, (f32*)value_ptr(frame.tintRGBA));
            glUniform3fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_COLOR_OFFSET), 1, (f32*)value_ptr(frame.offsetRGB));
            glUniformMatrix4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(layerTransform));
            
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        /* Root */
        if 
        (isRootFrame && animation->rootAnimation.isVisible && rootFrame.isVisible)
        {
            glm::mat4 rootTransform = previewTransform;
            glm::vec2 previewSize = {(f32)PREVIEW_SIZE.x, (f32)PREVIEW_SIZE.y};
            glm::vec2 ndcPos = (rootFrame.position - (PREVIEW_TARGET_SIZE / 2.0f)) / (previewSize / 2.0f);
            glm::vec2 ndcScale = PREVIEW_TARGET_SIZE / (previewSize * 0.5f);

            rootTransform = glm::translate(rootTransform, glm::vec3(ndcPos, 0.0f));
            rootTransform = glm::scale(rootTransform, glm::vec3(ndcScale, 1.0f));

            f32 vertices[] = 
            {
                0, 0, 0.0f, 0.0f,
                1, 0, 1.0f, 0.0f,
                1, 1, 1.0f, 1.0f,
                0, 1, 0.0f, 1.0f
            };

            glBindVertexArray(self->textureVAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, self->resources->textures[TEXTURE_TARGET].handle);
            glUniform1i(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TEXTURE), 0);
            glUniform4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TINT), 1, value_ptr(PREVIEW_ROOT_TINT));
            glUniform3fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_COLOR_OFFSET), 1, value_ptr(PREVIEW_ROOT_COLOR_OFFSET));
            glUniformMatrix4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(rootTransform));
            
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        /* Pivots */
        if (self->settings->isShowPivot)
        {
		    for (auto it = animation->layerAnimations.rbegin(); it != animation->layerAnimations.rend(); it++)
            {
                s32 id = it->first;
                Anm2LayerAnimation layerAnimation = it->second;

                Anm2Layer* layer = &self->anm2->layers[id];
                Anm2Frame frame;

                Texture* texture = &self->resources->loadedTextures[layer->spritesheetID];

                if (!layerAnimation.isVisible || layerAnimation.frames.size() <= 0)
                    continue;

                if (!anm2_frame_from_time(self->anm2, animation, &frame, ANM2_LAYER_ANIMATION, id, self->time))
                    continue;

                if (!frame.isVisible)
                    continue;


                glm::vec2 scale = self->settings->isRootTransform ?
                    (frame.scale / 100.0f) * (rootFrame.scale / 100.0f) :
                    (frame.scale / 100.0f);

                glm::vec2 position = self->settings->isRootTransform ?
                    (frame.position + rootFrame.position) :
                    frame.position;
                    
                glm::mat4 pivotTransform = previewTransform;
                glm::vec2 previewSize = {(f32)PREVIEW_SIZE.x, (f32)PREVIEW_SIZE.y};
                glm::vec2 scaledSize = PREVIEW_PIVOT_SIZE * scale;
                glm::vec2 ndcPos = (position - (PREVIEW_PIVOT_SIZE / 2.0f)) / (previewSize / 2.0f);
                glm::vec2 ndcScale = scaledSize / (previewSize * 0.5f);

                pivotTransform = glm::translate(pivotTransform, glm::vec3(ndcPos, 0.0f));
                pivotTransform = glm::scale(pivotTransform, glm::vec3(ndcScale, 1.0f));

                f32 vertices[] = 
                {
                    0, 0, 0.0f, 0.0f,
                    1, 0, 1.0f, 0.0f,
                    1, 1, 1.0f, 1.0f,
                    0, 1, 0.0f, 1.0f
                };

                glBindVertexArray(self->textureVAO);
                
                glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, self->resources->textures[TEXTURE_PIVOT].handle);
                glUniform1i(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TEXTURE), 0);
                glUniform4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TINT), 1, value_ptr(PREVIEW_PIVOT_TINT));
                glUniform3fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_COLOR_OFFSET), 1, value_ptr(PREVIEW_PIVOT_COLOR_OFFSET));
                glUniformMatrix4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(pivotTransform));
                
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }

        /* Nulls */
        for (auto & [id, nullAnimation] : animation->nullAnimations)
        {
            Anm2Frame frame;

            if (!nullAnimation.isVisible || nullAnimation.frames.size() <= 0)
                continue;

            if (!anm2_frame_from_time(self->anm2, animation, &frame, ANM2_NULL_ANIMATION, id, self->time))
                continue;

            if (!frame.isVisible)
                continue;

            glm::mat4 nullTransform = previewTransform;
            glm::vec2 previewSize = {(f32)PREVIEW_SIZE.x, (f32)PREVIEW_SIZE.y};
            
            glm::vec2 pos = self->settings->isRootTransform ? 
            frame.position + (rootFrame.position) - (PREVIEW_TARGET_SIZE / 2.0f): 
            frame.position - (PREVIEW_TARGET_SIZE / 2.0f);

            glm::vec2 ndcPos = pos / (previewSize / 2.0f);
            glm::vec2 ndcScale = PREVIEW_TARGET_SIZE / (previewSize * 0.5f);

            nullTransform = glm::translate(nullTransform, glm::vec3(ndcPos, 0.0f));
            nullTransform = glm::scale(nullTransform, glm::vec3(ndcScale, 1.0f));

           f32 vertices[] = 
            {
                0, 0, 0.0f, 0.0f,
                1, 0, 1.0f, 0.0f,
                1, 1, 1.0f, 1.0f,
                0, 1, 0.0f, 1.0f
            };

            glBindVertexArray(self->textureVAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, self->resources->textures[TEXTURE_TARGET].handle);
            glUniform1i(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TEXTURE), 0);
            glUniform4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TINT), 1, value_ptr(PREVIEW_NULL_TINT));
            glUniform3fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_COLOR_OFFSET), 1, value_ptr(PREVIEW_NULL_COLOR_OFFSET));
            glUniformMatrix4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(nullTransform));
            
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            
            glBindTexture(GL_TEXTURE_2D, 0);
        }
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