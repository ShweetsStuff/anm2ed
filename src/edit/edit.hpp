#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "anm2/anm2.hpp"

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

  Element element_clone(Element);
  Element* track_get(Anm2&, Reference);
  std::map<Reference, std::set<int>> frames_by_track_get(const std::set<Reference>&);

  Uids frame_insert(Anm2&, Reference, int);
  Uids frames_delete(Anm2&, const std::set<Reference>&);
  Uids frames_duplicate(Anm2&, const std::set<Reference>&, Reference);
  Uids frames_reverse(Anm2&, const std::set<Reference>&);
  Uids frames_bake(Anm2&, const std::set<Reference>&, int, bool, bool);
  Uids frame_split(Anm2&, Reference, int);
  Uids frames_move(Anm2&, const std::set<Reference>&, Reference, int);
  Uids frame_durations_set(Anm2&, const std::map<Reference, int>&);
  Uids trigger_at_frame_set(Anm2&, Reference, int);
  Uids frames_paste(Anm2&, Reference, const std::set<Reference>&, const std::string&, int, std::string*);
  Uids root_bake_into(Anm2&, const std::set<Reference>&, const std::set<Reference>&, RootBakeOptions);
  void frame_root_transform_apply(Element&, const Element&, bool, bool, bool);

  Uids items_remove(Anm2&, int, const std::map<int, std::set<int>>&, const std::map<int, std::set<int>>&);
  Uids items_group(Anm2&, int, int, const std::set<int>&, const std::string&);
  Uids items_move(Anm2&, int, int, const std::set<int>&, const std::set<int>&, RowTarget, bool, bool);
  Uids animation_length_fit(Anm2&, int);

  std::set<int> animation_group_ids_get(const Element&);
  std::set<int> animation_group_indices_get(const Element&, int);
  void animation_groups_remove(Element&, const std::set<int>&);
  Uids animation_add(Anm2&, int, int, int, const std::string&);
  Uids animations_remove(Anm2&, const std::set<int>&, const std::set<int>&);
  Uids animations_move(Anm2&, const std::set<int>&, const std::set<int>&, int, int);
  Uids animations_group(Anm2&, const std::set<int>&, const std::string&);
  Uids animations_paste(Anm2&, const std::string&, int, int, std::set<int>&, std::string*);
  Uids regions_remove_unused(Anm2&, int);
  Uids regions_paste(Anm2&, int, const std::string&, int, std::string*);
  Uids animation_grid_generate(Anm2&, const std::vector<Reference>&, const GridOptions&);
  Uids frames_change_apply(Anm2&, const ChangeTargets&, FrameChange, ChangeType);
  Uids animations_merge(Anm2&, int, std::set<int>, types::merge::Type, bool, const std::set<int>&);
}
