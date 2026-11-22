#include "canvas.h"

#include <algorithm>
#include "vec.h"

#include "math_util.h"
#include "texture.h"

static void canvas_blend_premultiplied_set()
{
    gl.Enable(GL_BLEND);
    gl.BlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

static void canvas_blend_straight_set()
{
    gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    gl.Enable(GL_BLEND);
}

static void vertex_array_make(GLuint& vao, GLuint& vbo, const void* data, GLsizeiptr size, GLenum usage,
                              std::initializer_list<int> attributeSizes)
{
    gl.GenVertexArrays(1, &vao);
    gl.GenBuffers(1, &vbo);
    gl.BindVertexArray(vao);
    gl.BindBuffer(GL_ARRAY_BUFFER, vbo);
    gl.BufferData(GL_ARRAY_BUFFER, size, data, usage);

    auto stride = 0;
    for (auto attributeSize : attributeSizes)
        stride += attributeSize;
    auto offset = 0;
    auto index = 0;
    for (auto attributeSize : attributeSizes)
    {
        gl.EnableVertexAttribArray(index);
        gl.VertexAttribPointer(index++, attributeSize, GL_FLOAT, GL_FALSE, stride * sizeof(float),
                               (void*)(offset * sizeof(float)));
        offset += attributeSize;
    }
}

Canvas::Canvas() = default;

Canvas::Canvas(Vec2 size)
{
    Framebuffer::size_set(size);
    vertex_array_make(axisVAO, axisVBO, AXIS_VERTICES, sizeof(AXIS_VERTICES), GL_STATIC_DRAW, {2});
    vertex_array_make(gridVAO, gridVBO, GRID_VERTICES, sizeof(GRID_VERTICES), GL_STATIC_DRAW, {2, 2});
    vertex_array_make(rectVAO, rectVBO, RECT_VERTICES, sizeof(RECT_VERTICES), GL_STATIC_DRAW, {2});
    vertex_array_make(textureVAO, textureVBO, nullptr, sizeof(TEXTURE_VERTICES), GL_DYNAMIC_DRAW, {2, 2});
    gl.GenBuffers(1, &textureEBO);
    gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, textureEBO);
    gl.BufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(TEXTURE_INDICES), TEXTURE_INDICES, GL_DYNAMIC_DRAW);
    gl.BindVertexArray(0);
}

Canvas::~Canvas()
{
    if (!Framebuffer::is_valid())
        return;
    for (auto vao : {axisVAO, gridVAO, rectVAO, textureVAO})
        gl.DeleteVertexArrays(1, &vao);
    for (auto buffer : {axisVBO, gridVBO, rectVBO, textureVBO, textureEBO})
        gl.DeleteBuffers(1, &buffer);
}

Mat4 Canvas::transform_get(float zoom, Vec2 pan) const
{
    auto zoomFactor = percent_to_unit(zoom);
    auto projection = mat4_ortho(0.0f, (float)size.x, 0.0f, (float)size.y, -1.0f, 1.0f);
    auto view = Mat4{1.0f};

    view = mat4_translate(view, Vec3((size * 0.5f) + pan, 0.0f));
    view = mat4_scale(view, Vec3(zoomFactor, zoomFactor, 1.0f));

    return projection * view;
}

void Canvas::axes_render(ShaderProgram& shader, float zoom, Vec2 pan, Vec4 color) const
{
    canvas_blend_premultiplied_set();

    auto originNDC = transform_get(zoom, pan) * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    originNDC /= originNDC.w;

    gl.UseProgram(shader.id);

    gl.Uniform4fv(gl.GetUniformLocation(shader.id, UNIFORM_COLOR), 1, values_get(color));
    gl.Uniform2f(gl.GetUniformLocation(shader.id, UNIFORM_ORIGIN_NDC), originNDC.x, originNDC.y);

    gl.BindVertexArray(axisVAO);

    gl.Uniform1i(gl.GetUniformLocation(shader.id, UNIFORM_AXIS), 0);
    gl.DrawArrays(GL_LINES, 0, 2);

    gl.Uniform1i(gl.GetUniformLocation(shader.id, UNIFORM_AXIS), 1);
    gl.DrawArrays(GL_LINES, 0, 2);

    gl.BindVertexArray(0);
    gl.UseProgram(0);
    canvas_blend_straight_set();
}

