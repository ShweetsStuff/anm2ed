#include "edit/edit.h"

#include "model/xml.h"

constexpr std::string_view ANIMATION_MERGED_SUFFIX = " (Merged)";

using AnimationPointer = std::shared_ptr<const Animation>;

std::set<int> animation_group_ids_get(const Model& model)
{
    std::set<int> ids;
    groups_each(model.animations.entries, [&](const AnimationGroup& group) { ids.insert(group.id); });
    return ids;
}

std::set<int> animation_group_indices_get(const Model& model, int groupId)
{
    std::set<int> indices;
    for (int i = 0; i < model.animations_count_get(); ++i)
        if (model.animation_group_id_get(i) == groupId)
            indices.insert(i);
    return indices;
}

// Takes out the animations at these flat indices (grouped ones too) and the groups in groupIds.
static EntriesTaken<std::vector<AnimationEntry>> animations_take(Model& model, const std::set<int>& indices,
                                                                 const std::set<int>& groupIds)
{
    std::set<std::uint64_t> uids;
    for (auto index : indices)
        if (auto animation = model.animation_get(index))
            uids.insert(animation->uid);
    return entries_take(
        model.animations.entries, [&](AnimationPointer& animation) { return uids.contains(animation->uid); }, groupIds);
}

// Where an animation with this uid sits: its top-level entry, and its group (or nullptr) and position there.
struct AnimationPlace
{
    int entry = -1;
    AnimationGroup* group = nullptr;
    int position = -1;
};

static AnimationPlace animation_place_get(Model& model, std::uint64_t uid)
{
    auto& entries = model.animations.entries;
    for (int i = 0; i < (int)entries.size(); ++i)
    {
        if (auto animation = std::get_if<AnimationPointer>(&entries[i]))
        {
            if ((*animation)->uid == uid)
                return {i, nullptr, -1};
            continue;
        }
        auto& group = std::get<AnimationGroup>(entries[i]);
        for (int j = 0; j < (int)group.items.size(); ++j)
            if (group.items[j]->uid == uid)
                return {i, &group, j};
    }
    return {};
}

// Inserts animations before the animation at a flat index (or at the end), inside `groupId` when given.
static void animations_insert(Model& model, int index, int groupId, std::vector<AnimationPointer> animations)
{
    auto before = model.animation_get(index);
    auto place = before ? animation_place_get(model, before->uid) : AnimationPlace{};
    auto& entries = model.animations.entries;
    if (auto group = group_find(entries, groupId))
    {
        auto position = place.group == group ? place.position : (int)group->items.size();
        group->items.insert(group->items.begin() + position, animations.begin(), animations.end());
        return;
    }
    entries.insert(entries.begin() + (place.entry == -1 ? (int)entries.size() : place.entry), animations.begin(),
                   animations.end());
}

void animation_groups_remove(Model& model, const std::set<int>& groupIds)
{
    groups_unwrap(model.animations.entries, groupIds);
}

// An empty copy of a track list: the same tracks and groups, without frames.
static std::vector<TrackEntry> track_entries_shell_get(const std::vector<TrackEntry>& entries)
{
    auto shell = entries_expand(entries, [](const Track& track, TrackGroup*) {
        return std::vector{Track{.type = track.type, .id = track.id, .isVisible = track.isVisible}};
    });
    groups_each(shell, [](TrackGroup& group) {
        group = TrackGroup{.id = group.id,
                           .name = group.name,
                           .isExpanded = group.isExpanded,
                           .isVisible = group.isVisible,
                           .items = std::move(group.items)};
    });
    return shell;
}

