// Handles the rendering of the spritesheet editor

#include "editor.h"

void editor_init(Editor* self, Anm2* anm2, Anm2Reference* reference, Resources* resources, Settings* settings)
{
    self->anm2 = anm2;
    self->reference = reference;
    self->resources = resources;
    self->settings = settings;

    canvas_init(&self->canvas);
}

void editor_draw(Editor* self)
{
    ivec2& gridSize = self->settings->editorGridSize;
    ivec2& gridOffset = self->settings->editorGridOffset;
    vec4& gridColor = self->settings->editorGridColor;
    GLuint& shaderLine = self->resources->shaders[SHADER_LINE];
    GLuint& shaderTexture = self->resources->shaders[SHADER_TEXTURE];
    mat4 transform = canvas_transform_get(&self->canvas, self->settings->editorPan, self->settings->editorZoom, ORIGIN_TOP_LEFT);
    
    canvas_texture_set(&self->canvas);
    
    canvas_bind(&self->canvas);
    canvas_viewport_set(&self->canvas);
    canvas_clear(self->settings->editorBackgroundColor);

    if (self->spritesheetID != ID_NONE)
    {
        Texture texture = self->resources->textures[self->spritesheetID];
        mat4 mvp = canvas_mvp_get(transform, texture.size);
        canvas_texture_draw(&self->canvas, shaderTexture, texture.id, mvp);

        if (self->settings->editorIsBorder)
            canvas_rect_draw(&self->canvas, shaderLine, mvp, EDITOR_BORDER_COLOR);

        Anm2Frame* frame = (Anm2Frame*)anm2_frame_from_reference(self->anm2, self->reference);
    
        if (frame)
        {
            mvp = canvas_mvp_get(transform, frame->size, frame->crop);
            canvas_rect_draw(&self->canvas, shaderLine, mvp, EDITOR_FRAME_COLOR);

            mvp = canvas_mvp_get(transform, CANVAS_PIVOT_SIZE, frame->crop + frame->pivot, CANVAS_PIVOT_SIZE * 0.5f);
            f32 vertices[] = ATLAS_UV_VERTICES(ATLAS_PIVOT);
            canvas_texture_draw(&self->canvas, shaderTexture, self->resources->atlas.id, mvp, vertices, EDITOR_PIVOT_COLOR);
        }
    }

    if (self->settings->editorIsGrid)
        canvas_grid_draw(&self->canvas, shaderLine, transform, self->settings->editorZoom, gridSize, gridOffset, gridColor);

    canvas_unbind();
}

void editor_free(Editor* self)
{
    canvas_free(&self->canvas);
}