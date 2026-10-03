#pragma once

#include "anm2.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <tinyxml2/tinyxml2.h>

#include "file.hpp"
#include "math.hpp"
#include "path.hpp"
#include "time.hpp"
#include "working_directory.hpp"
#include "xml.hpp"

namespace anm2ed
{
  inline constexpr std::string_view ANIMATION_MERGED_SUFFIX = " (Merged)";
  inline constexpr std::array<std::string_view, (std::size_t)Interpolation::COUNT> INTERPOLATION_VALUES = {
      "", "", "EaseIn", "EaseOut", "EaseInOut"};
  inline constexpr std::array<std::string_view, (std::size_t)Origin::COUNT> ORIGIN_VALUES = {"", "TopLeft", "Center"};
  inline constexpr std::string_view SOURCE_DOCUMENT_TAG = "SourceDocument";

  bool is_source_document_tag(std::string_view);
  ElementType element_type_get(std::string_view);
  std::string_view element_tag_get(ElementType);
  ElementType element_container_type_get(ElementType);
  int animations_count_get(const Element&);
  int animations_child_index_get(const Element&, int);
  int animations_child_insert_index_get(const Element&, int);
  Element root_animation_make();
  void group_frames_bake(Element&);
  void group_frames_restore(Element&);
  void group_metadata_embed(Element&);
  void group_metadata_extract(Element&, Flags);
  void groups_flatten(Element&);
  void groups_nest(Element&);
  void source_document_erase(Element&);
  void shader_ids_repair(Element&);
  void shader_frame_ids_repair(Element&);
  void shader_ids_remap(Element&);
  void region_ids_remap(Element&);
  void region_frame_ids_repair(Element&);
  void layer_animation_ids_remap(Element&, const std::unordered_map<int, int>&);
  Element* child_first_get(Element&, ElementType);
  const Element* child_first_get(const Element&, ElementType);
  Element* child_id_get(Element&, ElementType, int);
  const Element* child_id_get(const Element&, ElementType, int);
  bool is_track(const Element&);
  bool is_track_child_valid(ElementType, ElementType);
  bool is_track_group_visible(const Element&, const Element&);
  bool is_nested_group_parent(ElementType, Flags);
  bool is_group_id_serialized(Flags);
  bool element_write_skip(const Element&, ElementType, Flags);
  bool is_frame_bake_serialized(const Element&, Flags);
  bool is_special_interpolated_frames(const Element&);
  void special_interpolated_frames_bake(Element&, int, bool, bool);
  void all_interpolated_frames_bake(Element&, int, bool, bool);
  float interpolation_factor(Interpolation, float);
  int color_write(float);
  std::uint64_t anm2_hash_get(const Element&, Options);
  std::uint64_t element_hash(const Element&, Flags);
  ElementType track_frame_type_get(const Element&);
  ElementType item_type_to_container_type_get(ItemType);
  int track_id_get(const Element&);
  int track_frame_child_index_get(const Element&, int);
  int track_frames_count_get(const Element&);
  int track_frame_insert_child_index_get(const Element&, int);
  Element* track_find(Element&, ElementType, int);
  const Element* track_find(const Element&, ElementType, int);
  Element* track_group_find(Element&, ElementType, int, int);
  const Element* track_group_find(const Element&, ElementType, int, int);
  Element* animation_container_get(Element&, ElementType);

  template <typename Callback> void tracks_each(Element& parent, ElementType trackType, Callback&& callback)
  {
    for (auto& child : parent.children)
      if (child.type == trackType)
        callback(child);
      else if (child.type == ElementType::GROUP)
        tracks_each(child, trackType, callback);
  }

  template <typename Callback> void tracks_each(const Element& parent, ElementType trackType, Callback&& callback)
  {
    for (const auto& child : parent.children)
      if (child.type == trackType)
        callback(child);
      else if (child.type == ElementType::GROUP)
        tracks_each(child, trackType, callback);
  }
}
