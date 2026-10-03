#pragma once

#include <map>
#include <set>

#include "anm2/anm2.hpp"

namespace anm2ed
{
  enum class SelectionKind
  {
    ANIMATIONS,
    ANIMATION_GROUPS,
    TRACKS,
    GROUPS,
    FRAMES
  };

  // What is selected, by element uid, plus the focused animation/track/frame. uids follow elements through edits,
  // so nothing here is repaired by index; the UI reads it as References through a UidIndex.
  struct Selection
  {
    std::map<SelectionKind, std::set<std::uint64_t>> uids{};
    Handle focus{};

    bool operator==(const Selection&) const = default;
  };

  std::set<Reference> selection_references_get(const Selection&, const UidIndex&, SelectionKind);
  void selection_references_set(Selection&, const Anm2&, SelectionKind, const std::set<Reference>&);
  Reference selection_focus_get(const Selection&, const UidIndex&);
  void selection_focus_set(Selection&, const Anm2&, Reference);
  void selection_follow(Selection&, const Anm2&, const UidIndex&, Reference);
}
