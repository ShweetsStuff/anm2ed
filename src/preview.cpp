#include "preview.h"

static void _preview_axis_set(Preview* self);
static s32 _preview_grid_set(Preview* self);

// Sets the preview's axis (lines across x/y)
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

// Sets and returns the grid's vertices 
static s32
_preview_grid_set(Preview* self)
{
    std::vector<f32> vertices;

    s32 verticalLineCount = (s32)(PREVIEW_SIZE.x / MIN(self->settings->previewGridSizeX, PREVIEW_GRID_MIN));
    s32 horizontalLineCount = (s32)(PREVIEW_SIZE.y / MIN(self->settings->previewGridSizeY, PREVIEW_GRID_MIN));

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

    glBindVertexArray(self->gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, self->gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(f32), vertices.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);

    return (s32)vertices.size();
}

// Initializes preview
void
preview_init(Preview* self, Anm2* anm2, Anm2Reference* reference, f32* time, Resources* resources, Settings* settings)
{
    self->anm2 = anm2;
    self->reference = reference;
    self->time = time;
    self->resources = resources;
    self->settings = settings;

    // Framebuffer + texture
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

    // Axis
    glGenVertexArrays(1, &self->axisVAO);
    glGenBuffers(1, &self->axisVBO);

    // Grid
    glGenVertexArrays(1, &self->gridVAO);
    glGenBuffers(1, &self->gridVBO);

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
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 4 * 4, NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->textureEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GL_TEXTURE_INDICES), GL_TEXTURE_INDICES, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)0);

    // UV attribute
    glEnableVertexAttribArray(1); 
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void*)(2 * sizeof(f32)));

    glBindVertexArray(0);

    _preview_axis_set(self);
    _preview_grid_set(self);
}

// Ticks preview
void
preview_tick(Preview* self)
{
    self->settings->previewZoom = CLAMP(self->settings->previewZoom, PREVIEW_ZOOM_MIN, PREVIEW_ZOOM_MAX);
 
    Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);

    // If animation is valid, manage playback
    if (animation)
    {
        if (self->isPlaying)
        {  
            *self->time += (f32)self->anm2->fps / TICK_RATE;

            // If looping, return back to 0; if not, stop at length
            if (*self->time >= (f32)animation->frameNum - 1)
            {
                if (self->settings->playbackIsLoop && !self->isRecording)
                    *self->time = 0.0f;
                else
                    self->isPlaying = false;
            }
        }
        
        // Make sure to clamp time within appropriate range
        if (!self->isPlaying)
            *self->time = CLAMP(*self->time, 0.0f, (f32)animation->frameNum - 1);
    }
}