// Adds an animation with the same (empty) tracks as the template animation.
Uids animation_add(Model& model, int index, int groupId, int templateIndex, const std::string& name)
{
    Animation animation = {.name = name};
    if (auto source = model.animation_get(templateIndex))
    {
        animation.layers = track_entries_shell_get(source->layers);
        animation.nulls = track_entries_shell_get(source->nulls);
    }
    if (model.animations_count_get() == 0)
        model.animations.defaultAnimation = animation.name;
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

// Moves animations and whole groups beside a target animation or group. Animations dropped on their own join the
// target's group (or the group dropped into).
Uids animations_move(Model& model, const std::set<int>& indices, const std::set<int>& groupIds, AnimationTarget target)
{
    auto targetAnimation = model.animation_get(target.animationIndex);
    auto targetUid = targetAnimation ? targetAnimation->uid : 0;
    if (targetAnimation && groupIds.contains(model.animation_group_id_get(target.animationIndex)))
        return {};
    if (targetAnimation && indices.contains(target.animationIndex))
        return {};
    if (!targetAnimation && groupIds.contains(target.groupId))
        return {};

    auto taken = animations_take(model, indices, groupIds);
    if (taken.items.empty() && taken.groups.empty())
        return {};

    Uids uids = {};
    for (auto& animation : taken.items)
        uids.push_back(animation->uid);

    auto& entries = model.animations.entries;
    auto topIndex = (int)entries.size();
    if (targetUid)
    {
        auto place = animation_place_get(model, targetUid);
        if (place.group)
            place.group->items.insert(place.group->items.begin() + place.position + target.isAfter, taken.items.begin(),
                                      taken.items.end());
        else if (place.entry != -1)
            entries.insert(entries.begin() + place.entry + target.isAfter, taken.items.begin(), taken.items.end());
        place = animation_place_get(model, targetUid);
        if (place.entry != -1)
            topIndex = place.entry + (target.isAfter || place.group);
    }
    else if (auto entry = group_entry_get(entries, target.groupId); entry != -1)
    {
        auto& group = std::get<AnimationGroup>(entries[entry]);
        if (target.isInto)
            group.items.insert(group.items.end(), taken.items.begin(), taken.items.end());
        else
            entries.insert(entries.begin() + entry + target.isAfter, taken.items.begin(), taken.items.end());
        if (!target.isInto && !target.isAfter)
            entry += (int)taken.items.size();
        topIndex = entry + (target.isAfter || target.isInto);
    }
    else
        entries.insert(entries.end(), taken.items.begin(), taken.items.end());

    entries.insert(entries.begin() + std::clamp(topIndex, 0, (int)entries.size()),
                   std::make_move_iterator(taken.groups.begin()), std::make_move_iterator(taken.groups.end()));
    return uids;
}

// Gathers animations into a new group placed where the first of them was.
Uids animations_group(Model& model, const std::set<int>& indices, const std::string& name)
{
    auto first = indices.empty() ? nullptr : model.animation_get(*indices.begin());
    if (!first)
        return {};
    auto& entries = model.animations.entries;
    auto entry = animation_place_get(model, first->uid).entry;
    AnimationGroup group = {
        .id = group_next_id(entries), .name = name, .items = animations_take(model, indices, {}).items};
    entries.insert(entries.begin() + std::clamp(entry, 0, (int)entries.size()), std::move(group));
    return {};
}

// Pastes clipboard animations (and groups, which get new ids) before the animation at `start`. Lone animations join
// `targetGroupId` when no group came with them.
Uids animations_paste(Model& model, const std::string& text, int start, int targetGroupId, std::set<int>& groupIds,
                      std::string* errorString)
{
    auto entries = animations_from_string(text, errorString);
    if (entries.empty())
        return {};

    auto nextGroupId = group_next_id(model.animations.entries);
    Uids uids = {};
    std::vector<AnimationPointer> loose;
    std::vector<AnimationEntry> placed;
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
            for (auto& animation : group.items)
                uids.push_back(animation->uid);
            placed.push_back(std::move(entry));
        }

    if (groupIds.empty() && group_find(model.animations.entries, targetGroupId))
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

static void track_merge(Track& destination, const Track& source, Merge type)
{
    std::vector<Frame> frames;
    for (auto& frame : source.frames)
        frames.push_back(frame_clone(frame));
    switch (type)
    {
        case MERGE_APPEND:
            destination.frames.insert(destination.frames.end(), frames.begin(), frames.end());
            break;
        case MERGE_PREPEND:
            destination.frames.insert(destination.frames.begin(), frames.begin(), frames.end());
            break;
        case MERGE_REPLACE:
            if (destination.frames.size() < frames.size())
                destination.frames.resize(frames.size());
            std::ranges::copy(frames, destination.frames.begin());
            break;
        default:
            break;
    }
}

// Merges a source animation's layer or null tracks: groups match by name (else are added), tracks match by id
// within their group (else are added).
static void track_entries_merge(std::vector<TrackEntry>& destination, const std::vector<TrackEntry>& source, Merge type)
{
    std::map<int, int> groupRemap;
    std::set<int> matchedGroupIds;
    auto nextGroupId = group_next_id(destination);

    groups_each(source, [&](const TrackGroup& sourceGroup) {
        TrackGroup* matched = nullptr;
        groups_each(destination, [&](TrackGroup& group) {
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

    items_each(source, [&](const Track& sourceTrack, const TrackGroup* sourceGroup) {
        auto group = sourceGroup ? group_find(destination, groupRemap[sourceGroup->id]) : nullptr;
        Track* existing = nullptr;
        if (group)
        {
            for (auto& track : group->items)
                if (track.id == sourceTrack.id)
                    existing = &track;
        }
        else
            for (auto& entry : destination)
                if (auto track = std::get_if<Track>(&entry); track && track->id == sourceTrack.id)
                    existing = track;

        if (existing)
            track_merge(*existing, sourceTrack, type);
        else if (group)
            group->items.push_back(track_clone(sourceTrack));
        else
            destination.emplace_back(track_clone(sourceTrack));
    });
}

// Merges source animations into the target (frames appended, prepended, replacing or ignored), optionally deleting
// the sources. `ungroupIds` dissolves groups afterwards. Returns the merged animation's uid.
Uids animations_merge(Model& model, int target, std::set<int> sources, Merge type, bool isDeleteAfter,
                      const std::set<int>& ungroupIds)
{
    if (!model.animation_get(target))
        return {};
    auto isMerge = sources.size() > 1 || !sources.contains(target);
    if (isMerge)
    {
        auto& merged = *model.animation_edit(target);
        if (!merged.name.ends_with(ANIMATION_MERGED_SUFFIX))
            merged.name += std::string(ANIMATION_MERGED_SUFFIX);
        // Triggers are placed by time, so whichever animation ends up second has its triggers moved past the first.
        auto length = merged.frameNum;
        for (auto index : sources)
        {
            if (index == target)
                continue;
            auto source = model.animation_get(index);
            if (!source)
                continue;
            auto sourceCopy = *source;
            auto& destination = *model.animation_edit(target);
            if (type == MERGE_APPEND)
                for (auto& trigger : sourceCopy.triggers.frames)
                    trigger.atFrame += length;
            if (type == MERGE_PREPEND)
                for (auto& trigger : destination.triggers.frames)
                    trigger.atFrame += sourceCopy.frameNum;
            if (type == MERGE_APPEND || type == MERGE_PREPEND)
                length += sourceCopy.frameNum;
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
