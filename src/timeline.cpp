#include "timeline.h"

#include <ranges>

#include <imgui_internal.h>

#include "imgui.h"

using namespace anm2ed::types;
using namespace anm2ed::document_manager;
using namespace anm2ed::resources;
using namespace anm2ed::settings;
using namespace anm2ed::playback;
using namespace glm;

namespace anm2ed::timeline
{
  constexpr auto ROOT_COLOR = ImVec4(0.140f, 0.310f, 0.560f, 1.000f);
  constexpr auto ROOT_COLOR_ACTIVE = ImVec4(0.240f, 0.520f, 0.880f, 1.000f);
  constexpr auto ROOT_COLOR_HOVERED = ImVec4(0.320f, 0.640f, 1.000f, 1.000f);

  constexpr auto LAYER_COLOR = ImVec4(0.640f, 0.320f, 0.110f, 1.000f);
  constexpr auto LAYER_COLOR_ACTIVE = ImVec4(0.840f, 0.450f, 0.170f, 1.000f);
  constexpr auto LAYER_COLOR_HOVERED = ImVec4(0.960f, 0.560f, 0.240f, 1.000f);

  constexpr auto NULL_COLOR = ImVec4(0.140f, 0.430f, 0.200f, 1.000f);
  constexpr auto NULL_COLOR_ACTIVE = ImVec4(0.250f, 0.650f, 0.350f, 1.000f);
  constexpr auto NULL_COLOR_HOVERED = ImVec4(0.350f, 0.800f, 0.480f, 1.000f);

  constexpr auto TRIGGER_COLOR = ImVec4(0.620f, 0.150f, 0.260f, 1.000f);
  constexpr auto TRIGGER_COLOR_ACTIVE = ImVec4(0.820f, 0.250f, 0.380f, 1.000f);
  constexpr auto TRIGGER_COLOR_HOVERED = ImVec4(0.950f, 0.330f, 0.490f, 1.000f);

  constexpr auto COLOR_HIDDEN_MULTIPLIER = vec4(0.5f, 0.5f, 0.5f, 1.000f);

  constexpr auto FRAME_TIMELINE_COLOR = ImVec4(0.106f, 0.184f, 0.278f, 1.000f);

  constexpr auto FRAME_BORDER_COLOR = ImVec4(1.0f, 1.0f, 1.0f, 0.15f);
  constexpr auto FRAME_MULTIPLE_OVERLAY_COLOR = ImVec4(1.0f, 1.0f, 1.0f, 0.05f);
  constexpr auto PLAYHEAD_LINE_THICKNESS = 4.0f;

  constexpr auto TEXT_MULTIPLE_COLOR = to_imvec4(color::WHITE);
  constexpr auto PLAYHEAD_LINE_COLOR = to_imvec4(color::WHITE);

  constexpr auto FRAME_MULTIPLE = 5;

