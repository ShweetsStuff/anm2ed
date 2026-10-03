#include "internal.hpp"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed
{
  bool Anm2::animations_deserialize(const std::string& string, int start, std::set<int>& indices,
                                    std::string* errorString, std::set<int>* groupIds)
  {
    XMLDocument document{};
    if (document.Parse(string.c_str()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }

    auto animations = element_get(ElementType::ANIMATIONS);
    if (!animations)
    {
      if (errorString) *errorString = "No animations container.";
      return false;
    }

    start = std::clamp(start, 0, animations_count_get(*animations));

    std::vector<Element> items{};
    std::map<int, int> groupRemap{};
    auto nextGroupId = element_child_next_id_get(*animations, ElementType::GROUP);
    for (auto element = document.FirstChildElement(); element; element = element->NextSiblingElement())
    {
      auto item = element_read(element);
      if (item.type != ElementType::ANIMATION && item.type != ElementType::GROUP) continue;

      if (item.type == ElementType::GROUP)
      {
        auto sourceGroupId = item.id;
        if (!groupRemap.contains(sourceGroupId)) groupRemap[sourceGroupId] = nextGroupId++;
        item.id = groupRemap[sourceGroupId];
        if (groupIds) groupIds->insert(item.id);
      }

      items.push_back(item);
    }

    if (items.empty())
    {
      if (errorString) *errorString = "No valid animation(s).";
      return false;
    }

    int count{};
    for (auto& item : items)
    {
      if (item.type == ElementType::ANIMATION)
      {
        if (auto mapped = groupRemap.find(item.groupId); mapped != groupRemap.end()) item.groupId = mapped->second;
        indices.insert(start + count);
        ++count;
      }
    }

    auto childIndex = animations_child_insert_index_get(*animations, start);
    animations->children.insert(animations->children.begin() + childIndex, items.begin(), items.end());
    return true;
  }

  void track_merge(Element& destination, const Element& source, types::merge::Type type)
  {
    switch (type)
    {
      case types::merge::APPEND:
        destination.children.insert(destination.children.end(), source.children.begin(), source.children.end());
        break;
      case types::merge::PREPEND:
        destination.children.insert(destination.children.begin(), source.children.begin(), source.children.end());
        break;
      case types::merge::REPLACE:
        if (destination.children.size() < source.children.size()) destination.children.resize(source.children.size());
        for (int i = 0; i < (int)source.children.size(); ++i)
          destination.children[i] = source.children[i];
        break;
      case types::merge::IGNORE:
      default:
        break;
    }
  }

  void animation_tracks_merge(Element& destination, const Element& source, ElementType containerType,
                              types::merge::Type type)
  {
    auto sourceTracks = child_first_get(source, containerType);
    if (!sourceTracks) return;

    auto destinationTracks = child_first_get(destination, containerType);
    if (!destinationTracks)
    {
      destination.children.push_back(element_make(containerType));
      destinationTracks = &destination.children.back();
    }

    std::unordered_map<int, int> groupRemap{};
    std::set<int> matchedGroupIds{};
    for (const auto& sourceGroup : sourceTracks->children)
    {
      if (sourceGroup.type != ElementType::GROUP) continue;

      auto destinationGroup = std::find_if(destinationTracks->children.begin(), destinationTracks->children.end(),
                                           [&](const Element& child)
                                           {
                                             return child.type == ElementType::GROUP &&
                                                    child.name == sourceGroup.name && !matchedGroupIds.contains(child.id);
                                           });

      if (destinationGroup == destinationTracks->children.end())
      {
        auto group = sourceGroup;
        group.id = element_child_next_id_get(*destinationTracks, ElementType::GROUP);
        destinationTracks->children.push_back(std::move(group));
        destinationGroup = std::prev(destinationTracks->children.end());
      }
      else if (auto sourceRoot = child_first_get(sourceGroup, ElementType::ROOT_ANIMATION))
      {
        auto destinationRoot = child_first_get(*destinationGroup, ElementType::ROOT_ANIMATION);
        if (!destinationRoot)
        {
          destinationGroup->children.push_back(root_animation_make());
          destinationRoot = &destinationGroup->children.back();
        }
        track_merge(*destinationRoot, *sourceRoot, type);
      }

      groupRemap[sourceGroup.id] = destinationGroup->id;
      matchedGroupIds.insert(destinationGroup->id);
    }

    for (const auto& sourceTrack : sourceTracks->children)
    {
      if (sourceTrack.type == ElementType::GROUP) continue;

      auto mappedGroup = groupRemap.find(sourceTrack.groupId);
      auto groupId = mappedGroup != groupRemap.end() ? mappedGroup->second : -1;

      if (auto destinationTrack =
              track_group_find(*destinationTracks, sourceTrack.type, track_id_get(sourceTrack), groupId))
        track_merge(*destinationTrack, sourceTrack, type);
      else
      {
        auto track = sourceTrack;
        track.groupId = groupId;
        destinationTracks->children.push_back(std::move(track));
      }
    }
  }

  int Anm2::animations_merge(int target, std::set<int>& sources, types::merge::Type type, bool isDeleteAfter)
  {
    auto animations = element_get(ElementType::ANIMATIONS);
    auto targetAnimation = element_get(ElementType::ANIMATION, target);
    if (!animations || !targetAnimation) return target;

    if (!targetAnimation->name.ends_with(ANIMATION_MERGED_SUFFIX))
      targetAnimation->name += std::string(ANIMATION_MERGED_SUFFIX);

    for (auto index : sources)
    {
      if (index == target) continue;
      auto source = element_get(ElementType::ANIMATION, index);
      targetAnimation = element_get(ElementType::ANIMATION, target);
      if (!source || !targetAnimation) continue;

      if (auto sourceRoot = child_first_get(*source, ElementType::ROOT_ANIMATION))
      {
        auto targetRoot = child_first_get(*targetAnimation, ElementType::ROOT_ANIMATION);
        if (!targetRoot)
        {
          targetAnimation->children.push_back(element_make(ElementType::ROOT_ANIMATION));
          targetRoot = &targetAnimation->children.back();
        }
        track_merge(*targetRoot, *sourceRoot, type);
      }

      animation_tracks_merge(*targetAnimation, *source, ElementType::LAYER_ANIMATIONS, type);
      animation_tracks_merge(*targetAnimation, *source, ElementType::NULL_ANIMATIONS, type);

      if (auto sourceTriggers = child_first_get(*source, ElementType::TRIGGERS))
      {
        auto targetTriggers = child_first_get(*targetAnimation, ElementType::TRIGGERS);
        if (!targetTriggers)
        {
          targetAnimation->children.push_back(element_make(ElementType::TRIGGERS));
          targetTriggers = &targetAnimation->children.back();
        }
        track_merge(*targetTriggers, *sourceTriggers, type);
      }
    }

    int finalIndex = target;
    if (isDeleteAfter)
      for (auto it = sources.rbegin(); it != sources.rend(); ++it)
      {
        auto source = *it;
        auto sourceChildIndex = animations_child_index_get(*animations, source);
        if (source == target || sourceChildIndex == -1) continue;
        animations->children.erase(animations->children.begin() + sourceChildIndex);
        if (source < finalIndex) --finalIndex;
      }

    if (auto finalAnimation = element_get(ElementType::ANIMATION, finalIndex))
      finalAnimation->frameNum = animation_length_get(*finalAnimation);
    return finalIndex;
  }
}
