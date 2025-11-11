#include "document.h"

#include <utility>

#include "filesystem_.h"
#include "log.h"
#include "toast.h"

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

  Document::Document(Document&& other) noexcept
      : path(std::move(other.path)), snapshots(std::move(other.snapshots)), current(snapshots.current),
        anm2(current.anm2), reference(current.reference), playback(current.playback), animation(current.animation),
        merge(current.merge), event(current.event), layer(current.layer), null(current.null), sound(current.sound),
        spritesheet(current.spritesheet), frames(current.frames), message(current.message),
        previewZoom(other.previewZoom), previewPan(other.previewPan), editorPan(other.editorPan),
        editorZoom(other.editorZoom), overlayIndex(other.overlayIndex), saveHash(other.saveHash),
        autosaveHash(other.autosaveHash), lastAutosaveTime(other.lastAutosaveTime), isOpen(other.isOpen),
        isForceDirty(other.isForceDirty), isAnimationPreviewSet(other.isAnimationPreviewSet),
        isSpritesheetEditorSet(other.isSpritesheetEditorSet)
  {
  }

  Document& Document::operator=(Document&& other) noexcept
  {
    if (this != &other)
    {
      path = std::move(other.path);
      snapshots = std::move(other.snapshots);
      previewZoom = other.previewZoom;
      previewPan = other.previewPan;
      editorPan = other.editorPan;
      editorZoom = other.editorZoom;
      overlayIndex = other.overlayIndex;
      hash = other.hash;
      saveHash = other.saveHash;
      autosaveHash = other.autosaveHash;
      lastAutosaveTime = other.lastAutosaveTime;
      isOpen = other.isOpen;
      isForceDirty = other.isForceDirty;
      isAnimationPreviewSet = other.isAnimationPreviewSet;
      isSpritesheetEditorSet = other.isSpritesheetEditorSet;
    }
    return *this;
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

  std::filesystem::path Document::autosave_path_get()
  {
    return directory_get() / std::string("." + filename_get().string() + ".autosave");
  }

  std::filesystem::path Document::path_from_autosave_get(std::filesystem::path& path)
  {
    auto fileName = path.filename().string();
    if (!fileName.empty() && fileName.front() == '.') fileName.erase(fileName.begin());

    auto restorePath = path.parent_path() / fileName;
    restorePath.replace_extension("");

    return path;
  }

  bool Document::autosave(std::string* errorString)
  {
    auto autosavePath = autosave_path_get();
    if (anm2.serialize(autosavePath, errorString))
    {
      autosaveHash = hash;
      lastAutosaveTime = 0.0f;
      toasts.info("Autosaving...");
      logger.info(std::format("Autosaved document to: {}", autosavePath.string()));
      return true;
    }
    else if (errorString)
      toasts.warning(std::format("Could not autosave document to: {} ({})", autosavePath.string(), *errorString));

    return false;
  }

  void Document::hash_set() { hash = anm2.hash(); }

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

    auto layers_set = [&]() { layer.unused = anm2.layers_unused(); };
    auto nulls_set = [&]() { null.unused = anm2.nulls_unused(); };
    auto events_set = [&]()
    {
      event.unused = anm2.events_unused();
      event.labels_set(anm2.event_labels_get());
    };

    auto animations_set = [&]() { animation.labels_set(anm2.animation_labels_get()); };

    auto spritesheets_set = [&]()
    {
      spritesheet.unused = anm2.spritesheets_unused();
      spritesheet.labels_set(anm2.spritesheet_labels_get());
    };

    auto sounds_set = [&]()
    {
      sound.unused = anm2.sounds_unused();
      sound.labels_set(anm2.sound_labels_get());

      for (auto& animation : anm2.animations.items)
        for (auto& trigger : animation.triggers.frames)
          if (!anm2.content.sounds.contains(trigger.soundID)) trigger.soundID = -1;
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
      case SPRITESHEETS:
        spritesheets_set();
        break;
      case SOUNDS:
        sounds_set();
        break;
      case ANIMATIONS:
        animations_set();
        break;
      case ALL:
        layers_set();
        nulls_set();
        events_set();
        spritesheets_set();
        animations_set();
        sounds_set();
        break;
      default:
        break;
    }
  }

  bool Document::is_dirty() { return hash != saveHash; }
  bool Document::is_autosave_dirty() { return hash != autosaveHash; }
  std::filesystem::path Document::directory_get() { return path.parent_path(); }
  std::filesystem::path Document::filename_get() { return path.filename(); }
  bool Document::is_valid() { return !path.empty(); }

  anm2::Frame* Document::frame_get()
  {
    return anm2.frame_get(reference.animationIndex, reference.itemType, reference.frameIndex, reference.itemID);
  }

  anm2::Item* Document::item_get()
  {
    return anm2.item_get(reference.animationIndex, reference.itemType, reference.itemID);
  }
  anm2::Animation* Document::animation_get() { return anm2.animation_get(reference.animationIndex); }
  anm2::Spritesheet* Document::spritesheet_get() { return anm2.spritesheet_get(spritesheet.reference); }

  void Document::spritesheet_add(const std::string& path)
  {
    auto add = [&]()
    {
      int id{};
      if (anm2.spritesheet_add(directory_get(), path, id))
      {
        anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
        this->spritesheet.selection = {id};
        toasts.info(std::format("Initialized spritesheet #{}: {}", id, spritesheet.path.string()));
      }
      else
        toasts.error(std::format("Failed to initialize spritesheet: {}", path));
    };

    DOCUMENT_EDIT_PTR(this, "Add Spritesheet", Document::SPRITESHEETS, add());
  }

  void Document::snapshot(const std::string& message)
  {
    this->message = message;
    snapshots.push(current);
  }

  void Document::undo()
  {
    snapshots.undo();
    toasts.info(std::format("Undo: {}", message));
    change(Document::ALL);
  }

  void Document::redo()
  {
    snapshots.redo();
    toasts.info(std::format("Redo: {}", message));
    change(Document::ALL);
  }

  bool Document::is_able_to_undo() { return !snapshots.undoStack.is_empty(); }
  bool Document::is_able_to_redo() { return !snapshots.redoStack.is_empty(); }
}