void Canvas::grid_render(ShaderProgram& shader, float zoom, Vec2 pan, IVec2 size, IVec2 offset, Vec4 color) const
{
    canvas_blend_premultiplied_set();

    auto transform = mat4_inverse(transform_get(zoom, pan));

    gl.UseProgram(shader.id);

    gl.UniformMatrix4fv(gl.GetUniformLocation(shader.id, UNIFORM_TRANSFORM), 1, GL_FALSE, values_get(transform));
    gl.Uniform2f(gl.GetUniformLocation(shader.id, UNIFORM_SIZE), (float)size.x, (float)size.y);
    gl.Uniform2f(gl.GetUniformLocation(shader.id, UNIFORM_OFFSET), (float)offset.x, (float)offset.y);
    gl.Uniform4f(gl.GetUniformLocation(shader.id, UNIFORM_COLOR), color.r, color.g, color.b, color.a);

    gl.BindVertexArray(gridVAO);
    gl.DrawArrays(GL_TRIANGLES, 0, 3);
    gl.BindVertexArray(0);

    gl.UseProgram(0);
    canvas_blend_straight_set();
}

static void texture_uniform_set(const ShaderUniform& uniform, Vec4 value)
{
    if (uniform.valueType == UNIFORM_VALUE_INT || uniform.valueType == UNIFORM_VALUE_SAMPLER2D)
        return gl.Uniform1i(uniform.location, uniform.intValue);
    switch (UNIFORM_VALUE_TYPE_INFOS[uniform.valueType].componentCount)
    {
        case 2:
            return gl.Uniform2fv(uniform.location, 1, values_get(value));
        case 3:
            return gl.Uniform3fv(uniform.location, 1, values_get(value));
        case 4:
            return gl.Uniform4fv(uniform.location, 1, values_get(value));
        default:
            if (uniform.valueType == UNIFORM_VALUE_FLOAT)
                gl.Uniform1f(uniform.location, value.x);
    }
}

static Vec4 texture_uniform_components_get(const ShaderUniform& uniform, float playbackTime)
{
    Vec4 result{};
    for (int i = 0; i < (int)uniform.components.size(); ++i)
        result[i] =
            uniform.components[i].binding == UNIFORM_BINDING_PLAYBACK_TIME ? playbackTime : uniform.components[i].value;
    return result;
}

static void texture_uniforms_set(ShaderProgram& shader, Mat4 transform, Vec4 tint, Vec4 colorOffset, Vec2 textureSize,
                                 float playbackTime)
{
    if (shader.uniforms.empty())
    {
        gl.Uniform1i(gl.GetUniformLocation(shader.id, UNIFORM_TEXTURE), 0);
        gl.Uniform4fv(gl.GetUniformLocation(shader.id, UNIFORM_COLOR_OFFSET), 1, values_get(colorOffset));
        gl.Uniform4fv(gl.GetUniformLocation(shader.id, UNIFORM_TINT), 1, values_get(tint));
        gl.UniformMatrix4fv(gl.GetUniformLocation(shader.id, UNIFORM_TRANSFORM), 1, GL_FALSE, values_get(transform));
        return;
    }

    for (auto& uniform : shader.uniforms)
    {
        if (uniform.location == -1)
            continue;

        switch (uniform.binding)
        {
            case UNIFORM_BINDING_MAIN_TEXTURE:
                gl.Uniform1i(uniform.location, 0);
                break;
            case UNIFORM_BINDING_TRANSFORM:
                if (uniform.valueType == UNIFORM_VALUE_MAT4)
                    gl.UniformMatrix4fv(uniform.location, 1, GL_FALSE, values_get(transform));
                break;
            case UNIFORM_BINDING_FRAME_TINT:
                texture_uniform_set(uniform, tint);
                break;
            case UNIFORM_BINDING_COLOR_OFFSET:
                texture_uniform_set(uniform, colorOffset);
                break;
            case UNIFORM_BINDING_TEXTURE_SIZE:
                texture_uniform_set(uniform, Vec4(textureSize, 0.0f, 0.0f));
                break;
            case UNIFORM_BINDING_PLAYBACK_TIME:
                texture_uniform_set(uniform, Vec4(playbackTime));
                break;
            case UNIFORM_BINDING_COMPONENTS:
                texture_uniform_set(uniform, texture_uniform_components_get(uniform, playbackTime));
                break;
            case UNIFORM_BINDING_MANUAL:
                texture_uniform_set(uniform, uniform.value);
                break;
            default:
                break;
        }
    }
}

