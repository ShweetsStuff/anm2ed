#include "generate_preview.h"

void generate_preview_init(GeneratePreview* self, Anm2* anm2, Anm2Reference* reference, Resources* resources, Settings* settings)
{
    self->anm2 = anm2;
    self->reference = reference;
    self->resources = resources;
    self->settings = settings;

    canvas_init(&self->canvas, GENERATE_PREVIEW_SIZE);
}

void generate_preview_draw(GeneratePreview* self)
{
    
    /* TODO
    f32& zoom = self->settings->previewZoom;
    GLuint& shaderLine = self->resources->shaders[SHADER_LINE];
    GLuint& shaderTexture = self->resources->shaders[SHADER_TEXTURE];
    mat4 transform = canvas_transform_get(&self->canvas, self->settings->previewPan, self->settings->previewZoom, ORIGIN_CENTER);
 
    canvas_bind(&self->canvas);
    canvas_viewport_set(&self->canvas);
    canvas_clear(self->settings->previewBackgroundColor);
   
	self->time = std::clamp(self->time, 0.0f, 1.0f);
    
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

            Texture* texture = map_find(self->resources->textures, self->anm2->layers[id].spritesheetID);

            if (!texture || texture->isInvalid)
                continue;

            vec2 uvMin = frame.crop / vec2(texture->size);
            vec2 uvMax = (frame.crop + frame.size) / vec2(texture->size);
            f32 vertices[] = UV_VERTICES(uvMin, uvMax);

            mat4 model = quad_model_get(frame.size, frame.position, frame.pivot, frame.rotation, PERCENT_TO_UNIT(frame.scale));
            mat4 layerTransform = transform * (rootModel * model);

            canvas_texture_draw(&self->canvas, shaderTexture, texture->id, layerTransform, vertices, frame.tintRGBA, frame.offsetRGB);

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


    s32& animationOverlayID = self->animationOverlayID;
    Anm2Animation* animationOverlay = map_find(self->anm2->animations, animationOverlayID);

    canvas_unbind();
    */
}

void generate_preview_free(GeneratePreview* self)
{
    canvas_free(&self->canvas);
}