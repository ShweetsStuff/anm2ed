#include "document.h"

#include "anm2.h"
#include "filesystem.h"
#include "toast.h"
#include "util.h"
#include <algorithm>
#include <ranges>

using namespace anm2ed::anm2;
using namespace anm2ed::filesystem;
using namespace anm2ed::toast;
using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed::document
{
  Document::Document(const std::string& path, bool isNew, std::string* errorString)
  {
    if (!path_is_exist(path)) return;

    if (isNew)
      anm2 = anm2::Anm2();
    else
    {
      anm2 = Anm2(path, errorString);
      if (errorString && !errorString->empty()) return;
    }

    this->path = path;
    clean();
    change(change::ALL);
  }

  bool Document::save(const std::string& path, std::string* errorString)
  {
    this->path = !path.empty() ? path : this->path.string();

    if (anm2.serialize(this->path, errorString))
    {
      clean();
      return true;
    }

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
  }

  void Document::change(change::Type type)
  {
    hash_set();

    auto layer_set = [&]() { unusedLayerIDs = anm2.layers_unused(); };
    auto null_set = [&]() { unusedNullIDs = anm2.nulls_unused(); };
    auto event_set = [&]() { unusedEventIDs = anm2.events_unused(); };
    auto spritesheet_set = [&]()
    {
      unusedSpritesheetIDs = anm2.spritesheets_unused();
      spritesheetNames = anm2.spritesheet_names_get();
      spritesheetNamesCstr.clear();
      for (auto& name : spritesheetNames)
        spritesheetNamesCstr.push_back(name.c_str());
    };

    switch (type)
    {
      case change::LAYERS:
        layer_set();
        break;
      case change::NULLS:
        null_set();
        break;
      case change::EVENTS:
        event_set();
        break;
      case change::SPRITESHEETS:
        spritesheet_set();
        break;
      case change::ALL:
        layer_set();
        null_set();
        event_set();
        spritesheet_set();
        break;
      default:
        break;
    }
  }

  bool Document::is_dirty()
  {
    return hash != saveHash;
  }

  std::string Document::directory_get()
  {
    return path.parent_path();
  }

  std::string Document::filename_get()
  {
    return path.filename().string();
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
    change(change::FRAMES);
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
    change(change::FRAMES);
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
      change(change::SPRITESHEETS);
    }
    else
      toasts.error(std::format("Failed to initialize spritesheet: {}", path));
  }

  void Document::spritesheets_deserialize(const std::string& string, merge::Type type)
  {
    snapshot("Paste Spritesheet(s)");
    std::string errorString{};
    if (anm2.content.spritesheets_deserialize(string, directory_get(), type, &errorString))
      change(change::SPRITESHEETS);
    else
      toasts.error(std::format("Failed to deserialize spritesheet(s): {}", errorString));
  }

  void Document::layers_deserialize(const std::string& string, merge::Type type)
  {
    snapshot("Paste Layer(s)");
    std::string errorString{};
    if (anm2.content.layers_deserialize(string, type, &errorString))
      change(change::NULLS);
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
    change(change::LAYERS);
  }

  void Document::layers_remove_unused()
  {
    snapshot("Remove Unused Layers");
    for (auto& id : unusedLayerIDs)
      anm2.content.layers.erase(id);
    change(change::LAYERS);
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
    change(change::NULLS);
  }

  void Document::null_rect_toggle(anm2::Null& null)
  {
    snapshot("Null Rect");
    null.isShowRect = !null.isShowRect;
    change(change::NULLS);
  }

  void Document::nulls_remove_unused()
  {
    snapshot("Remove Unused Nulls");
    for (auto& id : unusedNullIDs)
      anm2.content.nulls.erase(id);
    change(change::NULLS);
    unusedNullIDs.clear();
  }

  void Document::nulls_deserialize(const std::string& string, merge::Type type)
  {
    snapshot("Paste Null(s)");
    std::string errorString{};
    if (anm2.content.nulls_deserialize(string, type, &errorString))
      change(change::NULLS);
    else
      toasts.error(std::format("Failed to deserialize null(s): {}", errorString));
  }

  void Document::event_add()
  {
    snapshot("Add Event");
    int id{};
    anm2.event_add(id);
    eventMultiSelect = {id};
    change(change::EVENTS);
  }

  void Document::events_remove_unused()
  {
    snapshot("Remove Unused Events");
    for (auto& id : unusedEventIDs)
      anm2.content.events.erase(id);
    change(change::EVENTS);
    unusedEventIDs.clear();
  }

  void Document::events_deserialize(const std::string& string, merge::Type type)
  {
    snapshot("Paste Event(s)");
    std::string errorString{};
    if (anm2.content.events_deserialize(string, type, &errorString))
      change(change::EVENTS);
    else
      toasts.error(std::format("Failed to deserialize event(s): {}", errorString));
  }

  void Document::item_visible_toggle(anm2::Item* item)
  {
    if (!item) return;

    snapshot("Item Visible");
    item->isVisible = !item->isVisible;

    change(change::ITEMS);
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

    change(change::ITEMS);
  }

  void Document::item_remove(anm2::Animation* animation)
  {
    snapshot("Remove Item");

    if (!animation) return;

    animation->item_remove(reference.itemType, reference.itemID);
    reference = {reference.animationIndex};
    change(change::ITEMS);
  }

  anm2::Animation* Document::animation_get()
  {
    return anm2.animation_get(reference);
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
    change(change::ANIMATIONS);
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
    change(change::ANIMATIONS);
  }

  void Document::animations_move(std::vector<int>& indices, int index)
  {
    snapshot("Move Animation(s)");
    animationMultiSelect = vector::move_indices(anm2.animations.items, indices, index);
    change(change::ANIMATIONS);
  }

  void Document::animation_remove()
  {
    snapshot("Remove Animation(s)");
    for (auto& i : animationMultiSelect | std::views::reverse)
      anm2.animations.items.erase(anm2.animations.items.begin() + i);
    animationMultiSelect.clear();

    change(change::ANIMATIONS);
  }

  void Document::animation_default()
  {
    snapshot("Default Animation");
    anm2.animations.defaultAnimation = anm2.animations.items[*animationMultiSelect.begin()].name;
    change(change::ANIMATIONS);
  }

  void Document::animations_deserialize(const std::string& string)
  {
    snapshot("Paste Animations");
    auto& multiSelect = animationMultiSelect;
    auto start = multiSelect.empty() ? anm2.animations.items.size() : *multiSelect.rbegin() + 1;
    std::set<int> indices{};
    std::string errorString{};
    if (anm2.animations.animations_deserialize(string, start, indices, &errorString))
    {
      multiSelect = indices;
      change(change::ANIMATIONS);
    }
    else
      toasts.error(std::format("Failed to deserialize animation(s): {}", errorString));
  }

  void Document::animations_merge_quick()
  {
    snapshot("Merge Animations");
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
    change(change::ANIMATIONS);
  }

  void Document::animations_merge(merge::Type type, bool isDeleteAnimationsAfter)
  {
    snapshot("Merge Animations");
    auto merged = anm2.animations.merge(mergeTarget, animationMergeMultiSelect, type, isDeleteAnimationsAfter);
    animationMultiSelect = {merged};
    reference = {merged};
    change(change::ANIMATIONS);
  }

  void Document::snapshot(const std::string& message)
  {
    snapshots.push(anm2, reference, message);
  }

  void Document::undo()
  {
    snapshots.undo(anm2, reference, message);
    toasts.info(std::format("Undo: {}", message));
    change(change::ALL);
  }

  void Document::redo()
  {
    toasts.info(std::format("Redo: {}", message));
    snapshots.redo(anm2, reference, message);
    change(change::ALL);
  }

  bool Document::is_undo()
  {
    return !snapshots.undoStack.is_empty();
  }

  bool Document::is_redo()
  {
    return !snapshots.redoStack.is_empty();
  }
}