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
                ivec2& size = self->canvas.size;
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

            time += (f32)self->anm2->fps / TICK_DELAY;

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
    GLuint& shaderAxis = self->resources->shaders[SHADER_AXIS];
    GLuint& shaderTexture = self->resources->shaders[SHADER_TEXTURE];
    GLuint& shaderGrid = self->resources->shaders[SHADER_GRID];
    mat4 transform = canvas_transform_get(&self->canvas, self->settings->previewPan, self->settings->previewZoom, ORIGIN_CENTER);
 
    canvas_framebuffer_resize_check(&self->canvas);
    
    canvas_bind(&self->canvas);
    canvas_viewport_set(&self->canvas);
    canvas_clear(self->settings->previewBackgroundColor);
    
    if (self->settings->previewIsGrid)
        canvas_grid_draw(&self->canvas, shaderGrid, transform, gridSize, gridOffset, gridColor);

    if (self->settings->previewIsAxes)
        canvas_axes_draw(&self->canvas, shaderAxis, transform, self->settings->previewAxesColor);

    auto animation_draw = [&](s32 animationID)
    {
        Anm2Animation* animation = map_find(self->anm2->animations, animationID);
        if (!animation) return;

        auto root_draw = [&](Anm2Frame root, vec3 colorOffset = {}, f32 alphaOffset = {}, bool isOnionskin = {})
        {
            mat4 model = quad_model_get(PREVIEW_TARGET_SIZE, root.position, PREVIEW_TARGET_SIZE * 0.5f, PERCENT_TO_UNIT(root.scale), root.rotation);
            mat4 rootTransform = transform * model;
            vec4 color = isOnionskin ? vec4(colorOffset, 1.0f - alphaOffset) : PREVIEW_ROOT_COLOR;
            AtlasType atlas = self->settings->previewIsAltIcons ? ATLAS_TARGET_ALT : ATLAS_TARGET;
            f32 vertices[] = ATLAS_UV_VERTICES(atlas);
            canvas_texture_draw(&self->canvas, shaderTexture, self->resources->atlas.id, rootTransform, vertices, color);
        };

        auto layer_draw = [&](mat4 rootModel, s32 id, f32 time, vec3 colorOffset = {}, f32 alphaOffset = {}, bool isOnionskin = {})
        {
            Anm2Item& layerAnimation = animation->layerAnimations[id];
            if (!layerAnimation.isVisible || layerAnimation.frames.size() <= 0) return;

            Anm2Frame frame;
            anm2_frame_from_time(self->anm2, &frame, Anm2Reference{animationID, ANM2_LAYER, id}, time);
            if (!frame.isVisible) return;

            mat4 model = quad_model_get(frame.size, frame.position, frame.pivot, PERCENT_TO_UNIT(frame.scale), frame.rotation);
            mat4 layerTransform = transform * (rootModel * model);
            vec3 frameColorOffset = frame.offsetRGB + colorOffset;
            vec4 frameTint = frame.tintRGBA;
            frameTint.a = std::max(0.0f, frameTint.a - alphaOffset);
            
            Anm2Spritesheet* spritesheet = map_find(self->anm2->spritesheets, self->anm2->layers[id].spritesheetID);
            if (!spritesheet) return;

            Texture& texture = spritesheet->texture;
            if (texture.isInvalid) return;
            
            vec2 uvMin = frame.crop / vec2(texture.size);
            vec2 uvMax = (frame.crop + frame.size) / vec2(texture.size);
            f32 vertices[] = UV_VERTICES(uvMin, uvMax);
            canvas_texture_draw(&self->canvas, shaderTexture, texture.id, layerTransform, vertices, frameTint, frameColorOffset);

            if (self->settings->previewIsBorder)
            {
                vec4 borderColor = isOnionskin ? vec4(colorOffset, 1.0f - alphaOffset) : PREVIEW_BORDER_COLOR;
                canvas_rect_draw(&self->canvas, shaderLine, layerTransform, borderColor);
            }

            if (self->settings->previewIsPivots)
            {
                vec4 pivotColor = isOnionskin ? vec4(colorOffset, 1.0f - alphaOffset) : PREVIEW_PIVOT_COLOR;
                f32 vertices[] = ATLAS_UV_VERTICES(ATLAS_PIVOT);
                mat4 pivotModel = quad_model_get(CANVAS_PIVOT_SIZE, frame.position, CANVAS_PIVOT_SIZE * 0.5f, PERCENT_TO_UNIT(frame.scale), frame.rotation);
                mat4 pivotTransform = transform * (rootModel * pivotModel);
                canvas_texture_draw(&self->canvas, shaderTexture, self->resources->atlas.id, pivotTransform, vertices, pivotColor);
            }
        };

        auto null_draw = [&](mat4 rootModel, s32 id, f32 time, vec3 colorOffset = {}, f32 alphaOffset = {}, bool isOnionskin = {}) 
        {
            Anm2Item& nullAnimation = animation->nullAnimations[id];
            if (!nullAnimation.isVisible || nullAnimation.frames.size() <= 0) return;

            Anm2Frame frame;
            anm2_frame_from_time(self->anm2, &frame, Anm2Reference{animationID, ANM2_NULL, id}, time);
            if (!frame.isVisible) return;

            Anm2Null null = self->anm2->nulls[id];

            vec4 color = isOnionskin ? vec4(colorOffset, 1.0f - alphaOffset) : 
            (self->reference->itemType == ANM2_NULL && self->reference->itemID == id) ? 
            PREVIEW_NULL_SELECTED_COLOR : PREVIEW_NULL_COLOR;

            vec2 size = null.isShowRect ? CANVAS_PIVOT_SIZE : PREVIEW_TARGET_SIZE;
            AtlasType atlas = null.isShowRect ? ATLAS_SQUARE : self->settings->previewIsAltIcons ? ATLAS_TARGET_ALT : ATLAS_TARGET;
      
            mat4 model = quad_model_get(size, frame.position, size * 0.5f, PERCENT_TO_UNIT(frame.scale), frame.rotation);
            mat4 nullTransform = transform * (rootModel * model);
 
            f32 vertices[] = ATLAS_UV_VERTICES(atlas);
    
            canvas_texture_draw(&self->canvas, shaderTexture, self->resources->atlas.id, nullTransform, vertices, color);

            if (null.isShowRect)
            {
                mat4 rectModel = quad_model_get(PREVIEW_NULL_RECT_SIZE, frame.position, PREVIEW_NULL_RECT_SIZE * 0.5f, PERCENT_TO_UNIT(frame.scale), frame.rotation);
                mat4 rectTransform = transform * (rootModel * rectModel);
                canvas_rect_draw(&self->canvas, shaderLine, rectTransform, color);
            }
        };

        auto base_draw = [&](f32 time, vec3 colorOffset = {}, f32 alphaOffset = {}, bool isOnionskin = {})
        {
            Anm2Frame root;
            anm2_frame_from_time(self->anm2, &root, Anm2Reference{animationID, ANM2_ROOT}, time);

            mat4 rootModel = self->settings->previewIsRootTransform ?
            quad_model_parent_get(root.position, {}, PERCENT_TO_UNIT(root.scale), root.rotation) : mat4(1.0f);

            if (self->settings->previewIsIcons && animation->rootAnimation.isVisible && root.isVisible)
                root_draw(root, colorOffset, alphaOffset, isOnionskin);

            for (auto [i, id] : self->anm2->layerMap)
                layer_draw(rootModel, id, time, colorOffset, alphaOffset, isOnionskin);

            if (self->settings->previewIsIcons)
                for (auto& [id, _] : animation->nullAnimations)
                    null_draw(rootModel, id, time, colorOffset, alphaOffset, isOnionskin); 
        };

        auto onionskin_draw = [&](s32 count, s32 direction, vec3 colorOffset)
        {
            for (s32 i = 1; i <= count; i++)
            {
                f32 time  = self->time + (f32)(direction * i);
                f32 alphaOffset = (1.0f / (count + 1)) * i;
                base_draw(time, colorOffset, alphaOffset, true);
            }
        };

        auto onionskins_draw = [&]()
        {
            if (!self->settings->onionskinIsEnabled) return;
            onionskin_draw(self->settings->onionskinBeforeCount, -1, self->settings->onionskinBeforeColorOffset);
            onionskin_draw(self->settings->onionskinAfterCount, 1, self->settings->onionskinAfterColorOffset);
        };

        if (self->settings->onionskinDrawOrder == ONIONSKIN_BELOW) onionskins_draw();
        base_draw(self->time);
        if (self->settings->onionskinDrawOrder == ONIONSKIN_ABOVE) onionskins_draw();
    };

    animation_draw(self->reference->animationID);
    animation_draw(self->animationOverlayID);

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