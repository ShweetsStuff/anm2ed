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

  template <class Callback> void animation_track_containers_each(Element& root, Callback&& callback)
  {
    element_each(root,
                 [&](Element& element)
                 {
                   if (element.type != ElementType::ANIMATION) return;
                   for (const auto& row : TRACK_CONTAINERS)
                     if (auto container = child_first_get(element, row.container)) callback(element, *container, row);
                 });
  }

  void group_frames_bake(Element& root)
  {
    animation_track_containers_each(root, [](Element& animation, Element& container, const TrackContainer& row)
                                    { group_frames_bake(container, row.track, animation.frameNum); });
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

  void group_frames_restore(Element& root)
  {
    animation_track_containers_each(root, [](Element&, Element& container, const TrackContainer& row)
                                    { group_frames_restore(container, row.track); });
  }

  int child_index_get(const Element& element, ElementType type)
  {
    auto child = child_first_get(element, type);
    return child ? (int)(child - element.children.data()) : -1;
  }

  ElementType group_child_type_get(ElementType parentType)
  {
    if (parentType == ElementType::ANIMATIONS) return ElementType::ANIMATION;
    auto row = track_container_get(parentType);
    return row && row->container == parentType ? row->track : ElementType::UNKNOWN;
  }

  int track_group_insert_index_get(const Element& container, ElementType trackType, int groupId)
  {
    for (int i = 0; i < (int)container.children.size(); ++i)
      if (container.children[i].type == trackType && container.children[i].groupId == groupId) return i;
    return (int)container.children.size();
  }

  void group_metadata_embed(Element& animation, const TrackContainer& row)
  {
    auto groupContainer = child_first_get(animation, row.groups);
    if (!groupContainer) return;

    std::vector<Element> groups{};
    for (const auto& child : groupContainer->children)
      if (child.type == ElementType::GROUP) groups.push_back(child);

    if (!groups.empty())
    {
      auto& container = child_ensure(animation, row.container);
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
                                            : track_group_insert_index_get(container, row.track, group.id);
        group.index = -1;
        container.children.insert(container.children.begin() + insertIndex, std::move(group));
      }
    }

    std::erase_if(animation.children, [&](const Element& child) { return child.type == row.groups; });
  }

  void group_metadata_embed(Element& root)
  {
    element_each(root,
                 [](Element& element)
                 {
                   if (element.type != ElementType::ANIMATION) return;
                   for (const auto& row : TRACK_CONTAINERS)
                     group_metadata_embed(element, row);
                 });
  }

  void group_metadata_extract(Element& animation, const TrackContainer& row, Flags flags)
  {
    std::erase_if(animation.children, [&](const Element& child) { return child.type == row.groups; });
    if (!has_flag(flags, SERIALIZE_GROUPS)) return;

    auto trackContainerIndex = child_index_get(animation, row.container);
    if (trackContainerIndex < 0) return;

    auto& container = animation.children[trackContainerIndex];
    auto groupContainer = element_make(row.groups);
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

  void group_metadata_extract(Element& root, Flags flags)
  {
    element_each(root,
                 [&](Element& element)
                 {
                   if (element.type != ElementType::ANIMATION) return;
                   for (const auto& row : TRACK_CONTAINERS)
                     group_metadata_extract(element, row, flags);
                 });
  }

  void group_children_flatten(Element& container, ElementType childType)
  {
    if (childType == ElementType::UNKNOWN) return;

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
      auto children = std::move(item.children);
      item.children.clear();

      if (childType != ElementType::ANIMATION)
      {
        auto root = std::ranges::find(children, ElementType::ROOT_ANIMATION, &Element::type);
        item.children.push_back(root == children.end() ? root_animation_make() : *root);
      }

      flattened.push_back(item);

      for (auto child : children)
      {
        if (child.type != childType) continue;
        child.groupId = item.id;
        flattened.push_back(child);
      }
    }

    std::set<int> groupIds{};
    for (const auto& item : flattened)
      if (item.type == ElementType::GROUP) groupIds.insert(item.id);

    for (auto& item : flattened)
      if (item.type == childType && !groupIds.contains(item.groupId)) item.groupId = -1;

    container.children = std::move(flattened);
  }

  void groups_flatten(Element& root)
  {
    element_each(root, [](Element& element) { group_children_flatten(element, group_child_type_get(element.type)); });
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

  void groups_nest(Element& root)
  {
    element_each(root, [](Element& element) { group_children_nest(element, group_child_type_get(element.type)); });
  }

  void source_document_erase(Element& root)
  {
    element_each(root, [](Element& element)
                 { std::erase_if(element.children, [](const Element& child) { return is_source_document_tag(child.tag); }); });
  }
}
