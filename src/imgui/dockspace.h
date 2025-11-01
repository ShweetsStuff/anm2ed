#pragma once

#include "documents.h"
#include "taskbar.h"
#include "window/animation_preview.h"
#include "window/animations.h"
#include "window/events.h"
#include "window/frame_properties.h"
#include "window/layers.h"
#include "window/nulls.h"
#include "window/onionskin.h"
#include "window/sounds.h"
#include "window/spritesheet_editor.h"
#include "window/spritesheets.h"
#include "window/timeline.h"
#include "window/tools.h"
#include "window/welcome.h"

namespace anm2ed::imgui
{
  class Dockspace
  {
    AnimationPreview animationPreview;
    Animations animations;
    Events events;
    FrameProperties frameProperties;
    Layers layers;
    Nulls nulls;
    Onionskin onionskin;
    SpritesheetEditor spritesheetEditor;
    Spritesheets spritesheets;
    Sounds sounds;
    Timeline timeline;
    Tools tools;
    Welcome welcome;

  public:
    void tick(Manager&, Settings&);
    void update(Taskbar&, Documents&, Manager&, Settings&, Resources&, Dialog&, Clipboard&);
  };
}
