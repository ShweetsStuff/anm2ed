#pragma once

#include "anm2.h"
#include "document.h"
#include "document_manager.h"
#include "playback.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::timeline
{
  class Timeline
  {
    bool isDragging{};
    bool isWindowHovered{};
    bool isHorizontalScroll{};
    glm::vec2 scroll{};
    ImDrawList* pickerLineDrawList{};
    ImGuiStyle style{};

    void item_child(anm2::Anm2& anm2, anm2::Reference& reference, anm2::Animation* animation,
                    settings::Settings& settings, resources::Resources& resources, anm2::Type type, int id, int& index);
    void items_child(anm2::Anm2& anm2, anm2::Reference& reference, anm2::Animation* animation,
                     settings::Settings& settings, resources::Resources& resources);
    void frame_child(document::Document& document, anm2::Animation* animation, settings::Settings& settings,
                     resources::Resources& resources, playback::Playback& playback, anm2::Type type, int id, int& index,
                     float width);
    void frames_child(document::Document& document, anm2::Animation* animation, settings::Settings& settings,
                      resources::Resources& resources, playback::Playback& playback);

  public:
    void update(document_manager::DocumentManager& manager, settings::Settings& settings,
                resources::Resources& resources, playback::Playback& playback);
  };
}