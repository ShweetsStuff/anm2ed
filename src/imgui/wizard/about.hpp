#pragma once

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../../anm2/anm2.hpp"
#include "../../canvas.hpp"
#include "../../resource/friends.hpp"
#include "../../resources.hpp"

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

    struct FriendState
    {
      Anm2 anm2{};
      std::unique_ptr<Canvas> canvas{};
      std::unordered_map<int, resource::Texture> textures{};
      glm::vec4 rect{-1.0f};
      float time{};
      float fps{30.0f};
      bool isLoaded{};
    };

    int creditsIndex{};
    CreditsState creditsState{};
    std::array<FriendState, resource::friends::COUNT> friendStates{};

    void reset(Resources& resources);
    void update(Resources& resources);
  };

}