void Canvas::texture_render(ShaderProgram& shader, GLuint texture, Mat4 transform, Vec4 tint, Vec4 colorOffset,
                            float* vertices, Vec2 textureSize, float playbackTime, TextureBlur blur) const
{
    canvas_blend_premultiplied_set();

    gl.UseProgram(shader.id);

    texture_uniforms_set(shader, transform, tint, colorOffset, textureSize, playbackTime);
    // The editor's own texture shader blurs. A custom shader without these uniforms ignores them.
    gl.Uniform1f(gl.GetUniformLocation(shader.id, UNIFORM_BLUR), blur.radius);
    gl.Uniform4fv(gl.GetUniformLocation(shader.id, UNIFORM_REGION), 1, values_get(blur.region));
    gl.Uniform2fv(gl.GetUniformLocation(shader.id, UNIFORM_TEXEL), 1,
                  values_get(textureSize.x > 0 && textureSize.y > 0 ? 1.0f / textureSize : Vec2()));
    gl.VertexAttrib4fv(2, values_get(tint));

    gl.BindVertexArray(textureVAO);

    gl.BindBuffer(GL_ARRAY_BUFFER, textureVBO);
    gl.BufferData(GL_ARRAY_BUFFER, sizeof(TEXTURE_VERTICES), vertices, GL_DYNAMIC_DRAW);

    gl.ActiveTexture(GL_TEXTURE0);
    gl.BindTexture(GL_TEXTURE_2D, texture);

    gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    gl.BindVertexArray(0);
    gl.BindBuffer(GL_ARRAY_BUFFER, 0);
    gl.BindTexture(GL_TEXTURE_2D, 0);
    gl.UseProgram(0);

    canvas_blend_straight_set();
}

static void rect_begin(ShaderProgram& shader, const Mat4& transform, const Mat4& model, Vec4 color)
{
    canvas_blend_premultiplied_set();
    gl.UseProgram(shader.id);
    gl.UniformMatrix4fv(gl.GetUniformLocation(shader.id, UNIFORM_TRANSFORM), 1, GL_FALSE, values_get(transform));
    if (auto location = gl.GetUniformLocation(shader.id, UNIFORM_MODEL); location != -1)
        gl.UniformMatrix4fv(location, 1, GL_FALSE, values_get(model));
    gl.Uniform4fv(gl.GetUniformLocation(shader.id, UNIFORM_COLOR), 1, values_get(color));
}

static void rect_end(GLuint vao, GLenum mode, int count = 4)
{
    gl.BindVertexArray(vao);
    gl.DrawArrays(mode, 0, count);
    gl.BindVertexArray(0);
    gl.UseProgram(0);
    canvas_blend_straight_set();
}

void Canvas::dashed_render(ShaderProgram& shader, const Mat4& transform, const Mat4& model, Vec4 color,
                           float dashLength, float dashGap, float dashOffset, GLenum mode, int count) const
{
    rect_begin(shader, transform, model, color);

    auto origin = model * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    for (auto [name, axis] : {std::pair{UNIFORM_AXIS_X, Vec4(1.0f, 0.0f, 0.0f, 1.0f)},
                              std::pair{UNIFORM_AXIS_Y, Vec4(0.0f, 1.0f, 0.0f, 1.0f)}})
        if (auto location = gl.GetUniformLocation(shader.id, name); location != -1)
            gl.Uniform2fv(location, 1, values_get(Vec2(model * axis - origin)));
    for (auto [name, value] : {std::pair{UNIFORM_DASH_LENGTH, dashLength}, std::pair{UNIFORM_DASH_GAP, dashGap},
                               std::pair{UNIFORM_DASH_OFFSET, dashOffset}})
        if (auto location = gl.GetUniformLocation(shader.id, name); location != -1)
            gl.Uniform1f(location, value);

    rect_end(rectVAO, mode, count);
}

// A layer draw's crop of its spritesheet. A blurred one (Flash's BlurFilter: a box `blur` canvas pixels wide) grows by
// the box's reach, in texels at the draw's scale, and averages its crop over it.
void Canvas::layer_render(ShaderProgram& shader, GLuint texture, Vec2 textureSize, const Mat4& transform,
                          const Draw& draw, Vec4 tint, Vec4 colorOffset) const
{
    auto blurred = draw;
    auto& frame = blurred.frame;
    TextureBlur blur = {.region = Vec4(frame.crop, frame.crop + frame.size) / Vec4(textureSize, textureSize)};
    if (frame.blur > 0.0f && frame.size.x > 0.0f && frame.size.y > 0.0f)
    {
        auto matrix = draw.parent * draw_quad_model_get(draw);
        auto canvasArea = std::abs(matrix[0][0] * matrix[1][1] - matrix[1][0] * matrix[0][1]);
        auto canvasPerTexel = std::sqrt(canvasArea / (frame.size.x * frame.size.y));
        blur.radius = canvasPerTexel > 0.0f ? std::round(frame.blur * 0.5f / canvasPerTexel) : 0.0f;
        frame.crop -= blur.radius;
        frame.size += blur.radius * 2.0f;
        frame.pivot += blur.radius;
    }
    auto vertices = uv_vertices_get(frame.crop / textureSize, (frame.crop + frame.size) / textureSize);
    texture_render(shader, texture, transform * blurred.parent * draw_quad_model_get(blurred), tint, colorOffset,
                   vertices.data(), textureSize, draw.time, blur);
}

