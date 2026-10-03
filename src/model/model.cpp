#include "model.hpp"

#include <algorithm>
#include <unordered_set>

namespace anm2ed::model
{
  // The slot holding the animation at a flat index (animations inside groups count in order), or nullptr.
  template <class Entries> auto animation_slot_get(Entries& entries, int index) -> decltype(&std::get<0>(entries[0]))
  {
    if (index < 0) return nullptr;
    for (auto& entry : entries)
    {
      if (auto animation = std::get_if<std::shared_ptr<const Animation>>(&entry))
      {
        if (index-- == 0) return animation;
        continue;
      }
      auto& group = std::get<AnimationGroup>(entry);
      if (index < (int)group.animations.size()) return &group.animations[index];
      index -= (int)group.animations.size();
    }
    return nullptr;
  }

  int Model::animations_count_get() const
  {
    int count{};
    for (const auto& entry : animations.entries)
      count +=
          std::holds_alternative<AnimationGroup>(entry) ? (int)std::get<AnimationGroup>(entry).animations.size() : 1;
    return count;
  }

  const Animation* Model::animation_get(int index) const
  {
    auto slot = animation_slot_get(animations.entries, index);
    return slot ? slot->get() : nullptr;
  }

  // Copy-on-write: an animation shared with undo history is cloned before it is changed.
  Animation* Model::animation_edit(int index)
  {
    auto slot = animation_slot_get(animations.entries, index);
    if (!slot || !*slot) return nullptr;
    if (slot->use_count() > 1) *slot = std::make_shared<const Animation>(**slot);
    return const_cast<Animation*>(slot->get());
  }

  std::vector<std::pair<int, const Animation*>> Model::animations_get() const
  {
    std::vector<std::pair<int, const Animation*>> result{};
    for (int i = 0; i < animations_count_get(); ++i)
      result.emplace_back(i, animation_get(i));
    return result;
  }

  template <class Self> auto animation_group_find(Self& self, int index)
  {
    using Group = std::conditional_t<std::is_const_v<Self>, const AnimationGroup, AnimationGroup>;
    Group* result{};
    if (index < 0) return result;
    for (auto& entry : self.animations.entries)
    {
      if (std::holds_alternative<std::shared_ptr<const Animation>>(entry))
      {
        if (index-- == 0) return result;
        continue;
      }
      auto& group = std::get<AnimationGroup>(entry);
      if (index < (int)group.animations.size()) return &group;
      index -= (int)group.animations.size();
    }
    return result;
  }

  const AnimationGroup* Model::animation_group_get(int index) const { return animation_group_find(*this, index); }
  AnimationGroup* Model::animation_group_edit(int index) { return animation_group_find(*this, index); }

  int Model::animation_group_id_get(int index) const
  {
    auto group = animation_group_get(index);
    return group ? group->id : -1;
  }

  template <class A> auto animation_track_group_find(A& animation, int groupType, int groupId)
  {
    using Group = std::conditional_t<std::is_const_v<A>, const TrackGroup, TrackGroup>;
    Group* result{};
    if (groupId < 0 || (groupType != LAYER && groupType != NULL_)) return result;
    groups_each(animation_entries_get(animation, groupType),
                [&](auto& group)
                {
                  if (!result && group.id == groupId) result = &group;
                });
    return result;
  }

  TrackGroup* animation_track_group_get(Animation& animation, int groupType, int groupId)
  {
    return animation_track_group_find(animation, groupType, groupId);
  }

  const TrackGroup* animation_track_group_get(const Animation& animation, int groupType, int groupId)
  {
    return animation_track_group_find(animation, groupType, groupId);
  }

  // A track by item type and id; a group root when the item is ROOT with a group, and grouped tracks only match their
  // own group when one is given.
  template <class A> auto animation_track_find(A& animation, int itemType, int id, int groupType, int groupId)
  {
    using T = std::conditional_t<std::is_const_v<A>, const Track, Track>;
    T* result{};
    auto isGrouped = groupType != NONE && groupId != -1;
    if (itemType == ROOT)
    {
      if (!isGrouped) return &animation.root;
      auto group = animation_track_group_find(animation, groupType, groupId);
      return group ? &group->root : result;
    }
    if (itemType == TRIGGER) return &animation.triggers;
    if (itemType != LAYER && itemType != NULL_) return result;

    tracks_each(animation_entries_get(animation, itemType),
                [&](auto& track, auto* group)
                {
                  if (result || track.id != id) return;
                  if (isGrouped && (!group || group->id != groupId)) return;
                  result = &track;
                });
    return result;
  }

