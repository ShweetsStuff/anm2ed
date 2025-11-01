#include "document.h"

#include <algorithm>
#include <ranges>

#include "filesystem_.h"
#include "log.h"
#include "map_.h"
#include "toast.h"
#include "vector_.h"

using namespace anm2ed::anm2;
using namespace anm2ed::imgui;
using namespace anm2ed::types;
using namespace anm2ed::util;

using namespace glm;

namespace anm2ed
{
  Document::Document(const std::string& path, bool isNew, std::string* errorString)
  {
    if (!filesystem::path_is_exist(path)) return;

    if (isNew)
      anm2 = anm2::Anm2();
    else
    {
      anm2 = Anm2(path, errorString);
      if (errorString && !errorString->empty()) return;
    }

    this->path = path;
    clean();
    change(Document::ALL);
  }

  bool Document::save(const std::string& path, std::string* errorString)
  {
    this->path = !path.empty() ? path : this->path.string();

    if (anm2.serialize(this->path, errorString))
    {
      toasts.info(std::format("Saved document to: {}", this->path.string()));
      clean();
      return true;
    }
    else if (errorString)
      toasts.warning(std::format("Could not save document to: {} ({})", this->path.string(), *errorString));

    return false;
  }

  bool Document::autosave(const std::string& path, std::string* errorString)
  {
    if (anm2.serialize(path, errorString))
    {
      autosaveHash = hash;
      lastAutosaveTime = 0.0f;
      toasts.info("Autosaving...");
      logger.info(std::format("Autosaved document to: {}", path));
      return true;
    }
    else if (errorString)
      toasts.warning(std::format("Could not autosave document to: {} ({})", path, *errorString));

    return false;
  }

  void Document::hash_set()
  {
    hash = anm2.hash();
  }

  void Document::clean()
  {
    saveHash = anm2.hash();
    hash = saveHash;
    lastAutosaveTime = 0.0f;
    isForceDirty = false;
  }

  void Document::change(ChangeType type)
  {
    hash_set();

    auto layers_set = [&]() { unusedLayerIDs = anm2.layers_unused(); };
    auto nulls_set = [&]() { unusedNullIDs = anm2.nulls_unused(); };
    auto events_set = [&]()
    {
      unusedEventIDs = anm2.events_unused();
      eventNames = anm2.event_names_get();
      for (auto& name : eventNames)
        eventNamesCStr.push_back(name.c_str());
    };

    auto animations_set = [&]()
    {
      animationNames = anm2.animation_names_get();
      animationNamesCStr.clear();
      animationNames.insert(animationNames.begin(), "None");
      for (auto& name : animationNames)
        animationNamesCStr.push_back(name.c_str());
    };

    auto spritesheets_set = [&]()
    {
      unusedSpritesheetIDs = anm2.spritesheets_unused();
      spritesheetNames = anm2.spritesheet_names_get();
      spritesheetNamesCStr.clear();
      for (auto& name : spritesheetNames)
        spritesheetNamesCStr.push_back(name.c_str());
    };

    auto sounds_set = [&]()
    {
      unusedSoundIDs = anm2.sounds_unused();
      soundNames = anm2.sound_names_get();
      soundNamesCStr.clear();
      for (auto& name : soundNames)
        soundNamesCStr.push_back(name.c_str());
    };

    switch (type)
    {
      case LAYERS:
        layers_set();
        break;
      case NULLS:
        nulls_set();
        break;
      case EVENTS:
        events_set();
        break;
      case ANIMATIONS:
        animations_set();
        break;
      case SPRITESHEETS:
        spritesheets_set();
        break;
      case SOUNDS:
        sounds_set();
        break;
      case ITEMS:
        break;
      case ALL:
        layers_set();
        nulls_set();
        events_set();
        animations_set();
        spritesheets_set();
        sounds_set();
        break;
      default:
        break;
    }
  }

  bool Document::is_dirty()
  {
    return hash != saveHash;
  }

  bool Document::is_autosave_dirty()
  {
    return hash != autosaveHash;
  }

  std::filesystem::path Document::directory_get()
  {
    return path.parent_path();
  }

  std::filesystem::path Document::filename_get()
  {
    return path.filename();
  }

  bool Document::is_valid()
  {
    return !path.empty();
  }

  anm2::Frame* Document::frame_get()
  {
    return anm2.frame_get(reference);
  }

