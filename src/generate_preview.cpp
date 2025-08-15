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
    static auto& columns = self->settings->generateColumns;
    static auto& count = self->settings->generateCount;
    static GLuint& shaderTexture = self->resources->shaders[SHADER_TEXTURE];
    const mat4 transform = canvas_transform_get(&self->canvas, {}, CANVAS_ZOOM_DEFAULT, ORIGIN_CENTER);
 
    vec2 startPosition = {self->settings->generateStartPosition.x, self->settings->generateStartPosition.y};
    vec2 size = {self->settings->generateSize.x, self->settings->generateSize.y};
    vec2 pivot = {self->settings->generatePivot.x, self->settings->generatePivot.y};

    canvas_bind(&self->canvas);
    canvas_viewport_set(&self->canvas);
    canvas_clear(self->settings->previewBackgroundColor);
 
    Anm2Item* item = anm2_item_from_reference(self->anm2, self->reference);
    Texture* texture = map_find(self->resources->textures, self->anm2->layers[self->reference->itemID].spritesheetID);
        
    if (item && texture && !texture->isInvalid)
    {
        const s32 index = std::clamp((s32)(self->time * count), 0, count);
        const s32 row = index / columns;
        const s32 column = index % columns;
        vec2 crop = startPosition + vec2(size.x * column, size.y * row);

        vec2 uvMin = crop / vec2(texture->size);
        vec2 uvMax = (crop + size) / vec2(texture->size);
        f32 vertices[] = UV_VERTICES(uvMin, uvMax);

        mat4 model = quad_model_get(size, {}, pivot, {}, CANVAS_SCALE_DEFAULT);
        mat4 generateTransform = transform * model;

        canvas_texture_draw(&self->canvas, shaderTexture, texture->id, generateTransform, vertices, COLOR_OPAQUE, COLOR_OFFSET_NONE);
    }
    
    canvas_unbind();
}

void generate_preview_free(GeneratePreview* self)
{
    canvas_free(&self->canvas);
}