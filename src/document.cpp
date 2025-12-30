#include "document.h"

#include <utility>

#include <format>

#include "log.h"
#include "path_.h"
#include "strings.h"
#include "toast.h"

using namespace anm2ed::anm2;
using namespace anm2ed::imgui;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed
{
  Document::Document(Anm2& anm2, const std::filesystem::path& path)
  {
    this->anm2 = std::move(anm2);
    this->path = path;
    isValid = this->anm2.isValid;
    if (!isValid) return;
    clean();
    change(Document::ALL);
  }

  Document::Document(const std::filesystem::path& path, bool isNew, std::string* errorString)
  {
    if (isNew)
    {
      anm2 = anm2::Anm2();
      if (!save(path, errorString))
      {
        isValid = false;
        this->path.clear();
        return;
      }
    }
    else
    {
      anm2 = Anm2(path, errorString);
      if (!anm2.isValid)
      {
        isValid = false;
        this->path.clear();
        return;
      }
    }

    this->path = path;
    isValid = anm2.isValid;
    clean();
    change(Document::ALL);
  }

  Document::Document(Document&& other) noexcept
      : path(std::move(other.path)), snapshots(std::move(other.snapshots)), current(snapshots.current),
        anm2(current.anm2), reference(current.reference), playback(current.playback), animation(current.animation),
        merge(current.merge), event(current.event), layer(current.layer), null(current.null), sound(current.sound),
        spritesheet(current.spritesheet), frames(current.frames), message(current.message),
        previewZoom(other.previewZoom), previewPan(other.previewPan), editorPan(other.editorPan),
        editorZoom(other.editorZoom), overlayIndex(other.overlayIndex), hash(other.hash), saveHash(other.saveHash),
        autosaveHash(other.autosaveHash), lastAutosaveTime(other.lastAutosaveTime), isValid(other.isValid),
        isOpen(other.isOpen), isForceDirty(other.isForceDirty), isAnimationPreviewSet(other.isAnimationPreviewSet),
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
      isValid = other.isValid;
      isOpen = other.isOpen;
      isForceDirty = other.isForceDirty;
      isAnimationPreviewSet = other.isAnimationPreviewSet;
      isSpritesheetEditorSet = other.isSpritesheetEditorSet;
    }
    return *this;
  }

  bool Document::save(const std::filesystem::path& path, std::string* errorString)
  {
    this->path = !path.empty() ? path : this->path;

    auto absolutePath = this->path;
    auto absolutePathUtf8 = path::to_utf8(absolutePath);
    if (anm2.serialize(absolutePath, errorString))
    {
      toasts.push(std::vformat(localize.get(TOAST_SAVE_DOCUMENT), std::make_format_args(absolutePathUtf8)));
      logger.info(
          std::vformat(localize.get(TOAST_SAVE_DOCUMENT, anm2ed::ENGLISH), std::make_format_args(absolutePathUtf8)));
      clean();
      return true;
    }
    else if (errorString)
    {
      toasts.push(std::vformat(localize.get(TOAST_SAVE_DOCUMENT_FAILED),
                               std::make_format_args(absolutePathUtf8, *errorString)));
      logger.error(std::vformat(localize.get(TOAST_SAVE_DOCUMENT_FAILED, anm2ed::ENGLISH),
                                std::make_format_args(absolutePathUtf8, *errorString)));
    }

    return false;
  }

  std::filesystem::path Document::autosave_path_get()
  {
    auto fileNameUtf8 = path::to_utf8(filename_get());
    auto autosaveNameUtf8 = "." + fileNameUtf8 + ".autosave";
    return directory_get() / path::from_utf8(autosaveNameUtf8);
  }

  std::filesystem::path Document::path_from_autosave_get(std::filesystem::path& path)
  {
    auto fileName = path::to_utf8(path.filename());
    if (!fileName.empty() && fileName.front() == '.') fileName.erase(fileName.begin());

    auto restorePath = path.parent_path() / std::filesystem::path(std::u8string(fileName.begin(), fileName.end()));
    restorePath.replace_extension("");

    return restorePath;
  }

  bool Document::autosave(std::string* errorString)
  {
    auto autosavePath = autosave_path_get();
    auto autosavePathUtf8 = path::to_utf8(autosavePath);
    if (anm2.serialize(autosavePath, errorString))
    {
      autosaveHash = hash;
      lastAutosaveTime = 0.0f;
      toasts.push(localize.get(TOAST_AUTOSAVING));
      logger.info(localize.get(TOAST_AUTOSAVING, anm2ed::ENGLISH));
      logger.info(std::format("Autosaved document to: {}", autosavePathUtf8));
      return true;
    }
    else if (errorString)
    {
      toasts.push(
          std::vformat(localize.get(TOAST_AUTOSAVE_FAILED), std::make_format_args(autosavePathUtf8, *errorString)));
      logger.error(std::vformat(localize.get(TOAST_AUTOSAVE_FAILED, anm2ed::ENGLISH),
                                std::make_format_args(autosavePathUtf8, *errorString)));
    }

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
      case FRAMES:
        events_set();
        sounds_set();
        break;
      case ITEMS:
        layers_set();
        nulls_set();
        spritesheets_set();
        break;
      case ANIMATIONS:
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

  bool Document::is_dirty() const { return hash != saveHash; }
  bool Document::is_autosave_dirty() const { return hash != autosaveHash; }
  std::filesystem::path Document::directory_get() const { return path.parent_path(); }
  std::filesystem::path Document::filename_get() const { return path.filename(); }
  bool Document::is_valid() const { return isValid && !path.empty(); }

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

  void Document::spritesheet_add(const std::filesystem::path& path)
  {
    auto add = [&]()
    {
      int id{};
      auto pathCopy = path;
      if (anm2.spritesheet_add(directory_get(), path, id))
      {
        anm2::Spritesheet& spritesheet = anm2.content.spritesheets[id];
        auto pathString = path::to_utf8(spritesheet.path);
        this->spritesheet.selection = {id};
        this->spritesheet.reference = id;
        toasts.push(std::vformat(localize.get(TOAST_SPRITESHEET_INITIALIZED), std::make_format_args(id, pathString)));
        logger.info(std::vformat(localize.get(TOAST_SPRITESHEET_INITIALIZED, anm2ed::ENGLISH),
                                 std::make_format_args(id, pathString)));
      }
      else
      {
        auto pathUtf8 = path::to_utf8(pathCopy);
        toasts.push(std::vformat(localize.get(TOAST_SPRITESHEET_INIT_FAILED), std::make_format_args(pathUtf8)));
        logger.error(std::vformat(localize.get(TOAST_SPRITESHEET_INIT_FAILED, anm2ed::ENGLISH),
                                  std::make_format_args(pathUtf8)));
      }
    };

    DOCUMENT_EDIT_PTR(this, localize.get(EDIT_ADD_SPRITESHEET), Document::SPRITESHEETS, add());
  }

  void Document::sound_add(const std::filesystem::path& path)
  {
    auto add = [&]()
    {
      int id{};
      auto pathCopy = path;
      if (anm2.sound_add(directory_get(), path, id))
      {
        auto& soundInfo = anm2.content.sounds[id];
        auto soundPath = path::to_utf8(soundInfo.path);
        sound.selection = {id};
        sound.reference = id;
        toasts.push(std::vformat(localize.get(TOAST_SOUND_INITIALIZED), std::make_format_args(id, soundPath)));
        logger.info(
            std::vformat(localize.get(TOAST_SOUND_INITIALIZED, anm2ed::ENGLISH), std::make_format_args(id, soundPath)));
      }
      else
      {
        auto pathUtf8 = path::to_utf8(pathCopy);
        toasts.push(std::vformat(localize.get(TOAST_SOUND_INITIALIZE_FAILED), std::make_format_args(pathUtf8)));
        logger.error(std::vformat(localize.get(TOAST_SOUND_INITIALIZE_FAILED, anm2ed::ENGLISH),
                                  std::make_format_args(pathUtf8)));
      }
    };

    DOCUMENT_EDIT_PTR(this, localize.get(EDIT_ADD_SOUND), Document::SOUNDS, add());
  }

  void Document::snapshot(const std::string& message)
  {
    this->message = message;
    snapshots.push(current);
  }

  void Document::undo()
  {
    snapshots.undo();
    toasts.push(std::vformat(localize.get(TOAST_UNDO), std::make_format_args(message)));
    logger.info(std::vformat(localize.get(TOAST_UNDO, anm2ed::ENGLISH), std::make_format_args(message)));
    change(Document::ALL);
  }

  void Document::redo()
  {
    snapshots.redo();
    toasts.push(std::vformat(localize.get(TOAST_REDO), std::make_format_args(message)));
    logger.info(std::vformat(localize.get(TOAST_REDO, anm2ed::ENGLISH), std::make_format_args(message)));
    change(Document::ALL);
  }

  bool Document::is_able_to_undo() { return !snapshots.undoStack.is_empty(); }
  bool Document::is_able_to_redo() { return !snapshots.redoStack.is_empty(); }
}