// An animation's layers alone (no markers or helpers). Spritesheet images are looked up by draw, custom shaders by id.
void Canvas::layers_render(ShaderProgram& shader, const Model& model, const Animation& animation,
                           const DrawOptions& options, const Mat4& transform,
                           const std::function<const Image*(const Draw&)>& image_get,
                           const std::function<ShaderProgram*(int)>& shader_get) const
{
    for (auto& draw : animation_draws_get(model, animation, options))
    {
        if (draw.type != DrawType::LAYER)
            continue;
        auto& frame = draw.frame;
        auto image = image_get(draw);
        if (!image || !image->is_valid() || frame.size == Vec2())
            continue;
        auto customShader = shader_get ? shader_get(frame.shaderId) : nullptr;
        layer_render(customShader ? *customShader : shader, texture_id_get(*image), image->unit_size_get(), transform,
                     draw, frame.tint, frame.colorOffset);
    }
}

void Canvas::rect_render(ShaderProgram& shader, const Mat4& transform, const Mat4& model, Vec4 color, float dashLength,
                         float dashGap, float dashOffset) const
{
    dashed_render(shader, transform, model, color, dashLength, dashGap, dashOffset, GL_LINE_LOOP, RECT_VERTEX_COUNT);
}

// A line from start to end: the bottom edge of a rectangle laid along it.
void Canvas::line_render(ShaderProgram& shader, const Mat4& transform, Vec2 start, Vec2 end, Vec4 color,
                         float dashLength, float dashGap, float dashOffset) const
{
    auto direction = end - start;
    auto normal = vec_length(direction) > 0.0f ? vec_normalize(Vec2(-direction.y, direction.x)) : Vec2(0.0f, 1.0f);
    auto model = Mat4(Vec4(direction, 0.0f, 0.0f), Vec4(normal, 0.0f, 0.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f),
                      Vec4(start, 0.0f, 1.0f));
    dashed_render(shader, transform, model, color, dashLength, dashGap, dashOffset, GL_LINES, LINE_VERTEX_COUNT);
}

void Canvas::rect_fill_render(ShaderProgram& shader, const Mat4& transform, const Mat4& model, Vec4 color) const
{
    rect_begin(shader, transform, model, color);
    rect_end(rectVAO, GL_TRIANGLE_FAN);
}

float Canvas::zoom_level_get(float zoom, int levelDelta) const
{
    if (levelDelta == 0)
        return zoom;
    if (zoom < ZOOM_LEVELS.front())
        return ZOOM_LEVELS.front();
    if (zoom > ZOOM_LEVELS.back())
        return ZOOM_LEVELS.back();

    auto levelCount = (int)ZOOM_LEVELS.size();
    int levelIndex = 0;
    if (levelDelta > 0)
    {
        auto it = std::upper_bound(ZOOM_LEVELS.begin(), ZOOM_LEVELS.end(), zoom);
        levelIndex = (int)std::distance(ZOOM_LEVELS.begin(), it) + levelDelta - 1;
    }
    else
    {
        auto it = std::lower_bound(ZOOM_LEVELS.begin(), ZOOM_LEVELS.end(), zoom);
        levelIndex = (int)std::distance(ZOOM_LEVELS.begin(), it) + levelDelta;
    }

    return ZOOM_LEVELS[std::clamp(levelIndex, 0, levelCount - 1)];
}

void Canvas::zoom_level_adjust(float& zoom, Vec2& pan, Vec2 focus, int levelDelta) const
{
    auto zoomFactor = percent_to_unit(zoom);
    float newZoom = zoom_level_get(zoom, levelDelta);
    if (newZoom != zoom)
    {
        float newZoomFactor = percent_to_unit(newZoom);
        pan += focus * (zoomFactor - newZoomFactor);
        zoom = newZoom;
    }
}

Vec2 Canvas::position_translate(float& zoom, Vec2& pan, Vec2 position) const
{
    auto zoomFactor = percent_to_unit(zoom);
    return (position - pan - (size * 0.5f)) / zoomFactor;
}

void Canvas::set_to_rect(float& zoom, Vec2& pan, Vec4 rect) const
{
    if (rect != Vec4(-1.0f) && (rect.z > 0 && rect.w > 0))
    {
        float scaleX = size.x / rect.z;
        float scaleY = size.y / rect.w;
        float fitScale = std::min(scaleX, scaleY);

        zoom = unit_to_percent(fitScale);

        Vec2 rectCenter = {rect.x + rect.z * 0.5f, rect.y + rect.w * 0.5f};
        pan = -rectCenter * fitScale;
    }
}
