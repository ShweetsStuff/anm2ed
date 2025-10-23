#pragma once

#include <filesystem>
#include <set>

#include "anm2.h"
#include "types.h"

#include <glm/glm.hpp>

namespace anm2ed::document
{
  class Document
  {
  public:
    std::filesystem::path path{};
    anm2::Anm2 anm2{};
    anm2::Reference reference{};

    float previewZoom{200};
    glm::vec2 previewPan{};
    glm::vec2 editorPan{};
    float editorZoom{200};
    int overlayIndex{};

    int referenceSpritesheet{-1};
    int referenceLayer{-1};

    std::set<int> selectedEvents{};
    std::set<int> selectedLayers{};
    std::set<int> selectedNulls{};
    std::set<int> selectedAnimations{};
    std::set<int> selectedSpritesheets{};

    std::vector<std::string> spritesheetNames{};
    std::vector<const char*> spritesheetNamesCstr{};

    uint64_t hash{};
    uint64_t saveHash{};
    bool isJustChanged[types::change::COUNT]{};
    bool isOpen{true};

    Document();

    Document(const std::string& path, bool isNew = false, std::string* errorString = nullptr);
    bool save(const std::string& path = {}, std::string* errorString = nullptr);
    void hash_set();
    void clean();
    void on_change();
    void change(types::change::Type type);
    bool is_just_changed(types::change::Type type);
    bool is_dirty();
    void update();
    std::string directory_get();
    std::string filename_get();
    anm2::Animation* animation_get();
    anm2::Frame* frame_get();
    anm2::Item* item_get();
    anm2::Spritesheet* spritesheet_get();
    bool is_valid();
  };
};