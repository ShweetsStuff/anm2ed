#include "edit/edit.h"

static std::vector<TrackEntry>* track_entries_edit(Model& model, int animationIndex, int type)
{
    if (type != LAYER && type != NULL_)
        return nullptr;
    auto animation = model.animation_edit(animationIndex);
    return animation ? &animation_entries_get(*animation, type) : nullptr;
}

void items_remove(Model& model, int animationIndex, const std::map<int, std::set<int>>& ids,
                  const std::map<int, std::set<int>>& groupIds)
{
    for (auto type : {LAYER, NULL_})
        if (auto entries = track_entries_edit(model, animationIndex, type))
        {
            auto typeIds = ids.contains(type) ? ids.at(type) : std::set<int>{};
            auto typeGroupIds = groupIds.contains(type) ? groupIds.at(type) : std::set<int>{};
            entries_take(*entries, [&](Track& track) { return typeIds.contains(track.id); }, typeGroupIds);
        }
}

// Wraps ungrouped tracks in a new group placed where the first of them was.
Uids items_group(Model& model, int animationIndex, int type, const std::set<int>& ids, const std::string& name)
{
    auto entries = track_entries_edit(model, animationIndex, type);
    if (!entries)
        return {};
    TrackGroup group = {.id = group_next_id(*entries), .name = name};
    auto uid = group.uid;
    if (!entries_wrap(*entries, [&](Track& track) { return ids.contains(track.id); }, std::move(group)))
        return {};
    return {uid};
}

// Moves tracks and whole groups next to (or into) a target row. Layers are listed bottom-up, so for them "after" on
// screen is "before" in the file.
void items_move(Model& model, const ItemsMove& move)
{
    auto& [animationIndex, type, ids, groupIds, target, zone] = move;
    auto isDropAfter = zone != DROP_BEFORE;
    auto isDropIntoGroup = zone == DROP_INSIDE;
    if ((target.groupId != -1 && groupIds.contains(target.groupId)) || (target.isGroup && groupIds.contains(target.id)))
        return;
    auto entries = track_entries_edit(model, animationIndex, type);
    if (!entries)
        return;
    auto taken = entries_take(*entries, [&](Track& track) { return ids.contains(track.id); }, groupIds);
    if (taken.items.empty() && taken.groups.empty())
        return;

    // Where top-level items go: beside the target row, or beside the group that holds it.
    auto isAfter = type == LAYER ? !isDropAfter : isDropAfter;
    auto topIndex = (int)entries->size();
    if (target.isGroup || target.groupId != -1)
    {
        if (auto index = group_entry_get(*entries, target.isGroup ? target.id : target.groupId); index != -1)
            topIndex = index + isAfter;
    }
    else if (target.type == type)
    {
        auto track = std::ranges::find_if(*entries, [&](const TrackEntry& entry) {
            auto track = std::get_if<Track>(&entry);
            return track && track->id == target.id;
        });
        if (track != entries->end())
            topIndex = (int)(track - entries->begin()) + isAfter;
    }

    // Into a group header lands just under it on screen. Beside a grouped track lands beside it.
    auto intoGroupId = target.isGroup && isDropIntoGroup ? target.id : !target.isGroup ? target.groupId : -1;
    if (auto group = group_find(*entries, intoGroupId))
    {
        auto& tracks = group->items;
        auto beside = std::ranges::find_if(tracks, [&](const Track& track) { return track.id == target.id; });
        auto position = target.isGroup           ? (type == LAYER ? tracks.end() : tracks.begin())
                        : beside != tracks.end() ? beside + isAfter
                                                 : tracks.end();
        tracks.insert(position, std::make_move_iterator(taken.items.begin()),
                      std::make_move_iterator(taken.items.end()));
        taken.items.clear();
    }
    entries_insert(*entries, topIndex, std::move(taken));
}

void animation_length_fit(Model& model, int animationIndex)
{
    if (auto animation = model.animation_edit(animationIndex))
        animation->frameNum = animation_length_get(*animation);
}

// Adds a layer or null (or reuses the one with the id) and gives it a track in one or every animation.
int item_add(Model& model, ItemAdd add)
{
    auto& [type, animationIndex, id, insertBeforeId, destination, layer, null] = add;
    auto& content = model.content;
    auto item_insert = [&](auto& items, auto item) {
        if (id == -1)
            id = item_next_id_get(items);
        if (item_get(items, id))
            return;
        item.id = id;
        items.push_back(item);
    };
    if (!item_get(content.spritesheets, layer.spritesheetId))
        layer.spritesheetId = 0;
    if (type == ItemType::LAYER)
        item_insert(content.layers, layer);
    else if (type == ItemType::NULL_)
        item_insert(content.nulls, null);
    else
        return -1;

    auto track_add = [&](int index) {
        if (model.track_get({index, (int)type, id}))
            return;
        auto& entries = animation_entries_get(*model.animation_edit(index), (int)type);
        auto before = std::ranges::find_if(entries, [&](const TrackEntry& entry) {
            auto track = std::get_if<Track>(&entry);
            return track && track->id == insertBeforeId;
        });
        entries.insert(insertBeforeId == -1 ? entries.end() : before, Track{.type = type, .id = id});
    };

    if (destination == DESTINATION_ALL)
        for (int i = 0; i < model.animations_count_get(); ++i)
            track_add(i);
    else if (model.animation_get(animationIndex))
        track_add(animationIndex);
    return id;
}
