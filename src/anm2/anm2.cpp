#include "anm2.h"

#include <algorithm>
#include <filesystem>
#include <unordered_map>

#include "filesystem_.h"
#include "map_.h"
#include "time_.h"
#include "vector_.h"
#include "xml_.h"

using namespace tinyxml2;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::anm2
{
  Anm2::Anm2() { info.createdOn = time::get("%m/%d/%Y %I:%M:%S %p"); }

  Anm2::Anm2(const std::string& path, std::string* errorString)
  {
    XMLDocument document;

    if (document.LoadFile(path.c_str()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return;
    }

    filesystem::WorkingDirectory workingDirectory(path, true);

    const XMLElement* element = document.RootElement();

    if (auto infoElement = element->FirstChildElement("Info")) info = Info((XMLElement*)infoElement);
    if (auto contentElement = element->FirstChildElement("Content")) content = Content((XMLElement*)contentElement);
    if (auto animationsElement = element->FirstChildElement("Animations"))
      animations = Animations((XMLElement*)animationsElement);
  }

  XMLElement* Anm2::to_element(XMLDocument& document)
  {
    auto element = document.NewElement("AnimatedActor");
    document.InsertFirstChild(element);

    info.serialize(document, element);
    content.serialize(document, element);
    animations.serialize(document, element);

    return element;
  }

  bool Anm2::serialize(const std::string& path, std::string* errorString)
  {
    XMLDocument document;
    document.InsertFirstChild(to_element(document));

    if (document.SaveFile(path.c_str()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }
    return true;
  }

  std::string Anm2::to_string()
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document));
    return xml::document_to_string(document);
  }

  uint64_t Anm2::hash() { return std::hash<std::string>{}(to_string()); }

  Frame* Anm2::frame_get(int animationIndex, Type itemType, int frameIndex, int itemID)
  {
    if (auto item = item_get(animationIndex, itemType, itemID); item)
      if (vector::in_bounds(item->frames, frameIndex)) return &item->frames[frameIndex];
    return nullptr;
  }

  void Anm2::merge(const Anm2& source, const std::filesystem::path& destinationDirectory,
                   const std::filesystem::path& sourceDirectory)
  {
    using util::map::next_id_get;

    auto remap_path = [&](const std::filesystem::path& original) -> std::filesystem::path
    {
      if (destinationDirectory.empty()) return original;
      std::error_code ec{};
      std::filesystem::path absolute{};
      bool hasAbsolute = false;

      if (!original.empty())
      {
        if (original.is_absolute())
        {
          absolute = original;
          hasAbsolute = true;
        }
        else if (!sourceDirectory.empty())
        {
          absolute = std::filesystem::weakly_canonical(sourceDirectory / original, ec);
          if (ec)
          {
            ec.clear();
            absolute = sourceDirectory / original;
          }
          hasAbsolute = true;
        }
      }

      if (!hasAbsolute) return original;

      auto relative = std::filesystem::relative(absolute, destinationDirectory, ec);
      if (!ec) return relative;
      ec.clear();
      try
      {
        return std::filesystem::relative(absolute, destinationDirectory);
      }
      catch (const std::filesystem::filesystem_error&)
      {
        return original.empty() ? absolute : original;
      }
      return original;
    };

    auto remap_id = [](const auto& table, int value)
    {
      if (value < 0) return value;
      if (auto it = table.find(value); it != table.end()) return it->second;
      return value;
    };

    std::unordered_map<int, int> spritesheetRemap{};
    std::unordered_map<int, int> layerRemap{};
    std::unordered_map<int, int> nullRemap{};
    std::unordered_map<int, int> eventRemap{};
    std::unordered_map<int, int> soundRemap{};

    // Spritesheets
    for (auto& [sourceID, sprite] : source.content.spritesheets)
    {
      auto sheet = sprite;
      sheet.path = remap_path(sheet.path);
      if (!destinationDirectory.empty() && !sheet.path.empty()) sheet.reload(destinationDirectory);

      int destinationID = next_id_get(content.spritesheets);
      content.spritesheets[destinationID] = std::move(sheet);
      spritesheetRemap[sourceID] = destinationID;
    }

    // Sounds
    for (auto& [sourceID, soundEntry] : source.content.sounds)
    {
      auto sound = soundEntry;
      sound.path = remap_path(sound.path);
      if (!destinationDirectory.empty() && !sound.path.empty()) sound.reload(destinationDirectory);

      int destinationID = -1;
      for (auto& [id, existing] : content.sounds)
        if (existing.path == sound.path)
        {
          destinationID = id;
          existing = sound;
          break;
        }

      if (destinationID == -1)
      {
        destinationID = next_id_get(content.sounds);
        content.sounds[destinationID] = sound;
      }
      soundRemap[sourceID] = destinationID;
    }

    auto find_by_name = [](auto& container, const std::string& name) -> int
    {
      for (auto& [id, value] : container)
        if (value.name == name) return id;
      return -1;
    };

    // Layers
    for (auto& [sourceID, sourceLayer] : source.content.layers)
    {
      auto layer = sourceLayer;
      layer.spritesheetID = remap_id(spritesheetRemap, layer.spritesheetID);

      int destinationID = find_by_name(content.layers, layer.name);
      if (destinationID != -1)
        content.layers[destinationID] = layer;
      else
      {
        destinationID = next_id_get(content.layers);
        content.layers[destinationID] = layer;
      }
      layerRemap[sourceID] = destinationID;
    }

    // Nulls
    for (auto& [sourceID, sourceNull] : source.content.nulls)
    {
      auto null = sourceNull;
      int destinationID = find_by_name(content.nulls, null.name);
      if (destinationID != -1)
        content.nulls[destinationID] = null;
      else
      {
        destinationID = next_id_get(content.nulls);
        content.nulls[destinationID] = null;
      }
      nullRemap[sourceID] = destinationID;
    }

    // Events
    for (auto& [sourceID, sourceEvent] : source.content.events)
    {
      auto event = sourceEvent;
      event.soundID = remap_id(soundRemap, event.soundID);

      int destinationID = find_by_name(content.events, event.name);
      if (destinationID != -1)
        content.events[destinationID] = event;
      else
      {
        destinationID = next_id_get(content.events);
        content.events[destinationID] = event;
      }
      eventRemap[sourceID] = destinationID;
    }

    auto remap_item = [&](Item& item)
    {
      for (auto& frame : item.frames)
      {
        frame.soundID = remap_id(soundRemap, frame.soundID);
        frame.eventID = remap_id(eventRemap, frame.eventID);
      }
    };

    auto build_animation = [&](const Animation& incoming) -> Animation
    {
      Animation remapped{};
      remapped.name = incoming.name;
      remapped.frameNum = incoming.frameNum;
      remapped.isLoop = incoming.isLoop;
      remapped.rootAnimation = incoming.rootAnimation;
      remapped.triggers = incoming.triggers;
      remap_item(remapped.rootAnimation);
      remap_item(remapped.triggers);

      for (auto layerID : incoming.layerOrder)
      {
        auto mapped = remap_id(layerRemap, layerID);
        if (mapped >= 0 &&
            std::find(remapped.layerOrder.begin(), remapped.layerOrder.end(), mapped) == remapped.layerOrder.end())
          remapped.layerOrder.push_back(mapped);
      }

      for (auto& [layerID, item] : incoming.layerAnimations)
      {
        auto mapped = remap_id(layerRemap, layerID);
        if (mapped < 0) continue;
        auto copy = item;
        remap_item(copy);
        remapped.layerAnimations[mapped] = std::move(copy);
        if (std::find(remapped.layerOrder.begin(), remapped.layerOrder.end(), mapped) == remapped.layerOrder.end())
          remapped.layerOrder.push_back(mapped);
      }

      for (auto& [nullID, item] : incoming.nullAnimations)
      {
        auto mapped = remap_id(nullRemap, nullID);
        if (mapped < 0) continue;
        auto copy = item;
        remap_item(copy);
        remapped.nullAnimations[mapped] = std::move(copy);
      }

      remap_item(remapped.triggers);
      return remapped;
    };

    auto find_animation = [&](const std::string& name) -> Animation*
    {
      for (auto& animation : animations.items)
        if (animation.name == name) return &animation;
      return nullptr;
    };

    auto merge_item_map = [&](auto& destination, const auto& incoming)
    {
      for (auto& [id, item] : incoming)
      {
        if (!item.frames.empty())
          destination[id] = item;
        else if (!destination.contains(id))
          destination[id] = item;
      }
    };

    for (auto& animation : source.animations.items)
    {
      auto processed = build_animation(animation);
      if (auto destination = find_animation(processed.name))
      {
        destination->frameNum = std::max(destination->frameNum, processed.frameNum);
        destination->isLoop = processed.isLoop;
        if (!processed.rootAnimation.frames.empty()) destination->rootAnimation = processed.rootAnimation;
        if (!processed.triggers.frames.empty()) destination->triggers = processed.triggers;

        merge_item_map(destination->layerAnimations, processed.layerAnimations);
        merge_item_map(destination->nullAnimations, processed.nullAnimations);

        for (auto id : processed.layerOrder)
          if (std::find(destination->layerOrder.begin(), destination->layerOrder.end(), id) ==
              destination->layerOrder.end())
            destination->layerOrder.push_back(id);

        destination->fit_length();
      }
      else
        animations.items.push_back(std::move(processed));
    }

    if (animations.defaultAnimation.empty() && !source.animations.defaultAnimation.empty()) {
      animations.defaultAnimation = source.animations.defaultAnimation;
    }
  }
}
