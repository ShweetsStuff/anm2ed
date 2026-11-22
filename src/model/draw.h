#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "model/model.h"

// What an animation draws at a time, as plain data: every canvas (preview, render, about) draws from this list.
enum class DrawType
{
    ROOT,
    GROUP_ROOT,
    LAYER,
    NULL_
};

// An onion-skin sample: a time offset, or (when sampling by index) an offset in each track's frames.
struct DrawSample
{
    float timeOffset = 0;
    int indexOffset = 0;
};

// The document a library id of a document (the one drawn, or a library) points at. Nullptr when not loaded.
using LibraryGet = std::function<const Model*(const Model&, int)>;

struct DrawOptions
{
    float time = 0;
    bool isRootTransform = false;
    bool isIndexSampled = false;
    std::vector<DrawSample> samples;
    LibraryGet library_get = {};
};

// An item's frame at a time. `parent` is the transform of its root and group root when root transforms are shown
// (whose tint and color offset are then folded into a layer's frame). `sample` indexes the samples, -1 for now.
// A layer draw shows spritesheet `spritesheetId` of `source`. An animation layer's child animation draws as its
// layers' draws, with the animation layer's id and transform.
struct Draw
{
    DrawType type = {};
    int id = -1;
    int groupType = NONE;
    int sample = -1;
    float time = 0;
    Frame frame = {};
    Mat4 parent{1.0f};
    const Model* source = nullptr;
    int spritesheetId = -1;
    int sourceId = -1; // the layer of `source` drawn (a child's, for an animation layer's draws)
};

// A trigger that fires, from the animation played or a child animation (whose ids belong to `source`).
struct TriggerFire
{
    const Model* source = nullptr;
    Frame trigger = {};
    int layerId = -1; // the animation's layer whose child fired it. -1 for the animation's own
};

Mat4 draw_quad_model_get(const Draw&);
Mat4 frame_child_model_get(const Frame&);
std::pair<const Model*, const Animation*> child_get(const Model&, const Layer&, const Frame&, const LibraryGet&);
float child_time_get(const Track&, float, int, int);
std::optional<Mat4> layer_child_transform_get(const Animation&, int, float, bool);
float draw_scale_get(const Draw&);
std::vector<Draw> animation_draws_get(const Model&, const Animation&, const DrawOptions&);
std::vector<TriggerFire> animation_triggers_get(const Model&, const Animation&, int, const LibraryGet& = {});
