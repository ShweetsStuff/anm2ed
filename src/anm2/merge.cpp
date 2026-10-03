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

    auto destinationTracks = &child_ensure(destination, containerType);

    std::unordered_map<int, int> groupRemap{};
    std::set<int> matchedGroupIds{};
    for (const auto& sourceGroup : sourceTracks->children)
    {
      if (sourceGroup.type != ElementType::GROUP) continue;

      auto destinationGroup = std::find_if(destinationTracks->children.begin(), destinationTracks->children.end(),
                                           [&](const Element& child)
                                           {
                                             return child.type == ElementType::GROUP &&
                                                    child.name == sourceGroup.name &&
                                                    !matchedGroupIds.contains(child.id);
                                           });

      if (destinationGroup == destinationTracks->children.end())
      {
        auto group = sourceGroup;
        group.id = element_child_next_id_get(*destinationTracks, ElementType::GROUP);
        destinationTracks->children.push_back(std::move(group));
        destinationGroup = std::prev(destinationTracks->children.end());
      }
      else if (auto sourceRoot = child_first_get(sourceGroup, ElementType::ROOT_ANIMATION))
        track_merge(child_ensure(*destinationGroup, ElementType::ROOT_ANIMATION), *sourceRoot, type);

      groupRemap[sourceGroup.id] = destinationGroup->id;
      matchedGroupIds.insert(destinationGroup->id);
    }

    for (const auto& sourceTrack : sourceTracks->children)
    {
      if (sourceTrack.type == ElementType::GROUP) continue;

      auto mappedGroup = groupRemap.find(sourceTrack.groupId);
      auto groupId = mappedGroup != groupRemap.end() ? mappedGroup->second : -1;

      if (auto destinationTrack = track_find(*destinationTracks, sourceTrack.type, track_id_get(sourceTrack), groupId))
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

      auto single_track_merge = [&](ElementType trackType)
      {
        if (auto sourceTrack = child_first_get(*source, trackType))
          track_merge(child_ensure(*targetAnimation, trackType), *sourceTrack, type);
      };

      single_track_merge(ElementType::ROOT_ANIMATION);
      for (const auto& row : TRACK_CONTAINERS)
        animation_tracks_merge(*targetAnimation, *source, row.container, type);
      single_track_merge(ElementType::TRIGGERS);
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

  bool Anm2::file_merge(const std::filesystem::path& path, const std::filesystem::path& directory,
                        FileMergePreset preset)
  {
    Anm2 source(path);
    if (!source.isValid) return false;

    auto isAppendAsNew = preset == FILE_MERGE_PRESET_APPEND_AS_NEW;
    std::map<ElementType, std::unordered_map<int, int>> remaps{};

    auto path_remap = [&](const std::filesystem::path& original)
    {
      if (directory.empty() || original.empty()) return original;
      std::error_code ec{};
      auto absolute =
          original.is_absolute() ? original : std::filesystem::weakly_canonical(path.parent_path() / original, ec);
      if (ec) absolute = path.parent_path() / original;
      auto relative = std::filesystem::relative(absolute, directory, ec);
      return ec ? original : relative;
    };

    auto id_remap = [&](ElementType type, int id)
    {
      auto it = remaps[type].find(id);
      return it == remaps[type].end() ? -1 : it->second;
    };

    auto name_find = [](Element& container, ElementType type, const std::string& name)
    { return child_find(container, [&](const Element& child) { return child.type == type && child.name == name; }); };

    auto name_unique_get = [&](Element& container, ElementType type, const std::string& name)
    {
      auto candidate = name;
      for (int i = 2; name_find(container, type, candidate); ++i)
        candidate = std::format("{} {}", name, i);
      return candidate;
    };

    auto named_element_merge = [&](Element& container, Element item)
    {
      auto existing = isAppendAsNew ? nullptr : name_find(container, item.type, item.name);
      if (existing)
      {
        item.id = existing->id;
        *existing = item;
        return item.id;
      }
      item.id = element_child_next_id_get(container, item.type);
      if (isAppendAsNew) item.name = name_unique_get(container, item.type, item.name);
      container.children.push_back(item);
      return item.id;
    };

    auto spritesheet_import = [&](int sourceId)
    {
      auto& remap = remaps[ElementType::SPRITESHEET];
      if (remap.contains(sourceId)) return remap[sourceId];
      auto spritesheet = source.element_get(ElementType::SPRITESHEET, sourceId);
      auto destination = element_get(ElementType::SPRITESHEETS);
      if (sourceId < 0 || !spritesheet || !destination) return -1;

      auto imported = *spritesheet;
      imported.id = element_child_next_id_get(*destination, ElementType::SPRITESHEET);
      imported.path = path_remap(imported.path);
      destination->children.push_back(imported);
      return remap[sourceId] = imported.id;
    };

    auto sourceSounds = source.element_get(ElementType::SOUNDS);
    auto destinationSounds = element_get(ElementType::SOUNDS);
    if (sourceSounds && destinationSounds)
      for (auto sound : sourceSounds->children)
      {
        if (sound.type != ElementType::SOUND_ELEMENT) continue;
        auto sourceId = sound.id;
        sound.path = path_remap(sound.path);
        auto existing = isAppendAsNew ? nullptr
                                      : child_find(*destinationSounds, [&](const Element& child)
                                                   { return child.type == sound.type && child.path == sound.path; });
        sound.id = existing ? existing->id : element_child_next_id_get(*destinationSounds, sound.type);
        if (existing)
          *existing = sound;
        else
          destinationSounds->children.push_back(sound);
        remaps[sound.type][sourceId] = sound.id;
      }

    for (auto type : {ElementType::LAYER_ELEMENT, ElementType::NULL_ELEMENT, ElementType::EVENT_ELEMENT})
    {
      auto sourceContainer = source.element_get(ELEMENT_CONTAINERS[(int)type]);
      auto destinationContainer = element_get(ELEMENT_CONTAINERS[(int)type]);
      if (!sourceContainer || !destinationContainer) continue;
      for (auto item : sourceContainer->children)
      {
        if (item.type != type) continue;
        if (type == ElementType::LAYER_ELEMENT)
        {
          auto existing = isAppendAsNew ? nullptr : name_find(*destinationContainer, type, item.name);
          item.spritesheetId = existing ? existing->spritesheetId : spritesheet_import(item.spritesheetId);
        }
        remaps[type][item.id] = named_element_merge(*destinationContainer, item);
      }
    }

    auto item_remap = [&](Element& item)
    {
      for (auto& frame : item.children)
      {
        if (frame.type != ElementType::FRAME && frame.type != ElementType::TRIGGER) continue;
        for (auto& soundId : frame.soundIds)
          soundId = id_remap(ElementType::SOUND_ELEMENT, soundId);
        frame.eventId = id_remap(ElementType::EVENT_ELEMENT, frame.eventId);
      }
    };

    auto animation_build = [&](const Element& incoming)
    {
      auto animation = element_make(ElementType::ANIMATION);
      animation.name = incoming.name;
      animation.frameNum = incoming.frameNum;
      animation.isLoop = incoming.isLoop;

      auto single_track_build = [&](ElementType type)
      {
        if (auto track = child_first_get(incoming, type)) item_remap(animation.children.emplace_back(*track));
      };

      single_track_build(ElementType::ROOT_ANIMATION);
      for (const auto& row : TRACK_CONTAINERS)
      {
        auto sourceTracks = child_first_get(incoming, row.container);
        if (!sourceTracks) continue;
        auto container = element_make(row.container);
        for (auto item : sourceTracks->children)
        {
          if (item.type == ElementType::GROUP)
          {
            std::erase_if(item.children,
                          [](const Element& child) { return child.type != ElementType::ROOT_ANIMATION; });
            if (item.children.empty()) item.children.push_back(root_animation_make());
            if (item.id == -1) item.id = element_child_next_id_get(container, ElementType::GROUP);
            container.children.push_back(item);
            continue;
          }
          if (item.type != row.track) continue;
          item.*(row.id) = id_remap(row.element, item.*(row.id));
          if (item.*(row.id) < 0) continue;
          item_remap(item);
          container.children.push_back(item);
        }
        animation.children.push_back(container);
      }
      single_track_build(ElementType::TRIGGERS);
      return animation;
    };

    auto sourceAnimations = source.element_get(ElementType::ANIMATIONS);
    auto destinationAnimations = element_get(ElementType::ANIMATIONS);
    if (!sourceAnimations || !destinationAnimations) return true;

    std::string defaultAnimationName{};
    for (const auto& incoming : sourceAnimations->children)
    {
      if (incoming.type != ElementType::ANIMATION) continue;
      auto processed = animation_build(incoming);
      auto destination = name_find(*destinationAnimations, ElementType::ANIMATION, processed.name);
      if (isAppendAsNew)
        processed.name = name_unique_get(*destinationAnimations, ElementType::ANIMATION, processed.name);
      if (incoming.name == sourceAnimations->defaultAnimation) defaultAnimationName = processed.name;

      if (!destination || isAppendAsNew)
      {
        destinationAnimations->children.push_back(processed);
        continue;
      }

      if (preset == FILE_MERGE_PRESET_REPLACE_MATCHING)
      {
        *destination = processed;
        continue;
      }

      destination->isLoop = processed.isLoop;
      for (auto type : {ElementType::ROOT_ANIMATION, ElementType::TRIGGERS})
        if (auto track = child_first_get(processed, type); track && !track->children.empty())
          child_ensure(*destination, type) = *track;

      for (const auto& row : TRACK_CONTAINERS)
      {
        auto sourceTracks = child_first_get(processed, row.container);
        if (!sourceTracks) continue;
        auto& destinationTracks = child_ensure(*destination, row.container);

        std::unordered_map<int, int> groupRemap{};
        for (auto item : sourceTracks->children)
        {
          if (item.type != ElementType::GROUP) continue;
          auto sourceGroupId = item.id;
          item.id = element_child_next_id_get(destinationTracks, ElementType::GROUP);
          destinationTracks.children.push_back(item);
          groupRemap[sourceGroupId] = item.id;
        }

        for (auto item : sourceTracks->children)
        {
          if (item.type != row.track) continue;
          if (item.groupId != -1) item.groupId = groupRemap.contains(item.groupId) ? groupRemap[item.groupId] : -1;
          if (auto existing = track_find(destinationTracks, row.track, track_id_get(item)))
          {
            if (!item.children.empty()) *existing = item;
          }
          else
            destinationTracks.children.push_back(item);
        }
      }

      destination->frameNum = std::max({destination->frameNum, processed.frameNum, animation_length_get(*destination)});
    }

    if (destinationAnimations->defaultAnimation.empty() && !sourceAnimations->defaultAnimation.empty())
      destinationAnimations->defaultAnimation =
          defaultAnimationName.empty() ? sourceAnimations->defaultAnimation : defaultAnimationName;
    return true;
  }
}
