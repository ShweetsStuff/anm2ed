#pragma once

#include <filesystem>
#include <set>

#include "anm2/anm2.h"
#include "imgui_.h"
#include "playback.h"
#include "snapshots.h"
#include "types.h"

#include <glm/glm.hpp>

namespace anm2ed
{
  class Document
  {
  public:
    enum ChangeType
    {
      LAYERS,
      NULLS,
      SPRITESHEETS,
      EVENTS,
      ANIMATIONS,
      ITEMS,
      FRAMES,
      SOUNDS,
      ALL,
      COUNT
    };

    std::filesystem::path path{};
    anm2::Anm2 anm2{};
    std::string message{};
    Playback playback{};
    Snapshots snapshots{};

    float previewZoom{200};
    glm::vec2 previewPan{};
    glm::vec2 editorPan{};
    float editorZoom{200};

    int overlayIndex{};

    anm2::Reference reference{};
    int hoveredAnimation{-1};
    int mergeTarget{-1};
    imgui::MultiSelectStorage animationMultiSelect;
    imgui::MultiSelectStorage animationMergeMultiSelect;
    std::vector<const char*> animationNamesCStr{};
    std::vector<std::string> animationNames{};

    anm2::Reference hoveredFrame{anm2::REFERENCE_DEFAULT};

    int referenceSpritesheet{-1};
    int hoveredSpritesheet{-1};
    std::set<int> unusedSpritesheetIDs{};
    std::vector<std::string> spritesheetNames{};
    std::vector<const char*> spritesheetNamesCStr{};
    imgui::MultiSelectStorage spritesheetMultiSelect;

    int referenceLayer{-1};
    int hoveredLayer{-1};
    std::set<int> unusedLayerIDs{};
    imgui::MultiSelectStorage layersMultiSelect;

    int referenceNull{-1};
    int hoveredNull{-1};
    std::set<int> unusedNullIDs{};
    imgui::MultiSelectStorage nullMultiSelect;

    int referenceEvent{-1};
    int hoveredEvent{-1};
    std::set<int> unusedEventIDs{};
    imgui::MultiSelectStorage eventMultiSelect;
    std::vector<const char*> eventNamesCStr{};
    std::vector<std::string> eventNames{};

    int referenceSound{-1};
    int hoveredSound{-1};
    std::set<int> unusedSoundIDs{};
    imgui::MultiSelectStorage soundMultiSelect;
    std::vector<const char*> soundNamesCStr{};
    std::vector<std::string> soundNames{};

    uint64_t hash{};
    uint64_t saveHash{};
    uint64_t autosaveHash{};
    double lastAutosaveTime{};
    bool isOpen{true};
    bool isForceDirty{false};

    Document(const std::string&, bool = false, std::string* = nullptr);
    bool save(const std::string& = {}, std::string* = nullptr);
    bool autosave(const std::string&, std::string* = nullptr);
    void hash_set();
    void clean();
    void on_change();
    void change(ChangeType);
    bool is_dirty();
    bool is_autosave_dirty();
    std::filesystem::path directory_get();
    std::filesystem::path filename_get();
    bool is_valid();

    anm2::Frame* frame_get();
    void frames_add(anm2::Item* item);
    void frames_delete(anm2::Item* item);
    void frames_bake(int, bool, bool);
    void frame_crop_set(anm2::Frame*, glm::vec2);
    void frame_size_set(anm2::Frame*, glm::vec2);
    void frame_position_set(anm2::Frame*, glm::vec2);
    void frame_pivot_set(anm2::Frame*, glm::vec2);
    void frame_scale_set(anm2::Frame*, glm::vec2);
    void frame_rotation_set(anm2::Frame*, float);
    void frame_delay_set(anm2::Frame*, int);
    void frame_tint_set(anm2::Frame*, glm::vec4);
    void frame_color_offset_set(anm2::Frame*, glm::vec3);
    void frame_is_visible_set(anm2::Frame*, bool);
    void frame_is_interpolated_set(anm2::Frame*, bool);
    void frame_flip_x(anm2::Frame* frame);
    void frame_flip_y(anm2::Frame* frame);
    void frame_shorten();
    void frame_extend();
    void frames_change(anm2::FrameChange&, anm2::ChangeType, bool, int = -1);
    void frames_deserialize(const std::string&);

    anm2::Item* item_get();
    void item_add(anm2::Type, int, std::string&, types::locale::Type, int);
    void item_remove(anm2::Animation*);
    void item_visible_toggle(anm2::Item*);

    anm2::Spritesheet* spritesheet_get();
    void spritesheet_add(const std::string&);
    void spritesheets_deserialize(const std::string&, types::merge::Type);

    void layer_set(anm2::Layer&);
    void layers_remove_unused();
    void layers_deserialize(const std::string&, types::merge::Type);

    void null_set(anm2::Null&);
    void null_rect_toggle(anm2::Null&);
    void nulls_remove_unused();
    void nulls_deserialize(const std::string&, types::merge::Type);

    void event_set(anm2::Event&);
    void events_remove_unused();
    void events_deserialize(const std::string&, types::merge::Type);

    void sound_add(const std::string& path);
    void sounds_remove_unused();
    void sounds_deserialize(const std::string& string, types::merge::Type);

    void animation_add();
    void animation_set(int);
    void animation_duplicate();
    void animation_default();
    void animations_remove();
    void animations_move(std::vector<int>&, int);
    void animations_merge(types::merge::Type, bool);
    void animations_merge_quick();
    anm2::Animation* animation_get();
    void animations_deserialize(const std::string& string);

    void generate_animation_from_grid(glm::ivec2, glm::ivec2, glm::ivec2, int, int, int);

    void snapshot(const std::string& message);
    void undo();
    void redo();

    bool is_able_to_undo();
    bool is_able_to_redo();
  };

#define DOCUMENT_SNAPSHOT(document, message) document.snapshot(message);
#define DOCUMENT_CHANGE(document, changeType) document.change(changeType);

#define DOCUMENT_EDIT(document, message, changeType, body)                                                             \
  {                                                                                                                    \
                                                                                                                       \
    DOCUMENT_SNAPSHOT(document, message)                                                                               \
    body;                                                                                                              \
    DOCUMENT_CHANGE(document, changeType)                                                                              \
  }

}
