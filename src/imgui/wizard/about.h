#pragma once

#include "../../resources.h"

namespace anm2ed::imgui::wizard
{
  class About
  {
  public:
    struct Credit
    {
      const char* string{};
      resource::font::Type font{resource::font::REGULAR};
    };

    struct ScrollingCredit
    {
      int index{};
      float offset{};
    };

    struct CreditsState
    {
      std::vector<ScrollingCredit> active{};
      float spawnTimer{1.0f};
      int nextIndex{};
    };

    int creditsIndex{};
    CreditsState creditsState{};

    void reset(Resources& resources);
    void update(Resources& resources);
  };

}