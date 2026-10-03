#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "model/model.hpp"

// Document edits as plain functions of the model. Each returns the uids of the elements it created or changed that
// the caller may want to select (the first one is the focus); selection itself stays with the UI.
namespace anm2ed::edit
{
  using Uids = std::vector<std::uint64_t>;

  struct RowTarget
  {
    bool isGroup{};
    int type{NONE};
    int id{-1};
    int groupId{-1};
  };

  struct RootBakeOptions
  {
    bool isRoundScale{};
    bool isRoundRotation{};
    bool isMatchRootInterpolation{};
    bool isUseRootPivot{};
  };

  struct GridOptions
  {
    glm::ivec2 start{};
    glm::ivec2 size{};
    glm::vec2 pivot{};
    int columns{};
    int count{};
    int delay{};
    bool isMakeRegions{};
    std::string regionNameFormat{};
  };

  struct ChangeTargets
  {
    std::set<Reference> references{};
    std::set<int> animations{};
    bool isRoot{};
    bool isLayers{};
    bool isNulls{};
  };

  struct AnimationTarget
  {
    int animationIndex{-1};
    int groupId{-1};
    bool isAfter{};
    bool isInto{};
  };

  std::map<Reference, std::set<int>> frames_by_track_get(const std::set<Reference>&);

  Uids frame_insert(model::Model&, Reference, int);
  Uids frames_delete(model::Model&, const std::set<Reference>&);
  Uids frames_duplicate(model::Model&, const std::set<Reference>&, Reference);
  Uids frames_reverse(model::Model&, const std::set<Reference>&);
  Uids frames_bake(model::Model&, const std::set<Reference>&, int, bool, bool);
  Uids frame_split(model::Model&, Reference, int);
  Uids frames_move(model::Model&, const std::set<Reference>&, Reference, int);
  Uids frame_durations_set(model::Model&, const std::map<Reference, int>&);
  Uids trigger_at_frame_set(model::Model&, Reference, int);
  Uids triggers_sort(model::Model&, Reference);
  Uids frames_paste(model::Model&, Reference, const std::set<Reference>&, const std::string&, int, std::string*);
  Uids root_bake_into(model::Model&, const std::set<Reference>&, const std::set<Reference>&, RootBakeOptions);
  void frame_root_transform_apply(model::Frame&, const model::Frame&, bool, bool, bool);

  Uids items_remove(model::Model&, int, const std::map<int, std::set<int>>&, const std::map<int, std::set<int>>&);
  Uids items_group(model::Model&, int, int, const std::set<int>&, const std::string&);
  Uids items_move(model::Model&, int, int, const std::set<int>&, const std::set<int>&, RowTarget, bool, bool);
  Uids animation_length_fit(model::Model&, int);
  int item_add(model::Model&, ItemType, int, int, const std::string&, int, bool, int, types::destination::Type);

  std::set<int> animation_group_ids_get(const model::Model&);
  std::set<int> animation_group_indices_get(const model::Model&, int);
  void animation_groups_remove(model::Model&, const std::set<int>&);
  Uids animation_add(model::Model&, int, int, int, const std::string&);
  Uids animations_remove(model::Model&, const std::set<int>&, const std::set<int>&);
  Uids animations_move(model::Model&, const std::set<int>&, const std::set<int>&, AnimationTarget);
  Uids animations_group(model::Model&, const std::set<int>&, const std::string&);
  Uids animations_paste(model::Model&, const std::string&, int, int, std::set<int>&, std::string*);
  Uids animations_merge(model::Model&, int, std::set<int>, types::merge::Type, bool, const std::set<int>&);

  std::string region_name_get(const std::string&, int);
  Uids regions_generate(model::Model&, const std::set<int>&, const std::set<Reference>&, const std::string&,
                        RegionFrameMapping);
  Uids regions_scan(model::Model&);
  Uids region_frames_sync(model::Model&);
  Uids regions_remove_unused(model::Model&, int);
  Uids regions_paste(model::Model&, int, const std::string&, int, std::string*);
  Uids animation_grid_generate(model::Model&, const std::vector<Reference>&, const GridOptions&);
  Uids frames_change_apply(model::Model&, const ChangeTargets&, FrameChange, ChangeType);
  bool file_merge(model::Model&, const std::filesystem::path&, const std::filesystem::path&, FileMergePreset);
}
