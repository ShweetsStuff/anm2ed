#pragma once

#include <filesystem>

#include "snapshots.h"

#include <glm/glm.hpp>

namespace anm2ed
{

  class Document
  {
  public:
    enum ChangeType
    {
      INFO,
      LAYERS,
      NULLS,
      SPRITESHEETS,
      TEXTURES,
      EVENTS,
      ANIMATIONS,
      ITEMS,
      FRAMES,
      SOUNDS,
      ALL,
      COUNT
    };

    std::filesystem::path path{};

    Snapshots snapshots{};
    Snapshot& current = snapshots.current;

    anm2::Anm2& anm2 = current.anm2;
    anm2::Reference& reference = current.reference;
    float& frameTime = current.frameTime;
    Playback& playback = current.playback;
    Storage& animation = current.animation;
    Storage& merge = current.merge;
    Storage& event = current.event;
    Storage& layer = current.layer;
    Storage& null = current.null;
    Storage& sound = current.sound;
    Storage& spritesheet = current.spritesheet;
    Storage& items = current.items;
    Storage& frames = current.frames;
    std::string& message = current.message;

    float previewZoom{200};
    glm::vec2 previewPan{};
    glm::vec2 editorPan{};
    float editorZoom{200};
    int overlayIndex{-1};

    uint64_t hash{};
    uint64_t saveHash{};
    uint64_t autosaveHash{};
    double lastAutosaveTime{};
    bool isOpen{true};
    bool isForceDirty{false};
    bool isAnimationPreviewSet{false};
    bool isSpritesheetEditorSet{false};

    Document(anm2::Anm2& anm2, const std::string&);
    Document(const std::string&, bool = false, std::string* = nullptr);
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;
    Document(Document&&) noexcept;
    Document& operator=(Document&&) noexcept;
    bool save(const std::string& = {}, std::string* = nullptr);
    void hash_set();
    void clean();
    void change(ChangeType);
    bool is_dirty() const;
    bool is_autosave_dirty() const;
    std::filesystem::path directory_get() const;
    std::filesystem::path filename_get() const;
    bool is_valid() const;

    anm2::Frame* frame_get();
    anm2::Item* item_get();
    anm2::Spritesheet* spritesheet_get();
    anm2::Animation* animation_get();

    void spritesheet_add(const std::string&);
    void sound_add(const std::string&);

    bool autosave(std::string* = nullptr);
    std::filesystem::path autosave_path_get();
    std::filesystem::path path_from_autosave_get(std::filesystem::path&);

    void snapshot(const std::string& message);
    void undo();
    void redo();
    bool is_able_to_undo();
    bool is_able_to_redo();
  };

#define DOCUMENT_EDIT(document, message, changeType, body)                                                             \
  {                                                                                                                    \
    document.snapshot(message);                                                                                        \
    body;                                                                                                              \
    document.change(changeType);                                                                                       \
  }

#define DOCUMENT_EDIT_PTR(document, message, changeType, body)                                                         \
  {                                                                                                                    \
    document->snapshot(message);                                                                                       \
    body;                                                                                                              \
    document->change(changeType);                                                                                      \
  }

}