  void Document::frames_bake(int interval, bool isRoundScale, bool isRoundRotation)
  {
    snapshot("Bake Frames");
    anm2.bake(reference, interval, isRoundScale, isRoundRotation);
    change(Document::FRAMES);
  }

  void Document::frames_add(anm2::Item* item)
  {
    if (!item) return;

    auto frame = frame_get();

    if (frame)
    {
      item->frames.insert(item->frames.begin() + reference.frameIndex, *frame);
      reference.frameIndex++;
    }
    else if (!item->frames.empty())
    {
      auto frame = item->frames.back();
      item->frames.emplace_back(frame);
      reference.frameIndex = item->frames.size() - 1;
    }
  }

  void Document::frames_delete(anm2::Item* item)
  {
    if (!item) return;

    snapshot("Delete Frames");
    item->frames.erase(item->frames.begin() + reference.frameIndex);
    reference.frameIndex = glm::max(-1, --reference.frameIndex);
    change(Document::FRAMES);
  }

  void Document::frame_crop_set(anm2::Frame* frame, vec2 crop)
  {
    if (!frame) return;
    snapshot("Frame Crop");
    frame->crop = crop;
    change(Document::FRAMES);
  }

  void Document::frame_size_set(anm2::Frame* frame, vec2 size)
  {
    if (!frame) return;
    snapshot("Frame Size");
    frame->size = size;
    change(Document::FRAMES);
  }

  void Document::frame_position_set(anm2::Frame* frame, vec2 position)
  {
    if (!frame) return;
    snapshot("Frame Position");
    frame->position = position;
    change(Document::FRAMES);
  }

  void Document::frame_pivot_set(anm2::Frame* frame, vec2 pivot)
  {
    if (!frame) return;
    snapshot("Frame Pivot");
    frame->pivot = pivot;
    change(Document::FRAMES);
  }

  void Document::frame_scale_set(anm2::Frame* frame, vec2 scale)
  {
    if (!frame) return;
    snapshot("Frame Scale");
    frame->scale = scale;
    change(Document::FRAMES);
  }

  void Document::frame_rotation_set(anm2::Frame* frame, float rotation)
  {
    if (!frame) return;
    snapshot("Frame Rotation");
    frame->rotation = rotation;
    change(Document::FRAMES);
  }

  void Document::frame_delay_set(anm2::Frame* frame, int delay)
  {
    if (!frame) return;
    snapshot("Frame Delay");
    frame->delay = delay;
    change(Document::FRAMES);
  }

  void Document::frame_tint_set(anm2::Frame* frame, vec4 tint)
  {
    if (!frame) return;
    snapshot("Frame Tint");
    frame->tint = tint;
    change(Document::FRAMES);
  }

  void Document::frame_color_offset_set(anm2::Frame* frame, vec3 colorOffset)
  {
    if (!frame) return;
    snapshot("Frame Color Offset");
    frame->colorOffset = colorOffset;
    change(Document::FRAMES);
  }

  void Document::frame_is_visible_set(anm2::Frame* frame, bool isVisible)
  {
    if (!frame) return;
    snapshot("Frame Visibility");
    frame->isVisible = isVisible;
    change(Document::FRAMES);
  }

  void Document::frame_is_interpolated_set(anm2::Frame* frame, bool isInterpolated)
  {
    if (!frame) return;
    snapshot("Frame Interpolation");
    frame->isInterpolated = isInterpolated;
    change(Document::FRAMES);
  }

  void Document::frame_flip_x(anm2::Frame* frame)
  {
    if (!frame) return;
    snapshot("Frame Flip X");
    frame->scale.x = -frame->scale.x;
    change(Document::FRAMES);
  }

  void Document::frame_flip_y(anm2::Frame* frame)
  {
    if (!frame) return;
    snapshot("Frame Flip Y");
    frame->scale.y = -frame->scale.y;
    change(Document::FRAMES);
  }

  void Document::frame_shorten()
  {
    auto frame = frame_get();
    if (!frame) return;
    snapshot("Shorten Frame");
    frame->shorten();
    change(Document::FRAMES);
  }

  void Document::frame_extend()
  {
    auto frame = frame_get();
    if (!frame) return;
    snapshot("Extend Frame");
    frame->extend();
    change(Document::FRAMES);
  }

