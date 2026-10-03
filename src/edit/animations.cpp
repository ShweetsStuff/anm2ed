#include "edit.hpp"

#include "vector.hpp"

namespace anm2ed::edit
{
  std::set<int> animation_group_ids_get(const Element& animations)
  {
    std::set<int> result{};
    for (const auto& item : animations.children)
      if (item.type == ElementType::GROUP) result.insert(item.id);
    return result;
  }

  std::set<int> animation_group_indices_get(const Element& animations, int groupId)
  {
    std::set<int> result{};
    int animationIndex{};
    for (const auto& animation : animations.children)
      if (animation.type == ElementType::ANIMATION)
      {
        if (animation.groupId == groupId) result.insert(animationIndex);
        ++animationIndex;
      }
    return result;
  }

  // An empty copy of a track container: same tracks and groups, no frames.
  Element track_container_shell_copy(const Element* source, const TrackContainer& row)
  {
    auto destination = element_make(row.container);
    if (!source) return destination;

    auto nextGroupId = element_child_next_id_get(*source, ElementType::GROUP);
    std::set<int> copiedTrackIds{};
    auto track_push = [&](const Element& track, int groupId)
    {
      if (track.type != row.track || !copiedTrackIds.insert(track_id_get(track)).second) return;
      auto item = element_make(row.track);
      item.*(row.id) = track.*(row.id);
      item.groupId = track.groupId != -1 ? track.groupId : groupId;
      item.isVisible = track.isVisible;
      destination.children.push_back(item);
    };

    for (const auto& sourceItem : source->children)
    {
      if (sourceItem.type != ElementType::GROUP)
      {
        track_push(sourceItem, -1);
        continue;
      }
      auto group = element_make(ElementType::GROUP);
      group.id = sourceItem.id >= 0 ? sourceItem.id : nextGroupId++;
      group.name = sourceItem.name;
      group.isExpanded = sourceItem.isExpanded;
      group.isVisible = sourceItem.isVisible;
      group.children.push_back(root_animation_make());
      destination.children.push_back(group);
      for (const auto& child : sourceItem.children)
        track_push(child, group.id);
    }
    return destination;
  }

  // Adds an animation with the same (empty) tracks as the template animation.
  Uids animation_add(Anm2& anm2, int index, int groupId, int templateIndex, const std::string& name)
  {
    auto animations = anm2.element_get(ElementType::ANIMATIONS);
    if (!animations) return {};

    auto animation = element_make(ElementType::ANIMATION);
    animation.name = name;
    animation.groupId = groupId;
    animation.children.push_back(root_animation_make());
    auto templateAnimation = anm2.element_get(ElementType::ANIMATION, templateIndex);
    for (const auto& row : TRACK_CONTAINERS)
      animation.children.push_back(track_container_shell_copy(
          templateAnimation ? child_first_get(*templateAnimation, row.container) : nullptr, row));
    animation.children.push_back(element_make(ElementType::TRIGGERS));
    animation = element_clone(animation);

    if (animations_count_get(*animations) == 0) animations->defaultAnimation = animation.name;
    animations->children.insert(animations->children.begin() + animations_child_insert_index_get(*animations, index),
                                animation);
    return {animation.uid};
  }

  Uids animations_remove(Anm2& anm2, const std::set<int>& indices, const std::set<int>& groupIds)
  {
    auto animations = anm2.element_get(ElementType::ANIMATIONS);
    if (!animations) return {};
    animation_groups_remove(*animations, groupIds);
    for (auto it = indices.rbegin(); it != indices.rend(); ++it)
      if (auto childIndex = animations_child_index_get(*animations, *it); childIndex != -1)
        animations->children.erase(animations->children.begin() + childIndex);
    return {};
  }

  void animation_groups_remove(Element& animations, const std::set<int>& groupIds)
  {
    for (auto& item : animations.children)
      if (item.type == ElementType::ANIMATION && groupIds.contains(item.groupId)) item.groupId = -1;
    std::erase_if(animations.children,
                  [&](const Element& item) { return item.type == ElementType::GROUP && groupIds.contains(item.id); });
  }

  // Moves animations and whole groups to a child position; animations moved on their own join targetGroupId.
  Uids animations_move(Anm2& anm2, const std::set<int>& indices, const std::set<int>& groupIds, int targetChildIndex,
                       int targetGroupId)
  {
    auto animations = anm2.element_get(ElementType::ANIMATIONS);
    if (!animations) return {};
    auto existingGroupIds = animation_group_ids_get(*animations);
    auto groupId = existingGroupIds.contains(targetGroupId) ? targetGroupId : -1;

    Uids uids{};
    std::vector<int> childIndices{};
    int animationIndex{};
    for (int i = 0; i < (int)animations->children.size(); ++i)
    {
      auto& item = animations->children[i];
      if (item.type == ElementType::GROUP && groupIds.contains(item.id)) childIndices.push_back(i);
      if (item.type != ElementType::ANIMATION) continue;
      auto isGroupMoved = groupIds.contains(item.groupId);
      auto isMoved = indices.contains(animationIndex++);
      if (isMoved && !isGroupMoved)
      {
        item.groupId = groupId;
        uids.push_back(item.uid);
      }
      if (isGroupMoved || isMoved) childIndices.push_back(i);
    }

    util::vector::move_indices_to_position(animations->children, childIndices, targetChildIndex);
    return uids;
  }

  Uids animations_group(Anm2& anm2, const std::set<int>& indices, const std::string& name)
  {
    auto animations = anm2.element_get(ElementType::ANIMATIONS);
    if (!animations || indices.empty()) return {};

    auto group = element_make(ElementType::GROUP);
    group.id = element_child_next_id_get(*animations, ElementType::GROUP);
    group.name = name;
    for (auto index : indices)
      if (auto animation = anm2.element_get(ElementType::ANIMATION, index)) animation->groupId = group.id;
    animations->children.insert(
        animations->children.begin() + animations_child_index_get(*animations, *indices.begin()), group);
    return {};
  }

  Uids animations_paste(Anm2& anm2, const std::string& text, int start, int targetGroupId, std::set<int>& groupIds,
                        std::string* errorString)
  {
    std::set<int> indices{};
    if (!anm2.animations_deserialize(text, start, indices, errorString, &groupIds)) return {};
    anm2.uids_repair();

    Uids uids{};
    for (auto index : indices)
      if (auto animation = anm2.element_get(ElementType::ANIMATION, index))
      {
        if (targetGroupId != -1 && groupIds.empty()) animation->groupId = targetGroupId;
        uids.push_back(animation->uid);
      }
    return uids;
  }

  Uids animations_merge(Anm2& anm2, int target, std::set<int> sources, types::merge::Type type, bool isDeleteAfter,
                        const std::set<int>& ungroupIds)
  {
    auto merged = sources.size() > 1 || !sources.contains(target)
                      ? anm2.animations_merge(target, sources, type, isDeleteAfter)
                      : target;
    if (merged == -1) return {};
    if (auto animations = anm2.element_get(ElementType::ANIMATIONS)) animation_groups_remove(*animations, ungroupIds);
    auto animation = anm2.element_get(ElementType::ANIMATION, merged);
    return animation ? Uids{animation->uid} : Uids{};
  }
}
