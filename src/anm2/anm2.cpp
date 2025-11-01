#include "anm2.h"

#include <ranges>

#include "filesystem_.h"
#include "map_.h"
#include "time_.h"
#include "unordered_map_.h"
#include "vector_.h"

using namespace tinyxml2;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::anm2
{
  Anm2::Anm2()
  {
    info.createdOn = time::get("%d-%B-%Y %I:%M:%S");
  }

  bool Anm2::serialize(const std::string& path, std::string* errorString)
  {
    XMLDocument document;

    auto* element = document.NewElement("AnimatedActor");
    document.InsertFirstChild(element);

    info.serialize(document, element);
    content.serialize(document, element);
    animations.serialize(document, element);

    if (document.SaveFile(path.c_str()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }
    return true;
  }

  std::string Anm2::to_string()
  {
    XMLDocument document;

    auto* element = document.NewElement("AnimatedActor");
    document.InsertFirstChild(element);

    info.serialize(document, element);
    content.serialize(document, element);
    animations.serialize(document, element);

    XMLPrinter printer;
    document.Print(&printer);
    return std::string(printer.CStr());
  }

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

  uint64_t Anm2::hash()
  {
    return std::hash<std::string>{}(to_string());
  }

  Animation* Anm2::animation_get(Reference reference)
  {
    return vector::find(animations.items, reference.animationIndex);
  }

  std::vector<std::string> Anm2::animation_names_get()
  {
    std::vector<std::string> names{};
    for (auto& animation : animations.items)
      names.push_back(animation.name);
    return names;
  }

  Item* Anm2::item_get(Reference reference)
  {
    if (Animation* animation = animation_get(reference))
    {
      switch (reference.itemType)
      {
        case ROOT:
          return &animation->rootAnimation;
        case LAYER:
          return unordered_map::find(animation->layerAnimations, reference.itemID);
        case NULL_:
          return map::find(animation->nullAnimations, reference.itemID);
        case TRIGGER:
          return &animation->triggers;
        default:
          return nullptr;
      }
    }
    return nullptr;
  }

  Frame* Anm2::frame_get(Reference reference)
  {
    Item* item = item_get(reference);
    if (!item) return nullptr;
    return vector::find(item->frames, reference.frameIndex);
    return nullptr;
  }

  bool Anm2::spritesheet_add(const std::string& directory, const std::string& path, int& id)
  {
    Spritesheet spritesheet(directory, path);
    if (!spritesheet.is_valid()) return false;
    id = map::next_id_get(content.spritesheets);
    content.spritesheets[id] = std::move(spritesheet);
    return true;
  }

  void Anm2::spritesheet_remove(int id)
  {
    content.spritesheets.erase(id);
  }

  Spritesheet* Anm2::spritesheet_get(int id)
  {
    return map::find(content.spritesheets, id);
  }

  std::set<int> Anm2::spritesheets_unused()
  {
    return content.spritesheets_unused();
  }

  std::vector<std::string> Anm2::spritesheet_names_get()
  {
    std::vector<std::string> names{};
    for (auto& [id, spritesheet] : content.spritesheets)
      names.push_back(std::format(SPRITESHEET_FORMAT, id, spritesheet.path.c_str()));
    return names;
  }

  Reference Anm2::layer_add(Reference reference, std::string name, int spritesheetID, locale::Type locale)
  {
    auto id = reference.itemID == -1 ? map::next_id_get(content.layers) : reference.itemID;
    auto& layer = content.layers[id];

    layer.name = !name.empty() ? name : layer.name;
    layer.spritesheetID = content.spritesheets.contains(spritesheetID) ? spritesheetID : 0;

    auto add = [&](Animation* animation, int id)
    {
      animation->layerAnimations[id] = Item();
      animation->layerOrder.push_back(id);
    };

    if (locale == locale::GLOBAL)
    {
      for (auto& animation : animations.items)
        if (!animation.layerAnimations.contains(id)) add(&animation, id);
    }
    else if (locale == locale::LOCAL)
    {
      if (auto animation = animation_get(reference))
        if (!animation->layerAnimations.contains(id)) add(animation, id);
    }

    return {reference.animationIndex, LAYER, id};
  }

  Reference Anm2::null_add(Reference reference, std::string name, locale::Type locale)
  {
    auto id = reference.itemID == -1 ? map::next_id_get(content.nulls) : reference.itemID;
    auto& null = content.nulls[id];

    null.name = !name.empty() ? name : null.name;

    auto add = [&](Animation* animation, int id) { animation->nullAnimations[id] = Item(); };

    if (locale == locale::GLOBAL)
    {
      for (auto& animation : animations.items)
        if (!animation.nullAnimations.contains(id)) add(&animation, id);
    }
    else if (locale == locale::LOCAL)
    {
      if (auto animation = animation_get(reference))
        if (!animation->nullAnimations.contains(id)) add(animation, id);
    }

    return {reference.animationIndex, LAYER, id};
  }

  void Anm2::event_add(int& id)
  {
    content.event_add(id);
  }

  std::set<int> Anm2::events_unused(Reference reference)
  {
    std::set<int> used{};
    std::set<int> unused{};

    if (auto animation = animation_get(reference); animation)
      for (auto& frame : animation->triggers.frames)
        used.insert(frame.eventID);
    else
      for (auto& animation : animations.items)
        for (auto& frame : animation.triggers.frames)
          used.insert(frame.eventID);

    for (auto& id : content.events | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  std::set<int> Anm2::layers_unused(Reference reference)
  {
    std::set<int> used{};
    std::set<int> unused{};

    if (auto animation = animation_get(reference); animation)
      for (auto& id : animation->layerAnimations | std::views::keys)
        used.insert(id);
    else
      for (auto& animation : animations.items)
        for (auto& id : animation.layerAnimations | std::views::keys)
          used.insert(id);

    for (auto& id : content.layers | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  std::set<int> Anm2::nulls_unused(Reference reference)
  {
    std::set<int> used{};
    std::set<int> unused{};

    if (auto animation = animation_get(reference); animation)
      for (auto& id : animation->nullAnimations | std::views::keys)
        used.insert(id);
    else
      for (auto& animation : animations.items)
        for (auto& id : animation.nullAnimations | std::views::keys)
          used.insert(id);

    for (auto& id : content.nulls | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  std::vector<std::string> Anm2::event_names_get()
  {
    std::vector<std::string> names{};
    for (auto& event : content.events | std::views::values)
      names.push_back(event.name);
    return names;
  }

  bool Anm2::sound_add(const std::string& directory, const std::string& path, int& id)
  {
    id = map::next_id_get(content.sounds);
    content.sounds[id] = Sound(directory, path);
    return true;
  }

  std::set<int> Anm2::sounds_unused()
  {
    std::set<int> used;
    for (auto& event : content.events | std::views::values)
      used.insert(event.soundID);

    std::set<int> unused;
    for (auto& id : content.sounds | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  std::vector<std::string> Anm2::sound_names_get()
  {
    std::vector<std::string> names{};
    for (auto& [id, sound] : content.sounds)
      names.push_back(std::format(SOUND_FORMAT, id, sound.path.c_str()));
    return names;
  }

  void Anm2::bake(Reference reference, int interval, bool isRoundScale, bool isRoundRotation)
  {
    Item* item = item_get(reference);
    if (!item) return;

    Frame* frame = frame_get(reference);
    if (!frame) return;

    if (frame->delay == FRAME_DELAY_MIN) return;

    Reference referenceNext = reference;
    referenceNext.frameIndex = reference.frameIndex + 1;

    Frame* frameNext = frame_get(referenceNext);
    if (!frameNext) frameNext = frame;

    Frame baseFrame = *frame;
    Frame baseFrameNext = *frameNext;

    int delay{};
    int index = reference.frameIndex;

    while (delay < baseFrame.delay)
    {
      float interpolation = (float)delay / baseFrame.delay;

      Frame baked = baseFrame;
      baked.delay = std::min(interval, baseFrame.delay - delay);
      baked.isInterpolated = (index == reference.frameIndex) ? baseFrame.isInterpolated : false;

      baked.rotation = glm::mix(baseFrame.rotation, baseFrameNext.rotation, interpolation);
      baked.position = glm::mix(baseFrame.position, baseFrameNext.position, interpolation);
      baked.scale = glm::mix(baseFrame.scale, baseFrameNext.scale, interpolation);
      baked.colorOffset = glm::mix(baseFrame.colorOffset, baseFrameNext.colorOffset, interpolation);
      baked.tint = glm::mix(baseFrame.tint, baseFrameNext.tint, interpolation);

      if (isRoundScale) baked.scale = vec2(ivec2(baked.scale));
      if (isRoundRotation) baked.rotation = (int)baked.rotation;

      if (index == reference.frameIndex)
        item->frames[index] = baked;
      else
        item->frames.insert(item->frames.begin() + index, baked);
      index++;

      delay += baked.delay;
    }
  }

  void Anm2::generate_from_grid(Reference reference, ivec2 startPosition, ivec2 size, ivec2 pivot, int columns,
                                int count, int delay)
  {
    auto item = item_get(reference);
    if (!item) return;

    for (int i = 0; i < count; i++)
    {
      auto row = i / columns;
      auto column = i % columns;

      Frame frame{};

      frame.delay = delay;
      frame.pivot = pivot;
      frame.size = size;
      frame.crop = startPosition + ivec2(size.x * column, size.y * row);

      item->frames.emplace_back(frame);
    }
  }
}
