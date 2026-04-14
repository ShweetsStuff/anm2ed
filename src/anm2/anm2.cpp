#include "anm2.hpp"

#include <algorithm>
#include <filesystem>
#include <limits>
#include <set>
#include <unordered_map>

#include "file_.hpp"
#include "map_.hpp"
#include "math_.hpp"
#include "time_.hpp"
#include "vector_.hpp"
#include "working_directory.hpp"
#include "xml_.hpp"

using namespace tinyxml2;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace
{
  int remap_id(const std::unordered_map<int, int>& table, int value)
  {
    if (value < 0) return value;
    if (auto it = table.find(value); it != table.end()) return it->second;
    return value;
  }

  void region_frames_sync(anm2ed::anm2::Anm2& anm2, bool clearInvalid)
  {
    for (auto& animation : anm2.animations.items)
    {
      for (auto& [layerId, layerAnimation] : animation.layerAnimations)
      {
        if (!anm2.content.layers.contains(layerId)) continue;
        auto& layer = anm2.content.layers.at(layerId);
        auto spritesheet = anm2.spritesheet_get(layer.spritesheetID);
        if (!spritesheet) continue;

        for (auto& frame : layerAnimation.frames)
        {
          if (frame.regionID == -1) continue;
          auto regionIt = spritesheet->regions.find(frame.regionID);
          if (regionIt == spritesheet->regions.end())
          {
            if (clearInvalid) frame.regionID = -1;
            continue;
          }
          frame.crop = regionIt->second.crop;
          frame.size = regionIt->second.size;
          frame.pivot = regionIt->second.pivot;
        }
      }
    }
  }
}

namespace anm2ed::anm2
{
  Anm2::Anm2() { info.createdOn = time::get("%m/%d/%Y %I:%M:%S %p"); }

  Frame Anm2::frame_effective(int layerId, const Frame& frame) const
  {
    auto resolved = frame;
    if (frame.regionID == -1) return resolved;
    if (!content.layers.contains(layerId)) return resolved;

    auto spritesheet = const_cast<Anm2*>(this)->spritesheet_get(content.layers.at(layerId).spritesheetID);
    if (!spritesheet) return resolved;

    auto regionIt = spritesheet->regions.find(frame.regionID);
    if (regionIt == spritesheet->regions.end()) return resolved;

    resolved.crop = regionIt->second.crop;
    resolved.size = regionIt->second.size;
    resolved.pivot = regionIt->second.pivot;
    return resolved;
  }