  void Document::frames_change(anm2::FrameChange& frameChange, anm2::ChangeType type, bool isFromSelectedFrame,
                               int numberFrames)
  {
    auto item = item_get();
    if (!item) return;
    snapshot("Change All Frame Properties");
    item->frames_change(frameChange, type, isFromSelectedFrame && frame_get() ? reference.frameIndex : 0,
                        isFromSelectedFrame ? numberFrames : -1);
    change(Document::FRAMES);
  }

  void Document::frames_deserialize(const std::string& string)
  {
    if (auto item = item_get())
    {
      snapshot("Paste Frame(s)");
      std::set<int> indices{};
      std::string errorString{};
      auto start = reference.frameIndex + 1;
      if (item->frames_deserialize(string, reference.itemType, start, indices, &errorString))
        change(Document::FRAMES);
      else
        toasts.error(std::format("Failed to deserialize frame(s): {}", errorString));
    }
    else
      toasts.error(std::format("Failed to deserialize frame(s): select an item first!"));
  }

  anm2::Item* Document::item_get()
  {
    return anm2.item_get(reference);
  }

  anm2::Spritesheet* Document::spritesheet_get()
  {
    return anm2.spritesheet_get(referenceSpritesheet);
  }

  void Document::spritesheet_add(const std::string& path)
  {
    int id{};
    snapshot("Add Spritesheet");
    if (anm2.spritesheet_add(directory_get(), path, id))
    {
      spritesheetMultiSelect = {id};
      toasts.info(std::format("Initialized spritesheet #{}: {}", id, path));
      change(Document::SPRITESHEETS);
    }
    else
      toasts.error(std::format("Failed to initialize spritesheet: {}", path));
  }

  void Document::spritesheets_deserialize(const std::string& string, merge::Type type)
  {
    snapshot("Paste Spritesheet(s)");
    std::string errorString{};
    if (anm2.content.spritesheets_deserialize(string, directory_get(), type, &errorString))
      change(Document::SPRITESHEETS);
    else
      toasts.error(std::format("Failed to deserialize spritesheet(s): {}", errorString));
  }

  void Document::layers_deserialize(const std::string& string, merge::Type type)
  {
    snapshot("Paste Layer(s)");
    std::string errorString{};
    if (anm2.content.layers_deserialize(string, type, &errorString))
      change(Document::NULLS);
    else
      toasts.error(std::format("Failed to deserialize layer(s): {}", errorString));
  }

  void Document::layer_set(anm2::Layer& layer)
  {
    if (referenceLayer > -1)
    {
      snapshot("Set Layer");
      anm2.content.layers[referenceLayer] = layer;
      layersMultiSelect = {referenceLayer};
    }
    else
    {
      snapshot("Add Layer");
      auto id = map::next_id_get(anm2.content.layers);
      anm2.content.layers[id] = layer;
      layersMultiSelect = {id};
    }
    change(Document::LAYERS);
  }

  void Document::layers_remove_unused()
  {
    snapshot("Remove Unused Layers");
    for (auto& id : unusedLayerIDs)
      anm2.content.layers.erase(id);
    change(Document::LAYERS);
    unusedLayerIDs.clear();
  }

  void Document::null_set(anm2::Null& null)
  {
    if (referenceNull > -1)
    {
      snapshot("Set Null");
      anm2.content.nulls[referenceNull] = null;
      nullMultiSelect = {referenceNull};
    }
    else
    {
      snapshot("Add Null");
      auto id = map::next_id_get(anm2.content.nulls);
      anm2.content.nulls[id] = null;
      nullMultiSelect = {id};
    }
    change(Document::NULLS);
  }

  void Document::null_rect_toggle(anm2::Null& null)
  {
    snapshot("Null Rect");
    null.isShowRect = !null.isShowRect;
    change(Document::NULLS);
  }

  void Document::nulls_remove_unused()
  {
    snapshot("Remove Unused Nulls");
    for (auto& id : unusedNullIDs)
      anm2.content.nulls.erase(id);
    change(Document::NULLS);
    unusedNullIDs.clear();
  }

  void Document::nulls_deserialize(const std::string& string, merge::Type type)
  {
    snapshot("Paste Null(s)");
    std::string errorString{};
    if (anm2.content.nulls_deserialize(string, type, &errorString))
      change(Document::NULLS);
    else
      toasts.error(std::format("Failed to deserialize null(s): {}", errorString));
  }

