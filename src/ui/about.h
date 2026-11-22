#pragma once

#include <array>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

#include <imgui/imgui.h>

#include "canvas.h"
#include "model/model.h"
#include "friends.h"
#include "resources.h"

  class About
  {
  public:
    struct Credit
    {
      const char* string = nullptr;
      FontStyle font = FONT_REGULAR;
    };

    // One 30 Hz tick of the credits roll, after the original editor's About box. Lengths are in its pixels (for
    // 14 px text); `trailAlpha` is how much of what was drawn before this tick it faded out (255 clears it).
    struct RollFrame
    {
      float scroll = 0;
      float offsetX = 0;
      float stretch = 1.0f;
      int effect = 0;
      int trailAlpha = 255;
      glm::vec2 levels{};
      float gradientHeight = 0;
      ImU32 gradientColor = 0;
    };

    // The credits scroll up through a box, one of four effects per pass (none, sway, stretch to the music, wave),
    // between volume bars; recent ticks are kept to draw the trails they leave.
    struct RollState
    {
      std::deque<RollFrame> history;
      float scroll = 0;
      float tickTime = 0;
      int tick = 0;
      int pass = 0;
      float levelPeak = 0;
      ImU32 titleColor = 0;
    };

    struct FriendState
    {
      Model model = {};
      std::unique_ptr<Canvas> canvas;
      std::unordered_map<int, Image> textures;
      glm::vec4 rect{-1.0f};
      float time = 0;
      float fps = 30.0f;
      bool isLoaded = false;
    };

    RollState roll = {};
    std::array<FriendState, FRIEND_COUNT> friendStates = {};

    void reset(Resources& resources);
    void update(Resources& resources);
  };