  const Track* animation_track_get(const Animation& animation, int itemType, int id, int groupType, int groupId)
  {
    return animation_track_find(animation, itemType, id, groupType, groupId);
  }

  Track* animation_track_get(Animation& animation, int itemType, int id, int groupType, int groupId)
  {
    return animation_track_find(animation, itemType, id, groupType, groupId);
  }

  const Track* Model::track_get(Reference reference) const
  {
    auto animation = animation_get(reference.animationIndex);
    return animation ? animation_track_get(*animation, reference.itemType, reference.itemID, reference.groupType,
                                           reference.groupId)
                     : nullptr;
  }

  Track* Model::track_edit(Reference reference)
  {
    if (!track_get(reference)) return nullptr;
    auto animation = animation_edit(reference.animationIndex);
    return animation_track_get(*animation, reference.itemType, reference.itemID, reference.groupType,
                               reference.groupId);
  }

  const TrackGroup* Model::track_group_get(int animationIndex, int groupType, int groupId) const
  {
    auto animation = animation_get(animationIndex);
    return animation ? animation_track_group_get(*animation, groupType, groupId) : nullptr;
  }

  TrackGroup* Model::track_group_edit(int animationIndex, int groupType, int groupId)
  {
    if (!track_group_get(animationIndex, groupType, groupId)) return nullptr;
    return animation_track_group_get(*animation_edit(animationIndex), groupType, groupId);
  }

  const Frame* Model::frame_get(Reference reference) const
  {
    auto track = track_get(reference);
    if (!track || reference.frameIndex < 0 || reference.frameIndex >= (int)track->frames.size()) return nullptr;
    return &track->frames[reference.frameIndex];
  }

  Frame* Model::frame_edit(Reference reference)
  {
    if (!frame_get(reference)) return nullptr;
    return &track_edit(reference)->frames[reference.frameIndex];
  }

  const Spritesheet* Model::layer_spritesheet_get(int layerId) const
  {
    auto layer = item_get(content.layers, layerId);
    return layer ? item_get(content.spritesheets, layer->spritesheetId) : nullptr;
  }

  // The frame with its region's crop/size/pivot applied, as it is drawn.
  Frame Model::frame_effective(int layerId, const Frame& frame) const
  {
    auto resolved = frame;
    auto spritesheet = frame.regionId == -1 ? nullptr : layer_spritesheet_get(layerId);
    auto region = spritesheet ? item_get(spritesheet->regions, frame.regionId) : nullptr;
    if (!region) return resolved;
    resolved.crop = region->crop;
    resolved.size = region->size;
    resolved.pivot = region->pivot;
    return resolved;
  }

  template <class Items> std::set<int> unused_ids_get(const Items& items, const std::set<int>& used)
  {
    std::set<int> unused{};
    for (const auto& item : items)
      if (!used.contains(item.id)) unused.insert(item.id);
    return unused;
  }

  std::set<int> Model::unused_get(ElementType type, const Animation* only) const
  {
    std::set<int> used{};
    auto used_insert = [&](const Animation& animation)
    {
      if (type == ElementType::LAYER_ELEMENT || type == ElementType::NULL_ELEMENT)
        tracks_each(type == ElementType::LAYER_ELEMENT ? animation.layers : animation.nulls,
                    [&](const Track& track, auto*) { used.insert(track.id); });
      for (const auto& trigger : animation.triggers.frames)
      {
        if (type == ElementType::EVENT_ELEMENT) used.insert(trigger.eventId);
        if (type == ElementType::SOUND_ELEMENT) used.insert(trigger.soundIds.begin(), trigger.soundIds.end());
      }
      if (type == ElementType::SHADER)
        tracks_each(animation.layers,
                    [&](const Track& track, auto*)
                    {
                      for (const auto& frame : track.frames)
                        used.insert(frame.shaderId);
                    });
    };

    if (type == ElementType::SPRITESHEET)
      for (const auto& layer : content.layers)
        used.insert(layer.spritesheetId);
    else if (only)
      used_insert(*only);
    else
      for (auto [index, animation] : animations_get())
        used_insert(*animation);

    switch (type)
    {
      case ElementType::SPRITESHEET:
        return unused_ids_get(content.spritesheets, used);
      case ElementType::SHADER:
        return unused_ids_get(content.shaders, used);
      case ElementType::LAYER_ELEMENT:
        return unused_ids_get(content.layers, used);
      case ElementType::NULL_ELEMENT:
        return unused_ids_get(content.nulls, used);
      case ElementType::EVENT_ELEMENT:
        return unused_ids_get(content.events, used);
      case ElementType::SOUND_ELEMENT:
        return unused_ids_get(content.sounds, used);
      default:
        return {};
    }
  }

