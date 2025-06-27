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

    s32 verticalLineCount = PREVIEW_SIZE.x / MIN(self->settings->previewGridSizeX, PREVIEW_GRID_MIN);
    s32 horizontalLineCount = PREVIEW_SIZE.y / MIN(self->settings->previewGridSizeY, PREVIEW_GRID_MIN);

    /* Vertical */
    for (s32 i = 0; i <= verticalLineCount; i++)
    {
        s32 x = i * self->settings->previewGridSizeX - self->settings->previewGridOffsetX;
        f32 normX = (2.0f * x) / PREVIEW_SIZE.x - 1.0f;

        vertices.push_back(normX);
        vertices.push_back(-1.0f);
        vertices.push_back(normX);
        vertices.push_back(1.0f);
    }

    /* Horizontal */
    for (s32 i = 0; i <= horizontalLineCount; i++)
    {
        s32 y = i * self->settings->previewGridSizeY - self->settings->previewGridOffsetY;
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

    /* Framebuffer + texture */
    glGenFramebuffers(1, &self->fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);

    glGenTextures(1, &self->texture);
    glBindTexture(GL_TEXTURE_2D, self->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (s32)PREVIEW_SIZE.x, (s32)PREVIEW_SIZE.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->texture, 0);

    glGenRenderbuffers(1, &self->rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, self->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (s32)PREVIEW_SIZE.x, (s32)PREVIEW_SIZE.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, self->rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* Axis */
    glGenVertexArrays(1, &self->axisVAO);
    glGenBuffers(1, &self->axisVBO);

    /* Grid */
    glGenVertexArrays(1, &self->gridVAO);
    glGenBuffers(1, &self->gridVBO);

    /* Rect */
    glGenVertexArrays(1, &self->rectVAO);
    glGenBuffers(1, &self->rectVBO);

    glBindVertexArray(self->rectVAO);

    glBindBuffer(GL_ARRAY_BUFFER, self->rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GL_VERTICES), GL_VERTICES, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);

    /* Texture */
    glGenVertexArrays(1, &self->textureVAO);
    glGenBuffers(1, &self->textureVBO);
    glGenBuffers(1, &self->textureEBO);

    glBindVertexArray(self->textureVAO);

    glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 4 * 4, NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->textureEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GL_TEXTURE_INDICES), GL_TEXTURE_INDICES, GL_STATIC_DRAW);

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
    self->settings->previewZoom = CLAMP(self->settings->previewZoom, PREVIEW_ZOOM_MIN, PREVIEW_ZOOM_MAX);
    
    self->oldGridSize = glm::vec2(self->settings->previewGridSizeX, self->settings->previewGridSizeY);
    self->oldGridOffset = glm::vec2(self->settings->previewGridOffsetX, self->settings->previewGridOffsetY);

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
    
    f32 zoomFactor = self->settings->previewZoom / 100.0f;
    glm::vec2 ndcPan = glm::vec2(-self->settings->previewPanX / (PREVIEW_SIZE.x / 2.0f), -self->settings->previewPanY / (PREVIEW_SIZE.y / 2.0f));
    glm::mat4 previewTransform = glm::translate(glm::mat4(1.0f), glm::vec3(ndcPan, 0.0f));
    previewTransform = glm::scale(previewTransform, glm::vec3(zoomFactor, zoomFactor, 1.0f));

    glBindFramebuffer(GL_FRAMEBUFFER, self->fbo);
    glViewport(0, 0, (s32)PREVIEW_SIZE.x, (s32)PREVIEW_SIZE.y);

    glClearColor
    (
        self->settings->previewBackgroundColorR, 
        self->settings->previewBackgroundColorG, 
        self->settings->previewBackgroundColorB, 
        self->settings->previewBackgroundColorA
    ); 
    glClear(GL_COLOR_BUFFER_BIT);

    /* Grid */
    if (self->settings->previewIsGrid)
    {
        if 
        (
            (ivec2(self->settings->previewGridSizeX, self->settings->previewGridSizeY) != self->oldGridSize) ||
            (ivec2(self->settings->previewGridOffsetX, self->settings->previewGridOffsetY) != self->oldGridOffset)
        )
            _preview_grid_set(self);

        glUseProgram(shaderLine);
        glBindVertexArray(self->gridVAO);
        glUniformMatrix4fv(glGetUniformLocation(shaderLine, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, (f32*)value_ptr(previewTransform));
        
        glUniform4f
        (
            glGetUniformLocation(shaderLine, SHADER_UNIFORM_COLOR),
            self->settings->previewGridColorR, self->settings->previewGridColorG, self->settings->previewGridColorB, self->settings->previewGridColorA
        );

        glDrawArrays(GL_LINES, 0, self->gridVertexCount);
    
        glBindVertexArray(0);
        glUseProgram(0);
    }

    /* Axes */
    if (self->settings->previewIsAxis)
    {
        glUseProgram(shaderLine);
        glBindVertexArray(self->axisVAO);

        glUniformMatrix4fv(glGetUniformLocation(shaderLine, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, (f32*)value_ptr(previewTransform));

        glUniform4f
        (
            glGetUniformLocation(shaderLine, SHADER_UNIFORM_COLOR), 
            self->settings->previewAxisColorR, self->settings->previewAxisColorG, self->settings->previewAxisColorB, self->settings->previewAxisColorA
        );

        glDrawArrays(GL_LINES, 0, 2);
        
        glUniform4f
        (
            glGetUniformLocation(shaderLine, SHADER_UNIFORM_COLOR), 
            self->settings->previewAxisColorR, self->settings->previewAxisColorG, self->settings->previewAxisColorB, self->settings->previewAxisColorA
        );

        glDrawArrays(GL_LINES, 2, 2);


        glBindVertexArray(0);
        glUseProgram(0);
    }

    /* Animation */
    if (self->animationID > -1)
    {
        Anm2Frame rootFrame = Anm2Frame{};
        Anm2Animation* animation = &self->anm2->animations[self->animationID];     
        bool isRootFrame = anm2_frame_from_time(self->anm2, animation, &rootFrame, ANM2_ROOT_ANIMATION, 0, self->time);

        /* Layers (Reversed) */
        for (auto & [id, layerAnimation] : animation->layerAnimations)
        {
            if (!layerAnimation.isVisible || layerAnimation.frames.size() <= 0)
                continue;

            Anm2Layer* layer = &self->anm2->layers[id];
            Anm2Frame frame = layerAnimation.frames[0];

            anm2_frame_from_time(self->anm2, animation, &frame, ANM2_LAYER_ANIMATION, id, self->time);

            if (!frame.isVisible)
                continue;

            Texture* texture = &self->resources->textures[layer->spritesheetID];

            if (texture->isInvalid)
                continue;

            glm::mat4 layerTransform = previewTransform;

            glm::vec2 position = self->settings->previewIsRootTransform ? (frame.position + rootFrame.position) : frame.position;
            glm::vec2 scale = self->settings->previewIsRootTransform ? (frame.scale / 100.0f) * (rootFrame.scale / 100.0f) : (frame.scale / 100.0f);
            glm::vec2 ndcPos = position / (PREVIEW_SIZE / 2.0f);
            glm::vec2 ndcPivotOffset = (frame.pivot * scale) / (PREVIEW_SIZE / 2.0f);
            glm::vec2 ndcScale = (frame.size * scale) / (PREVIEW_SIZE / 2.0f);
            f32 rotation = frame.rotation;

            layerTransform = glm::translate(layerTransform, glm::vec3(ndcPos - ndcPivotOffset, 0.0f));
            layerTransform = glm::translate(layerTransform, glm::vec3(ndcPivotOffset, 0.0f));
            layerTransform = glm::rotate(layerTransform, glm::radians(rotation), glm::vec3(0, 0, 1));
            layerTransform = glm::translate(layerTransform, glm::vec3(-ndcPivotOffset, 0.0f));
            layerTransform = glm::scale(layerTransform, glm::vec3(ndcScale, 1.0f));


            glm::vec2 uvMin = frame.crop / glm::vec2(texture->size);
            glm::vec2 uvMax = (frame.crop + frame.size) / glm::vec2(texture->size);

            f32 vertices[] = UV_VERTICES(uvMin, uvMax);

            glUseProgram(shaderTexture);
                        
            glBindVertexArray(self->textureVAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture->id);

            glUniform1i(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TEXTURE), 0);
            glUniform4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TINT), 1, (f32*)value_ptr(frame.tintRGBA));
            glUniform3fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_COLOR_OFFSET), 1, (f32*)value_ptr(frame.offsetRGB));
            glUniformMatrix4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(layerTransform));
            
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glUseProgram(0);
        }

        /* Root */
        if 
        (isRootFrame && animation->rootAnimation.isVisible && rootFrame.isVisible)
        {
            glm::mat4 rootTransform = previewTransform;
            glm::vec2 ndcPos = (rootFrame.position - (ATLAS_SIZES[TEXTURE_TARGET] / 2.0f)) / (PREVIEW_SIZE / 2.0f);
            glm::vec2 ndcScale = ATLAS_SIZES[TEXTURE_TARGET] / (PREVIEW_SIZE / 2.0f);
            glm::vec2 ndcPivot = (-ATLAS_SIZES[TEXTURE_TARGET] / 2.0f) / (PREVIEW_SIZE / 2.0f);

            rootTransform = glm::translate(rootTransform, glm::vec3(ndcPos, 0.0f));
            rootTransform = glm::rotate(rootTransform, glm::radians(rootFrame.rotation), glm::vec3(0, 0, 1));
            rootTransform = glm::scale(rootTransform, glm::vec3(ndcScale, 1.0f));

            f32 vertices[] = ATLAS_UV_VERTICES(TEXTURE_TARGET);
            
            glUseProgram(shaderTexture);
            
            glBindVertexArray(self->textureVAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, self->resources->atlas.id);
            
            glUniform1i(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TEXTURE), 0);
            glUniform4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TINT), 1, value_ptr(PREVIEW_ROOT_TINT));
            glUniform3fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_COLOR_OFFSET), 1, value_ptr(COLOR_OFFSET_NONE));
            glUniformMatrix4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(rootTransform));
            
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glUseProgram(0);
        }

        /* Pivots */
        if (self->settings->previewIsShowPivot)
        {
		    for (auto it = animation->layerAnimations.rbegin(); it != animation->layerAnimations.rend(); it++)
            {
                s32 id = it->first;
                Anm2LayerAnimation layerAnimation = it->second;

                if (!layerAnimation.isVisible || layerAnimation.frames.size() <= 0)
                    continue;
                
                Anm2Frame frame = layerAnimation.frames[0];

                anm2_frame_from_time(self->anm2, animation, &frame, ANM2_LAYER_ANIMATION, id, self->time);

                if (!frame.isVisible)
                    continue;

                glm::mat4 pivotTransform = previewTransform;
                    
                glm::vec2 position = self->settings->previewIsRootTransform ? (frame.position + rootFrame.position) : frame.position;
                
                glm::vec2 ndcPos = (position - (ATLAS_SIZES[TEXTURE_PIVOT] / 2.0f)) / (PREVIEW_SIZE / 2.0f);
                glm::vec2 ndcScale = ATLAS_SIZES[TEXTURE_PIVOT] / (PREVIEW_SIZE / 2.0f);

                pivotTransform = glm::translate(pivotTransform, glm::vec3(ndcPos, 0.0f));
                pivotTransform = glm::scale(pivotTransform, glm::vec3(ndcScale, 1.0f));

                f32 vertices[] = ATLAS_UV_VERTICES(TEXTURE_PIVOT);
                
                glUseProgram(shaderTexture);
                
                glBindVertexArray(self->textureVAO);
                
                glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, self->resources->atlas.id);
                
                glUniform1i(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TEXTURE), 0);
                glUniform4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TINT), 1, value_ptr(PREVIEW_PIVOT_TINT));
                glUniform3fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_COLOR_OFFSET), 1, value_ptr(COLOR_OFFSET_NONE));
                glUniformMatrix4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(pivotTransform));
                
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
                glUseProgram(0); 
            }
        }

        /* Nulls */
        for (auto & [id, nullAnimation] : animation->nullAnimations)
        {
            if (!nullAnimation.isVisible || nullAnimation.frames.size() <= 0)
                continue;

            Anm2Frame frame = nullAnimation.frames[0];
            
            anm2_frame_from_time(self->anm2, animation, &frame, ANM2_NULL_ANIMATION, id, self->time);

            if (!frame.isVisible)
                continue;

            Anm2Null* null = NULL;

            null = &self->anm2->nulls[id];

            glm::mat4 nullTransform = previewTransform;

            TextureType textureType = null->isShowRect ? TEXTURE_SQUARE : TEXTURE_TARGET;
            glm::vec2 size = null->isShowRect ? PREVIEW_POINT_SIZE : ATLAS_SIZES[TEXTURE_TARGET];
            glm::vec2 pos = self->settings->previewIsRootTransform ? frame.position + (rootFrame.position) - (size / 2.0f) : frame.position - (size / 2.0f);

            glm::vec2 ndcPos = pos / (PREVIEW_SIZE / 2.0f);
            glm::vec2 ndcScale = size / (PREVIEW_SIZE / 2.0f);

            nullTransform = glm::translate(nullTransform, glm::vec3(ndcPos, 0.0f));
            nullTransform = glm::scale(nullTransform, glm::vec3(ndcScale, 1.0f));

            f32 vertices[] = ATLAS_UV_VERTICES(textureType);
            
            glUseProgram(shaderTexture);

            glBindVertexArray(self->textureVAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, self->textureVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, self->resources->atlas.id);
            
            glUniform1i(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TEXTURE), 0);
            glUniform4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TINT), 1, value_ptr(PREVIEW_NULL_TINT));
            glUniform3fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_COLOR_OFFSET), 1, value_ptr(COLOR_OFFSET_NONE));
            glUniformMatrix4fv(glGetUniformLocation(shaderTexture, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, value_ptr(nullTransform));
            
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glUseProgram(0);

            if (null->isShowRect)
            {
                glm::mat4 rectTransform = previewTransform;  

                glm::vec2 rectPos = pos - (PREVIEW_NULL_RECT_SIZE / 2.0f);  
                glm::vec2 rectNDCPos = rectPos / (PREVIEW_SIZE / 2.0f);  
                glm::vec2 rectNDCScale = PREVIEW_NULL_RECT_SIZE / (PREVIEW_SIZE / 2.0f);  

                rectTransform = glm::translate(rectTransform, glm::vec3(rectNDCPos, 0.0f));  
                rectTransform = glm::scale(rectTransform, glm::vec3(rectNDCScale, 1.0f));  

                glUseProgram(shaderLine);

                glBindVertexArray(self->rectVAO);

                glUniformMatrix4fv(glGetUniformLocation(shaderLine, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, glm::value_ptr(rectTransform));

                glUniform4fv(glGetUniformLocation(shaderLine, SHADER_UNIFORM_COLOR), 1, glm::value_ptr(PREVIEW_NULL_TINT));

                glDrawArrays(GL_LINE_LOOP, 0, 4);

                glBindVertexArray(0);
                glUseProgram(0);
            }
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
}