#include "selection.hpp"

namespace anm2ed
{
  std::set<Reference> selection_references_get(const Selection& selection, const UidIndex& index, SelectionKind kind)
  {
    std::set<Reference> references{};
    if (auto uids = selection.uids.find(kind); uids != selection.uids.end())
      for (auto uid : uids->second)
        if (auto reference = index.reference_get(uid)) references.insert(*reference);
    return references;
  }

  std::uint64_t handle_uid_get(Handle handle)
  {
    return handle.frame ? handle.frame : handle.item ? handle.item : handle.animation;
  }

  void selection_references_set(Selection& selection, const Anm2& anm2, SelectionKind kind,
                                const std::set<Reference>& references)
  {
    auto& uids = selection.uids[kind];
    uids.clear();
    for (auto reference : references)
      if (auto uid = handle_uid_get(anm2.handle_get(reference, kind == SelectionKind::GROUPS))) uids.insert(uid);
  }

  // The most specific part of the focus that still exists.
  Reference selection_focus_get(const Selection& selection, const UidIndex& index)
  {
    for (auto uid : {selection.focus.frame, selection.focus.item, selection.focus.animation})
      if (auto reference = index.reference_get(uid)) return *reference;
    return {};
  }

  void selection_focus_set(Selection& selection, const Anm2& anm2, Reference reference)
  {
    selection.focus = anm2.handle_get(reference);
  }

  // After an edit: drops uids that no longer exist, and moves a focus whose element was removed to its nearest
  // survivor (a frame at the same index of its track, the animation's root track, or the nearest animation).
  void selection_follow(Selection& selection, const Anm2& anm2, const UidIndex& index, Reference before)
  {
    for (auto& [kind, uids] : selection.uids)
      std::erase_if(uids, [&](std::uint64_t uid) { return !index.contains(uid); });

    auto [animationUid, itemUid, frameUid] = selection.focus;
    auto is_removed = [&](std::uint64_t uid) { return uid && !index.contains(uid); };
    auto reference = selection_focus_get(selection, index);

    if (is_removed(animationUid) || !animationUid)
    {
      auto animations = anm2.element_get(ElementType::ANIMATIONS);
      auto count = animations ? animations_count_get(*animations) : 0;
      reference =
          count > 0 && animationUid ? Reference{std::clamp(before.animationIndex, 0, count - 1), ROOT} : Reference{};
    }
    else if (is_removed(itemUid))
      reference = {reference.animationIndex, ROOT};
    else if (is_removed(frameUid))
    {
      auto track = anm2.element_get(reference);
      auto count = track ? track_frames_count_get(*track) : 0;
      reference.frameIndex = count > 0 ? std::clamp(before.frameIndex, 0, count - 1) : -1;
    }
    else
      return;

    selection_focus_set(selection, anm2, reference);
  }
}
