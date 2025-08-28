#include "editor.h"

void editor_init(Editor* self, Anm2* anm2, Anm2Reference* reference, Resources* resources, Settings* settings)
{
    self->anm2 = anm2;
    self->reference = reference;
    self->resources = resources;
    self->settings = settings;

    canvas_init(&self->canvas, vec2());
}

void editor_draw(Editor* self)
{
    ivec2& gridSize = self->settings->editorGridSize;
    ivec2& gridOffset = self->settings->editorGridOffset;
    vec4& gridColor = self->settings->editorGridColor;
    GLuint& shaderLine = self->resources->shaders[SHADER_LINE];
    GLuint& shaderTexture = self->resources->shaders[SHADER_TEXTURE];
    GLuint& shaderGrid = self->resources->shaders[SHADER_GRID];
    mat4 transform = canvas_transform_get(&self->canvas, self->settings->editorPan, self->settings->editorZoom, ORIGIN_TOP_LEFT);
    
    canvas_framebuffer_resize_check(&self->canvas);
    
    canvas_bind(&self->canvas);
    canvas_viewport_set(&self->canvas);
    canvas_clear(self->settings->editorBackgroundColor);

    if (Anm2Spritesheet* spritesheet = map_find(self->anm2->spritesheets, self->spritesheetID))
    {
        Texture& texture = spritesheet->texture;
        
        mat4 spritesheetTransform = transform * canvas_model_get(texture.size);
        canvas_texture_draw(&self->canvas, shaderTexture, texture.id, spritesheetTransform);

        if (self->settings->editorIsBorder)
            canvas_rect_draw(&self->canvas, shaderLine, spritesheetTransform, EDITOR_BORDER_COLOR);

        Anm2Frame* frame = (Anm2Frame*)anm2_frame_from_reference(self->anm2, self->reference);
    
        if (frame)
        {
            mat4 cropTransform = transform * canvas_model_get(frame->size, frame->crop);
            canvas_rect_draw(&self->canvas, shaderLine, cropTransform, EDITOR_FRAME_COLOR);

            mat4 pivotTransform = transform * canvas_model_get(CANVAS_PIVOT_SIZE, frame->crop + frame->pivot, CANVAS_PIVOT_SIZE * 0.5f);
            f32 vertices[] = ATLAS_UV_VERTICES(ATLAS_PIVOT);
            canvas_texture_draw(&self->canvas, shaderTexture, self->resources->atlas.id, pivotTransform, vertices, EDITOR_PIVOT_COLOR);
        }
    }

    if (self->settings->editorIsGrid)
        canvas_grid_draw(&self->canvas, shaderGrid, transform, gridSize, gridOffset, gridColor);

    canvas_unbind();
}

void editor_free(Editor* self)
{
    canvas_free(&self->canvas);
}