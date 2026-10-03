#include "edit.hpp"

using namespace anm2ed::model;

namespace anm2ed::edit
{
  std::vector<TrackEntry>* track_entries_edit(Model& model, int animationIndex, int type)
  {
    if (type != LAYER && type != NULL_) return nullptr;
    auto animation = model.animation_edit(animationIndex);
    return animation ? &animation_entries_get(*animation, type) : nullptr;
  }

  Uids items_remove(Model& model, int animationIndex, const std::map<int, std::set<int>>& ids,
                    const std::map<int, std::set<int>>& groupIds)
  {
    for (auto type : {LAYER, NULL_})
    {
      auto typeIds = ids.contains(type) ? ids.at(type) : std::set<int>{};
      auto typeGroupIds = groupIds.contains(type) ? groupIds.at(type) : std::set<int>{};
      if (typeIds.empty() && typeGroupIds.empty()) continue;
      auto entries = track_entries_edit(model, animationIndex, type);
      if (!entries) continue;

      std::erase_if(*entries,
                    [&](TrackEntry& entry)
                    {
                      if (auto group = std::get_if<TrackGroup>(&entry))
                      {
                        std::erase_if(group->tracks, [&](const Track& track) { return typeIds.contains(track.id); });
                        return typeGroupIds.contains(group->id);
                      }
                      return typeIds.contains(std::get<Track>(entry).id);
                    });
    }
    return {};
  }

  // Wraps ungrouped tracks in a new group placed where the first of them was.
  Uids items_group(Model& model, int animationIndex, int type, const std::set<int>& ids, const std::string& name)
  {
    auto entries = track_entries_edit(model, animationIndex, type);
    if (!entries) return {};

    int nextId{};
    groups_each(*entries, [&](const TrackGroup& group) { nextId = std::max(nextId, group.id + 1); });
    TrackGroup group{.id = nextId, .name = name};

    auto insertIndex = -1;
    for (int i = 0; i < (int)entries->size(); ++i)
    {
      auto track = std::get_if<Track>(&(*entries)[i]);
      if (!track || !ids.contains(track->id)) continue;
      if (insertIndex == -1) insertIndex = i;
      group.tracks.push_back(std::move(*track));
      entries->erase(entries->begin() + i--);
    }
    if (insertIndex == -1) return {};

    auto uid = group.uid;
    entries->insert(entries->begin() + insertIndex, std::move(group));
    return {uid};
  }

