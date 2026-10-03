#pragma once

#include <map>
#include <set>

#include "model/model.hpp"

namespace anm2ed
{
  enum class SelectionKind
  {
    ANIMATIONS,
    ANIMATION_GROUPS,
    TRACKS,
    GROUPS,
    FRAMES,
    SPRITESHEETS,
    REGIONS,
    SHADERS,
    LAYERS,
    NULLS,
    EVENTS,
    SOUNDS
  };

  // What is selected, by element uid, plus the focused animation/track/frame. uids follow elements through edits,
  // so nothing here is repaired by index; the UI reads it as References through a UidIndex.
  struct Selection
  {
    std::map<SelectionKind, std::set<std::uint64_t>> uids{};
    Handle focus{};
    std::map<SelectionKind, std::uint64_t> focuses{};

    bool operator==(const Selection&) const = default;
  };

  std::set<Reference> selection_references_get(const Selection&, const model::UidIndex&, SelectionKind);
  void selection_references_set(Selection&, const model::Model&, SelectionKind, const std::set<Reference>&);
  Reference selection_focus_get(const Selection&, const model::UidIndex&);
  void selection_focus_set(Selection&, const model::Model&, Reference);
  // Content items (spritesheets, regions of the focused spritesheet, shaders, layers, ...) are selected by uid and
  // read back as ids.
  std::set<int> selection_ids_get(const Selection&, const model::Model&, SelectionKind);
  void selection_ids_set(Selection&, const model::Model&, SelectionKind, const std::set<int>&);
  int selection_focus_id_get(const Selection&, const model::Model&, SelectionKind);
  void selection_focus_id_set(Selection&, const model::Model&, SelectionKind, int);
  void selection_follow(Selection&, const model::Model&, const model::UidIndex&, Reference);
}
