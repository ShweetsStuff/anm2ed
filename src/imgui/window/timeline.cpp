#include "timeline.h"

#include <ranges>

#include <imgui_internal.h>

#include "toast.h"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace glm;

namespace anm2ed::imgui
{
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
- Press {} to shorten the selected frame, by one frame.
- Press {} to extend the selected frame, by one frame.
- Hold Alt while clicking a non-trigger frame to toggle interpolation.)";

  void Timeline::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& playback = document.playback;
    auto& reference = document.reference;
    auto animation = document.animation_get();

    style = ImGui::GetStyle();

    auto context_menu = [&]()
    {
      auto& hoveredFrame = document.hoveredFrame;
      auto& anm2 = document.anm2;

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      auto copy = [&]()
      {
        if (auto frame = anm2.frame_get(hoveredFrame)) clipboard.set(frame->to_string(hoveredFrame.itemType));
      };

      auto cut = [&]()
      {
        copy();
        auto frames_delete = [&]()
        {
          if (auto item = anm2.item_get(reference); item)
          {
            item->frames.erase(item->frames.begin() + reference.frameIndex);
            reference.frameIndex = glm::max(-1, --reference.frameIndex);
          }
        };

        DOCUMENT_EDIT(document, "Cut Frame(s)", Document::FRAMES, frames_delete());
      };

      auto paste = [&]()
      {
        if (auto item = document.item_get())
        {
          document.snapshot("Paste Frame(s)");
          std::set<int> indices{};
          std::string errorString{};
          auto start = reference.frameIndex + 1;
          if (item->frames_deserialize(clipboard.get(), reference.itemType, start, indices, &errorString))
            document.change(Document::FRAMES);
          else
            toasts.error(std::format("Failed to deserialize frame(s): {}", errorString));
        }
        else
          toasts.error(std::format("Failed to deserialize frame(s): select an item first!"));
      };

      if (shortcut(settings.shortcutCut, shortcut::FOCUSED)) cut();
      if (shortcut(settings.shortcutCopy, shortcut::FOCUSED)) copy();
      if (shortcut(settings.shortcutPaste, shortcut::FOCUSED)) paste();

      if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
      {
        if (ImGui::MenuItem("Cut", settings.shortcutCut.c_str(), false, hoveredFrame != anm2::Reference{})) cut();
        if (ImGui::MenuItem("Copy", settings.shortcutCopy.c_str(), false, hoveredFrame != anm2::Reference{})) copy();
        if (ImGui::MenuItem("Paste", nullptr, false, !clipboard.is_empty())) paste();
        ImGui::EndPopup();
      }

      ImGui::PopStyleVar(2);
    };

    auto item_child = [&](anm2::Type type, int id, int& index)
    {
      auto& anm2 = document.anm2;

      auto item = animation ? animation->item_get(type, id) : nullptr;
      auto isVisible = item ? item->isVisible : false;
      auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;
      if (isOnlyShowLayers && type != anm2::LAYER) isVisible = false;
      auto isActive = reference.itemType == type && reference.itemID == id;

      auto label = type == anm2::LAYER   ? std::format(anm2::LAYER_FORMAT, id, anm2.content.layers.at(id).name,
                                                       anm2.content.layers[id].spritesheetID)
                   : type == anm2::NULL_ ? std::format(anm2::NULL_FORMAT, id, anm2.content.nulls[id].name)
                                         : anm2::TYPE_STRINGS[type];
      auto icon = anm2::TYPE_ICONS[type];
      auto color = to_imvec4(isActive ? anm2::TYPE_COLOR_ACTIVE[type] : anm2::TYPE_COLOR[type]);
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

          if (ImGui::IsWindowHovered())
          {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
              switch (type)
              {
                case anm2::LAYER:
                  manager.layer_properties_open(id);
                  break;
                case anm2::NULL_:
                  manager.null_properties_open(id);
                default:
                  break;
              }
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) reference = itemReference;
          }

          ImGui::Image(resources.icons[icon].id, icon_size_get());
          ImGui::SameLine();
          ImGui::TextUnformatted(label.c_str());

          anm2::Item* itemPtr = animation->item_get(type, id);
          bool& itemVisible = itemPtr->isVisible;

          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
          ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());

          ImGui::SetCursorPos(
              ImVec2(itemSize.x - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x,
                     (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
          int visibleIcon = itemVisible ? icon::VISIBLE : icon::INVISIBLE;

          if (ImGui::ImageButton("##Visible Toggle", resources.icons[visibleIcon].id, icon_size_get()))
            DOCUMENT_EDIT(document, "Item Visibility", Document::FRAMES, itemVisible = !itemVisible);
          ImGui::SetItemTooltip(itemVisible ? "The item is shown. Press to hide."
                                            : "The item is hidden. Press to show.");

          if (type == anm2::NULL_)
          {
            auto& null = anm2.content.nulls.at(id);
            auto& isShowRect = null.isShowRect;

            auto rectIcon = isShowRect ? icon::SHOW_RECT : icon::HIDE_RECT;
            ImGui::SetCursorPos(
                ImVec2(itemSize.x - (ImGui::GetTextLineHeightWithSpacing() * 2) - ImGui::GetStyle().ItemSpacing.x,
                       (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
            if (ImGui::ImageButton("##Rect Toggle", resources.icons[rectIcon].id, icon_size_get()))
              DOCUMENT_EDIT(document, "Null Rect", Document::FRAMES, null.isShowRect = !null.isShowRect);
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

          ImGui::SetCursorPos(
              ImVec2(itemSize.x - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x,
                     (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));

          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
          ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());

          auto unusedIcon = isShowUnused ? icon::SHOW_UNUSED : icon::HIDE_UNUSED;
          if (ImGui::ImageButton("##Unused Toggle", resources.icons[unusedIcon].id, icon_size_get()))
            isShowUnused = !isShowUnused;
          ImGui::SetItemTooltip(isShowUnused ? "Unused layers/nulls are shown. Press to hide."
                                             : "Unused layers/nulls are hidden. Press to show.");

          auto& showLayersOnly = settings.timelineIsOnlyShowLayers;
          auto layersIcon = showLayersOnly ? icon::SHOW_LAYERS : icon::HIDE_LAYERS;

          ImGui::SetCursorPos(
              ImVec2(itemSize.x - (ImGui::GetTextLineHeightWithSpacing() * 2) - ImGui::GetStyle().ItemSpacing.x,
                     (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));

          if (ImGui::ImageButton("##Layers Toggle", resources.icons[layersIcon].id, icon_size_get()))
            showLayersOnly = !showLayersOnly;
          ImGui::SetItemTooltip(showLayersOnly ? "Only layers are visible. Press to show all items."
                                               : "All items are visible. Press to only show layers.");

          ImGui::PopStyleVar();
          ImGui::PopStyleColor(3);

          ImGui::SetCursorPos(cursorPos);

          ImGui::BeginDisabled();
          ImGui::Text("(?)");
          ImGui::SetItemTooltip("%s",
                                std::format(HELP_FORMAT, settings.shortcutNextFrame, settings.shortcutPreviousFrame,
                                            settings.shortcutShortenFrame, settings.shortcutExtendFrame)
                                    .c_str());
          ImGui::EndDisabled();
        }
      }
      ImGui::EndChild();
      ImGui::PopStyleColor();
      ImGui::PopStyleVar(2);
      index++;
    };

    auto items_child = [&]()
    {
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
              item_child(type, id, index);
            };

            item_child_row(anm2::NONE);

            if (animation)
            {
              item_child_row(anm2::ROOT);

              for (auto& id : animation->layerOrder)
              {
                if (!settings.timelineIsShowUnused && animation->layerAnimations[id].frames.empty()) continue;
                item_child_row(anm2::LAYER, id);
              }

              for (auto& [id, nullAnimation] : animation->nullAnimations)
              {
                if (!settings.timelineIsShowUnused && nullAnimation.frames.empty()) continue;
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

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + style.WindowPadding.x, ImGui::GetCursorPosY()));
        auto widgetSize = widget_size_with_row_get(2, ImGui::GetContentRegionAvail().x - style.WindowPadding.x);

        ImGui::BeginDisabled(!animation);
        {
          shortcut(settings.shortcutAdd);
          if (ImGui::Button("Add", widgetSize)) propertiesPopup.open();
          set_item_tooltip_shortcut("Add a new item to the animation.", settings.shortcutAdd);
          ImGui::SameLine();

          ImGui::BeginDisabled(!document.item_get() && reference.itemType != anm2::LAYER &&
                               reference.itemType != anm2::NULL_);
          {
            shortcut(settings.shortcutRemove);
            if (ImGui::Button("Remove", widgetSize))
            {
              auto remove = [&]()
              {
                animation->item_remove(reference.itemType, reference.itemID);
                reference = {reference.animationIndex};
              };

              DOCUMENT_EDIT(document, "Remove Item", Document::ITEMS, remove());
            }
            set_item_tooltip_shortcut("Remove the selected item(s) from the animation.", settings.shortcutRemove);
          }
          ImGui::EndDisabled();
        }
        ImGui::EndDisabled();

        ImGui::PopStyleVar();
      }
      ImGui::EndChild();
    };

    auto frame_child = [&](anm2::Type type, int id, int& index, float width)
    {
      auto& anm2 = document.anm2;
      auto& hoveredFrame = document.hoveredFrame;

      auto item = animation ? animation->item_get(type, id) : nullptr;
      auto isVisible = item ? item->isVisible : false;
      auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;
      if (isOnlyShowLayers && type != anm2::LAYER) isVisible = false;

      auto color = to_imvec4(anm2::TYPE_COLOR[type]);
      auto colorActive = to_imvec4(anm2::TYPE_COLOR_ACTIVE[type]);
      auto colorHovered = to_imvec4(anm2::TYPE_COLOR_HOVERED[type]);
      auto colorHidden = to_imvec4(to_vec4(color) * COLOR_HIDDEN_MULTIPLIER);
      auto colorActiveHidden = to_imvec4(to_vec4(colorActive) * COLOR_HIDDEN_MULTIPLIER);
      auto colorHoveredHidden = to_imvec4(to_vec4(colorHidden) * COLOR_HIDDEN_MULTIPLIER);

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

      auto childSize = ImVec2(width, ImGui::GetTextLineHeightWithSpacing() + (ImGui::GetStyle().WindowPadding.y * 2));

      ImGui::PopStyleVar(2);

      ImGui::PushID(index);

      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());

      if (ImGui::BeginChild("##Frames Child", childSize, ImGuiChildFlags_Borders))
      {
        auto length = animation ? animation->frameNum : anm2.animations.length();
        auto frameSize = ImVec2(ImGui::GetTextLineHeight(), ImGui::GetContentRegionAvail().y);
        auto framesSize = ImVec2(frameSize.x * length, frameSize.y);
        auto cursorPos = ImGui::GetCursorPos();
        auto cursorScreenPos = ImGui::GetCursorScreenPos();
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
          anm2::Reference itemReference{reference.animationIndex, type, id};

          ImGui::PushStyleColor(ImGuiCol_ButtonActive, isVisible ? colorActive : colorActiveHidden);
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, isVisible ? colorHovered : colorHoveredHidden);

          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

          float frameTime{};

          for (auto [i, frame] : std::views::enumerate(item->frames))
          {
            ImGui::PushID(i);

            auto frameReference =
                anm2::Reference(itemReference.animationIndex, itemReference.itemType, itemReference.itemID, i);
            auto isSelected = reference == frameReference;
            vec2 frameMin = {frameTime * frameSize.x, cursorPos.y};
            vec2 frameMax = {frameMin.x + frame.delay * frameSize.x, frameMin.y + frameSize.y};
            auto buttonSize = to_imvec2(frameMax - frameMin);
            auto buttonPos = ImVec2(cursorPos.x + frameMin.x, cursorPos.y);
            ImGui::SetCursorPos(buttonPos);
            ImGui::PushStyleColor(ImGuiCol_Button, isSelected && isVisible    ? colorActive
                                                   : isSelected && !isVisible ? colorActiveHidden
                                                   : isVisible                ? color
                                                                              : colorHidden);
            if (ImGui::Button("##Frame Button", buttonSize))
            {
              if (type == anm2::LAYER)
              {
                document.spritesheet.reference = anm2.content.layers[id].spritesheetID;
                document.layer.selection = {id};
              }
              reference = frameReference;
              reference.frameTime = frameTime;

              if (ImGui::IsKeyDown(ImGuiMod_Alt))
                DOCUMENT_EDIT(document, "Frame Interpolation", Document::FRAMES,
                              frame.isInterpolated = !frame.isInterpolated);
            }
            if (ImGui::IsItemHovered()) hoveredFrame = frameReference;
            ImGui::PopStyleColor();
            auto icon = type == anm2::TRIGGER  ? icon::TRIGGER
                        : frame.isInterpolated ? icon::INTERPOLATED
                                               : icon::UNINTERPOLATED;
            auto iconPos = ImVec2(cursorPos.x + (frameTime * frameSize.x),
                                  cursorPos.y + (frameSize.y / 2) - (icon_size_get().y / 2));
            ImGui::SetCursorPos(iconPos);
            ImGui::Image(resources.icons[icon].id, icon_size_get());

            frameTime += frame.delay;

            ImGui::PopID();
          }

          ImGui::PopStyleVar();
          ImGui::PopStyleColor(2);
        }
      }
      ImGui::EndChild();
      ImGui::PopStyleVar();

      index++;
      ImGui::PopID();
    };

    auto frames_child = [&]()
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
              frame_child(type, id, index, childWidth);
            };

            frames_child_row(anm2::NONE);

            if (animation)
            {
              ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

              frames_child_row(anm2::ROOT);

              for (auto& id : animation->layerOrder)
              {
                if (auto itemPtr = animation->item_get(anm2::LAYER, id); itemPtr)
                  if (!settings.timelineIsShowUnused && itemPtr->frames.empty()) continue;

                frames_child_row(anm2::LAYER, id);
              }

              for (auto& id : animation->nullAnimations | std::views::keys)
              {
                if (auto itemPtr = animation->item_get(anm2::NULL_, id); itemPtr)
                  if (!settings.timelineIsShowUnused && itemPtr->frames.empty()) continue;
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

          context_menu();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

        ImGui::SetCursorPos(
            ImVec2(ImGui::GetStyle().WindowPadding.x, ImGui::GetCursorPos().y + ImGui::GetStyle().ItemSpacing.y));

        auto widgetSize = widget_size_with_row_get(10);

        ImGui::BeginDisabled(!animation);
        {
          auto label = playback.isPlaying ? "Pause" : "Play";
          auto tooltip = playback.isPlaying ? "Pause the animation." : "Play the animation.";

          shortcut(settings.shortcutPlayPause);
          if (ImGui::Button(label, widgetSize)) playback.toggle();
          set_item_tooltip_shortcut(tooltip, settings.shortcutPlayPause);

          ImGui::SameLine();

          auto itemPtr = document.item_get();

          ImGui::BeginDisabled(!itemPtr);
          {
            shortcut(settings.shortcutAdd);
            if (ImGui::Button("Insert Frame", widgetSize))
            {
              auto insert_frame = [&]()
              {
                auto frame = document.frame_get();
                if (frame)
                {
                  itemPtr->frames.insert(itemPtr->frames.begin() + reference.frameIndex, *frame);
                  reference.frameIndex++;
                }
                else if (!itemPtr->frames.empty())
                {
                  auto lastFrame = itemPtr->frames.back();
                  itemPtr->frames.emplace_back(lastFrame);
                  reference.frameIndex = static_cast<int>(itemPtr->frames.size()) - 1;
                }
              };

              DOCUMENT_EDIT(document, "Insert Frame", Document::FRAMES, insert_frame());
            }
            set_item_tooltip_shortcut("Insert a frame, based on the current selection.", settings.shortcutAdd);

            ImGui::SameLine();

            ImGui::BeginDisabled(!document.frame_get());
            {
              shortcut(settings.shortcutRemove);
              if (ImGui::Button("Delete Frame", widgetSize))
              {
                auto delete_frame = [&]()
                {
                  itemPtr->frames.erase(itemPtr->frames.begin() + reference.frameIndex);
                  reference.frameIndex = glm::max(-1, --reference.frameIndex);
                };

                DOCUMENT_EDIT(document, "Delete Frame(s)", Document::FRAMES, delete_frame());
              }
              set_item_tooltip_shortcut("Delete the selected frames.", settings.shortcutRemove);

              ImGui::SameLine();

              if (ImGui::Button("Bake", widgetSize)) bakePopup.open();
              ImGui::SetItemTooltip("Turn interpolated frames into uninterpolated ones.");
            }
            ImGui::EndDisabled();
          }
          ImGui::EndDisabled();

          ImGui::SameLine();

          if (ImGui::Button("Fit Animation Length", widgetSize)) animation->frameNum = animation->length();
          ImGui::SetItemTooltip("The animation length will be set to the effective length of the animation.");

          ImGui::SameLine();

          ImGui::SetNextItemWidth(widgetSize.x);
          ImGui::InputInt("Animation Length", animation ? &animation->frameNum : &dummy_value<int>(), STEP, STEP_FAST,
                          !animation ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0);
          if (animation) animation->frameNum = clamp(animation->frameNum, anm2::FRAME_NUM_MIN, anm2::FRAME_NUM_MAX);
          ImGui::SetItemTooltip("Set the animation's length.");

          ImGui::SameLine();

          ImGui::SetNextItemWidth(widgetSize.x);
          ImGui::Checkbox("Loop", animation ? &animation->isLoop : &dummy_value<bool>());
          ImGui::SetItemTooltip("Toggle the animation looping.");
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::SetNextItemWidth(widgetSize.x);
        ImGui::InputInt("FPS", &anm2.info.fps, 1, 5);
        anm2.info.fps = clamp(anm2.info.fps, anm2::FPS_MIN, anm2::FPS_MAX);
        ImGui::SetItemTooltip("Set the FPS of all animations.");

        ImGui::SameLine();

        ImGui::SetNextItemWidth(widgetSize.x);
        input_text_string("Author", &anm2.info.createdBy);
        ImGui::SetItemTooltip("Set the author of the document.");

        ImGui::SameLine();

        ImGui::SetNextItemWidth(widgetSize.x);
        ImGui::Checkbox("Sound", &settings.timelineIsSound);
        ImGui::SetItemTooltip("Toggle sounds playing with triggers.\nBind sounds to events in the Events window.");

        ImGui::PopStyleVar();
      }
      ImGui::EndChild();

      ImGui::SetCursorPos(cursorPos);
    };

    auto popups_fn = [&]()
    {
      auto item_properties_reset = [&]()
      {
        addItemName.clear();
        addItemSpritesheetID = {};
        addItemID = -1;
        isUnusedItemsSet = false;
      };

      auto& anm2 = document.anm2;

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

        auto footerSize = footer_size_get();
        auto optionsSize = child_size_get(11);
        auto itemsSize = ImVec2(0, ImGui::GetContentRegionAvail().y -
                                       (optionsSize.y + footerSize.y + ImGui::GetStyle().ItemSpacing.y * 4));
        if (ImGui::BeginChild("Options", optionsSize, ImGuiChildFlags_Borders))
        {
          ImGui::SeparatorText("Type");

          auto size = ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetFrameHeightWithSpacing());

          if (ImGui::BeginChild("Type Layer", size))
          {
            ImGui::RadioButton("Layer", &type, anm2::LAYER);
            ImGui::SetItemTooltip(
                "Layers are a basic visual element in an animation, used for displaying spritesheets.");
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
            input_text_string("Name", &addItemName);
            ImGui::SetItemTooltip("Set the item's name.");
            ImGui::BeginDisabled(type != anm2::LAYER);
            {
              combo_negative_one_indexed("Spritesheet", &addItemSpritesheetID, document.spritesheet.labels);
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

        auto widgetSize = widget_size_with_row_get(2);

        if (ImGui::Button("Add", widgetSize))
        {
          anm2::Reference addReference{};

          document.snapshot("Add Item");
          if (type == anm2::LAYER)
            addReference = anm2.layer_animation_add({reference.animationIndex, anm2::LAYER, addItemID}, addItemName,
                                                    addItemSpritesheetID - 1, (locale::Type)locale);
          else if (type == anm2::NULL_)
            addReference = anm2.null_animation_add({reference.animationIndex, anm2::LAYER, addItemID}, addItemName,
                                                   (locale::Type)locale);

          document.change(Document::ITEMS);

          reference = addReference;

          item_properties_close();
        }
        ImGui::SetItemTooltip("Add the item, with the settings specified.");

        ImGui::SameLine();

        if (ImGui::Button("Cancel", widgetSize)) item_properties_close();
        ImGui::SetItemTooltip("Cancel adding an item.");

        ImGui::EndPopup();
      }

      bakePopup.trigger();

      if (ImGui::BeginPopupModal(bakePopup.label, &bakePopup.isOpen, ImGuiWindowFlags_NoResize))
      {
        auto& interval = settings.bakeInterval;
        auto& isRoundRotation = settings.bakeIsRoundRotation;
        auto& isRoundScale = settings.bakeIsRoundScale;

        auto frame = document.frame_get();

        input_int_range("Interval", interval, anm2::FRAME_DELAY_MIN, frame ? frame->delay : anm2::FRAME_DELAY_MIN);
        ImGui::SetItemTooltip("Set the maximum delay of each frame that will be baked.");

        ImGui::Checkbox("Round Rotation", &isRoundRotation);
        ImGui::SetItemTooltip("Rotation will be rounded to the nearest whole number.");

        ImGui::Checkbox("Round Scale", &isRoundScale);
        ImGui::SetItemTooltip("Scale will be rounded to the nearest whole number.");

        auto widgetSize = widget_size_with_row_get(2);

        if (ImGui::Button("Bake", widgetSize))
        {
          if (auto itemPtr = document.item_get())
            DOCUMENT_EDIT(document, "Bake Frames", Document::FRAMES,
                          itemPtr->frames_bake(reference.frameIndex, interval, isRoundScale, isRoundRotation));
          bakePopup.close();
        }
        ImGui::SetItemTooltip("Bake the selected frame(s) with the options selected.");

        ImGui::SameLine();

        if (ImGui::Button("Cancel", widgetSize)) bakePopup.close();
        ImGui::SetItemTooltip("Cancel baking frames.");

        ImGui::EndPopup();
      }
    };

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
    if (ImGui::Begin("Timeline", &settings.windowIsTimeline))
    {
      isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
      frames_child();
      items_child();
    }
    ImGui::PopStyleVar();
    ImGui::End();

    popups_fn();

    if (shortcut(settings.shortcutPlayPause, shortcut::GLOBAL)) playback.toggle();

    if (animation)
    {
      if (chord_repeating(string_to_chord(settings.shortcutPreviousFrame)))
      {
        playback.decrement(settings.playbackIsClampPlayhead ? animation->frameNum : anm2::FRAME_NUM_MAX);
        reference.frameTime = playback.time;
      }

      if (chord_repeating(string_to_chord(settings.shortcutNextFrame)))
      {
        playback.increment(settings.playbackIsClampPlayhead ? animation->frameNum : anm2::FRAME_NUM_MAX);
        reference.frameTime = playback.time;
      }
    }

    if (ImGui::IsKeyChordPressed(string_to_chord(settings.shortcutShortenFrame))) document.snapshot("Shorten Frame");
    if (chord_repeating(string_to_chord(settings.shortcutShortenFrame)))
    {
      if (auto frame = document.frame_get())
      {
        frame->shorten();
        document.change(Document::FRAMES);
      }
    }

    if (ImGui::IsKeyChordPressed(string_to_chord(settings.shortcutExtendFrame))) document.snapshot("Extend Frame");
    if (chord_repeating(string_to_chord(settings.shortcutExtendFrame)))
    {
      if (auto frame = document.frame_get())
      {
        frame->extend();
        document.change(Document::FRAMES);
      }
    }
  }
}