  vec4 Anm2::animation_rect(Animation& animation, bool isRootTransform) const
  {
    constexpr ivec2 CORNERS[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

    float minX = std::numeric_limits<float>::infinity();
    float minY = std::numeric_limits<float>::infinity();
    float maxX = -std::numeric_limits<float>::infinity();
    float maxY = -std::numeric_limits<float>::infinity();
    bool any = false;

    for (float t = 0.0f; t < (float)animation.frameNum; t += 1.0f)
    {
      mat4 transform(1.0f);

      if (isRootTransform)
      {
        auto root = animation.rootAnimation.frame_generate(t, ROOT);
        transform *= math::quad_model_parent_get(root.position, {}, math::percent_to_unit(root.scale), root.rotation);
      }

      for (auto& [id, layerAnimation] : animation.layerAnimations)
      {
        if (!layerAnimation.isVisible) continue;

        auto frame = frame_effective(id, layerAnimation.frame_generate(t, LAYER));
        if (frame.size == vec2() || !frame.isVisible) continue;

        auto layerTransform = transform * math::quad_model_get(frame.size, frame.position, frame.pivot,
                                                               math::percent_to_unit(frame.scale), frame.rotation);
        for (auto& corner : CORNERS)
        {
          vec4 world = layerTransform * vec4(corner, 0.0f, 1.0f);
          minX = std::min(minX, world.x);
          minY = std::min(minY, world.y);
          maxX = std::max(maxX, world.x);
          maxY = std::max(maxY, world.y);
          any = true;
        }
      }
    }

    if (!any) return vec4(-1.0f);
    return {minX, minY, maxX - minX, maxY - minY};
  }

  void Anm2::bake_special_interpolated_frames(int interval, bool isRoundScale, bool isRoundRotation)
  {
    auto bake_item = [&](Item& item)
    {
      for (int i = (int)item.frames.size() - 1; i >= 0; --i)
      {
        auto interpolation = item.frames[i].interpolation;
        if (interpolation == Frame::Interpolation::NONE || interpolation == Frame::Interpolation::LINEAR) continue;
        item.frames_bake(i, interval, isRoundScale, isRoundRotation);
      }
    };

    for (auto& animation : animations.items)
    {
      bake_item(animation.rootAnimation);
      for (auto& item : animation.layerAnimations | std::views::values)
        bake_item(item);
      for (auto& item : animation.nullAnimations | std::views::values)
        bake_item(item);
    }
  }

  Anm2::Anm2(const std::filesystem::path& path, std::string* errorString)
  {
    XMLDocument document;

    File file(path, "rb");
    if (!file)
    {
      if (errorString) *errorString = localize.get(ERROR_FILE_NOT_FOUND);
      isValid = false;
      return;
    }

    if (document.LoadFile(file.get()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      isValid = false;
      return;
    }

    WorkingDirectory workingDirectory(path, WorkingDirectory::FILE);

    const XMLElement* element = document.RootElement();

    if (auto infoElement = element->FirstChildElement("Info")) info = Info((XMLElement*)infoElement);
    if (auto contentElement = element->FirstChildElement("Content")) content = Content((XMLElement*)contentElement);
    if (auto animationsElement = element->FirstChildElement("Animations"))
      animations = Animations((XMLElement*)animationsElement);

    region_frames_sync(*this, true);
  }

  XMLElement* Anm2::to_element(XMLDocument& document, Flags flags)
  {
    auto normalized = normalized_for_serialize();
    region_frames_sync(normalized, true);
    auto element = document.NewElement("AnimatedActor");

    normalized.info.serialize(document, element);
    normalized.content.serialize(document, element, flags);
    normalized.animations.serialize(document, element, flags);

    return element;
  }

  bool Anm2::serialize(const std::filesystem::path& path, std::string* errorString, Flags flags,
                       bool isBakeSpecialInterpolatedFramesOnSave, bool isRoundScale, bool isRoundRotation)
  {
    XMLDocument document;
    auto serialized = *this;
    if (isBakeSpecialInterpolatedFramesOnSave) serialized.bake_special_interpolated_frames(1, isRoundScale, isRoundRotation);
    document.InsertFirstChild(serialized.to_element(document, flags));

    File file(path, "wb");
    if (!file)
    {
      if (errorString) *errorString = localize.get(ERROR_FILE_PERMISSIONS);
      return false;
    }

    if (document.SaveFile(file.get()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }
    return true;
  }

  std::string Anm2::to_string(Flags flags)
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document, flags));
    return xml::document_to_string(document);
  }

  uint64_t Anm2::hash() { return std::hash<std::string>{}(to_string()); }

  Anm2 Anm2::normalized_for_serialize() const
  {
    auto normalized = *this;
    auto sanitize_layer_order = [](Animation& animation)
    {
      std::vector<int> sanitized{};
      sanitized.reserve(animation.layerAnimations.size());
      std::set<int> seen{};

      for (auto id : animation.layerOrder)
      {
        if (!animation.layerAnimations.contains(id)) continue;
        if (!seen.insert(id).second) continue;
        sanitized.push_back(id);
      }

      std::vector<int> missing{};
      missing.reserve(animation.layerAnimations.size());
      for (auto& id : animation.layerAnimations | std::views::keys)
        if (!seen.contains(id)) missing.push_back(id);

      std::sort(missing.begin(), missing.end());
      sanitized.insert(sanitized.end(), missing.begin(), missing.end());
      animation.layerOrder = std::move(sanitized);
    };
    std::unordered_map<int, int> layerRemap{};

    int normalizedID = 0;
    for (auto& [layerID, layer] : content.layers)
    {
      layerRemap[layerID] = normalizedID;
      ++normalizedID;
    }

    normalized.content.layers.clear();
    for (auto& [layerID, layer] : content.layers)
      normalized.content.layers[remap_id(layerRemap, layerID)] = layer;

    for (auto& animation : normalized.animations.items)
    {
      sanitize_layer_order(animation);
      std::unordered_map<int, Item> layerAnimations{};
      std::vector<int> layerOrder{};

      for (auto layerID : animation.layerOrder)
      {
        auto mappedID = remap_id(layerRemap, layerID);
        if (mappedID >= 0) layerOrder.push_back(mappedID);
      }

      for (auto& [layerID, item] : animation.layerAnimations)
      {
        auto mappedID = remap_id(layerRemap, layerID);
        if (mappedID >= 0) layerAnimations[mappedID] = item;
      }

      animation.layerAnimations = std::move(layerAnimations);
      animation.layerOrder = std::move(layerOrder);
      sanitize_layer_order(animation);
    }

    return normalized;
  }

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
        for (auto& soundID : frame.soundIDs)
          soundID = remap_id(soundRemap, soundID);
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

    if (animations.defaultAnimation.empty() && !source.animations.defaultAnimation.empty())
    {
      animations.defaultAnimation = source.animations.defaultAnimation;
    }
  }
}
