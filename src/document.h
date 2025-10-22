#pragma once

#include "anm2.h"
#include <filesystem>
#include <set>

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

    std::set<int> selectedEvents{};
    std::set<int> selectedLayers{};
    std::set<int> selectedNulls{};
    std::set<int> selectedAnimations{};
    std::set<int> selectedSpritesheets{};

    uint64_t hash{};
    uint64_t saveHash{};
    double lastHashTime{};
    bool isOpen{true};

    Document();

    Document(const std::string& path, bool isNew = false, std::string* errorString = nullptr);
    bool save(const std::string& path = {}, std::string* errorString = nullptr);
    void hash_set();
    void hash_time(double time, double interval = 1.0);
    bool is_dirty();
    std::string directory_get();
    std::string filename_get();
    anm2::Animation* animation_get();
    anm2::Frame* frame_get();
    anm2::Spritesheet* spritesheet_get();
    bool is_valid();
  };
};