#include "edit.hpp"

namespace anm2ed::edit
{
  Element* track_container_get(Anm2& anm2, int animationIndex, int type)
  {
    auto animation = anm2.element_get(ElementType::ANIMATION, animationIndex);
    return animation ? child_first_get(*animation, TYPE_CONTAINERS[type]) : nullptr;
  }

  Uids items_remove(Anm2& anm2, int animationIndex, const std::map<int, std::set<int>>& ids,
                    const std::map<int, std::set<int>>& groupIds)
  {
    for (auto type : {LAYER, NULL_})
    {
      auto container = track_container_get(anm2, animationIndex, type);
      if (!container) continue;
      auto typeIds = ids.contains(type) ? ids.at(type) : std::set<int>{};
      auto typeGroupIds = groupIds.contains(type) ? groupIds.at(type) : std::set<int>{};
      std::erase_if(container->children,
                    [&](const Element& item)
                    {
                      if (item.type == ElementType::GROUP) return typeGroupIds.contains(item.id);
                      return item.type == TYPE_TRACKS[type] &&
                             (typeIds.contains(track_id_get(item)) || typeGroupIds.contains(item.groupId));
                    });
    }
    return {};
  }

  Uids items_group(Anm2& anm2, int animationIndex, int type, const std::set<int>& ids, const std::string& name)
  {
    auto container = track_container_get(anm2, animationIndex, type);
    if (!container) return {};

    auto group = element_clone(element_make(ElementType::GROUP));
    group.id = element_child_next_id_get(*container, ElementType::GROUP);
    group.name = name;
    group.children.push_back(element_clone(root_animation_make()));

    auto insertIndex = (int)container->children.size();
    for (int i = 0; i < (int)container->children.size(); ++i)
    {
      auto& item = container->children[i];
      if (item.type != TYPE_TRACKS[type] || !ids.contains(track_id_get(item))) continue;
      insertIndex = std::min(insertIndex, i);
      item.groupId = group.id;
    }
    if (insertIndex == (int)container->children.size()) return {};

    container->children.insert(container->children.begin() + insertIndex, group);
    return {group.uid};
  }

  // Moves tracks and whole groups next to (or into) a target row. Layers list top-down in reverse of nulls, so the
  // drop side flips for them.
  Uids items_move(Anm2& anm2, int animationIndex, int type, const std::set<int>& ids, const std::set<int>& groupIds,
                  RowTarget target, bool isDropAfter, bool isDropIntoGroup)
  {
    auto container = track_container_get(anm2, animationIndex, type);
    if (!container) return {};
    if (target.groupId != -1 && groupIds.contains(target.groupId)) return {};
    if (target.isGroup && groupIds.contains(target.id)) return {};

    auto trackType = TYPE_TRACKS[type];
    auto& children = container->children;
    auto is_moved = [&](const Element& item)
    {
      return (item.type == ElementType::GROUP && groupIds.contains(item.id)) ||
             (item.type == trackType && (ids.contains(track_id_get(item)) || groupIds.contains(item.groupId)));
    };
    std::vector<Element> moved{};
    std::ranges::copy_if(children, std::back_inserter(moved), is_moved);
    if (moved.empty()) return {};

    auto targetIndex = (int)children.size();
    if (target.isGroup || target.type == type)
    {
      auto rowIndex = -1;
      auto groupEndIndex = -1;
      for (int i = 0; i < (int)children.size(); ++i)
      {
        auto& item = children[i];
        auto isGroupRow = item.type == ElementType::GROUP && item.id == target.id;
        if ((target.isGroup && isGroupRow) ||
            (!target.isGroup && item.type == trackType && track_id_get(item) == target.id))
          if (rowIndex == -1) rowIndex = i;
        if (isGroupRow || (item.type == trackType && item.groupId == target.id)) groupEndIndex = i;
      }
      if (rowIndex == -1) return {};
      if (target.isGroup)
        targetIndex = type == LAYER ? (isDropAfter ? rowIndex : groupEndIndex + 1) : rowIndex + isDropAfter;
      else
        targetIndex = rowIndex + (type == LAYER ? !isDropAfter : isDropAfter);
    }

    for (int i = (int)children.size() - 1; i >= 0; --i)
      if (is_moved(children[i]))
      {
        if (i < targetIndex) --targetIndex;
        children.erase(children.begin() + i);
      }

    auto targetGroupId = target.isGroup && isDropIntoGroup ? target.id : target.type == type ? target.groupId : -1;
    for (auto& item : moved)
      if (item.type == trackType && !groupIds.contains(item.groupId)) item.groupId = targetGroupId;

    children.insert(children.begin() + std::clamp(targetIndex, 0, (int)children.size()), moved.begin(), moved.end());
    return {};
  }

  Uids animation_length_fit(Anm2& anm2, int animationIndex)
  {
    if (auto animation = anm2.element_get(ElementType::ANIMATION, animationIndex))
      animation->frameNum = animation_length_get(*animation);
    return {};
  }
}
