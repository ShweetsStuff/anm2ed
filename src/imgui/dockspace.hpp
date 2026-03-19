#pragma once

#include "documents.hpp"
#include "taskbar.hpp"
#include "window/animation_preview.hpp"
#include "window/animations.hpp"
#include "window/regions.hpp"
#include "window/events.hpp"
#include "window/frame_properties.hpp"
#include "window/layers.hpp"
#include "window/nulls.hpp"
#include "window/onionskin.hpp"
#include "window/sounds.hpp"
#include "window/spritesheet_editor.hpp"
#include "window/spritesheets.hpp"
#include "window/timeline.hpp"
#include "window/tools.hpp"
#include "window/welcome.hpp"

namespace anm2ed::imgui
{
  class Dockspace
  {
    AnimationPreview animationPreview;
    Animations animations;
    Regions regions;
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
