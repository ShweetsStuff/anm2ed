#pragma once

#include "clipboard.hpp"
#include "dialog.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"

namespace anm2ed::imgui
{
  class Sounds
  {
    int newSoundId{-1};

  public:
    void update(Manager&, Settings&, Resources&, Dialog&, Clipboard&);
  };
}
