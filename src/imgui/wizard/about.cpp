#include "about.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <format>

#include <imgui.h>
#include <imgui_internal.h>

#include "audio.hpp"
#include "log.hpp"
#include "math.hpp"
#include "model/draw.hpp"
#include "model/frames.hpp"
#include "model/xml.hpp"
#include "strings.hpp"
#include "util/imgui/draw.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui::wizard
{
  // The credits roll, after the original editor's About box: lengths are its pixels (for 14 px text), at its 30 Hz.
  enum RollEffect
  {
    EFFECT_NONE,
    EFFECT_SWAY,
    EFFECT_STRETCH,
    EFFECT_WAVE,
    EFFECT_COUNT
  };

  static constexpr float ROLL_FONT_SIZE = 14.0f;
  static constexpr float ROLL_TICK = 1.0f / 30.0f;
  static constexpr int ROLL_TICKS_MAX = 4;
  static constexpr std::size_t ROLL_HISTORY_MAX = 72;
  static constexpr float ROLL_SCROLL_STEP = 1.0f;
  static constexpr float ROLL_SCROLL_GAP = 20.0f;
  static constexpr float ROLL_FADE_HEIGHT = 30.0f;
  static constexpr float ROLL_WAVE_FREQUENCY = 0.1f;
  static constexpr float ROLL_SWAY_AMPLITUDE = 10.0f;
  static constexpr float ROLL_WAVE_AMPLITUDE = 15.0f;
  static constexpr float ROLL_STRETCH = 0.2f;
  static constexpr int ROLL_TRAIL_ALPHA_SWAY = 25;
  static constexpr int ROLL_TRAIL_ALPHA_STRETCH = 50;
  static constexpr float ROLL_WEIGHT_MIN = 1.0f / 255.0f;
  static constexpr float BAR_WIDTH = 10.0f;
  static constexpr float BAR_LEVEL_HEIGHT = 2.0f;
  static constexpr float BAR_GRADIENT_MIN = 50.0f;
  static constexpr int BAR_TRAIL_ALPHA = 20;
  static constexpr int BAR_RED_STEP = 100;
  static constexpr ImU32 ROLL_BACKGROUND = IM_COL32(0, 0, 0, 255);
  static constexpr ImU32 ROLL_CLEAR = IM_COL32(0, 0, 0, 0);
  static constexpr ImU32 ROLL_HEADER_COLOR = IM_COL32(255, 255, 255, 255);
  static constexpr ImU32 ROLL_NAME_COLOR = IM_COL32(200, 255, 255, 255);
  static constexpr ImU32 BAR_BASE_COLOR = IM_COL32(255, 0, 0, 255);
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
      {"Claude Code"},
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

  const model::Animation* friend_animation_get(const About::FriendState& state)
  {
    auto animations = state.model.animations_get();
    for (auto [index, animation] : animations)
      if (animation->name == state.model.animations.defaultAnimation) return animation;
    return animations.empty() ? nullptr : animations.front().second;
  }

  void friend_state_load(About::FriendState& state, const resource::friends::Info& info)
  {
    if (!state.canvas) state.canvas = std::make_unique<Canvas>(vec2(1.0f, 1.0f));

    state.model = model::Model{};
    state.textures.clear();
    state.rect = vec4(-1.0f);
    state.time = 0.0f;
    state.fps = 30.0f;
    state.isLoaded = false;

    std::string errorString{};
    if (!model::model_load_string(state.model, info.anm2, &errorString))
    {
      logger.error(std::format("Unable to load friend animation {}: {}", info.name, errorString));
      return;
    }

    if (state.model.info.fps > 0) state.fps = (float)state.model.info.fps;

    for (auto& spritesheet : state.model.content.spritesheets)
      state.textures[spritesheet.id] = resource::Image(info.png, info.pngSize);

    auto animation = friend_animation_get(state);
    if (!animation)
    {
      logger.error(std::format("Friend animation {} has no animation.", info.name));
      return;
    }

    state.rect = model::animation_rect(state.model, *animation, true);
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
    for (const auto& draw :
         model::animation_draws_get(state.model, *animation, {.time = state.time, .isRootTransform = true}))
    {
      auto& frame = draw.frame;
      auto layer = draw.type == model::DrawType::LAYER ? model::item_get(state.model.content.layers, draw.id) : nullptr;
      auto texture = layer ? state.textures.find(layer->spritesheetId) : state.textures.end();
      if (texture == state.textures.end() || !texture->second.is_valid() || frame.size == vec2()) continue;
      auto textureSize = vec2(texture->second.size);
      auto uvVertices = math::uv_vertices_get(frame.crop / textureSize, (frame.crop + frame.size) / textureSize);
      state.canvas->texture_render(resources.shaders[shader::TEXTURE], resource::texture::id_get(texture->second),
                                   transform * draw.parent * model::draw_quad_model_get(draw), frame.tint,
                                   frame.colorOffset, uvVertices.data());
    }

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
    auto textColor = about.roll.titleColor ? about.roll.titleColor : ImGui::GetColorU32(ImGuiCol_Text);

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
      image_premultiplied_draw(drawList, (ImTextureID)(intptr_t)about.friendStates[index].canvas->texture, min, max);
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

  // One tick: the credits scroll up (a new pass, and effect, once they are gone) and the bars follow the music.
  void roll_tick(About::RollState& roll, Resources& resources, float boxHeight, float textHeight)
  {
    auto levels = glm::min(audio::levels_get(resources.music_track()) * BAR_LEVEL_HEIGHT, 1.0f) * boxHeight;
    roll.scroll -= ROLL_SCROLL_STEP;
    if (roll.scroll < -(textHeight + ROLL_SCROLL_GAP))
    {
      roll.scroll = boxHeight;
      ++roll.pass;
    }
    ++roll.tick;

    About::RollFrame frame{.scroll = roll.scroll,
                           .effect = roll.pass % EFFECT_COUNT,
                           .levels = levels,
                           .gradientHeight = std::max({BAR_GRADIENT_MIN, levels.x, levels.y}),
                           .gradientColor =
                               IM_COL32((roll.pass * BAR_RED_STEP) % 256, 255 - roll.tick % 256, roll.tick % 256, 255)};
    if (frame.effect == EFFECT_SWAY)
    {
      frame.trailAlpha = ROLL_TRAIL_ALPHA_SWAY;
      frame.offsetX = std::sin(roll.scroll * ROLL_WAVE_FREQUENCY) * ROLL_SWAY_AMPLITUDE;
    }
    if (frame.effect == EFFECT_STRETCH)
    {
      frame.trailAlpha = ROLL_TRAIL_ALPHA_STRETCH;
      frame.stretch = 1.0f + (levels.x + levels.y) / boxHeight * ROLL_STRETCH;
    }
    // While the credits stretch, the title flashes random colors.
    roll.titleColor =
        frame.effect == EFFECT_STRETCH ? IM_COL32(std::rand() % 256, std::rand() % 256, std::rand() % 256, 255) : 0;

    roll.history.push_front(frame);
    if (roll.history.size() > ROLL_HISTORY_MAX) roll.history.pop_back();
  }

  ImU32 color_weighted_get(ImU32 color, float weight)
  {
    auto value = ImGui::ColorConvertU32ToFloat4(color);
    value.w *= weight;
    return ImGui::ColorConvertFloat4ToU32(value);
  }

  // The credits as one tick drew them; `weight` fades the copies that linger as trails.
  void roll_text_draw(ImDrawList* drawList, Resources& resources, const About::RollFrame& frame, ImVec2 min, ImVec2 max,
                      float scale, float lineHeight, float weight)
  {
    auto centerX = (min.x + max.x) * 0.5f;
    for (int i = 0; i < CREDIT_COUNT; ++i)
    {
      auto& credit = CREDITS[i];
      auto y = min.y + frame.scroll * scale + (float)i * lineHeight;
      if (!*credit.string || y + lineHeight < min.y || y > max.y) continue;

      auto font = resources.fonts[credit.font].get();
      auto x = centerX - font->CalcTextSizeA(lineHeight, FLT_MAX, 0.0f, credit.string).x * 0.5f + frame.offsetX * scale;
      if (frame.effect == EFFECT_WAVE)
        x += std::round(std::sin((y - min.y) / scale * ROLL_WAVE_FREQUENCY) * ROLL_WAVE_AMPLITUDE) * scale;
      auto color = credit.font == font::BOLD ? ROLL_HEADER_COLOR : ROLL_NAME_COLOR;

      auto vertexStart = drawList->VtxBuffer.Size;
      drawList->AddText(font, lineHeight, ImVec2(x, y), color_weighted_get(color, weight), credit.string);
      if (frame.stretch != 1.0f)
        for (int vertex = vertexStart; vertex < drawList->VtxBuffer.Size; ++vertex)
          drawList->VtxBuffer[vertex].pos.x = centerX + (drawList->VtxBuffer[vertex].pos.x - centerX) * frame.stretch;
    }
  }

  // A volume bar's color at a height: red at the bottom into the tick's color at its gradient height.
  ImU32 bar_color_get(const About::RollFrame& frame, float height, float weight)
  {
    auto color = ImLerp(ImGui::ColorConvertU32ToFloat4(BAR_BASE_COLOR),
                        ImGui::ColorConvertU32ToFloat4(frame.gradientColor), height / frame.gradientHeight);
    color.w = weight;
    return ImGui::ColorConvertFloat4ToU32(color);
  }

  // The left and right volume bars; older, taller bars show above newer ones, fading as they age.
  void roll_bars_draw(ImDrawList* drawList, const std::deque<About::RollFrame>& history, ImVec2 min, ImVec2 max,
                      float scale)
  {
    auto width = BAR_WIDTH * scale;
    for (int side = 0; side < 2; ++side)
    {
      auto left = side == 0 ? min.x : max.x - width;
      auto covered = 0.0f;
      auto weight = 1.0f;
      for (const auto& frame : history)
      {
        if (weight < ROLL_WEIGHT_MIN) break;
        auto height = frame.levels[side];
        if (height > covered)
        {
          auto top = bar_color_get(frame, height, weight);
          auto bottom = bar_color_get(frame, covered, weight);
          drawList->AddRectFilledMultiColor(ImVec2(left, max.y - height * scale),
                                            ImVec2(left + width, max.y - covered * scale), top, top, bottom, bottom);
          covered = height;
        }
        weight *= 1.0f - (float)BAR_TRAIL_ALPHA / 255.0f;
      }
    }
  }

  void roll_draw(About::RollState& roll, Resources& resources, ImVec2 min, ImVec2 size)
  {
    auto max = ImVec2(min.x + size.x, min.y + size.y);
    auto lineHeight = ImGui::GetFontSize();
    auto scale = lineHeight / ROLL_FONT_SIZE;
    auto boxHeight = size.y / scale;
    auto textHeight = (float)CREDIT_COUNT * lineHeight / scale;

    if (roll.history.empty())
    {
      roll.scroll = boxHeight;
      roll_tick(roll, resources, boxHeight, textHeight);
    }
    roll.tickTime += ImGui::GetIO().DeltaTime;
    for (int i = 0; roll.tickTime >= ROLL_TICK; ++i, roll.tickTime -= ROLL_TICK)
      if (i < ROLL_TICKS_MAX) roll_tick(roll, resources, boxHeight, textHeight);

    auto drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(min, max, true);
    drawList->AddRectFilled(min, max, ROLL_BACKGROUND);

    // Each tick faded what came before it by its trail alpha; the copies still visible are drawn oldest first.
    std::vector<std::pair<const About::RollFrame*, float>> copies{};
    auto weight = 1.0f;
    for (const auto& frame : roll.history)
    {
      if (weight < ROLL_WEIGHT_MIN) break;
      copies.emplace_back(&frame, weight);
      weight *= 1.0f - (float)frame.trailAlpha / 255.0f;
    }
    for (auto it = copies.rbegin(); it != copies.rend(); ++it)
      roll_text_draw(drawList, resources, *it->first, min, max, scale, lineHeight, it->second);

    auto barWidth = BAR_WIDTH * scale;
    drawList->AddRectFilledMultiColor(ImVec2(min.x + barWidth, min.y),
                                      ImVec2(max.x - barWidth, min.y + ROLL_FADE_HEIGHT * scale), ROLL_BACKGROUND,
                                      ROLL_BACKGROUND, ROLL_CLEAR, ROLL_CLEAR);
    roll_bars_draw(drawList, roll.history, min, max, scale);
    drawList->PopClipRect();
    ImGui::Dummy(size);
  }

  void About::reset(Resources& resources)
  {
    resource::audio::play(resources.music_track(), true);
    roll = {};

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

    auto rollSize = ImGui::GetContentRegionAvail();
    if (rollSize.x > 0.0f && rollSize.y > 0.0f) roll_draw(roll, resources, ImGui::GetCursorScreenPos(), rollSize);
  }
}
