#include "selection.hpp"

namespace anm2ed
{
  std::set<Reference> selection_references_get(const Selection& selection, const model::UidIndex& index,
                                               SelectionKind kind)
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

  void selection_references_set(Selection& selection, const model::Model& model, SelectionKind kind,
                                const std::set<Reference>& references)
  {
    auto& uids = selection.uids[kind];
    uids.clear();
    for (auto reference : references)
      if (auto uid = handle_uid_get(model.handle_get(reference, kind == SelectionKind::GROUPS))) uids.insert(uid);
  }

  // The most specific part of the focus that still exists.
  Reference selection_focus_get(const Selection& selection, const model::UidIndex& index)
  {
    for (auto uid : {selection.focus.frame, selection.focus.item, selection.focus.animation})
      if (auto reference = index.reference_get(uid)) return *reference;
    return {};
  }

  void selection_focus_set(Selection& selection, const model::Model& model, Reference reference)
  {
    selection.focus = model.handle_get(reference);
  }

  // (id, uid) of every content item a kind selects from.
  std::vector<std::pair<int, std::uint64_t>> content_keys_get(const Selection& selection, const model::Model& model,
                                                              SelectionKind kind)
  {
    std::vector<std::pair<int, std::uint64_t>> keys{};
    auto add = [&](const auto& items)
    {
      for (const auto& item : items)
        keys.emplace_back(item.id, item.uid);
    };
    auto& content = model.content;
    if (kind == SelectionKind::SPRITESHEETS) add(content.spritesheets);
    if (kind == SelectionKind::SHADERS) add(content.shaders);
    if (kind == SelectionKind::LAYERS) add(content.layers);
    if (kind == SelectionKind::NULLS) add(content.nulls);
    if (kind == SelectionKind::EVENTS) add(content.events);
    if (kind == SelectionKind::SOUNDS) add(content.sounds);
    if (kind == SelectionKind::REGIONS)
      if (auto spritesheet = model::item_get(content.spritesheets,
                                             selection_focus_id_get(selection, model, SelectionKind::SPRITESHEETS)))
        add(spritesheet->regions);
    return keys;
  }

  std::set<int> selection_ids_get(const Selection& selection, const model::Model& model, SelectionKind kind)
  {
    std::set<int> ids{};
    auto uids = selection.uids.find(kind);
    if (uids == selection.uids.end()) return ids;
    for (auto [id, uid] : content_keys_get(selection, model, kind))
      if (uids->second.contains(uid)) ids.insert(id);
    return ids;
  }

  void selection_ids_set(Selection& selection, const model::Model& model, SelectionKind kind, const std::set<int>& ids)
  {
    auto& uids = selection.uids[kind];
    uids.clear();
    for (auto [id, uid] : content_keys_get(selection, model, kind))
      if (ids.contains(id)) uids.insert(uid);
  }

  int selection_focus_id_get(const Selection& selection, const model::Model& model, SelectionKind kind)
  {
    auto focus = selection.focuses.find(kind);
    if (focus == selection.focuses.end()) return -1;
    for (auto [id, uid] : content_keys_get(selection, model, kind))
      if (uid == focus->second) return id;
    return -1;
  }

  void selection_focus_id_set(Selection& selection, const model::Model& model, SelectionKind kind, int id)
  {
    selection.focuses.erase(kind);
    for (auto [itemId, uid] : content_keys_get(selection, model, kind))
      if (itemId == id)
      {
        selection.focuses[kind] = uid;
        return;
      }
  }

  // After an edit: drops uids that no longer exist, and moves a focus whose element was removed to its nearest
  // survivor (a frame at the same index of its track, the animation's root track, or the nearest animation).
  void selection_follow(Selection& selection, const model::Model& model, const model::UidIndex& index, Reference before)
  {
    for (auto& [kind, uids] : selection.uids)
      if (kind < SelectionKind::SPRITESHEETS)
        std::erase_if(uids, [&](std::uint64_t uid) { return !index.contains(uid); });

    auto [animationUid, itemUid, frameUid] = selection.focus;
    auto is_removed = [&](std::uint64_t uid) { return uid && !index.contains(uid); };
    auto reference = selection_focus_get(selection, index);

    if (is_removed(animationUid) || !animationUid)
    {
      auto count = model.animations_count_get();
      reference =
          count > 0 && animationUid ? Reference{std::clamp(before.animationIndex, 0, count - 1), ROOT} : Reference{};
    }
    else if (is_removed(itemUid))
      reference = {reference.animationIndex, ROOT};
    else if (is_removed(frameUid))
    {
      auto track = model.track_get(reference);
      auto count = track ? (int)track->frames.size() : 0;
      reference.frameIndex = count > 0 ? std::clamp(before.frameIndex, 0, count - 1) : -1;
    }
    else
      return;

    selection_focus_set(selection, model, reference);
  }
}