// Draws preview
void
preview_draw(Preview* self)
{
    GLuint shaderLine = self->resources->shaders[SHADER_LINE];
    GLuint shaderTexture = self->resources->shaders[SHADER_TEXTURE];
    static bool isRecordThisFrame = false;
    static f32 recordFrameTimeNext = 0.0f;
    static s32 recordFrameIndex = 0;

    f32 zoomFactor = PERCENT_TO_UNIT(self->settings->previewZoom);
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

    // Grid
    if (self->settings->previewIsGrid)
    {
        static ivec2 previousGridSize = {-1, -1};
        static ivec2 previousGridOffset = {-1, -1};
        static s32 gridVertexCount = -1;
        ivec2 gridSize = ivec2(self->settings->previewGridSizeX, self->settings->previewGridSizeY);
        ivec2 gridOffset = ivec2(self->settings->previewGridOffsetX, self->settings->previewGridOffsetY);

        if (previousGridSize != gridSize || previousGridOffset != gridOffset)
        {
            gridVertexCount = _preview_grid_set(self);
            previousGridSize = gridSize;
            previousGridOffset = gridOffset;
        }

        glUseProgram(shaderLine);
        glBindVertexArray(self->gridVAO);
        glUniformMatrix4fv(glGetUniformLocation(shaderLine, SHADER_UNIFORM_TRANSFORM), 1, GL_FALSE, (f32*)value_ptr(previewTransform));
        
        glUniform4f
        (
            glGetUniformLocation(shaderLine, SHADER_UNIFORM_COLOR),
            self->settings->previewGridColorR, self->settings->previewGridColorG, self->settings->previewGridColorB, self->settings->previewGridColorA
        );

        glDrawArrays(GL_LINES, 0, gridVertexCount);
    
        glBindVertexArray(0);
        glUseProgram(0);
    }

    // Axes
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

    Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);
    s32& animationID = self->reference->animationID;

    // Animation
    if (animation)
    {
        Anm2Frame rootFrame;
        Anm2Frame frame;
        anm2_frame_from_time(self->anm2, &rootFrame, Anm2Reference{animationID, ANM2_ROOT, 0, 0}, *self->time);

        // Layers
        for (auto & [id, layerAnimation] : animation->layerAnimations)
        {
            if (!layerAnimation.isVisible || layerAnimation.frames.size() <= 0)
                continue;

            anm2_frame_from_time(self->anm2, &frame, Anm2Reference{animationID, ANM2_LAYER, id, 0}, *self->time);

            if (!frame.isVisible)
                continue;

            Texture* texture = &self->resources->textures[self->anm2->layers[id].spritesheetID];

            if (texture->isInvalid)
                continue;

            glm::mat4 layerTransform = previewTransform;

            glm::vec2 position = self->settings->previewIsRootTransform ? 
            (frame.position + rootFrame.position) : frame.position;
 
            glm::vec2 scale = self->settings->previewIsRootTransform ? 
            PERCENT_TO_UNIT(frame.scale) *  PERCENT_TO_UNIT(rootFrame.scale) : PERCENT_TO_UNIT(frame.scale);

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

        // Root target
        if (animation->rootAnimation.isVisible && rootFrame.isVisible)
        {
            glm::mat4 rootTransform = previewTransform;
            glm::vec2 ndcPos = (rootFrame.position - (PREVIEW_TARGET_SIZE / 2.0f)) / (PREVIEW_SIZE / 2.0f);
            glm::vec2 ndcScale = PREVIEW_TARGET_SIZE / (PREVIEW_SIZE / 2.0f);

            rootTransform = glm::translate(rootTransform, glm::vec3(ndcPos, 0.0f));
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

        // Layer pivots
        if (self->settings->previewIsShowPivot)
        {
            // Layers (Reversed)
            for (auto & [id, layerAnimation] : animation->layerAnimations)
            {
                if (!layerAnimation.isVisible || layerAnimation.frames.size() <= 0)
                    continue;
                
                anm2_frame_from_time(self->anm2, &frame, Anm2Reference{animationID, ANM2_LAYER, id, 0}, *self->time);

                if (!frame.isVisible)
                    continue;

                glm::mat4 pivotTransform = previewTransform;
                    
                glm::vec2 position = self->settings->previewIsRootTransform 
                ? (frame.position + rootFrame.position) : frame.position;

                glm::vec2 scale = self->settings->previewIsRootTransform ? 
                PERCENT_TO_UNIT(frame.scale) *  PERCENT_TO_UNIT(rootFrame.scale) : PERCENT_TO_UNIT(frame.scale);

                glm::vec2 ndcPos = (position - (PREVIEW_PIVOT_SIZE / 2.0f)) / (PREVIEW_SIZE / 2.0f);
                glm::vec2 ndcScale = PREVIEW_PIVOT_SIZE / (PREVIEW_SIZE / 2.0f);

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

        // Null target/rect
        for (auto & [id, nullAnimation] : animation->nullAnimations)
        {
            if (!nullAnimation.isVisible || nullAnimation.frames.size() <= 0)
                continue;

            anm2_frame_from_time(self->anm2, &frame, Anm2Reference{animationID, ANM2_NULL, id, 0}, *self->time);

            if (!frame.isVisible)
                continue;

            Anm2Null* null = &self->anm2->nulls[id];

            glm::mat4 nullTransform = previewTransform;

            TextureType textureType = null->isShowRect ? TEXTURE_SQUARE : TEXTURE_TARGET;
            glm::vec2 size = null->isShowRect ? PREVIEW_POINT_SIZE : PREVIEW_TARGET_SIZE;
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

            // Null rect
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

    // Manage recording
    if (self->isRecording && animation)
    {
        if (recordFrameIndex == 0)
        {
            // Create frames directory, if it exists
            if 
            (
                std::filesystem::exists(STRING_PREVIEW_FRAMES_DIRECTORY) && 
                std::filesystem::is_directory(STRING_PREVIEW_FRAMES_DIRECTORY)) 
            {
                for (const auto & entry : std::filesystem::directory_iterator(STRING_PREVIEW_FRAMES_DIRECTORY)) 
                    std::filesystem::remove(entry);
            }
            else
                std::filesystem::create_directories(STRING_PREVIEW_FRAMES_DIRECTORY); 
            self->isPlaying = true;
        }

        if (isRecordThisFrame)
        {
            size_t frameSize = (self->recordSize.x * self->recordSize.y * 4);
            u8* frame = (u8*)calloc(frameSize, 1);
            std:: string path;

            vec2 position = 
            {
                self->settings->previewPanX - (PREVIEW_SIZE.x / 2.0f) + (self->recordSize.x / 2.0f),
                self->settings->previewPanY - (PREVIEW_SIZE.y / 2.0f) + (self->recordSize.y / 2.0f)
            };

            path = std::format(STRING_PREVIEW_FRAMES_FORMAT, STRING_PREVIEW_FRAMES_DIRECTORY, recordFrameIndex);

            glReadBuffer(GL_FRONT);
            glReadPixels
            (
                (s32)position.x,
                (s32)position.y,
                self->recordSize.x, 
                self->recordSize.y, 
                GL_RGBA, 
                GL_UNSIGNED_BYTE, 
                frame
            );

            texture_from_data_write(path, frame, self->recordSize.x, self->recordSize.y);

            free(frame);

            isRecordThisFrame = false;
            recordFrameIndex++;
        }
        else
        {
            if (*self->time >= (f32)animation->frameNum - 1)
            {
                self->isRecording = false;
                self->isPlaying = false;
                recordFrameIndex = 0;
                recordFrameTimeNext = 0;
                *self->time = 0.0f;
            }
            else if (*self->time >= recordFrameTimeNext)
            {
                isRecordThisFrame = true;
                recordFrameTimeNext = *self->time + (f32)self->anm2->fps / TICK_RATE;
            }
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); 
}

void
preview_free(Preview* self)
{
    glDeleteTextures(1, &self->texture);
    glDeleteFramebuffers(1, &self->fbo);
    glDeleteRenderbuffers(1, &self->rbo);
}