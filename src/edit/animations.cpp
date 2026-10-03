#include "edit.hpp"

#include "model/xml.hpp"

using namespace anm2ed::model;

namespace anm2ed::edit
{
  constexpr std::string_view ANIMATION_MERGED_SUFFIX = " (Merged)";

  using AnimationPointer = std::shared_ptr<const Animation>;

  std::set<int> animation_group_ids_get(const Model& model)
  {
    std::set<int> ids{};
    for (const auto& entry : model.animations.entries)
      if (auto group = std::get_if<AnimationGroup>(&entry)) ids.insert(group->id);
    return ids;
  }

  std::set<int> animation_group_indices_get(const Model& model, int groupId)
  {
    std::set<int> indices{};
    for (int i = 0; i < model.animations_count_get(); ++i)
      if (model.animation_group_id_get(i) == groupId) indices.insert(i);
    return indices;
  }

  // Animations and whole groups taken out of the list, remembering the order they had.
  struct AnimationsTaken
  {
    std::vector<AnimationPointer> animations{};
    std::vector<AnimationGroup> groups{};
  };

  AnimationsTaken animations_take(Model& model, const std::set<int>& indices, const std::set<int>& groupIds)
  {
    AnimationsTaken taken{};
    int index{};
    std::erase_if(model.animations.entries,
                  [&](AnimationEntry& entry)
                  {
                    if (auto animation = std::get_if<AnimationPointer>(&entry))
                    {
                      if (!indices.contains(index++)) return false;
                      taken.animations.push_back(std::move(*animation));
                      return true;
                    }
                    auto& group = std::get<AnimationGroup>(entry);
                    if (groupIds.contains(group.id))
                    {
                      index += (int)group.animations.size();
                      taken.groups.push_back(std::move(group));
                      return true;
                    }
                    std::erase_if(group.animations,
                                  [&](AnimationPointer& animation)
                                  {
                                    if (!indices.contains(index++)) return false;
                                    taken.animations.push_back(std::move(animation));
                                    return true;
                                  });
                    return false;
                  });
    return taken;
  }

  // Where an animation with this uid sits: its top-level entry, and its group (or nullptr) and position there.
  struct AnimationPlace
  {
    int entry{-1};
    AnimationGroup* group{};
    int position{-1};
  };

  AnimationPlace animation_place_get(Model& model, std::uint64_t uid)
  {
    auto& entries = model.animations.entries;
    for (int i = 0; i < (int)entries.size(); ++i)
    {
      if (auto animation = std::get_if<AnimationPointer>(&entries[i]))
      {
        if ((*animation)->uid == uid) return {i, nullptr, -1};
        continue;
      }
      auto& group = std::get<AnimationGroup>(entries[i]);
      for (int j = 0; j < (int)group.animations.size(); ++j)
        if (group.animations[j]->uid == uid) return {i, &group, j};
    }
    return {};
  }

  int animation_group_entry_get(const Model& model, int groupId)
  {
    auto& entries = model.animations.entries;
    for (int i = 0; i < (int)entries.size(); ++i)
      if (auto group = std::get_if<AnimationGroup>(&entries[i]); group && group->id == groupId) return i;
    return -1;
  }

  AnimationGroup* animation_group_find(Model& model, int groupId)
  {
    auto entry = animation_group_entry_get(model, groupId);
    return entry == -1 ? nullptr : &std::get<AnimationGroup>(model.animations.entries[entry]);
  }

  // Inserts animations before the animation at a flat index (or at the end), inside `groupId` when given.
  void animations_insert(Model& model, int index, int groupId, std::vector<AnimationPointer> animations)
  {
    auto before = model.animation_get(index);
    auto place = before ? animation_place_get(model, before->uid) : AnimationPlace{};
    if (auto group = animation_group_find(model, groupId))
    {
      auto position = place.group == group ? place.position : (int)group->animations.size();
      group->animations.insert(group->animations.begin() + position, animations.begin(), animations.end());
      return;
    }
    auto& entries = model.animations.entries;
    auto position = place.entry == -1 ? (int)entries.size() : place.entry;
    entries.insert(entries.begin() + position, animations.begin(), animations.end());
  }