  // Moves tracks and whole groups next to (or into) a target row. Layers are listed bottom-up, so for them "after" on
  // screen is "before" in the file.
  Uids items_move(Model& model, int animationIndex, int type, const std::set<int>& ids, const std::set<int>& groupIds,
                  RowTarget target, bool isDropAfter, bool isDropIntoGroup)
  {
    if (target.groupId != -1 && groupIds.contains(target.groupId)) return {};
    if (target.isGroup && groupIds.contains(target.id)) return {};
    auto entries = track_entries_edit(model, animationIndex, type);
    if (!entries) return {};

    std::vector<Track> movedTracks{};
    std::vector<TrackEntry> movedGroups{};
    std::erase_if(*entries,
                  [&](TrackEntry& entry)
                  {
                    if (auto group = std::get_if<TrackGroup>(&entry))
                    {
                      if (groupIds.contains(group->id))
                      {
                        movedGroups.push_back(std::move(entry));
                        return true;
                      }
                      std::erase_if(group->tracks,
                                    [&](Track& track)
                                    {
                                      if (!ids.contains(track.id)) return false;
                                      movedTracks.push_back(std::move(track));
                                      return true;
                                    });
                      return false;
                    }
                    auto& track = std::get<Track>(entry);
                    if (!ids.contains(track.id)) return false;
                    movedTracks.push_back(std::move(track));
                    return true;
                  });
    if (movedTracks.empty() && movedGroups.empty()) return {};

    auto isAfter = type == LAYER ? !isDropAfter : isDropAfter;
    auto group_find = [&](int groupId)
    {
      return std::ranges::find_if(*entries,
                                  [&](const TrackEntry& entry)
                                  {
                                    auto group = std::get_if<TrackGroup>(&entry);
                                    return group && group->id == groupId;
                                  });
    };
    // Where top-level items go: beside the target row, or beside the group that holds it.
    auto topIndex = (int)entries->size();
    if (target.isGroup || target.groupId != -1)
    {
      auto group = group_find(target.isGroup ? target.id : target.groupId);
      if (group != entries->end()) topIndex = (int)(group - entries->begin()) + isAfter;
    }
    else if (target.type == type)
    {
      auto track = std::ranges::find_if(*entries,
                                        [&](const TrackEntry& entry)
                                        {
                                          auto track = std::get_if<Track>(&entry);
                                          return track && track->id == target.id;
                                        });
      if (track != entries->end()) topIndex = (int)(track - entries->begin()) + isAfter;
    }

    auto intoGroupId = target.isGroup && isDropIntoGroup ? target.id : !target.isGroup ? target.groupId : -1;
    auto intoGroup = intoGroupId == -1 ? entries->end() : group_find(intoGroupId);
    if (intoGroup != entries->end())
    {
      // Into a group header lands just under it on screen; beside a grouped track lands beside it.
      auto& tracks = std::get<TrackGroup>(*intoGroup).tracks;
      auto beside = std::ranges::find_if(tracks, [&](const Track& track) { return track.id == target.id; });
      auto position = target.isGroup           ? (type == LAYER ? tracks.end() : tracks.begin())
                      : beside != tracks.end() ? beside + isAfter
                                               : tracks.end();
      tracks.insert(position, std::make_move_iterator(movedTracks.begin()), std::make_move_iterator(movedTracks.end()));
      movedTracks.clear();
    }

    std::vector<TrackEntry> topMoved{};
    for (auto& track : movedTracks)
      topMoved.emplace_back(std::move(track));
    topMoved.insert(topMoved.end(), std::make_move_iterator(movedGroups.begin()),
                    std::make_move_iterator(movedGroups.end()));
    entries->insert(entries->begin() + std::clamp(topIndex, 0, (int)entries->size()),
                    std::make_move_iterator(topMoved.begin()), std::make_move_iterator(topMoved.end()));
    return {};
  }

  Uids animation_length_fit(Model& model, int animationIndex)
  {
    if (auto animation = model.animation_edit(animationIndex)) animation->frameNum = animation_length_get(*animation);
    return {};
  }

  // Adds a layer or null (or updates the one with the item's id) and gives it a track in one or every animation.
  int item_add(Model& model, ItemType type, int animationIndex, int id, const std::string& name, int spritesheetId,
               bool isShowRect, int insertBeforeId, types::destination::Type destination)
  {
    auto& content = model.content;
    if (type == ItemType::LAYER)
    {
      if (id == -1) id = item_next_id_get(content.layers);
      auto layer = item_get(content.layers, id);
      if (!layer) layer = &content.layers.emplace_back(Layer{.id = id});
      if (!name.empty()) layer->name = name;
      layer->spritesheetId = item_get(content.spritesheets, spritesheetId) ? spritesheetId : 0;
    }
    else if (type == ItemType::NULL_)
    {
      if (id == -1) id = item_next_id_get(content.nulls);
      auto null = item_get(content.nulls, id);
      if (!null) null = &content.nulls.emplace_back(Null{.id = id});
      if (!name.empty()) null->name = name;
      null->isShowRect = isShowRect;
    }
    else
      return -1;

    auto add = [&](int index)
    {
      if (model.track_get({index, (int)type, id})) return;
      auto& entries = animation_entries_get(*model.animation_edit(index), (int)type);
      auto before = std::ranges::find_if(entries,
                                         [&](const TrackEntry& entry)
                                         {
                                           auto track = std::get_if<Track>(&entry);
                                           return track && track->id == insertBeforeId;
                                         });
      entries.insert(insertBeforeId == -1 ? entries.end() : before, Track{.type = type, .id = id});
    };

    if (destination == types::destination::ALL)
      for (int i = 0; i < model.animations_count_get(); ++i)
        add(i);
    else if (model.animation_get(animationIndex))
      add(animationIndex);
    return id;
  }
}
