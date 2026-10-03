#pragma once

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

  struct DrawOptions
  {
    float time{};
    bool isRootTransform{};
    bool isIndexSampled{};
    std::vector<DrawSample> samples{};
  };

  // An item's frame at a time; `parent` is the transform of its root and group root when root transforms are shown
  // (whose tint and color offset are then folded into a layer's frame). `sample` indexes the samples, -1 for now.
  struct Draw
  {
    DrawType type{};
    int id{-1};
    int groupType{NONE};
    int sample{-1};
    float time{};
    Frame frame{};
    glm::mat4 parent{1.0f};
  };

  glm::mat4 draw_quad_model_get(const Draw&);
  std::vector<Draw> animation_draws_get(const Model&, const Animation&, const DrawOptions&);
}