  void animation_groups_remove(Model& model, const std::set<int>& groupIds)
  {
    auto& entries = model.animations.entries;
    for (int i = 0; i < (int)entries.size(); ++i)
    {
      auto group = std::get_if<AnimationGroup>(&entries[i]);
      if (!group || !groupIds.contains(group->id)) continue;
      auto animations = std::move(group->animations);
      entries.erase(entries.begin() + i);
      entries.insert(entries.begin() + i, animations.begin(), animations.end());
      i += (int)animations.size() - 1;
    }
  }

  // An empty copy of a track list: same tracks and groups, no frames.
  std::vector<TrackEntry> track_entries_shell_get(const std::vector<TrackEntry>& entries)
  {
    std::vector<TrackEntry> shell{};
    auto track_shell_get = [](const Track& track)
    { return Track{.type = track.type, .id = track.id, .isVisible = track.isVisible}; };
    for (const auto& entry : entries)
      if (auto track = std::get_if<Track>(&entry))
        shell.emplace_back(track_shell_get(*track));
      else
      {
        const auto& group = std::get<TrackGroup>(entry);
        TrackGroup copy{
            .id = group.id, .name = group.name, .isExpanded = group.isExpanded, .isVisible = group.isVisible};
        for (const auto& groupTrack : group.tracks)
          copy.tracks.push_back(track_shell_get(groupTrack));
        shell.emplace_back(std::move(copy));
      }
    return shell;
  }

  // Adds an animation with the same (empty) tracks as the template animation.
  Uids animation_add(Model& model, int index, int groupId, int templateIndex, const std::string& name)
  {
    Animation animation{.name = name};
    if (auto source = model.animation_get(templateIndex))
    {
      animation.layers = track_entries_shell_get(source->layers);
      animation.nulls = track_entries_shell_get(source->nulls);
    }
    if (model.animations_count_get() == 0) model.animations.defaultAnimation = animation.name;
    auto uid = animation.uid;
    animations_insert(model, index, groupId, {std::make_shared<const Animation>(std::move(animation))});
    return {uid};
  }

  Uids animations_remove(Model& model, const std::set<int>& indices, const std::set<int>& groupIds)
  {
    animations_take(model, indices, {});
    animation_groups_remove(model, groupIds);
    return {};
  }

  // Moves animations and whole groups beside a target animation or group; animations dropped on their own join the
  // target's group (or the group dropped into).
  Uids animations_move(Model& model, const std::set<int>& indices, const std::set<int>& groupIds,
                       AnimationTarget target)
  {
    auto targetAnimation = model.animation_get(target.animationIndex);
    auto targetUid = targetAnimation ? targetAnimation->uid : 0;
    if (targetAnimation && groupIds.contains(model.animation_group_id_get(target.animationIndex))) return {};
    if (targetAnimation && indices.contains(target.animationIndex)) return {};
    if (!targetAnimation && groupIds.contains(target.groupId)) return {};

    auto taken = animations_take(model, indices, groupIds);
    if (taken.animations.empty() && taken.groups.empty()) return {};

    Uids uids{};
    for (const auto& animation : taken.animations)
      uids.push_back(animation->uid);

    auto& entries = model.animations.entries;
    auto topIndex = (int)entries.size();
    if (targetUid)
    {
      auto place = animation_place_get(model, targetUid);
      if (place.group)
        place.group->animations.insert(place.group->animations.begin() + place.position + target.isAfter,
                                       taken.animations.begin(), taken.animations.end());
      else if (place.entry != -1)
        entries.insert(entries.begin() + place.entry + target.isAfter, taken.animations.begin(),
                       taken.animations.end());
      place = animation_place_get(model, targetUid);
      if (place.entry != -1) topIndex = place.entry + (target.isAfter || place.group);
    }
    else if (auto entry = animation_group_entry_get(model, target.groupId); entry != -1)
    {
      auto& group = std::get<AnimationGroup>(entries[entry]);
      if (target.isInto)
        group.animations.insert(group.animations.end(), taken.animations.begin(), taken.animations.end());
      else
        entries.insert(entries.begin() + entry + target.isAfter, taken.animations.begin(), taken.animations.end());
      if (!target.isInto && !target.isAfter) entry += (int)taken.animations.size();
      topIndex = entry + (target.isAfter || target.isInto);
    }
    else
      entries.insert(entries.end(), taken.animations.begin(), taken.animations.end());

    entries.insert(entries.begin() + std::clamp(topIndex, 0, (int)entries.size()),
                   std::make_move_iterator(taken.groups.begin()), std::make_move_iterator(taken.groups.end()));
    return uids;
  }

