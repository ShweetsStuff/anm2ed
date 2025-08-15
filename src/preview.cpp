#include "preview.h"

static void _preview_render_textures_free(Preview* self)
{
    for (auto& texture : self->renderFrames)
        texture_free(&texture);

    self->renderFrames.clear();
}

void preview_init(Preview* self, Anm2* anm2, Anm2Reference* reference, Resources* resources, Settings* settings)
{
    self->anm2 = anm2;
    self->reference = reference;
    self->resources = resources;
    self->settings = settings;

    canvas_init(&self->canvas, vec2());
}

void preview_tick(Preview* self)
{
    f32& time = self->time;
    Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);

    if (animation)
    {
        if (self->isPlaying)
        {  
            if (self->isRender)
            {
                vec2& size = self->canvas.size;
                u32 framebufferPixelCount = size.x * size.y * TEXTURE_CHANNELS;
                std::vector<u8> framebufferPixels(framebufferPixelCount);
                Texture frameTexture;

                glBindFramebuffer(GL_READ_FRAMEBUFFER, self->canvas.fbo);
                glReadBuffer(GL_COLOR_ATTACHMENT0);
                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, framebufferPixels.data());
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

                texture_from_rgba_init(&frameTexture, size, TEXTURE_CHANNELS, framebufferPixels.data());
                self->renderFrames.push_back(frameTexture);
            }

            time += (f32)self->anm2->fps / TICK_RATE;

            if (time >= (f32)animation->frameNum - 1)
            {
                if (self->isRender)
                {
                    self->isRender = false;
                    self->isRenderFinished = true;
                    time = 0.0f;
                    self->isPlaying = false;
                }
                else
                {
                    if (self->settings->playbackIsLoop)
                        time = 0.0f;
                    else
                    {
			            time = std::clamp(time, 0.0f, std::max(0.0f, (f32)animation->frameNum - 1));
                        self->isPlaying = false;
                    }
                }
            }
        }

		if (self->settings->playbackIsClampPlayhead)
			time = std::clamp(time, 0.0f, std::max(0.0f, (f32)animation->frameNum - 1));
		else
			time = std::max(time, 0.0f);

    }


}

