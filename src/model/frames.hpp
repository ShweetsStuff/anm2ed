#pragma once

#include <set>
#include <vector>

#include "model.hpp"

namespace anm2ed::model
{
  void frame_mix(Frame&, const Frame&, float);
  std::vector<Frame> frame_bake_split(const Frame&, const Frame&, int, bool, bool);
  glm::mat4 frame_parent_model_get(const Frame&);
  void frame_root_transform_apply(Frame&, const Frame&, bool, bool, bool);
  Frame frame_generate(const Track&, float);
  int frame_index_from_at_frame_get(const Track&, int);
  int frame_index_from_time_get(const Track&, float);
  float frame_time_from_index_get(const Track&, int);
  void frame_bake(Track&, int, int, bool, bool);
  void frames_generate_from_grid(Track&, glm::ivec2, glm::ivec2, glm::vec2, int, int, int);
  void frames_sort_by_at_frame(Track&);
  void frames_change(Track&, FrameChange, ItemType, ChangeType, const std::set<int>&);
  glm::vec4 animation_rect(const Model&, const Animation&, bool);
}