  // Wraps ungrouped animations in a new group placed where the first of them was.
  Uids animations_group(Model& model, const std::set<int>& indices, const std::string& name)
  {
    if (indices.empty()) return {};
    auto first = model.animation_get(*indices.begin());
    if (!first) return {};
    auto firstUid = first->uid;
    auto entry = animation_place_get(model, firstUid).entry;
    auto groupIds = animation_group_ids_get(model);
    auto taken = animations_take(model, indices, {});
    AnimationGroup group{
        .id = groupIds.empty() ? 0 : *groupIds.rbegin() + 1, .name = name, .animations = std::move(taken.animations)};
    auto& entries = model.animations.entries;
    entries.insert(entries.begin() + std::clamp(entry, 0, (int)entries.size()), std::move(group));
    return {};
  }

  // Pastes clipboard animations (and groups, which get new ids) before the animation at `start`; lone animations join
  // `targetGroupId` when no group came with them.
  Uids animations_paste(Model& model, const std::string& text, int start, int targetGroupId, std::set<int>& groupIds,
                        std::string* errorString)
  {
    auto entries = animations_from_string(text, errorString);
    if (entries.empty()) return {};

    auto existingGroupIds = animation_group_ids_get(model);
    auto nextGroupId = existingGroupIds.empty() ? 0 : *existingGroupIds.rbegin() + 1;
    Uids uids{};
    std::vector<AnimationPointer> loose{};
    std::vector<AnimationEntry> placed{};
    for (auto& entry : entries)
      if (auto animation = std::get_if<AnimationPointer>(&entry))
      {
        uids.push_back((*animation)->uid);
        loose.push_back(*animation);
        placed.push_back(entry);
      }
      else
      {
        auto& group = std::get<AnimationGroup>(entry);
        group.id = nextGroupId++;
        groupIds.insert(group.id);
        for (const auto& animation : group.animations)
          uids.push_back(animation->uid);
        placed.push_back(std::move(entry));
      }

    if (groupIds.empty() && targetGroupId != -1 && animation_group_find(model, targetGroupId))
    {
      animations_insert(model, start, targetGroupId, std::move(loose));
      return uids;
    }

    auto before = model.animation_get(start);
    auto position = before ? animation_place_get(model, before->uid).entry : (int)model.animations.entries.size();
    model.animations.entries.insert(model.animations.entries.begin() + position,
                                    std::make_move_iterator(placed.begin()), std::make_move_iterator(placed.end()));
    return uids;
  }

  void track_merge(Track& destination, const Track& source, types::merge::Type type)
  {
    std::vector<Frame> frames{};
    for (const auto& frame : source.frames)
      frames.push_back(frame_clone(frame));
    switch (type)
    {
      case types::merge::APPEND:
        destination.frames.insert(destination.frames.end(), frames.begin(), frames.end());
        break;
      case types::merge::PREPEND:
        destination.frames.insert(destination.frames.begin(), frames.begin(), frames.end());
        break;
      case types::merge::REPLACE:
        if (destination.frames.size() < frames.size()) destination.frames.resize(frames.size());
        std::ranges::copy(frames, destination.frames.begin());
        break;
      default:
        break;
    }
  }