  std::set<int> Model::region_unused_get(int spritesheetId) const
  {
    std::set<int> used{};
    for (auto [index, animation] : animations_get())
      tracks_each(animation->layers,
                  [&](const Track& track, auto*)
                  {
                    auto layer = item_get(content.layers, track.id);
                    if (!layer || layer->spritesheetId != spritesheetId) return;
                    for (const auto& frame : track.frames)
                      used.insert(frame.regionId);
                  });
    auto spritesheet = item_get(content.spritesheets, spritesheetId);
    return spritesheet ? unused_ids_get(spritesheet->regions, used) : std::set<int>{};
  }

  Handle Model::handle_get(Reference reference, bool isGroup) const
  {
    Handle handle{};
    auto animation = animation_get(reference.animationIndex);
    if (!animation) return handle;
    handle.animation = animation->uid;

    if (isGroup)
    {
      if (auto group = animation_track_group_get(*animation, reference.itemType, reference.itemID))
        handle.item = group->uid;
      return handle;
    }

    auto track =
        animation_track_get(*animation, reference.itemType, reference.itemID, reference.groupType, reference.groupId);
    if (!track) return handle;
    handle.item = track->uid;
    if (reference.frameIndex >= 0 && reference.frameIndex < (int)track->frames.size())
      handle.frame = track->frames[reference.frameIndex].uid;
    return handle;
  }

  std::uint64_t Model::uid_get(Reference reference, bool isGroup) const
  {
    auto handle = handle_get(reference, isGroup);
    return handle.frame ? handle.frame : handle.item ? handle.item : handle.animation;
  }

  template <class Callback> void animation_uids_each(Animation& animation, Callback&& callback)
  {
    callback(animation.uid);
    for (auto* entries : {&animation.layers, &animation.nulls})
      groups_each(*entries, [&](TrackGroup& group) { callback(group.uid); });
    animation_tracks_each(animation,
                          [&](Track& track)
                          {
                            callback(track.uid);
                            for (auto& frame : track.frames)
                              callback(frame.uid);
                          });
  }

  // Gives any element that shares a uid with an earlier one a fresh uid (copies made without cloning).
  void Model::uids_repair()
  {
    std::unordered_set<std::uint64_t> seen{};
    auto repair = [&](std::uint64_t& uid)
    {
      if (!uid || !seen.insert(uid).second) uid = util::uid_next();
    };

    for (int i = 0; i < animations_count_get(); ++i)
    {
      auto seenBefore = seen;
      bool isDuplicate{};
      animation_uids_each(const_cast<Animation&>(*animation_get(i)),
                          [&](std::uint64_t& uid) { isDuplicate |= !uid || !seen.insert(uid).second; });
      if (!isDuplicate) continue;
      seen = std::move(seenBefore);
      animation_uids_each(*animation_edit(i), repair);
    }
    for (auto& entry : animations.entries)
      if (auto group = std::get_if<AnimationGroup>(&entry)) repair(group->uid);
    for (auto& spritesheet : content.spritesheets)
    {
      repair(spritesheet.uid);
      for (auto& region : spritesheet.regions)
        repair(region.uid);
    }
    for (auto& shader : content.shaders)
      repair(shader.uid);
    for (auto& layer : content.layers)
      repair(layer.uid);
    for (auto& null : content.nulls)
      repair(null.uid);
    for (auto& event : content.events)
      repair(event.uid);
    for (auto& sound : content.sounds)
      repair(sound.uid);
  }

  int track_frames_count_get(const Track& track) { return (int)track.frames.size(); }

  int track_length_get(const Track& track)
  {
    int length{};
    for (const auto& frame : track.frames)
      length = track.type == ItemType::TRIGGER ? std::max(length, frame.atFrame) : length + frame.duration;
    return length;
  }

  int animation_length_get(const Animation& animation)
  {
    int length{};
    animation_tracks_each(animation, [&](const Track& track) { length = std::max(length, track_length_get(track)); });
    return std::max(length, FRAME_DURATION_MIN);
  }