  void Document::event_set(anm2::Event& event)
  {
    if (referenceEvent > -1)
    {
      snapshot("Set Event");
      anm2.content.events[referenceEvent] = event;
      eventMultiSelect = {referenceEvent};
    }
    else
    {
      snapshot("Add Event");
      auto id = map::next_id_get(anm2.content.events);
      anm2.content.events[id] = event;
      eventMultiSelect = {id};
    }
    change(Document::EVENTS);
  }

  void Document::events_remove_unused()
  {
    snapshot("Remove Unused Events");
    for (auto& id : unusedEventIDs)
      anm2.content.events.erase(id);
    change(Document::EVENTS);
    unusedEventIDs.clear();
  }

  void Document::events_deserialize(const std::string& string, merge::Type type)
  {
    snapshot("Paste Event(s)");
    std::string errorString{};
    if (anm2.content.events_deserialize(string, type, &errorString))
      change(Document::EVENTS);
    else
      toasts.error(std::format("Failed to deserialize event(s): {}", errorString));
  }

  void Document::sound_add(const std::string& path)
  {
    int id{};
    snapshot("Add Sound");
    if (anm2.sound_add(directory_get(), path, id))
    {
      soundMultiSelect = {id};
      toasts.info(std::format("Initialized sound #{}: {}", id, path));
      change(Document::SOUNDS);
    }
    else
      toasts.error(std::format("Failed to initialize sound: {}", path));
  }

  void Document::sounds_remove_unused()
  {
    snapshot("Remove Unused Sounds");
    for (auto& id : unusedSoundIDs)
      anm2.content.sounds.erase(id);
    change(Document::LAYERS);
    unusedSoundIDs.clear();
  }

  void Document::sounds_deserialize(const std::string& string, merge::Type type)
  {
    snapshot("Paste Sound(s)");
    std::string errorString{};
    if (anm2.content.sounds_deserialize(string, directory_get(), type, &errorString))
      change(Document::EVENTS);
    else
      toasts.error(std::format("Failed to deserialize event(s): {}", errorString));
  }

  void Document::item_add(anm2::Type type, int id, std::string& name, locale::Type locale, int spritesheetID)
  {
    snapshot("Add Item");

    anm2::Reference addReference;

    if (type == anm2::LAYER)
      addReference =
          anm2.layer_add({reference.animationIndex, anm2::LAYER, id}, name, spritesheetID, (locale::Type)locale);
    else if (type == anm2::NULL_)
      addReference = anm2.null_add({reference.animationIndex, anm2::LAYER, id}, name, (locale::Type)locale);

    reference = addReference;

    change(Document::ITEMS);
  }

  void Document::item_remove(anm2::Animation* animation)
  {
    if (!animation) return;
    snapshot("Remove Item");
    animation->item_remove(reference.itemType, reference.itemID);
    reference = {reference.animationIndex};
    change(Document::ITEMS);
  }

  void Document::item_visible_toggle(anm2::Item* item)
  {
    if (!item) return;
    snapshot("Item Visibility");
    item->isVisible = !item->isVisible;
    change(Document::ITEMS);
  }

  anm2::Animation* Document::animation_get()
  {
    return anm2.animation_get(reference);
  }

  void Document::animation_set(int index)
  {
    snapshot("Select Animation");
    reference = {index};
    change(Document::ITEMS);
  }

  void Document::animation_add()
  {
    snapshot("Add Animation");
    anm2::Animation animation;
    if (anm2::Animation* referenceAnimation = animation_get())
    {
      for (auto [id, layerAnimation] : referenceAnimation->layerAnimations)
        animation.layerAnimations[id] = anm2::Item();
      animation.layerOrder = referenceAnimation->layerOrder;
      for (auto [id, nullAnimation] : referenceAnimation->nullAnimations)
        animation.nullAnimations[id] = anm2::Item();
    }
    animation.rootAnimation.frames.emplace_back(anm2::Frame());

    auto index =
        animationMultiSelect.empty() ? (int)anm2.animations.items.size() - 1 : *animationMultiSelect.rbegin() + 1;
    anm2.animations.items.insert(anm2.animations.items.begin() + index, animation);
    animationMultiSelect = {index};
    reference = {index};
    change(Document::ANIMATIONS);
  }