  // Merges a source animation's layer or null tracks: groups match by name (else are added), tracks match by id
  // within their group (else are added).
  void track_entries_merge(std::vector<TrackEntry>& destination, const std::vector<TrackEntry>& source,
                           types::merge::Type type)
  {
    std::map<int, int> groupRemap{};
    std::set<int> matchedGroupIds{};
    int nextGroupId{};
    groups_each(destination, [&](const TrackGroup& group) { nextGroupId = std::max(nextGroupId, group.id + 1); });

    groups_each(source,
                [&](const TrackGroup& sourceGroup)
                {
                  TrackGroup* matched{};
                  groups_each(destination,
                              [&](TrackGroup& group)
                              {
                                if (!matched && group.name == sourceGroup.name && !matchedGroupIds.contains(group.id))
                                  matched = &group;
                              });
                  if (matched)
                    track_merge(matched->root, sourceGroup.root, type);
                  else
                  {
                    TrackGroup group{.id = nextGroupId++,
                                     .name = sourceGroup.name,
                                     .isExpanded = sourceGroup.isExpanded,
                                     .isVisible = sourceGroup.isVisible,
                                     .root = track_clone(sourceGroup.root),
                                     .extras = sourceGroup.extras};
                    destination.emplace_back(std::move(group));
                    matched = &std::get<TrackGroup>(destination.back());
                  }
                  groupRemap[sourceGroup.id] = matched->id;
                  matchedGroupIds.insert(matched->id);
                });

    tracks_each(source,
                [&](const Track& sourceTrack, const TrackGroup* sourceGroup)
                {
                  TrackGroup* group{};
                  if (sourceGroup)
                    groups_each(destination,
                                [&](TrackGroup& candidate)
                                {
                                  if (candidate.id == groupRemap[sourceGroup->id]) group = &candidate;
                                });
                  Track* existing{};
                  if (group)
                  {
                    for (auto& track : group->tracks)
                      if (track.id == sourceTrack.id) existing = &track;
                  }
                  else
                    for (auto& entry : destination)
                      if (auto track = std::get_if<Track>(&entry); track && track->id == sourceTrack.id)
                        existing = track;

                  if (existing)
                    track_merge(*existing, sourceTrack, type);
                  else if (group)
                    group->tracks.push_back(track_clone(sourceTrack));
                  else
                    destination.emplace_back(track_clone(sourceTrack));
                });
  }

  // Merges source animations into the target (frames appended, prepended, replacing or ignored), optionally deleting
  // the sources; `ungroupIds` dissolves groups afterwards. Returns the merged animation's uid.
  Uids animations_merge(Model& model, int target, std::set<int> sources, types::merge::Type type, bool isDeleteAfter,
                        const std::set<int>& ungroupIds)
  {
    if (!model.animation_get(target)) return {};
    auto isMerge = sources.size() > 1 || !sources.contains(target);
    if (isMerge)
    {
      auto& merged = *model.animation_edit(target);
      if (!merged.name.ends_with(ANIMATION_MERGED_SUFFIX)) merged.name += std::string(ANIMATION_MERGED_SUFFIX);
      for (auto index : sources)
      {
        if (index == target) continue;
        auto source = model.animation_get(index);
        if (!source) continue;
        auto sourceCopy = *source;
        auto& destination = *model.animation_edit(target);
        track_merge(destination.root, sourceCopy.root, type);
        track_entries_merge(destination.layers, sourceCopy.layers, type);
        track_entries_merge(destination.nulls, sourceCopy.nulls, type);
        track_merge(destination.triggers, sourceCopy.triggers, type);
      }

      auto targetUid = model.animation_get(target)->uid;
      if (isDeleteAfter)
      {
        sources.erase(target);
        animations_take(model, sources, {});
      }
      for (int i = 0; i < model.animations_count_get(); ++i)
        if (model.animation_get(i)->uid == targetUid)
        {
          auto& finalAnimation = *model.animation_edit(i);
          finalAnimation.frameNum = animation_length_get(finalAnimation);
          target = i;
        }
    }

    auto uid = model.animation_get(target)->uid;
    animation_groups_remove(model, ungroupIds);
    return {uid};
  }
}
