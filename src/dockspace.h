#pragma once

#include "animation_preview.h"
#include "animations.h"
#include "documents.h"
#include "events.h"
#include "frame_properties.h"
#include "layers.h"
#include "nulls.h"
#include "onionskin.h"
#include "spritesheet_editor.h"
#include "spritesheets.h"
#include "taskbar.h"
#include "timeline.h"
#include "tools.h"

namespace anm2ed::dockspace
{
  class Dockspace
  {
    animation_preview::AnimationPreview animationPreview;
    animations::Animations animations;
    events::Events events;
    frame_properties::FrameProperties frameProperties;
    layers::Layers layers;
    nulls::Nulls nulls;
    onionskin::Onionskin onionskin;
    spritesheet_editor::SpritesheetEditor spritesheetEditor;
    spritesheets::Spritesheets spritesheets;
    timeline::Timeline timeline;
    tools::Tools tools;

  public:
    void update(taskbar::Taskbar& taskbar, documents::Documents& documents, document_manager::DocumentManager& manager,
                settings::Settings& settings, resources::Resources& resources, dialog::Dialog& dialog,
                playback::Playback& playback);
  };
}
