#include "internal.hpp"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed
{
  constexpr std::string_view ANM2ED_GROUP_FRAME_BACKUP_TAG = "FrameRestore";


  bool is_frame_transform_default(const Element& frame)
  {
    auto base = element_make(ElementType::FRAME);
    return frame.position == base.position && frame.scale == base.scale && frame.rotation == base.rotation &&
           frame.shear == base.shear && frame.tint == base.tint && frame.colorOffset == base.colorOffset;
  }

  bool is_root_animation_transform_default(const Element& root)
  {
    for (const auto& frame : root.children)
      if (frame.type == ElementType::FRAME && !is_frame_transform_default(frame)) return false;
    return true;
  }

  bool is_group_frame_backup(const Element& element)
  {
    return element.type == ElementType::UNKNOWN && element.tag == ANM2ED_GROUP_FRAME_BACKUP_TAG;
  }

  void group_frame_backups_erase(Element& group)
  {
    std::erase_if(group.children, [](const Element& child) { return is_group_frame_backup(child); });
  }

  Element group_frame_backup_make(const Element& container, const Element& group, ElementType trackType)
  {
    Element backup{};
    backup.type = ElementType::UNKNOWN;
    backup.tag = std::string(ANM2ED_GROUP_FRAME_BACKUP_TAG);

    if (auto root = child_first_get(group, ElementType::ROOT_ANIMATION)) backup.children.push_back(*root);

    for (const auto& item : container.children)
      if (item.type == trackType && item.groupId == group.id) backup.children.push_back(item);

    return backup;
  }

  void frame_group_root_apply(Element& frame, const Element& rootFrame)
  {
    auto rootScale = math::percent_to_unit(rootFrame.scale);
    auto offset = frame.position * rootScale;
    auto radians = glm::radians(rootFrame.rotation);
    auto cos = std::cos(radians);
    auto sin = std::sin(radians);

    frame.position = rootFrame.position + glm::vec2(offset.x * cos - offset.y * sin, offset.x * sin + offset.y * cos);
    frame.scale *= rootScale;
    frame.rotation += rootFrame.rotation;
    frame.tint *= rootFrame.tint;
    frame.colorOffset += rootFrame.colorOffset;
  }

  void track_group_root_bake(Element& track, const Element& root, int frameNum)
  {
    auto trackLength = track_length_get(track);
    if (trackLength <= 0) return;

    auto length = std::max({trackLength, track_length_get(root), frameNum, FRAME_DURATION_MIN});
    std::vector<Element> frames{};
    frames.reserve(length);

    for (int time = 0; time < length; ++time)
    {
      auto frame = frame_generate(track, (float)time);
      auto rootFrame = frame_generate(root, (float)time);
      frame_group_root_apply(frame, rootFrame);
      frame.duration = FRAME_DURATION_MIN;
      frame.interpolation = Interpolation::NONE;
      frames.push_back(std::move(frame));
    }

    track.children = std::move(frames);
  }

  void group_frames_bake(Element& container, ElementType trackType, int frameNum)
  {
    for (auto& group : container.children)
    {
      if (group.type != ElementType::GROUP) continue;
      group_frame_backups_erase(group);
      auto root = child_first_get(group, ElementType::ROOT_ANIMATION);
      if (!root || is_root_animation_transform_default(*root)) continue;

      auto backup = group_frame_backup_make(container, group, trackType);
      for (auto& item : container.children)
        if (item.type == trackType && item.groupId == group.id) track_group_root_bake(item, *root, frameNum);

      *root = root_animation_make();
      group.children.push_back(std::move(backup));
    }
  }

  void group_frames_bake(Element& element)
  {
    if (element.type == ElementType::ANIMATION)
    {
      if (auto layerAnimations = child_first_get(element, ElementType::LAYER_ANIMATIONS))
        group_frames_bake(*layerAnimations, ElementType::LAYER_ANIMATION, element.frameNum);
      if (auto nullAnimations = child_first_get(element, ElementType::NULL_ANIMATIONS))
        group_frames_bake(*nullAnimations, ElementType::NULL_ANIMATION, element.frameNum);
    }

    for (auto& child : element.children)
      group_frames_bake(child);
  }

  void group_frames_restore(Element& container, ElementType trackType)
  {
    std::set<int> restoredGroupIds{};

    for (auto& group : container.children)
    {
      if (group.type != ElementType::GROUP) continue;
      if (group.id < 0) continue;

      const Element* backup{};
      for (const auto& child : group.children)
        if (is_group_frame_backup(child))
        {
          backup = &child;
          break;
        }

      if (!backup) continue;

      auto root = root_animation_make();
      if (auto currentRoot = child_first_get(group, ElementType::ROOT_ANIMATION)) root = *currentRoot;

      std::vector<Element> restoredTracks{};
      bool isRootRestored{};
      for (const auto& child : backup->children)
      {
        if (child.type == ElementType::ROOT_ANIMATION && !isRootRestored)
        {
          root = child;
          isRootRestored = true;
        }
        else if (child.type == trackType)
        {
          auto track = child;
          track.groupId = group.id;
          restoredTracks.push_back(std::move(track));
        }
      }

      if (!isRootRestored && restoredTracks.empty()) continue;

      group.children = {std::move(root)};
      group.children.insert(group.children.end(), std::make_move_iterator(restoredTracks.begin()),
                            std::make_move_iterator(restoredTracks.end()));
      restoredGroupIds.insert(group.id);
    }

    if (restoredGroupIds.empty()) return;

    std::erase_if(container.children, [&](const Element& item)
                  { return item.type == trackType && restoredGroupIds.contains(item.groupId); });
  }

  void group_frames_restore(Element& element)
  {
    if (element.type == ElementType::ANIMATION)
    {
      if (auto layerAnimations = child_first_get(element, ElementType::LAYER_ANIMATIONS))
        group_frames_restore(*layerAnimations, ElementType::LAYER_ANIMATION);
      if (auto nullAnimations = child_first_get(element, ElementType::NULL_ANIMATIONS))
        group_frames_restore(*nullAnimations, ElementType::NULL_ANIMATION);
    }

    for (auto& child : element.children)
      group_frames_restore(child);
  }

  int child_index_get(const Element& element, ElementType type)
  {
    for (int i = 0; i < (int)element.children.size(); ++i)
      if (element.children[i].type == type) return i;
    return -1;
  }

  ElementType track_container_track_type_get(ElementType containerType)
  {
    if (containerType == ElementType::LAYER_ANIMATIONS) return ElementType::LAYER_ANIMATION;
    if (containerType == ElementType::NULL_ANIMATIONS) return ElementType::NULL_ANIMATION;
    return ElementType::UNKNOWN;
  }

  ElementType track_container_group_container_type_get(ElementType containerType)
  {
    if (containerType == ElementType::LAYER_ANIMATIONS) return ElementType::LAYER_ANIMATION_GROUPS;
    if (containerType == ElementType::NULL_ANIMATIONS) return ElementType::NULL_ANIMATION_GROUPS;
    return ElementType::UNKNOWN;
  }

  int track_group_insert_index_get(const Element& container, ElementType trackType, int groupId)
  {
    for (int i = 0; i < (int)container.children.size(); ++i)
      if (container.children[i].type == trackType && container.children[i].groupId == groupId) return i;
    return (int)container.children.size();
  }

  void group_metadata_embed(Element& animation, ElementType trackContainerType)
  {
    auto groupContainerType = track_container_group_container_type_get(trackContainerType);
    auto trackType = track_container_track_type_get(trackContainerType);
    auto groupContainerIndex = child_index_get(animation, groupContainerType);
    if (groupContainerIndex < 0 || trackType == ElementType::UNKNOWN) return;

    std::vector<Element> groups{};
    for (const auto& child : animation.children[groupContainerIndex].children)
      if (child.type == ElementType::GROUP) groups.push_back(child);

    if (!groups.empty())
    {
      auto trackContainerIndex = child_index_get(animation, trackContainerType);
      if (trackContainerIndex < 0)
      {
        animation.children.push_back(element_make(trackContainerType));
        trackContainerIndex = (int)animation.children.size() - 1;
      }

      auto& container = animation.children[trackContainerIndex];
      std::set<int> groupIds{};
      for (const auto& group : groups)
        if (group.id >= 0) groupIds.insert(group.id);
      std::erase_if(container.children, [&](const Element& child)
                    { return child.type == ElementType::GROUP && groupIds.contains(child.id); });

      std::stable_sort(groups.begin(), groups.end(),
                       [](const Element& left, const Element& right)
                       {
                         if (left.index == -1 || right.index == -1) return left.index != -1;
                         return left.index < right.index;
                       });

      for (auto& group : groups)
      {
        auto insertIndex = group.index >= 0 ? std::min(group.index, (int)container.children.size())
                                            : track_group_insert_index_get(container, trackType, group.id);
        group.index = -1;
        container.children.insert(container.children.begin() + insertIndex, std::move(group));
      }
    }

    std::erase_if(animation.children, [&](const Element& child) { return child.type == groupContainerType; });
  }

  void group_metadata_embed(Element& element)
  {
    if (element.type == ElementType::ANIMATION)
    {
      group_metadata_embed(element, ElementType::LAYER_ANIMATIONS);
      group_metadata_embed(element, ElementType::NULL_ANIMATIONS);
    }

    for (auto& child : element.children)
      group_metadata_embed(child);
  }

  void group_metadata_extract(Element& animation, ElementType trackContainerType, Flags flags)
  {
    auto groupContainerType = track_container_group_container_type_get(trackContainerType);
    if (groupContainerType == ElementType::UNKNOWN) return;

    std::erase_if(animation.children, [&](const Element& child) { return child.type == groupContainerType; });
    if (!has_flag(flags, SERIALIZE_GROUPS)) return;

    auto trackContainerIndex = child_index_get(animation, trackContainerType);
    if (trackContainerIndex < 0) return;

    auto& container = animation.children[trackContainerIndex];
    auto groupContainer = element_make(groupContainerType);
    std::vector<Element> tracks{};
    tracks.reserve(container.children.size());

    for (int i = 0; i < (int)container.children.size(); ++i)
    {
      auto child = container.children[i];
      if (child.type == ElementType::GROUP)
      {
        child.index = i;
        groupContainer.children.push_back(std::move(child));
      }
      else
        tracks.push_back(std::move(child));
    }

    container.children = std::move(tracks);
    if (groupContainer.children.empty()) return;
    animation.children.insert(animation.children.begin() + trackContainerIndex + 1, std::move(groupContainer));
  }

  void group_metadata_extract(Element& element, Flags flags)
  {
    if (element.type == ElementType::ANIMATION)
    {
      group_metadata_extract(element, ElementType::LAYER_ANIMATIONS, flags);
      group_metadata_extract(element, ElementType::NULL_ANIMATIONS, flags);
    }

    for (auto& child : element.children)
      group_metadata_extract(child, flags);
  }

  void groups_flatten(Element& element)
  {
    auto container_flatten = [](Element& container, ElementType childType, bool isRootKept)
    {
      int nextGroupId{};
      for (const auto& item : container.children)
        if (item.type == ElementType::GROUP) nextGroupId = std::max(nextGroupId, item.id + 1);

      std::vector<Element> flattened{};
      for (auto item : container.children)
      {
        if (item.type != ElementType::GROUP)
        {
          flattened.push_back(item);
          continue;
        }

        if (item.id < 0) item.id = nextGroupId++;
        auto groupId = item.id;
        auto children = std::move(item.children);
        item.children.clear();

        if (isRootKept)
        {
          bool isRootFound{};

          for (auto child : children)
            if (child.type == ElementType::ROOT_ANIMATION)
            {
              if (isRootFound) continue;
              item.children.push_back(child);
              isRootFound = true;
            }

          if (!isRootFound) item.children.push_back(root_animation_make());
        }

        flattened.push_back(item);

        for (auto child : children)
        {
          if (child.type != childType) continue;
          child.groupId = groupId;
          flattened.push_back(child);
        }
      }

      std::set<int> groupIds{};
      for (const auto& item : flattened)
        if (item.type == ElementType::GROUP) groupIds.insert(item.id);

      for (auto& item : flattened)
        if (item.type == childType && !groupIds.contains(item.groupId)) item.groupId = -1;

      container.children = std::move(flattened);
    };

    if (element.type == ElementType::ANIMATIONS)
      container_flatten(element, ElementType::ANIMATION, false);
    else if (element.type == ElementType::LAYER_ANIMATIONS)
      container_flatten(element, ElementType::LAYER_ANIMATION, true);
    else if (element.type == ElementType::NULL_ANIMATIONS)
      container_flatten(element, ElementType::NULL_ANIMATION, true);

    for (auto& child : element.children)
      groups_flatten(child);
  }

  void group_children_nest(Element& container, ElementType childType)
  {
    if (childType == ElementType::UNKNOWN) return;

    std::set<int> groupIds{};
    for (const auto& child : container.children)
      if (child.type == ElementType::GROUP && child.id >= 0) groupIds.insert(child.id);
    if (groupIds.empty()) return;

    std::unordered_map<int, std::vector<Element>> groupedChildren{};
    for (const auto& child : container.children)
    {
      if (child.type != childType || !groupIds.contains(child.groupId)) continue;
      auto groupedChild = child;
      groupedChild.groupId = -1;
      groupedChildren[child.groupId].push_back(std::move(groupedChild));
    }

    std::vector<Element> nested{};
    nested.reserve(container.children.size());
    for (auto child : container.children)
    {
      if (child.type == childType && groupIds.contains(child.groupId)) continue;
      if (child.type == ElementType::GROUP)
      {
        auto groupId = child.id;
        child.index = -1;
        if (auto grouped = groupedChildren.find(groupId); grouped != groupedChildren.end())
          child.children.insert(child.children.end(), std::make_move_iterator(grouped->second.begin()),
                                std::make_move_iterator(grouped->second.end()));
      }
      nested.push_back(std::move(child));
    }

    container.children = std::move(nested);
  }

  void groups_nest(Element& element)
  {
    if (element.type == ElementType::ANIMATIONS)
      group_children_nest(element, ElementType::ANIMATION);
    else if (element.type == ElementType::LAYER_ANIMATIONS)
      group_children_nest(element, ElementType::LAYER_ANIMATION);
    else if (element.type == ElementType::NULL_ANIMATIONS)
      group_children_nest(element, ElementType::NULL_ANIMATION);

    for (auto& child : element.children)
      groups_nest(child);
  }

  void source_document_erase(Element& element)
  {
    std::erase_if(element.children, [](const Element& child) { return is_source_document_tag(child.tag); });
    for (auto& child : element.children)
      source_document_erase(child);
  }
}