  void Document::animation_duplicate()
  {
    snapshot("Duplicate Animation(s)");
    auto duplicated = animationMultiSelect;
    auto duplicatedEnd = std::ranges::max(duplicated);
    for (auto& id : duplicated)
    {
      anm2.animations.items.insert(anm2.animations.items.begin() + duplicatedEnd, anm2.animations.items[id]);
      animationMultiSelect.insert(++duplicatedEnd);
      animationMultiSelect.erase(id);
    }
    change(Document::ANIMATIONS);
  }

  void Document::animations_move(std::vector<int>& indices, int index)
  {
    snapshot("Move Animation(s)");
    animationMultiSelect = vector::move_indices(anm2.animations.items, indices, index);
    change(Document::ANIMATIONS);
  }

  void Document::animations_remove()
  {
    snapshot("Remove Animation(s)");

    if (!animationMultiSelect.empty())
    {
      for (auto& i : animationMultiSelect | std::views::reverse)
        anm2.animations.items.erase(anm2.animations.items.begin() + i);
      animationMultiSelect.clear();
    }
    else if (hoveredAnimation > -1)
    {
      anm2.animations.items.erase(anm2.animations.items.begin() + hoveredAnimation);
      hoveredAnimation = -1;
    }

    change(Document::ANIMATIONS);
  }

  void Document::animation_default()
  {
    snapshot("Default Animation");
    anm2.animations.defaultAnimation = anm2.animations.items[*animationMultiSelect.begin()].name;
    change(Document::ANIMATIONS);
  }

  void Document::animations_deserialize(const std::string& string)
  {
    snapshot("Paste Animation(s)");
    auto& multiSelect = animationMultiSelect;
    auto start = multiSelect.empty() ? anm2.animations.items.size() : *multiSelect.rbegin() + 1;
    std::set<int> indices{};
    std::string errorString{};
    if (anm2.animations.animations_deserialize(string, start, indices, &errorString))
    {
      multiSelect = indices;
      change(Document::ANIMATIONS);
    }
    else
      toasts.error(std::format("Failed to deserialize animation(s): {}", errorString));
  }

  void Document::generate_animation_from_grid(ivec2 startPosition, ivec2 size, ivec2 pivot, int columns, int count,
                                              int delay)
  {
    snapshot("Generate Animation from Grid");

    anm2.generate_from_grid(reference, startPosition, size, pivot, columns, count, delay);

    if (auto animation = animation_get()) animation->frameNum = animation->length();

    change(Document::ALL);
  }

  void Document::animations_merge_quick()
  {
    snapshot("Merge Animation(s)");
    int merged{};
    if (animationMultiSelect.size() > 1)
      merged = anm2.animations.merge(*animationMultiSelect.begin(), animationMultiSelect);
    else if (animationMultiSelect.size() == 1 && *animationMultiSelect.begin() != (int)anm2.animations.items.size() - 1)
    {
      auto start = *animationMultiSelect.begin();
      auto next = *animationMultiSelect.begin() + 1;
      std::set<int> animationSet{};
      animationSet.insert(start);
      animationSet.insert(next);

      merged = anm2.animations.merge(start, animationSet);
    }
    else
      return;

    animationMultiSelect = {merged};
    reference = {merged};
    change(Document::ANIMATIONS);
  }

  void Document::animations_merge(merge::Type type, bool isDeleteAnimationsAfter)
  {
    snapshot("Merge Animations");
    auto merged = anm2.animations.merge(mergeTarget, animationMergeMultiSelect, type, isDeleteAnimationsAfter);
    animationMultiSelect = {merged};
    reference = {merged};
    change(Document::ANIMATIONS);
  }

  void Document::snapshot(const std::string& message)
  {
    snapshots.push(anm2, reference, message);
  }

  void Document::undo()
  {
    snapshots.undo(anm2, reference, message);
    toasts.info(std::format("Undo: {}", message));
    change(Document::ALL);
  }

  void Document::redo()
  {
    toasts.info(std::format("Redo: {}", message));
    snapshots.redo(anm2, reference, message);
    change(Document::ALL);
  }

  bool Document::is_able_to_undo()
  {
    return !snapshots.undoStack.is_empty();
  }

  bool Document::is_able_to_redo()
  {
    return !snapshots.redoStack.is_empty();
  }
}