#pragma once

#include "document_manager.h"
#include "settings.h"

namespace anm2ed::frame_properties
{
  class FrameProperties
  {
  public:
    void update(document_manager::DocumentManager& manager, settings::Settings& settings);
  };
}