#include "about.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <format>

#include <imgui.h>

#include "log.hpp"
#include "math.hpp"
#include "strings.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui::wizard
{
  static constexpr float CREDIT_DELAY = 0.66f;
  static constexpr float CREDIT_SCROLL_SPEED = 33.0f;
  static constexpr float FRIEND_HEIGHT_MULTIPLIER = 2.0f;
  static constexpr float FRIEND_PADDING_RATIO = 0.15f;
  static constexpr int FRIEND_ROW_SLOT_COUNT = 5;
  static constexpr int FRIEND_ORDER_LEFT[] = {resource::friends::MEAT_BOY, resource::friends::ISAAC};
  static constexpr int FRIEND_ORDER_RIGHT[] = {resource::friends::STACY, resource::friends::ASH};

  static constexpr About::Credit CREDITS[] = {
      {"Anm2Ed", font::BOLD},
      {"License: GPLv3"},
      {""},
      {"Design/Programming", font::BOLD},
      {"Shweet"},
      {"OpenAI Codex"},
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
      {"Based on the work of", font::BOLD},
      {"Adrian Gavrilita"},
      {"Simon Parzer"},
      {"Matt Kapuszczak"},
      {""},
      {"Music", font::BOLD},
      {"Soundbin"},
      {"\"Digital Antidepressant\""},
      {"https://www.newgrounds.com/audio/listen/1565433"},
      {"License: CC0"},
      {""},
      {"Dancing Characters", font::BOLD},
      {"Shweet"},
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
      {"enjoy the jams :)"},
      {""},
      {""},
      {""},
      {""},
      {""},
  };
  static constexpr auto CREDIT_COUNT = (int)(sizeof(CREDITS) / sizeof(About::Credit));

  const Element* friend_animation_get(const About::FriendState& state)
  {
    auto animations = state.anm2.element_get(ElementType::ANIMATIONS);
    if (!animations) return nullptr;

    if (!animations->defaultAnimation.empty())
      for (const auto& child : animations->children)
        if (child.type == ElementType::ANIMATION && child.name == animations->defaultAnimation) return &child;

    for (const auto& child : animations->children)
      if (child.type == ElementType::ANIMATION) return &child;

    return nullptr;
  }

  void friend_state_load(About::FriendState& state, const resource::friends::Info& info)
  {
    if (!state.canvas) state.canvas = std::make_unique<Canvas>(vec2(1.0f, 1.0f));

    state.anm2 = Anm2{};
    state.textures.clear();
    state.rect = vec4(-1.0f);
    state.time = 0.0f;
    state.fps = 30.0f;
    state.isLoaded = false;

    std::string errorString{};
    if (!state.anm2.load_string(info.anm2, &errorString))
    {
      logger.error(std::format("Unable to load friend animation {}: {}", info.name, errorString));
      return;
    }

    if (auto anm2Info = state.anm2.element_get(ElementType::INFO))
      if (anm2Info->fps > 0) state.fps = (float)anm2Info->fps;

    if (auto spritesheets = state.anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
        if (spritesheet.type == ElementType::SPRITESHEET)
          state.textures[spritesheet.id] = Texture(info.png, info.pngSize);

    auto animation = friend_animation_get(state);
    if (!animation)
    {
      logger.error(std::format("Friend animation {} has no animation.", info.name));
      return;
    }

    state.rect = state.anm2.animation_rect(*animation, true);
    state.isLoaded = state.rect != vec4(-1.0f) && !state.textures.empty();
  }

  void friend_state_tick(About::FriendState& state, float delta)
  {
    if (!state.isLoaded) return;

    auto animation = friend_animation_get(state);
    if (!animation || animation->frameNum <= 0) return;

    state.time = std::fmod(state.time + delta * state.fps, (float)animation->frameNum);
    if (state.time < 0.0f) state.time += (float)animation->frameNum;
  }

  ImVec2 friend_size_get(const About::FriendState& state, float height)
  {
    auto width = height;
    if (state.rect.w > 0.0f && state.rect.z > 0.0f) width = height * (state.rect.z / state.rect.w);
    return ImVec2(std::max(width, 1.0f), std::max(height, 1.0f));
  }

  void friend_layer_draw(About::FriendState& state, Resources& resources, const mat4& baseTransform,
                         const Element& layerAnimation, const Element& rootFrame)
  {
    if (layerAnimation.type == ElementType::GROUP)
    {
      for (auto& child : layerAnimation.children)
        friend_layer_draw(state, resources, baseTransform, child, rootFrame);
      return;
    }

    if (layerAnimation.type != ElementType::LAYER_ANIMATION || !layerAnimation.isVisible) return;

    auto layer = state.anm2.element_get(ElementType::LAYER_ELEMENT, layerAnimation.layerId);
    if (!layer) return;

    auto textureIt = state.textures.find(layer->spritesheetId);
    if (textureIt == state.textures.end() || !textureIt->second.is_valid()) return;

    auto frame = state.anm2.frame_effective(layerAnimation.layerId, frame_generate(layerAnimation, state.time));
    if (!frame.isVisible || frame.size == vec2()) return;

    auto& texture = textureIt->second;
    auto textureSize = vec2(texture.size);
    if (textureSize.x <= 0.0f || textureSize.y <= 0.0f) return;

    auto transform = baseTransform * math::quad_model_get(frame.size, frame.position, frame.pivot,
                                                          math::percent_to_unit(frame.scale), frame.rotation);
    auto uvVertices = math::uv_vertices_get(frame.crop / textureSize, (frame.crop + frame.size) / textureSize);
    auto tint = frame.tint * rootFrame.tint;
    auto colorOffset = frame.colorOffset + rootFrame.colorOffset;

    state.canvas->texture_render(resources.shaders[shader::TEXTURE], texture.id, transform, tint, colorOffset,
                                 uvVertices.data());
  }

  void friend_canvas_draw(About::FriendState& state, Resources& resources, ImVec2 displaySize)
  {
    if (!state.isLoaded || !state.canvas) return;

    auto canvasSize = vec2(std::max(displaySize.x, 1.0f), std::max(displaySize.y, 1.0f));
    state.canvas->size_set(canvasSize);
    state.canvas->bind();
    state.canvas->viewport_set();
    state.canvas->clear(vec4(0.0f));

    auto animation = friend_animation_get(state);
    if (!animation)
    {
      state.canvas->unbind();
      return;
    }

    float zoom = 100.0f;
    vec2 pan{};
    auto rect = state.rect;
    auto padding = std::max(2.0f, std::max(rect.z, rect.w) * FRIEND_PADDING_RATIO);
    rect.x -= padding;
    rect.y -= padding;
    rect.z += padding * 2.0f;
    rect.w += padding * 2.0f;
    state.canvas->set_to_rect(zoom, pan, rect);

    auto transform = state.canvas->transform_get(zoom, pan);
    Element rootFrame{};
    if (auto root = animation_item_get(*animation, ItemType::ROOT))
    {
      rootFrame = frame_generate(*root, state.time);
      transform *= math::quad_model_parent_get(rootFrame.position, {}, math::percent_to_unit(rootFrame.scale),
                                               rootFrame.rotation);
    }

    if (auto layerAnimations = element_child_first_get(*animation, ElementType::LAYER_ANIMATIONS))
      for (auto& layerAnimation : layerAnimations->children)
        friend_layer_draw(state, resources, transform, layerAnimation, rootFrame);

    state.canvas->unbind();
  }

  void friend_row_draw(About& about, Resources& resources, const char* titleLabel, ImVec2 size)
  {
    ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE_LARGE);
    auto titleSize = ImGui::CalcTextSize(titleLabel);
    ImGui::PopFont();

    auto slotWidth = std::max(size.x / (float)FRIEND_ROW_SLOT_COUNT, 1.0f);
    float friendHeight = titleSize.y * FRIEND_HEIGHT_MULTIPLIER;
    float maxAspect{};
    for (auto index : FRIEND_ORDER_LEFT)
      maxAspect = std::max(maxAspect, about.friendStates[index].rect.w > 0.0f
                                          ? about.friendStates[index].rect.z / about.friendStates[index].rect.w
                                          : 1.0f);
    for (auto index : FRIEND_ORDER_RIGHT)
      maxAspect = std::max(maxAspect, about.friendStates[index].rect.w > 0.0f
                                          ? about.friendStates[index].rect.z / about.friendStates[index].rect.w
                                          : 1.0f);
    if (maxAspect > 0.0f) friendHeight = std::min(friendHeight, slotWidth / maxAspect);
    friendHeight = std::max(friendHeight, 1.0f);

    std::array<ImVec2, resource::friends::COUNT> friendSizes{};
    for (auto index : FRIEND_ORDER_LEFT)
      friendSizes[index] = friend_size_get(about.friendStates[index], friendHeight);
    for (auto index : FRIEND_ORDER_RIGHT)
      friendSizes[index] = friend_size_get(about.friendStates[index], friendHeight);

    for (auto index : FRIEND_ORDER_LEFT)
      friend_canvas_draw(about.friendStates[index], resources, friendSizes[index]);
    for (auto index : FRIEND_ORDER_RIGHT)
      friend_canvas_draw(about.friendStates[index], resources, friendSizes[index]);

    auto rowHeight = std::max(titleSize.y, friendHeight);
    auto rowMin = ImGui::GetCursorScreenPos();
    auto rowMax = ImVec2(rowMin.x + size.x, rowMin.y + rowHeight);
    auto drawList = ImGui::GetWindowDrawList();
    auto textColor = ImGui::GetColorU32(ImGuiCol_Text);

    auto slot_center_x_get = [&](int slot)
    {
      auto offset = slotWidth * ((float)slot + 0.5f);
      return std::min(rowMin.x + offset, rowMax.x - slotWidth * 0.5f);
    };

    auto image_draw = [&](int slot, int index)
    {
      auto centerX = slot_center_x_get(slot);
      auto size = friendSizes[index];
      auto min = ImVec2(centerX - size.x * 0.5f, rowMin.y + (rowHeight - size.y) * 0.5f);
      auto max = ImVec2(min.x + size.x, min.y + size.y);
      drawList->AddImage((ImTextureID)(intptr_t)about.friendStates[index].canvas->texture, min, max);
    };

    drawList->PushClipRect(rowMin, rowMax, true);
    image_draw(0, resource::friends::MEAT_BOY);
    image_draw(1, resource::friends::ISAAC);

    auto titleCenterX = slot_center_x_get(2);
    auto titlePos = ImVec2(titleCenterX - titleSize.x * 0.5f, rowMin.y + (rowHeight - titleSize.y) * 0.5f);
    drawList->AddText(resources.fonts[font::BOLD].get(), (float)font::SIZE_LARGE, titlePos, textColor, titleLabel);

    image_draw(3, resource::friends::STACY);
    image_draw(4, resource::friends::ASH);
    drawList->PopClipRect();

    ImGui::Dummy(ImVec2(size.x, rowHeight));
  }

  void About::reset(Resources& resources)
  {
    resources.music_track().play(true);
    creditsState = {};
    creditsState.spawnTimer = CREDIT_DELAY;

    for (int i = 0; i < resource::friends::COUNT; ++i)
      friend_state_load(friendStates[i], resource::friends::FRIENDS[i]);
  }

  void About::update(Resources& resources)
  {
    auto size = ImGui::GetContentRegionAvail();
    auto titleLabel = localize.get(LABEL_APPLICATION_NAME);
    auto delta = ImGui::GetIO().DeltaTime;

    for (auto& friendState : friendStates)
      friend_state_tick(friendState, delta);

    friend_row_draw(*this, resources, titleLabel, size);

    auto creditRegionPos = ImGui::GetCursorScreenPos();
    auto creditRegionSize = ImGui::GetContentRegionAvail();

    if (creditRegionSize.y > 0.0f && creditRegionSize.x > 0.0f)
    {
      auto fontSize = ImGui::GetFontSize();
      auto drawList = ImGui::GetWindowDrawList();
      auto clipMax = ImVec2(creditRegionPos.x + creditRegionSize.x, creditRegionPos.y + creditRegionSize.y);
      drawList->PushClipRect(creditRegionPos, clipMax, true);

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
