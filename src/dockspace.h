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
#include "welcome.h"

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
    welcome::Welcome welcome;

  public:
    void update(taskbar::Taskbar&, documents::Documents&, manager::Manager&, settings::Settings&, resources::Resources&,
                dialog::Dialog&, clipboard::Clipboard&);
  };
}
