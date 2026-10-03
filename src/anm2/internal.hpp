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

  struct ElementSink
  {
    virtual ~ElementSink() = default;
    virtual void open(std::string_view) = 0;
    virtual void attribute(const char*, const char*) = 0;
    virtual void attribute(const char*, int) = 0;
    virtual void attribute(const char*, bool) = 0;
    virtual void attribute(const char*, float) = 0;
    virtual void body() {}
    virtual void close() = 0;
  };

  struct BakeAttributes
  {
    Interpolation interpolation{Interpolation::NONE};
    int delay{FRAME_DURATION_MIN};
  };

  void element_emit(ElementSink&, const Element&, ElementType, Flags, std::optional<BakeAttributes> = std::nullopt);

  bool is_source_document_tag(std::string_view);
  ElementType element_type_get(std::string_view);
  std::string_view element_tag_get(ElementType);
  ElementType group_child_type_get(ElementType);
  Element* content_container_get(Element&, ElementType);
  std::unordered_map<int, int> child_ids_compact(Element&, ElementType);
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
  bool is_track_child_valid(ElementType, ElementType);
  bool is_track_group_visible(const Element&, const Element&);
  bool is_nested_group_parent(ElementType, Flags);
  bool is_group_id_serialized(Flags);
  bool element_write_skip(const Element&, ElementType, Flags);
  bool is_frame_bake_serialized(const Element&, Flags);
  bool is_special_interpolated_frames(const Element&);
  std::vector<Element> frame_bake_split(const Element&, const Element&, int, bool, bool);
  void special_interpolated_frames_bake(Element&, int, bool, bool);
  void all_interpolated_frames_bake(Element&, int, bool, bool);
  int color_write(float);
  std::uint64_t anm2_hash_get(const Element&, Options);
  std::uint64_t element_hash(const Element&, Flags);
}