  constexpr auto HELP_FORMAT = R"(- Press {} to decrement time.
- Press {} to increment time.
- Press {} to extend the selected frame, by one frame.
- Press {} to shorten the selected frame, by one frame.
- Hold Alt while clicking a non-trigger frame to toggle interpolation.)";

  void Timeline::item_child(anm2::Anm2& anm2, anm2::Reference& reference, anm2::Animation* animation,
                            Settings& settings, Resources& resources, anm2::Type type, int id, int& index)
  {
    auto item = animation ? animation->item_get(type, id) : nullptr;
    auto isVisible = item ? item->isVisible : false;
    auto isActive = reference.itemType == type && reference.itemID == id;
    std::string label = "##None";
    icon::Type icon{};
    ImVec4 color{};

    switch (type)
    {
      case anm2::ROOT:
        label = "Root";
        icon = icon::ROOT;
        color = isActive ? ROOT_COLOR_ACTIVE : ROOT_COLOR;
        break;
      case anm2::LAYER:
        label = std::format("#{} {} (Spritesheet: #{})", id, anm2.content.layers.at(id).name,
                            anm2.content.layers[id].spritesheetID);
        icon = icon::LAYER;
        color = isActive ? LAYER_COLOR_ACTIVE : LAYER_COLOR;
        break;
      case anm2::NULL_:
        label = std::format("#{} {}", id, anm2.content.nulls[id].name);
        icon = icon::NULL_;
        color = isActive ? NULL_COLOR_ACTIVE : NULL_COLOR;
        break;
      case anm2::TRIGGER:
        label = "Triggers";
        icon = icon::TRIGGERS;
        color = isActive ? TRIGGER_COLOR_ACTIVE : TRIGGER_COLOR;
        break;
      default:
        break;
    }

    color = !isVisible ? to_imvec4(to_vec4(color) * COLOR_HIDDEN_MULTIPLIER) : color;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, color);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

    auto itemSize = ImVec2(ImGui::GetContentRegionAvail().x,
                           ImGui::GetTextLineHeightWithSpacing() + (ImGui::GetStyle().WindowPadding.y * 2));

    if (ImGui::BeginChild(label.c_str(), itemSize, ImGuiChildFlags_Borders))
    {
      if (type != anm2::NONE)
      {
        anm2::Reference itemReference = {reference.animationIndex, type, id};

        if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) reference = itemReference;

        ImGui::Image(resources.icons[icon].id, imgui::icon_size_get());
        ImGui::SameLine();
        ImGui::TextUnformatted(label.c_str());

        anm2::Item* item = animation->item_get(type, id);
        bool& isVisible = item->isVisible;

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());

        ImGui::SetCursorPos(ImVec2(itemSize.x - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x,
                                   (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
        int visibleIcon = isVisible ? icon::VISIBLE : icon::INVISIBLE;

        if (ImGui::ImageButton("##Visible Toggle", resources.icons[visibleIcon].id, imgui::icon_size_get()))
          isVisible = !isVisible;
        ImGui::SetItemTooltip(isVisible ? "The item is shown. Press to hide." : "The item is hidden. Press to show.");

        if (type == anm2::NULL_)
        {
          auto& null = anm2.content.nulls.at(id);
          auto& isShowRect = null.isShowRect;

          auto rectIcon = isShowRect ? icon::SHOW_RECT : icon::HIDE_RECT;
          ImGui::SetCursorPos(
              ImVec2(itemSize.x - (ImGui::GetTextLineHeightWithSpacing() * 2) - ImGui::GetStyle().ItemSpacing.x,
                     (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
          if (ImGui::ImageButton("##Rect Toggle", resources.icons[rectIcon].id, imgui::icon_size_get()))
            isShowRect = !isShowRect;
          ImGui::SetItemTooltip(isShowRect ? "The null's rect is shown. Press to hide."
                                           : "The null's rect is hidden. Press to show.");
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
      }
      else
      {
        auto cursorPos = ImGui::GetCursorPos();
        auto& isShowUnused = settings.timelineIsShowUnused;

        ImGui::SetCursorPos(ImVec2(itemSize.x - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x,
                                   (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());

        auto unusedIcon = isShowUnused ? icon::SHOW_UNUSED : icon::HIDE_UNUSED;
        if (ImGui::ImageButton("##Unused Toggle", resources.icons[unusedIcon].id, imgui::icon_size_get()))
          isShowUnused = !isShowUnused;
        ImGui::SetItemTooltip(isShowUnused ? "Unused layers/nulls are shown. Press to hide."
                                           : "Unused layers/nulls are hidden. Press to show.");

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::SetCursorPos(cursorPos);

        ImGui::BeginDisabled();
        ImGui::Text("(?)");
        ImGui::SetItemTooltip("%s", std::format(HELP_FORMAT, settings.shortcutNextFrame, settings.shortcutPreviousFrame,
                                                settings.shortcutShortenFrame, settings.shortcutExtendFrame)
                                        .c_str());
        ImGui::EndDisabled();
      }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    index++;
  }

  void Timeline::items_child(Document& document, anm2::Animation* animation, Settings& settings, Resources& resources)
  {
    auto& anm2 = document.anm2;
    auto& reference = document.reference;

    auto itemsChildSize = ImVec2(ImGui::GetTextLineHeightWithSpacing() * 15, ImGui::GetContentRegionAvail().y);

    if (ImGui::BeginChild("##Items Child", itemsChildSize, ImGuiChildFlags_Borders))
    {
      auto itemsListChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y -
                                                                             ImGui::GetTextLineHeightWithSpacing() -
                                                                             ImGui::GetStyle().ItemSpacing.y * 2);

      if (ImGui::BeginChild("##Items List Child", itemsListChildSize, ImGuiChildFlags_Borders,
                            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
      {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2());
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0f);
        if (ImGui::BeginTable("##Item Table", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY))
        {
          ImGui::GetCurrentWindow()->Flags |= ImGuiWindowFlags_NoScrollWithMouse;
          ImGui::SetScrollY(scroll.y);

          int index{};

          ImGui::TableSetupScrollFreeze(0, 1);
          ImGui::TableSetupColumn("##Items");

          auto item_child_row = [&](anm2::Type type, int id = -1)
          {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            item_child(anm2, reference, animation, settings, resources, type, id, index);
          };

          item_child_row(anm2::NONE);

          if (animation)
          {
            item_child_row(anm2::ROOT);

            for (auto& id : animation->layerOrder)
            {
              if (anm2::Item* item = animation->item_get(anm2::LAYER, id); item)
                if (!settings.timelineIsShowUnused && item->frames.empty()) continue;

              item_child_row(anm2::LAYER, id);
            }

            for (auto& id : animation->nullAnimations | std::views::keys)
            {
              if (anm2::Item* item = animation->item_get(anm2::NULL_, id); item)
                if (!settings.timelineIsShowUnused && item->frames.empty()) continue;

              item_child_row(anm2::NULL_, id);
            }

            item_child_row(anm2::TRIGGER);
          }

          if (isHorizontalScroll && ImGui::GetCurrentWindow()->ScrollbarY)
          {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Dummy(ImVec2(0, style.ScrollbarSize));
          }

          ImGui::EndTable();
        }
        ImGui::PopStyleVar(2);
      }
      ImGui::EndChild();

      auto widgetSize = imgui::widget_size_with_row_get(2);

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

      ImGui::BeginDisabled(!animation);
      {
        if (ImGui::Button("Add", widgetSize)) propertiesPopup.open();
        ImGui::SetItemTooltip("%s", "Add a new item to the animation.");
        ImGui::SameLine();

        ImGui::BeginDisabled(document.item_get());
        {
          ImGui::Button("Remove", widgetSize);
          ImGui::SetItemTooltip("%s", "Remove the selected items from the animation.");
        }
        ImGui::EndDisabled();
      }
      ImGui::EndDisabled();

      ImGui::PopStyleVar();
    }
    ImGui::EndChild();
  }

  void Timeline::frame_child(Document& document, anm2::Animation* animation, Settings& settings, Resources& resources,
                             Playback& playback, anm2::Type type, int id, int& index, float width)
  {
    auto& anm2 = document.anm2;
    auto& reference = document.reference;
    auto item = animation ? animation->item_get(type, id) : nullptr;
    auto isVisible = item ? item->isVisible : false;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

    auto childSize = ImVec2(width, ImGui::GetTextLineHeightWithSpacing() + (ImGui::GetStyle().WindowPadding.y * 2));
    ImVec4 color{};
    ImVec4 colorActive{};
    ImVec4 colorHovered{};
    ImVec4 colorHidden{};
    ImVec4 colorActiveHidden{};
    ImVec4 colorHoveredHidden{};

    ImGui::PopStyleVar(2);

    ImGui::PushID(index);

    switch (type)
    {
      case anm2::ROOT:
        color = ROOT_COLOR;
        colorActive = ROOT_COLOR_ACTIVE;
        colorHovered = ROOT_COLOR_HOVERED;
        break;
      case anm2::LAYER:
        color = LAYER_COLOR;
        colorActive = LAYER_COLOR_ACTIVE;
        colorHovered = LAYER_COLOR_HOVERED;
        break;
      case anm2::NULL_:
        color = NULL_COLOR;
        colorActive = NULL_COLOR_ACTIVE;
        colorHovered = NULL_COLOR_HOVERED;
        break;
      case anm2::TRIGGER:
        color = TRIGGER_COLOR;
        colorActive = TRIGGER_COLOR_ACTIVE;
        colorHovered = TRIGGER_COLOR_HOVERED;
        break;
      default:
        color = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        colorActive = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
        colorHovered = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
        break;
    }

    colorHidden = to_imvec4(to_vec4(color) * COLOR_HIDDEN_MULTIPLIER);
    colorActiveHidden = to_imvec4(to_vec4(colorActive) * COLOR_HIDDEN_MULTIPLIER);
    colorHoveredHidden = to_imvec4(to_vec4(colorHovered) * COLOR_HIDDEN_MULTIPLIER);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());

    if (ImGui::BeginChild("##Frames Child", childSize, ImGuiChildFlags_Borders))
    {
      auto length = animation ? animation->frameNum : anm2.animations.length();
      auto frameSize = ImVec2(ImGui::GetTextLineHeight(), ImGui::GetContentRegionAvail().y);
      auto framesSize = ImVec2(frameSize.x * length, frameSize.y);
      auto cursorPos = ImGui::GetCursorPos();
      auto cursorScreenPos = ImGui::GetCursorScreenPos();
      auto imageSize = vec2(ImGui::GetTextLineHeight());
      auto border = ImGui::GetStyle().FrameBorderSize;
      auto borderLineLength = frameSize.y / 5;
      auto scrollX = ImGui::GetScrollX();
      auto available = ImGui::GetContentRegionAvail();
      auto frameMin = std::max(0, (int)std::floor(scrollX / frameSize.x) - 1);
      auto frameMax = std::min(anm2::FRAME_NUM_MAX, (int)std::ceil(scrollX + available.x / frameSize.x) + 1);
      auto drawList = ImGui::GetWindowDrawList();
      pickerLineDrawList = drawList;

      if (type == anm2::NONE)
      {
        drawList->AddRectFilled(cursorScreenPos,
                                ImVec2(cursorScreenPos.x + framesSize.x, cursorScreenPos.y + framesSize.y),
                                ImGui::GetColorU32(FRAME_TIMELINE_COLOR));

        for (int i = frameMin; i < frameMax; i++)
        {
          auto frameScreenPos = ImVec2(cursorScreenPos.x + frameSize.x * (float)i, cursorScreenPos.y);

          drawList->AddRect(frameScreenPos, ImVec2(frameScreenPos.x + border, frameScreenPos.y + borderLineLength),
                            ImGui::GetColorU32(FRAME_BORDER_COLOR));

          drawList->AddRect(ImVec2(frameScreenPos.x, frameScreenPos.y + frameSize.y - borderLineLength),
                            ImVec2(frameScreenPos.x + border, frameScreenPos.y + frameSize.y),
                            ImGui::GetColorU32(FRAME_BORDER_COLOR));

          if (i % FRAME_MULTIPLE == 0)
          {
            auto string = std::to_string(i);
            auto textSize = ImGui::CalcTextSize(string.c_str());
            auto textPos = ImVec2(frameScreenPos.x + (frameSize.x - textSize.x) / 2,
                                  frameScreenPos.y + (frameSize.y - textSize.y) / 2);

            drawList->AddRectFilled(frameScreenPos,
                                    ImVec2(frameScreenPos.x + frameSize.x, frameScreenPos.y + frameSize.y),
                                    ImGui::GetColorU32(FRAME_MULTIPLE_OVERLAY_COLOR));

            drawList->AddText(textPos, ImGui::GetColorU32(TEXT_MULTIPLE_COLOR), string.c_str());
          }
        }

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseDown(0))
          isDragging = true;

        if (isDragging)
        {
          auto childPos = ImGui::GetWindowPos();
          auto mousePos = ImGui::GetIO().MousePos;
          auto localMousePos = ImVec2(mousePos.x - childPos.x, mousePos.y - childPos.y);
          playback.time = floorf(localMousePos.x / frameSize.x);
          reference.frameTime = playback.time;
        }

        playback.clamp(settings.playbackIsClampPlayhead ? length : anm2::FRAME_NUM_MAX);

        if (ImGui::IsMouseReleased(0)) isDragging = false;

        ImGui::SetCursorPos(ImVec2(cursorPos.x + frameSize.x * floorf(playback.time), cursorPos.y));
        ImGui::Image(resources.icons[icon::PLAYHEAD].id, frameSize);
      }
      else if (animation)
      {
        anm2::Reference itemReference = {reference.animationIndex, type, id};
        if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(0)) reference = itemReference;

        for (int i = frameMin; i < frameMax; i++)
        {
          auto frameScreenPos = ImVec2(cursorScreenPos.x + (frameSize.x * i), cursorScreenPos.y);

          if (i % FRAME_MULTIPLE == 0)
          {
            drawList->AddRectFilled(frameScreenPos,
                                    ImVec2(frameScreenPos.x + frameSize.x, frameScreenPos.y + frameSize.y),
                                    ImGui::GetColorU32(FRAME_MULTIPLE_OVERLAY_COLOR));
          }
          drawList->AddRect(frameScreenPos, ImVec2(frameScreenPos.x + frameSize.x, frameScreenPos.y + frameSize.y),
                            ImGui::GetColorU32(FRAME_BORDER_COLOR));
        }

        auto item = animation->item_get(type, id);

        auto frameTime = 0;
        anm2::Reference baseReference = {reference.animationIndex, reference.itemType, reference.itemID,
                                         reference.frameIndex};

        for (auto [i, frame] : std::views::enumerate(item->frames))
        {
          anm2::Reference frameReference = {reference.animationIndex, type, id, (int)i};
          auto isSelected = baseReference == frameReference;
          auto isFrameVisible = isVisible && frame.isVisible;

          frameTime += frame.delay;

          ImGui::PushID(i);
          auto size = ImVec2(frameSize.x * frame.delay, frameSize.y);

          auto icon = type == anm2::TRIGGER  ? icon::TRIGGER
                      : frame.isInterpolated ? icon::INTERPOLATED
                                             : icon::UNINTERPOLATED;

          if (type == anm2::TRIGGER)
            ImGui::SetCursorPos(ImVec2(cursorPos.x + frameSize.x * frame.atFrame, cursorPos.y));

          ImGui::PushStyleColor(ImGuiCol_Button, isFrameVisible ? color : colorHidden);
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, isFrameVisible ? colorActive : colorActiveHidden);
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, isFrameVisible ? colorHovered : colorHoveredHidden);

          if (isSelected) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

          if (ImGui::Button("##Frame Button", size))
          {
            if (type != anm2::TRIGGER && ImGui::IsKeyDown(ImGuiMod_Alt)) frame.isInterpolated = !frame.isInterpolated;
            if (type == anm2::LAYER) document.referenceSpritesheet = anm2.content.layers[id].spritesheetID;
            reference = frameReference;
            reference.frameTime = frameTime;
          }
          if (type != anm2::TRIGGER) ImGui::SameLine();

          ImGui::PopStyleColor(3);
          if (isSelected) ImGui::PopStyleColor();

          auto imageMin = ImVec2(ImGui::GetItemRectMin().x,
                                 ImGui::GetItemRectMax().y - (ImGui::GetItemRectSize().y / 2) - (imageSize.y / 2));
          auto imageMax = to_imvec2(to_vec2(imageMin) + imageSize);

          drawList->AddImage(resources.icons[icon].id, imageMin, imageMax);

          ImGui::PopID();
        }
      }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    index++;
    ImGui::PopID();
  }

  void Timeline::frames_child(Document& document, anm2::Animation* animation, Settings& settings, Resources& resources,
                              Playback& playback)
  {
    auto& anm2 = document.anm2;

    auto itemsChildWidth = ImGui::GetTextLineHeightWithSpacing() * 15;

    auto cursorPos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(cursorPos.x + itemsChildWidth, cursorPos.y));

    auto framesChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);

    if (ImGui::BeginChild("##Frames Child", framesChildSize, ImGuiChildFlags_Borders))
    {
      auto viewListChildSize =
          ImVec2(ImGui::GetContentRegionAvail().x,
                 ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() - style.ItemSpacing.y * 2);

      auto childWidth = ImGui::GetContentRegionAvail().x > anm2.animations.length() * ImGui::GetTextLineHeight()
                            ? ImGui::GetContentRegionAvail().x
                            : anm2.animations.length() * ImGui::GetTextLineHeight();

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
      if (ImGui::BeginChild("##Frames List Child", viewListChildSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        auto cursorScreenPos = ImGui::GetCursorScreenPos();

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2());
        if (ImGui::BeginTable("##Frames List Table", 1,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY))
        {
          ImGuiWindow* window = ImGui::GetCurrentWindow();
          window->Flags |= ImGuiWindowFlags_NoScrollWithMouse;

          scroll.x = ImGui::GetScrollX();
          scroll.y = ImGui::GetScrollY();

          isHorizontalScroll = window->ScrollbarX;

          if (isWindowHovered)
          {
            auto& io = ImGui::GetIO();
            auto lineHeight = ImGui::GetTextLineHeightWithSpacing() * 2;

            scroll.x -= io.MouseWheelH * lineHeight;
            scroll.y -= io.MouseWheel * lineHeight;
          }

          ImGui::SetScrollX(scroll.x);
          ImGui::SetScrollY(scroll.y);

          int index{};

          ImGui::TableSetupScrollFreeze(0, 1);
          ImGui::TableSetupColumn("##Frames");

          auto frames_child_row = [&](anm2::Type type, int id = -1)
          {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            frame_child(document, animation, settings, resources, playback, type, id, index, childWidth);
          };

          frames_child_row(anm2::NONE);

          if (animation)
          {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

            frames_child_row(anm2::ROOT);

            for (auto& id : animation->layerOrder)
            {
              if (auto item = animation->item_get(anm2::LAYER, id); item)
                if (!settings.timelineIsShowUnused && item->frames.empty()) continue;

              frames_child_row(anm2::LAYER, id);
            }

            for (auto& id : animation->nullAnimations | std::views::keys)
            {
              if (auto item = animation->item_get(anm2::NULL_, id); item)
                if (!settings.timelineIsShowUnused && item->frames.empty()) continue;
              frames_child_row(anm2::NULL_, id);
            }

            frames_child_row(anm2::TRIGGER);

            ImGui::PopStyleVar();
          }
          ImGui::EndTable();
        }

        ImDrawList* windowDrawList = ImGui::GetWindowDrawList();

        auto frameSize = ImVec2(ImGui::GetTextLineHeight(),
                                ImGui::GetTextLineHeightWithSpacing() + (ImGui::GetStyle().WindowPadding.y * 2));
        auto linePos = ImVec2(cursorScreenPos.x + frameSize.x * floorf(playback.time) + (frameSize.x / 2) - scroll.x,
                              cursorScreenPos.y + frameSize.y);
        auto lineSize =
            ImVec2((PLAYHEAD_LINE_THICKNESS / 2.0f),
                   viewListChildSize.y - frameSize.y - (isHorizontalScroll ? ImGui::GetStyle().ScrollbarSize : 0.0f));

        auto rectMin = windowDrawList->GetClipRectMin();
        auto rectMax = windowDrawList->GetClipRectMax();
        pickerLineDrawList->PushClipRect(rectMin, rectMax);
        pickerLineDrawList->AddRectFilled(linePos, ImVec2(linePos.x + lineSize.x, linePos.y + lineSize.y),
                                          ImGui::GetColorU32(PLAYHEAD_LINE_COLOR));
        pickerLineDrawList->PopClipRect();

        ImGui::PopStyleVar();
      }
      ImGui::EndChild();
      ImGui::PopStyleVar();

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

      ImGui::SetCursorPos(
          ImVec2(ImGui::GetStyle().WindowPadding.x, ImGui::GetCursorPos().y + ImGui::GetStyle().ItemSpacing.y));

      auto widgetSize = imgui::widget_size_with_row_get(9);

      ImGui::BeginDisabled(!animation);
      {
        auto label = playback.isPlaying ? "Pause" : "Play";
        auto tooltip = playback.isPlaying ? "Pause the animation." : "Play the animation.";

        imgui::shortcut(settings.shortcutPlayPause);
        if (ImGui::Button(label, widgetSize)) playback.toggle();
        imgui::set_item_tooltip_shortcut(tooltip, settings.shortcutPlayPause);

        ImGui::SameLine();

        imgui::shortcut(settings.shortcutAdd);
        ImGui::Button("Insert Frame", widgetSize);
        imgui::set_item_tooltip_shortcut("Insert a frame, based on the current selection.", settings.shortcutAdd);

        ImGui::SameLine();

        imgui::shortcut(settings.shortcutRemove);
        ImGui::Button("Delete Frame", widgetSize);
        imgui::set_item_tooltip_shortcut("Delete the selected frames.", settings.shortcutRemove);

        ImGui::SameLine();

        ImGui::Button("Bake", widgetSize);
        ImGui::SetItemTooltip("%s", "Turn interpolated frames into uninterpolated ones.");

        ImGui::SameLine();

        if (ImGui::Button("Fit Animation Length", widgetSize)) animation->frameNum = animation->length();
        ImGui::SetItemTooltip("%s", "The animation length will be set to the effective length of the animation.");

        ImGui::SameLine();

        ImGui::SetNextItemWidth(widgetSize.x);
        ImGui::InputInt("Animation Length", animation ? &animation->frameNum : &dummy_value<int>(), step::NORMAL,
                        step::FAST, !animation ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0);
        if (animation) animation->frameNum = clamp(animation->frameNum, anm2::FRAME_NUM_MIN, anm2::FRAME_NUM_MAX);
        ImGui::SetItemTooltip("%s", "Set the animation's length.");

        ImGui::SameLine();

        ImGui::SetNextItemWidth(widgetSize.x);
        ImGui::Checkbox("Loop", animation ? &animation->isLoop : &dummy_value<bool>());
        ImGui::SetItemTooltip("%s", "Toggle the animation looping.");
      }
      ImGui::EndDisabled();

      ImGui::SameLine();

      ImGui::SetNextItemWidth(widgetSize.x);
      ImGui::InputInt("FPS", &anm2.info.fps, 1, 5);
      anm2.info.fps = clamp(anm2.info.fps, anm2::FPS_MIN, anm2::FPS_MAX);
      ImGui::SetItemTooltip("%s", "Set the FPS of all animations.");

      ImGui::SameLine();

      ImGui::SetNextItemWidth(widgetSize.x);
      imgui::input_text_string("Author", &anm2.info.createdBy);
      ImGui::SetItemTooltip("%s", "Set the author of the document.");

      ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    ImGui::SetCursorPos(cursorPos);
  }

  void Timeline::popups(Document& document, anm2::Animation* animation, Settings& settings)
  {
    auto item_properties_reset = [&]()
    {
      addItemName.clear();
      addItemSpritesheetID = {};
      addItemID = -1;
      isUnusedItemsSet = false;
    };

    auto& anm2 = document.anm2;
    auto& reference = document.reference;

    propertiesPopup.trigger();

    if (ImGui::BeginPopupModal(propertiesPopup.label, &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto item_properties_close = [&]()
      {
        item_properties_reset();
        propertiesPopup.close();
      };

      auto& type = settings.timelineAddItemType;
      auto& locale = settings.timelineAddItemLocale;
      auto& source = settings.timelineAddItemSource;

      if (!isUnusedItemsSet)
      {
        unusedItems = type == anm2::LAYER   ? anm2.layers_unused(reference)
                      : type == anm2::NULL_ ? anm2.nulls_unused(reference)
                                            : std::set<int>{};

        isUnusedItemsSet = true;
      }

      auto footerSize = imgui::footer_size_get();
      auto optionsSize = imgui::child_size_get(11);
      auto itemsSize = ImVec2(0, ImGui::GetContentRegionAvail().y -
                                     (optionsSize.y + footerSize.y + ImGui::GetStyle().ItemSpacing.y * 4));

      if (ImGui::BeginChild("Options", optionsSize, ImGuiChildFlags_Borders))
      {
        ImGui::SeparatorText("Type");

        auto size = ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetFrameHeightWithSpacing());

        if (ImGui::BeginChild("Type Layer", size))
        {
          ImGui::RadioButton("Layer", &type, anm2::LAYER);
          ImGui::SetItemTooltip("Layers are a basic visual element in an animation, used for displaying spritesheets.");
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("Type Null", size))
        {
          ImGui::RadioButton("Null", &type, anm2::NULL_);
          ImGui::SetItemTooltip(
              "Nulls are invisible elements in an animation, used for interfacing with a game engine.");
        }
        ImGui::EndChild();

        ImGui::SeparatorText("Source");

        bool isNewOnly = unusedItems.empty();
        if (isNewOnly) source = source::NEW;

        if (ImGui::BeginChild("Source New", size))
        {
          ImGui::RadioButton("New", &source, source::NEW);
          ImGui::SetItemTooltip("Create a new item to be used.");
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("Source Existing", size))
        {
          ImGui::BeginDisabled(isNewOnly);
          ImGui::RadioButton("Existing", &source, source::EXISTING);
          ImGui::EndDisabled();
          ImGui::SetItemTooltip("Use a pre-existing, presently unused item.");
        }
        ImGui::EndChild();

        ImGui::SeparatorText("Locale");

        if (ImGui::BeginChild("Locale Global", size))
        {
          ImGui::RadioButton("Global", &locale, locale::GLOBAL);
          ImGui::SetItemTooltip("The item will be inserted into all animations, if not already present.");
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("Locale Local", size))
        {
          ImGui::RadioButton("Local", &locale, locale::LOCAL);
          ImGui::SetItemTooltip("The item will only be inserted into this animation.");
        }
        ImGui::EndChild();

        ImGui::SeparatorText("Options");

        ImGui::BeginDisabled(source == source::EXISTING);
        {
          imgui::input_text_string("Name", &addItemName);
          ImGui::SetItemTooltip("Set the item's name.");
          ImGui::BeginDisabled(type != anm2::LAYER);
          {
            auto spritesheets = anm2.spritesheet_names_get();
            imgui::combo_strings("Spritesheet", &addItemSpritesheetID, spritesheets);
            ImGui::SetItemTooltip("Set the layer item's spritesheet.");
          }
          ImGui::EndDisabled();
        }
        ImGui::EndDisabled();
      }
      ImGui::EndChild();

      if (ImGui::BeginChild("Items", itemsSize, ImGuiChildFlags_Borders))
      {
        if (animation && source == source::EXISTING)
        {
          for (auto id : unusedItems)
          {
            auto isSelected = addItemID == id;

            ImGui::PushID(id);

            if (type == anm2::LAYER)
            {
              auto& layer = anm2.content.layers[id];
              if (ImGui::Selectable(
                      std::format("#{} {} (Spritesheet: #{})", id, layer.name, layer.spritesheetID).c_str(),
                      isSelected))
                addItemID = id;
            }
            else if (type == anm2::NULL_)
            {
              auto& null = anm2.content.nulls[id];
              if (ImGui::Selectable(std::format("#{} {}", id, null.name).c_str(), isSelected)) addItemID = id;
            }

            ImGui::PopID();
          }
        }
      }
      ImGui::EndChild();

      auto widgetSize = imgui::widget_size_with_row_get(2);

      if (ImGui::Button("Add", widgetSize))
      {
        anm2::Reference addReference;

        if (type == anm2::LAYER)
          addReference = anm2.layer_add({reference.animationIndex, anm2::LAYER, addItemID}, addItemName,
                                        addItemSpritesheetID, (locale::Type)locale);
        else if (type == anm2::NULL_)
          addReference =
              anm2.null_add({reference.animationIndex, anm2::LAYER, addItemID}, addItemName, (locale::Type)locale);

        reference = addReference;

        item_properties_close();
      }
      ImGui::SetItemTooltip("Add the item, with the settings specified.");

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) item_properties_close();
      ImGui::SetItemTooltip("Cancel adding an item.");

      ImGui::EndPopup();
    }
  }

  void Timeline::update(DocumentManager& manager, Settings& settings, Resources& resources, Playback& playback)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& reference = document.reference;
    auto animation = document.animation_get();

    style = ImGui::GetStyle();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
    if (ImGui::Begin("Timeline", &settings.windowIsTimeline))
    {
      isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
      frames_child(document, animation, settings, resources, playback);
      items_child(document, animation, settings, resources);
    }
    ImGui::PopStyleVar();
    ImGui::End();

    popups(document, animation, settings);

    if (imgui::shortcut(settings.shortcutPlayPause, shortcut::GLOBAL)) playback.toggle();

    if (animation)
    {
      if (imgui::chord_repeating(imgui::string_to_chord(settings.shortcutPreviousFrame)))
      {
        playback.decrement(settings.playbackIsClampPlayhead ? animation->frameNum : anm2::FRAME_NUM_MAX);
        reference.frameTime = playback.time;
      }

      if (imgui::chord_repeating(imgui::string_to_chord(settings.shortcutNextFrame)))
      {
        playback.increment(settings.playbackIsClampPlayhead ? animation->frameNum : anm2::FRAME_NUM_MAX);
        reference.frameTime = playback.time;
      }
    }

    if (imgui::chord_repeating(imgui::string_to_chord(settings.shortcutShortenFrame)))
      if (auto frame = anm2.frame_get(reference); frame) frame->shorten();

    if (imgui::chord_repeating(imgui::string_to_chord(settings.shortcutExtendFrame)))
      if (auto frame = anm2.frame_get(reference); frame) frame->extend();
  }
}