void preview_draw(Preview* self)
{
    ivec2& gridSize = self->settings->previewGridSize;
    ivec2& gridOffset = self->settings->previewGridOffset;
    vec4& gridColor = self->settings->previewGridColor;
    GLuint& shaderLine = self->resources->shaders[SHADER_LINE];
    GLuint& shaderTexture = self->resources->shaders[SHADER_TEXTURE];
    GLuint& shaderGrid = self->resources->shaders[SHADER_GRID];
    mat4 transform = canvas_transform_get(&self->canvas, self->settings->previewPan, self->settings->previewZoom, ORIGIN_CENTER);
 
    canvas_texture_set(&self->canvas);
    
    canvas_bind(&self->canvas);
    canvas_viewport_set(&self->canvas);
    canvas_clear(self->settings->previewBackgroundColor);
    
    if (self->settings->previewIsGrid)
        canvas_grid_draw(&self->canvas, shaderGrid, transform, gridSize, gridOffset, gridColor);

    if (self->settings->previewIsAxes)
        canvas_axes_draw(&self->canvas, shaderLine, transform, self->settings->previewAxesColor);

    Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);
    s32& animationID = self->reference->animationID;

    if (animation)
    {
        Anm2Frame root;
        mat4 rootModel = mat4(1.0f);
        
        anm2_frame_from_time(self->anm2, &root, Anm2Reference{animationID, ANM2_ROOT}, self->time);

        if (self->settings->previewIsRootTransform)
            rootModel = quad_parent_model_get(root.position, vec2(0.0f), root.rotation, PERCENT_TO_UNIT(root.scale));

        // Root
        if (self->settings->previewIsTargets && animation->rootAnimation.isVisible && root.isVisible)
        {
            mat4 model = quad_model_get(PREVIEW_TARGET_SIZE, root.position, PREVIEW_TARGET_SIZE * 0.5f, root.rotation, PERCENT_TO_UNIT(root.scale));
            mat4 rootTransform = transform * model;
            f32 vertices[] = ATLAS_UV_VERTICES(ATLAS_TARGET);
            canvas_texture_draw(&self->canvas, shaderTexture, self->resources->atlas.id, rootTransform, vertices, PREVIEW_ROOT_COLOR);
        }

        // Layers
		for (auto [i, id] : self->anm2->layerMap)
        {
            Anm2Frame frame;
            Anm2Item& layerAnimation = animation->layerAnimations[id];

            if (!layerAnimation.isVisible || layerAnimation.frames.size() <= 0)
                continue;

            anm2_frame_from_time(self->anm2, &frame, Anm2Reference{animationID, ANM2_LAYER, id}, self->time);

            if (!frame.isVisible)
                continue;

            mat4 model = quad_model_get(frame.size, frame.position, frame.pivot, frame.rotation, PERCENT_TO_UNIT(frame.scale));
            mat4 layerTransform = transform * (rootModel * model);

            Texture* texture = map_find(self->resources->textures, self->anm2->layers[id].spritesheetID);
           
            if (texture && !texture->isInvalid)
            {
                vec2 uvMin = frame.crop / vec2(texture->size);
                vec2 uvMax = (frame.crop + frame.size) / vec2(texture->size);
                f32 vertices[] = UV_VERTICES(uvMin, uvMax);

                canvas_texture_draw(&self->canvas, shaderTexture, texture->id, layerTransform, vertices, frame.tintRGBA, frame.offsetRGB);
            }
 
            if (self->settings->previewIsBorder)
                canvas_rect_draw(&self->canvas, shaderLine, layerTransform, PREVIEW_BORDER_COLOR);

            if (self->settings->previewIsPivots)
            {
                f32 vertices[] = ATLAS_UV_VERTICES(ATLAS_PIVOT);
                mat4 pivotModel = quad_model_get(CANVAS_PIVOT_SIZE, frame.position, CANVAS_PIVOT_SIZE * 0.5f, frame.rotation, PERCENT_TO_UNIT(frame.scale));
                mat4 pivotTransform = transform * (rootModel * pivotModel);
                canvas_texture_draw(&self->canvas, shaderTexture, self->resources->atlas.id, pivotTransform, vertices, PREVIEW_PIVOT_COLOR);
            }
        }

        // Nulls
        if (self->settings->previewIsTargets)
        {
            for (auto& [id, nullAnimation] : animation->nullAnimations)
            {
                if (!nullAnimation.isVisible || nullAnimation.frames.size() <= 0)
                    continue;

                Anm2Frame frame;
                anm2_frame_from_time(self->anm2, &frame, Anm2Reference{animationID, ANM2_NULL, id}, self->time);

                if (!frame.isVisible)
                    continue;

                Anm2Null null = self->anm2->nulls[id];

                vec4 color = (self->reference->itemType == ANM2_NULL && self->reference->itemID == id) ? 
                             PREVIEW_NULL_SELECTED_COLOR                                               : 
                             PREVIEW_NULL_COLOR;

                vec2 size = null.isShowRect ? CANVAS_PIVOT_SIZE : PREVIEW_TARGET_SIZE;
                AtlasType atlas = null.isShowRect ? ATLAS_SQUARE : ATLAS_TARGET;
          
                mat4 model = quad_model_get(size, frame.position, size * 0.5f, frame.rotation, PERCENT_TO_UNIT(frame.scale));
                mat4 nullTransform = transform * (rootModel * model);
     
                f32 vertices[] = ATLAS_UV_VERTICES(atlas);
        
                canvas_texture_draw(&self->canvas, shaderTexture, self->resources->atlas.id, nullTransform, vertices, color);

                if (null.isShowRect)
                {
                    mat4 rectModel = quad_model_get(PREVIEW_NULL_RECT_SIZE, frame.position, PREVIEW_NULL_RECT_SIZE * 0.5f, frame.rotation, PERCENT_TO_UNIT(frame.scale));
                    mat4 rectTransform = transform * (rootModel * rectModel);
                    canvas_rect_draw(&self->canvas, shaderLine, rectTransform, color);
                }
            }
        }
    }

    s32& animationOverlayID = self->animationOverlayID;
    Anm2Animation* animationOverlay = map_find(self->anm2->animations, animationOverlayID);

    if (animationOverlay)
    {
        Anm2Frame root;
        mat4 rootModel = mat4(1.0f);
        
        anm2_frame_from_time(self->anm2, &root, Anm2Reference{animationOverlayID, ANM2_ROOT}, self->time);

        if (self->settings->previewIsRootTransform)
            rootModel = quad_parent_model_get(root.position, vec2(0.0f), root.rotation, PERCENT_TO_UNIT(root.scale));

		for (auto [i, id] : self->anm2->layerMap)
        {
            Anm2Frame frame;
            Anm2Item& layerAnimation = animationOverlay->layerAnimations[id];

            if (!layerAnimation.isVisible || layerAnimation.frames.size() <= 0)
                continue;

            anm2_frame_from_time(self->anm2, &frame, Anm2Reference{animationOverlayID, ANM2_LAYER, id}, self->time);

            if (!frame.isVisible)
                continue;

            Texture* texture = map_find(self->resources->textures, self->anm2->layers[id].spritesheetID);
            
            if (!texture || texture->isInvalid)
                continue;

            vec2 uvMin = frame.crop / vec2(texture->size);
            vec2 uvMax = (frame.crop + frame.size) / vec2(texture->size);
            f32 vertices[] = UV_VERTICES(uvMin, uvMax);

            mat4 model = quad_model_get(frame.size, frame.position, frame.pivot, frame.rotation, PERCENT_TO_UNIT(frame.scale));
            mat4 layerTransform = transform * (rootModel * model);

            vec4 tint = frame.tintRGBA;
            tint.a *= U8_TO_FLOAT(self->settings->previewOverlayTransparency);

            canvas_texture_draw(&self->canvas, shaderTexture, texture->id, layerTransform, vertices, tint, frame.offsetRGB);
        }
    }

    canvas_unbind();
}

void preview_render_start(Preview* self)
{
    self->isRender = true;
    self->isPlaying = true;
    self->time = 0.0f;
    _preview_render_textures_free(self);
}

void preview_render_end(Preview* self)
{
    self->isRender = false;
    self->isPlaying = false;
    self->isRenderFinished = false;
    _preview_render_textures_free(self);
}

void preview_free(Preview* self)
{
    canvas_free(&self->canvas);
}