#pragma once

#include <functional>
#include <vector>

#include "model.hpp"

// What an animation draws at a time, as plain data: every canvas (preview, render, about) draws from this list.
namespace anm2ed::model
{
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
    float timeOffset{};
    int indexOffset{};
  };

  // The document a library id of a document (the one drawn, or a library) points at; nullptr when not loaded.
  using LibraryGet = std::function<const Model*(const Model&, int)>;

  struct DrawOptions
  {
    float time{};
    bool isRootTransform{};
    bool isIndexSampled{};
    std::vector<DrawSample> samples{};
    LibraryGet library_get{};
  };

  // An item's frame at a time; `parent` is the transform of its root and group root when root transforms are shown
  // (whose tint and color offset are then folded into a layer's frame). `sample` indexes the samples, -1 for now.
  // A layer draw shows spritesheet `spritesheetId` of `source`; an animation layer's child animation draws as its
  // layers' draws, with the animation layer's id and transform.
  struct Draw
  {
    DrawType type{};
    int id{-1};
    int groupType{NONE};
    int sample{-1};
    float time{};
    Frame frame{};
    glm::mat4 parent{1.0f};
    const Model* source{};
    int spritesheetId{-1};
  };

  glm::mat4 draw_quad_model_get(const Draw&);
  float draw_scale_get(const Draw&);
  std::vector<Draw> animation_draws_get(const Model&, const Animation&, const DrawOptions&);
}
