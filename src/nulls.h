#pragma once

#include "document.h"
#include "imgui.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::nulls
{
  class Nulls
  {
    imgui::MultiSelectStorage storage{};
    std::set<int> unusedNullsIDs{};

  public:
    void update(document::Document& document, int& documentIndex, settings::Settings& settings,
                resources::Resources& resources);
  };
}