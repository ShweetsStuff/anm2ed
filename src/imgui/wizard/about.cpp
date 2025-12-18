#include "about.h"

#include <cmath>
#include <imgui.h>
#include <vector>

#include "strings.h"

using namespace anm2ed::resource;

namespace anm2ed::imgui::wizard
{
  static constexpr auto CREDIT_DELAY = 1.0f;
  static constexpr auto CREDIT_SCROLL_SPEED = 25.0f;

  static constexpr About::Credit CREDITS[] = {
      {"Anm2Ed", font::BOLD},
      {"License: GPLv3"},
      {""},
      {"Designer", font::BOLD},
      {"Shweet"},
      {""},
      {"Additional Help", font::BOLD},
      {"im-tem"},
      {""},
      {"Localization", font::BOLD},
      {"Gabriel Asencio (Spanish (Latin America))"},
      {"ExtremeThreat (Russian)"},
      {"CxRedix (Chinese)"},
      {"sawalk/사왈이 (Korean)"},
      {""},
      {"Based on the work of:", font::BOLD},
      {"Adrian Gavrilita"},
      {"Simon Parzer"},
      {"Matt Kapuszczak"},
      {""},
      {"XM Music", font::BOLD},
      {"Drozerix"},
      {"\"Keygen Wraith\""},
      {"https://modarchive.org/module.php?207854"},
      {"License: CC0"},
      {""},
      {"Libraries", font::BOLD},
      {"Dear ImGui"},
      {"https://github.com/ocornut/imgui"},
      {"License: MIT"},
      {""},
      {"SDL"},
      {"https://github.com/libsdl-org/SDL"},
      {"License: zlib"},
      {""},
      {"SDL_mixer"},
      {"https://github.com/libsdl-org/SDL_mixer"},
      {"License: zlib"},
      {""},
      {"tinyxml2"},
      {"https://github.com/leethomason/tinyxml2"},
      {"License: zlib"},
      {""},
      {"glm"},
      {"https://github.com/g-truc/glm"},
      {"License: MIT"},
      {""},
      {"lunasvg"},
      {"https://github.com/sammycage/lunasvg"},
      {"License: MIT"},
      {""},
      {"Icons", font::BOLD},
      {"Remix Icons"},
      {"remixicon.com"},
      {"License: Apache"},
      {""},
      {"Font", font::BOLD},
      {"Noto Sans"},
      {"https://fonts.google.com/noto/specimen/Noto+Sans"},
      {"License: OFL"},
      {""},
      {"Special Thanks", font::BOLD},
      {"Edmund McMillen"},
      {"Florian Himsl"},
      {"Tyrone Rodriguez"},
      {"The-Vinh Truong (_kilburn)"},
      {"Isaac Reflashed team"},
      {"Everyone who waited patiently for this to be finished"},
      {"Everyone else who has worked on The Binding of Isaac!"},
      {""},
      {""},
      {""},
      {""},
      {""},
      {""},
      {""},
      {""},
      {""},
      {"enjoy the jams :)"},
      {""},
      {""},
      {""},
      {""},
      {""},
      {""},
      {""},
      {""},
      {""},
      {""},
  };
  static constexpr auto CREDIT_COUNT = (int)(sizeof(CREDITS) / sizeof(About::Credit));

  void About::reset(Resources& resources)
  {
    resources.music_track().play(true);
    creditsState = {};
    creditsState.spawnTimer = CREDIT_DELAY;
  }

  void About::update(Resources& resources)
  {
    auto size = ImGui::GetContentRegionAvail();
    auto applicationLabel = localize.get(LABEL_APPLICATION_NAME);
    auto versionLabel = localize.get(LABEL_APPLICATION_VERSION);

    ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE_LARGE);

    ImGui::SetCursorPosX((size.x - ImGui::CalcTextSize(applicationLabel).x) / 2);
    ImGui::TextUnformatted(applicationLabel);

    ImGui::SetCursorPosX((size.x - ImGui::CalcTextSize(versionLabel).x) / 2);
    ImGui::TextUnformatted(versionLabel);

    ImGui::PopFont();

    auto creditRegionPos = ImGui::GetCursorScreenPos();
    auto creditRegionSize = ImGui::GetContentRegionAvail();

    if (creditRegionSize.y > 0.0f && creditRegionSize.x > 0.0f)
    {
      auto fontSize = ImGui::GetFontSize();
      auto drawList = ImGui::GetWindowDrawList();
      auto clipMax = ImVec2(creditRegionPos.x + creditRegionSize.x, creditRegionPos.y + creditRegionSize.y);
      drawList->PushClipRect(creditRegionPos, clipMax, true);

      auto delta = ImGui::GetIO().DeltaTime;
      creditsState.spawnTimer -= delta;
      auto maxVisible = std::max(1, (int)std::floor(creditRegionSize.y / (float)fontSize));

      while (creditsState.active.size() < (size_t)maxVisible && creditsState.spawnTimer <= 0.0f)
      {
        creditsState.active.push_back({creditsState.nextIndex, 0.0f});
        creditsState.nextIndex = (creditsState.nextIndex + 1) % CREDIT_COUNT;
        creditsState.spawnTimer += CREDIT_DELAY;
      }

      auto baseY = clipMax.y - (float)fontSize;
      auto& baseColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
      auto fadeSpan = (float)fontSize * 2.0f;

      for (auto it = creditsState.active.begin(); it != creditsState.active.end();)
      {
        it->offset += CREDIT_SCROLL_SPEED * delta;
        auto yPos = baseY - it->offset;
        if (yPos + fontSize < creditRegionPos.y)
        {
          it = creditsState.active.erase(it);
          continue;
        }

        auto& credit = CREDITS[it->index];
        auto fontPtr = resources.fonts[credit.font].get();
        auto textSize = fontPtr->CalcTextSizeA((float)fontSize, FLT_MAX, 0.0f, credit.string);
        auto xPos = creditRegionPos.x + (creditRegionSize.x - textSize.x) * 0.5f;

        auto alpha = 1.0f;
        auto topDist = yPos - creditRegionPos.y;
        if (topDist < fadeSpan) alpha *= std::clamp(topDist / fadeSpan, 0.0f, 1.0f);
        auto bottomDist = (creditRegionPos.y + creditRegionSize.y) - (yPos + fontSize);
        if (bottomDist < fadeSpan) alpha *= std::clamp(bottomDist / fadeSpan, 0.0f, 1.0f);
        if (alpha <= 0.0f)
        {
          ++it;
          continue;
        }

        auto color = baseColor;
        color.w *= alpha;

        drawList->AddText(fontPtr, fontSize, ImVec2(xPos, yPos), ImGui::GetColorU32(color), credit.string);
        ++it;
      }

      drawList->PopClipRect();
    }
  }
}