  Frame frame_clone(Frame frame)
  {
    frame.uid = util::uid_next();
    return frame;
  }

  Track track_clone(Track track)
  {
    track.uid = util::uid_next();
    for (auto& frame : track.frames)
      frame.uid = util::uid_next();
    return track;
  }

  Animation animation_clone(Animation animation)
  {
    animation_uids_each(animation, [](std::uint64_t& uid) { uid = util::uid_next(); });
    return animation;
  }

  void track_locations_add(std::unordered_map<std::uint64_t, UidIndex::Location>& locations, const Track& track,
                           Reference reference)
  {
    constexpr ElementType TRACK_TYPES[] = {ElementType::UNKNOWN, ElementType::ROOT_ANIMATION,
                                           ElementType::LAYER_ANIMATION, ElementType::NULL_ANIMATION,
                                           ElementType::TRIGGERS};
    locations[track.uid] = {reference, TRACK_TYPES[(int)track.type]};
    for (const auto& frame : track.frames)
    {
      ++reference.frameIndex;
      locations[frame.uid] = {reference, track.type == ItemType::TRIGGER ? ElementType::TRIGGER : ElementType::FRAME};
    }
  }

  UidIndex::UidIndex(const Model& model)
  {
    for (auto [index, animation] : model.animations_get())
    {
      locations[animation->uid] = {{index}, ElementType::ANIMATION};
      track_locations_add(locations, animation->root, {index, ROOT});
      track_locations_add(locations, animation->triggers, {index, TRIGGER});
      for (auto type : {LAYER, NULL_})
        for (const auto& entry : animation_entries_get(*animation, type))
          if (auto track = std::get_if<Track>(&entry))
            track_locations_add(locations, *track, {index, type, track->id});
          else
          {
            const auto& group = std::get<TrackGroup>(entry);
            locations[group.uid] = {{index, type, group.id}, ElementType::GROUP};
            track_locations_add(locations, group.root, {index, ROOT, -1, -1, type, group.id});
            for (const auto& groupTrack : group.tracks)
              track_locations_add(locations, groupTrack, {index, type, groupTrack.id, -1, type, group.id});
          }
    }
  }

  bool UidIndex::contains(std::uint64_t uid) const { return uid && locations.contains(uid); }

  std::optional<Reference> UidIndex::reference_get(std::uint64_t uid) const
  {
    auto it = locations.find(uid);
    return it == locations.end() ? std::nullopt : std::optional<Reference>(it->second.reference);
  }

  ElementType UidIndex::type_get(std::uint64_t uid) const
  {
    auto it = locations.find(uid);
    return it == locations.end() ? ElementType::UNKNOWN : it->second.type;
  }

  // Equality by value; animations shared with another model compare by pointer first.
  bool is_model_equal(const Model& left, const Model& right)
  {
    if (left.info != right.info || left.content != right.content || left.layout != right.layout ||
        left.extras != right.extras || left.isValid != right.isValid ||
        left.animations.defaultAnimation != right.animations.defaultAnimation ||
        left.animations.extras != right.animations.extras ||
        left.animations.entries.size() != right.animations.entries.size())
      return false;

    auto is_animation_equal = [](const std::shared_ptr<const Animation>& a, const std::shared_ptr<const Animation>& b)
    { return a == b || (a && b && *a == *b); };
    for (std::size_t i = 0; i < left.animations.entries.size(); ++i)
    {
      const auto& a = left.animations.entries[i];
      const auto& b = right.animations.entries[i];
      if (a.index() != b.index()) return false;
      if (auto animation = std::get_if<std::shared_ptr<const Animation>>(&a))
      {
        if (!is_animation_equal(*animation, std::get<std::shared_ptr<const Animation>>(b))) return false;
        continue;
      }
      const auto& groupA = std::get<AnimationGroup>(a);
      const auto& groupB = std::get<AnimationGroup>(b);
      if (groupA.uid != groupB.uid || groupA.id != groupB.id || groupA.name != groupB.name ||
          groupA.isExpanded != groupB.isExpanded || groupA.isVisible != groupB.isVisible ||
          groupA.extras != groupB.extras || groupA.animations.size() != groupB.animations.size())
        return false;
      for (std::size_t j = 0; j < groupA.animations.size(); ++j)
        if (!is_animation_equal(groupA.animations[j], groupB.animations[j])) return false;
    }
    return true;
  }
}
