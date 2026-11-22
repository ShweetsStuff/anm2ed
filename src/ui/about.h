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
      const char* string{};
      FontStyle font{FONT_REGULAR};
    };

    // One 30 Hz tick of the credits roll, after the original editor's About box. Lengths are in its pixels (for
    // 14 px text); `trailAlpha` is how much of what was drawn before this tick it faded out (255 clears it).
    struct RollFrame
    {
      float scroll{};
      float offsetX{};
      float stretch{1.0f};
      int effect{};
      int trailAlpha{255};
      glm::vec2 levels{};
      float gradientHeight{};
      ImU32 gradientColor{};
    };

    // The credits scroll up through a box, one of four effects per pass (none, sway, stretch to the music, wave),
    // between volume bars; recent ticks are kept to draw the trails they leave.
    struct RollState
    {
      std::deque<RollFrame> history{};
      float scroll{};
      float tickTime{};
      int tick{};
      int pass{};
      float levelPeak{};
      ImU32 titleColor{};
    };

    struct FriendState
    {
      Model model{};
      std::unique_ptr<Canvas> canvas{};
      std::unordered_map<int, Image> textures{};
      glm::vec4 rect{-1.0f};
      float time{};
      float fps{30.0f};
      bool isLoaded{};
    };

    RollState roll{};
    std::array<FriendState, FRIEND_COUNT> friendStates{};

    void reset(Resources& resources);
    void update(Resources& resources);
  };

