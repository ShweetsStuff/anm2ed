// Handles the rendering of the animation preview

#include "preview.h"

void preview_init(Preview* self, Anm2* anm2, Anm2Reference* reference, Resources* resources, Settings* settings)
{
    self->anm2 = anm2;
    self->reference = reference;
    self->resources = resources;
    self->settings = settings;

    canvas_init(&self->canvas);
}

void preview_tick(Preview* self)
{
    Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);

    if (animation)
    {
        if (self->isPlaying)
        {  
            self->time += (f32)self->anm2->fps / TICK_RATE;

            if (self->time >= (f32)animation->frameNum - 1)
            {
                if (self->settings->playbackIsLoop && !self->isRecording)
                    self->time = 0.0f;
                else
                    self->isPlaying = false;
            }
        }
        
        if (!self->isPlaying)
            self->time = std::clamp(self->time, 0.0f, (f32)animation->frameNum - 1);
    }
}

void preview_draw(Preview* self)
{
    ivec2& gridSize = self->settings->previewGridSize;
    ivec2& gridOffset = self->settings->previewGridOffset;
    vec4& gridColor = self->settings->previewGridColor;
    f32& zoom = self->settings->previewZoom;
    GLuint& shaderLine = self->resources->shaders[SHADER_LINE];
    GLuint& shaderTexture = self->resources->shaders[SHADER_TEXTURE];
    mat4 transform = canvas_transform_get(&self->canvas, self->settings->previewPan, self->settings->previewZoom, ORIGIN_CENTER);
 
    canvas_texture_set(&self->canvas);
    
    canvas_bind(&self->canvas);
    canvas_viewport_set(&self->canvas);
    canvas_clear(self->settings->previewBackgroundColor);
    
    if (self->settings->previewIsGrid)
        canvas_grid_draw(&self->canvas, shaderLine, transform, zoom, gridSize, gridOffset, gridColor);

    if (self->settings->previewIsAxis)
        canvas_axes_draw(&self->canvas, shaderLine, transform, self->settings->previewAxisColor);

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
        if (animation->rootAnimation.isVisible && root.isVisible)
        {
            mat4 model = quad_model_get(PREVIEW_TARGET_SIZE, root.position, PREVIEW_TARGET_SIZE * 0.5f, root.rotation, PERCENT_TO_UNIT(root.scale));
            mat4 rootTransform = transform * model;
            f32 vertices[] = ATLAS_UV_VERTICES(TEXTURE_TARGET);
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

            Texture texture = self->resources->textures[self->anm2->layers[id].spritesheetID];

            if (texture.isInvalid)
                continue;

            vec2 uvMin = frame.crop / vec2(texture.size);
            vec2 uvMax = (frame.crop + frame.size) / vec2(texture.size);
            f32 vertices[] = UV_VERTICES(uvMin, uvMax);

            mat4 model = quad_model_get(frame.size, frame.position, frame.pivot, frame.rotation, PERCENT_TO_UNIT(frame.scale));
            mat4 layerTransform = transform * (rootModel * model);

            canvas_texture_draw(&self->canvas, shaderTexture, texture.id, layerTransform, vertices, frame.tintRGBA, frame.offsetRGB);

            if (self->settings->previewIsBorder)
                canvas_rect_draw(&self->canvas, shaderLine, layerTransform, PREVIEW_BORDER_COLOR);

            if (self->settings->previewIsShowPivot)
            {
                f32 vertices[] = ATLAS_UV_VERTICES(TEXTURE_PIVOT);
                mat4 pivotModel = quad_model_get(CANVAS_PIVOT_SIZE, frame.position, CANVAS_PIVOT_SIZE * 0.5f, frame.rotation, PERCENT_TO_UNIT(frame.scale));
                mat4 pivotTransform = transform * (rootModel * pivotModel);
                canvas_texture_draw(&self->canvas, shaderTexture, self->resources->atlas.id, pivotTransform, vertices, PREVIEW_PIVOT_COLOR);
            }
        }

        // Nulls
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
            TextureType texture = null.isShowRect ? TEXTURE_SQUARE : TEXTURE_TARGET;
      
            mat4 model = quad_model_get(size, frame.position, size * 0.5f, frame.rotation, PERCENT_TO_UNIT(frame.scale));
            mat4 nullTransform = transform * (rootModel * model);
 
            f32 vertices[] = ATLAS_UV_VERTICES(texture);
    
            canvas_texture_draw(&self->canvas, shaderTexture, self->resources->atlas.id, nullTransform, vertices, color);

            if (null.isShowRect)
            {
                mat4 rectModel = quad_model_get(PREVIEW_NULL_RECT_SIZE, frame.position, PREVIEW_NULL_RECT_SIZE * 0.5f, frame.rotation, PERCENT_TO_UNIT(frame.scale));
                mat4 rectTransform = transform * (rootModel * rectModel);
                canvas_rect_draw(&self->canvas, shaderLine, rectTransform, color);
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
            Anm2Item& layerAnimation = animation->layerAnimations[id];

            if (!layerAnimation.isVisible || layerAnimation.frames.size() <= 0)
                continue;

            anm2_frame_from_time(self->anm2, &frame, Anm2Reference{animationOverlayID, ANM2_LAYER, id}, self->time);

            if (!frame.isVisible)
                continue;

            Texture texture = self->resources->textures[self->anm2->layers[id].spritesheetID];

            if (texture.isInvalid)
                continue;

            vec2 uvMin = frame.crop / vec2(texture.size);
            vec2 uvMax = (frame.crop + frame.size) / vec2(texture.size);
            f32 vertices[] = UV_VERTICES(uvMin, uvMax);

            mat4 model = quad_model_get(frame.size, frame.position, frame.pivot, frame.rotation, PERCENT_TO_UNIT(frame.scale));
            mat4 layerTransform = transform * (rootModel * model);

            vec4 tint = frame.tintRGBA;
            tint.a *= U8_TO_FLOAT(self->settings->previewOverlayTransparency);

            canvas_texture_draw(&self->canvas, shaderTexture, texture.id, layerTransform, vertices, tint, frame.offsetRGB);
        }
    }

    canvas_unbind();
}

void preview_free(Preview* self)
{
    canvas_free(&self->canvas);
}