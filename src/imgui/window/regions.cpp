#include "regions.hpp"

#include <algorithm>
#include <ranges>

#include <format>

<<<<<<< HEAD
#include "document.hpp"
#include "log.hpp"
#include "map_.hpp"
#include "math_.hpp"
#include "strings.hpp"
#include "toast.hpp"
#include "vector_.hpp"
=======
#include "document.h"
#include "log.h"
#include "map_.h"
#include "math_.h"
#include "strings.h"
#include "toast.h"
#include "vector_.h"
>>>>>>> f58d894 (Render animation fixes, vs code tasks)

#include "../../util/map_.hpp"

using namespace anm2ed::types;
using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  void Regions::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& selection = document.region.selection;
    auto& frame = document.frames;
    auto& spritesheetReference = document.spritesheet.reference;
    auto& reference = document.region.reference;
    auto style = ImGui::GetStyle();

    auto spritesheet = map::find(anm2.content.spritesheets, spritesheetReference);

    auto remove_unused = [&]()
    {
      if (!spritesheet) return;

      auto unused = anm2.regions_unused(*spritesheet);
      if (unused.empty()) return;

      auto behavior = [&]()
      {
        for (auto& id : unused)
        {
          for (auto& animation : anm2.animations.items)
            for (auto& layerAnimation : animation.layerAnimations | std::views::values)
              for (auto& frame : layerAnimation.frames)
                if (frame.regionID == id) frame.regionID = -1;

          spritesheet->regions.erase(id);
          auto& order = spritesheet->regionOrder;
          order.erase(std::remove(order.begin(), order.end(), id), order.end());
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_REMOVE_UNUSED_REGIONS), Document::SPRITESHEETS, behavior());
    };

    auto trim = [&]()
    {
      if (!spritesheet || selection.empty()) return;

      auto behavior = [&]()
      {
        if (anm2.regions_trim(spritesheetReference, selection))
        {
          if (reference != -1 && !selection.contains(reference)) reference = *selection.begin();
          document.reference = {document.reference.animationIndex};
          frame.reference = -1;
          frame.selection.clear();
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_TRIM_REGIONS), Document::SPRITESHEETS, behavior());
    };

    auto copy = [&]()
    {
      if (!spritesheet || selection.empty()) return;

      std::string clipboardText{};
      for (auto& id : selection)
        clipboardText += spritesheet->region_to_string(id);
      clipboard.set(clipboardText);
    };

    auto paste = [&]()
    {
      if (!spritesheet || clipboard.is_empty()) return;

      auto behavior = [&]()
      {
        auto maxRegionIdBefore = spritesheet->regions.empty() ? -1 : spritesheet->regions.rbegin()->first;
        std::string errorString{};
        document.snapshot(localize.get(EDIT_PASTE_REGIONS));
        if (spritesheet->regions_deserialize(clipboard.get(), merge::APPEND, &errorString))
        {
          if (!spritesheet->regions.empty())
          {
            auto maxRegionIdAfter = spritesheet->regions.rbegin()->first;
            if (maxRegionIdAfter > maxRegionIdBefore)
            {
              newRegionId = maxRegionIdAfter;
              selection = {maxRegionIdAfter};
              reference = maxRegionIdAfter;
            }
          }
          document.change(Document::SPRITESHEETS);
        }
        else
        {
          toasts.push(std::vformat(localize.get(TOAST_DESERIALIZE_REGIONS_FAILED), std::make_format_args(errorString)));
          logger.error(std::vformat(localize.get(TOAST_DESERIALIZE_REGIONS_FAILED, anm2ed::ENGLISH),
                                    std::make_format_args(errorString)));
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_PASTE_REGIONS), Document::SPRITESHEETS, behavior());
    };

    auto add_open = [&]()
    {
      reference = -1;
      editRegion = anm2::Spritesheet::Region{};
      propertiesPopup.open();
    };

    auto properties_open = [&](int id)
    {
      if (!spritesheet || !spritesheet->regions.contains(id)) return;
      reference = id;
      propertiesPopup.open();
    };

    auto context_menu = [&]()
    {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByWindow) &&
          ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##Region Context Menu");

      if (ImGui::BeginPopup("##Region Context Menu"))
      {
        if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_UNDO), settings.shortcutUndo.c_str(), false,
                            document.is_able_to_undo()))
          document.undo();

        if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_REDO), settings.shortcutRedo.c_str(), false,
                            document.is_able_to_redo()))
          document.redo();

        ImGui::Separator();

        if (ImGui::MenuItem(localize.get(BASIC_PROPERTIES), nullptr, false, selection.size() == 1))
          properties_open(*selection.begin());
        if (ImGui::MenuItem(localize.get(BASIC_ADD), settings.shortcutAdd.c_str())) add_open();
        if (ImGui::MenuItem(localize.get(BASIC_REMOVE_UNUSED), settings.shortcutRemove.c_str())) remove_unused();
        if (ImGui::MenuItem(localize.get(BASIC_TRIM), nullptr, false, !selection.empty())) trim();
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_TRIM_REGIONS));

        ImGui::Separator();

        if (ImGui::MenuItem(localize.get(BASIC_COPY), settings.shortcutCopy.c_str(), false, !selection.empty())) copy();
        if (ImGui::MenuItem(localize.get(BASIC_PASTE), settings.shortcutPaste.c_str(), false, !clipboard.is_empty()))
          paste();

        ImGui::EndPopup();
      }
      ImGui::PopStyleVar(2);
    };

    if (ImGui::Begin(localize.get(LABEL_REGIONS_WINDOW), &settings.windowIsRegions))
    {
      if (!spritesheet)
      {
        ImGui::TextUnformatted(localize.get(TEXT_SELECT_SPRITESHEET));
        ImGui::End();
        return;
      }

      auto childSize = size_without_footer_get();

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
      if (ImGui::BeginChild("##Regions Child", childSize, ImGuiChildFlags_Borders))
      {
        auto regionChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 2);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());

        auto rebuild_order = [&]()
        {
          spritesheet->regionOrder.clear();
          spritesheet->regionOrder.reserve(spritesheet->regions.size());
          for (auto id : spritesheet->regions | std::views::keys)
            spritesheet->regionOrder.push_back(id);
        };
        if (spritesheet->regionOrder.size() != spritesheet->regions.size())
          rebuild_order();
        else
        {
          bool isOrderValid = true;
          for (auto id : spritesheet->regionOrder)
            if (!spritesheet->regions.contains(id))
            {
              isOrderValid = false;
              break;
            }
          if (!isOrderValid) rebuild_order();
        }

        selection.set_index_map(&spritesheet->regionOrder);
        selection.start(spritesheet->regionOrder.size());
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, ImGuiInputFlags_RouteFocused))
        {
          selection.clear();
          for (auto& id : spritesheet->regionOrder)
            selection.insert(id);
        }
        if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused)) selection.clear();
        auto scroll_to_item = [&](float itemHeight, bool isTarget)
        {
          if (!isTarget) return;
          auto windowHeight = ImGui::GetWindowHeight();
          auto targetTop = ImGui::GetCursorPosY();
          auto targetBottom = targetTop + itemHeight;
          auto visibleTop = ImGui::GetScrollY();
          auto visibleBottom = visibleTop + windowHeight;
          if (targetTop < visibleTop)
            ImGui::SetScrollY(targetTop);
          else if (targetBottom > visibleBottom)
            ImGui::SetScrollY(targetBottom - windowHeight);
        };
        int scrollTargetId = -1;
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) || ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)))
        {
          auto& order = spritesheet->regionOrder;
          if (!order.empty())
          {
            int delta = ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) ? -1 : 1;
            int current = reference;
            if (current == -1 && !selection.empty()) current = *selection.begin();
            auto it = std::find(order.begin(), order.end(), current);
            int index = it == order.end() ? 0 : (int)std::distance(order.begin(), it);
            index = std::clamp(index + delta, 0, (int)order.size() - 1);
            int nextId = order[index];
            selection = {nextId};
            reference = nextId;
            document.reference = {document.reference.animationIndex};
            frame.reference = -1;
            frame.selection.clear();
            scrollTargetId = nextId;
          }
        }
        bool isValid = spritesheet->is_valid();
        auto& texture = isValid ? spritesheet->texture : resources.icons[icon::NONE];
        auto tintColor = !isValid ? ImVec4(1.0f, 0.25f, 0.25f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

        for (int i = 0; i < (int)spritesheet->regionOrder.size(); i++)
        {
          int id = spritesheet->regionOrder[i];
          auto regionIt = spritesheet->regions.find(id);
          if (regionIt == spritesheet->regions.end()) continue;
          auto& region = regionIt->second;
          auto isNewRegion = newRegionId == id;
          auto nameCStr = region.name.c_str();
          auto isSelected = selection.contains(id);
          auto isReferenced = id == reference;

          ImGui::PushID(id);

          scroll_to_item(regionChildSize.y, scrollTargetId == id);

          if (ImGui::BeginChild("##Region Child", regionChildSize, ImGuiChildFlags_Borders))
          {
            auto cursorPos = ImGui::GetCursorPos();

            ImGui::SetNextItemSelectionUserData(i);
            ImGui::SetNextItemStorageID(id);
            if (ImGui::Selectable("##Region Selectable", isSelected, 0, regionChildSize))
            {
              reference = id;
              document.reference = {document.reference.animationIndex};
              frame.reference = -1;
              frame.selection.clear();
            }
            if (scrollTargetId == id) ImGui::SetItemDefaultFocus();
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) propertiesPopup.open();

            auto viewport = ImGui::GetMainViewport();
            auto maxPreviewSize = to_vec2(viewport->Size) * 0.5f;
            vec2 regionSize = glm::max(region.size, vec2(1.0f));
            vec2 previewSize = regionSize;
            if (previewSize.x > maxPreviewSize.x || previewSize.y > maxPreviewSize.y)
            {
              auto scale = glm::min(maxPreviewSize.x / previewSize.x, maxPreviewSize.y / previewSize.y);
              previewSize *= scale;
            }
            vec2 uvMin{};
            vec2 uvMax{1.0f, 1.0f};
            if (isValid)
            {
              uvMin = region.crop / vec2(texture.size);
              uvMax = (region.crop + region.size) / vec2(texture.size);
            }

            auto textWidth = ImGui::CalcTextSize(nameCStr).x;
            auto tooltipPadding = style.WindowPadding.x * 4.0f;
            auto minWidth = previewSize.x + style.ItemSpacing.x + textWidth + tooltipPadding;

            ImGui::SetNextWindowSize(ImVec2(minWidth, 0), ImGuiCond_Appearing);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
            if (ImGui::BeginItemTooltip())
            {
              ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
              auto childFlags = ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;
              auto noScrollFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

              if (ImGui::BeginChild("##Region Tooltip Image Child", to_imvec2(previewSize), childFlags, noScrollFlags))
                ImGui::ImageWithBg(texture.id, to_imvec2(previewSize), to_imvec2(uvMin), to_imvec2(uvMax), ImVec4(),
                                   tintColor);
              ImGui::EndChild();
              ImGui::PopStyleVar();

              ImGui::SameLine();

              auto infoChildFlags = ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY;
              if (ImGui::BeginChild("##Region Info Tooltip Child", ImVec2(), infoChildFlags, noScrollFlags))
              {
                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::TextUnformatted(nameCStr);
                ImGui::PopFont();

                ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_CROP), std::make_format_args(region.crop.x, region.crop.y))
                        .c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_SIZE), std::make_format_args(region.size.x, region.size.y))
                        .c_str());
                if (region.origin == anm2::Spritesheet::Region::CUSTOM)
                {
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_PIVOT), std::make_format_args(region.pivot.x, region.pivot.y))
                          .c_str());
                }
                else
                {
                  StringType originString = LABEL_REGION_ORIGIN_CENTER;
                  if (region.origin == anm2::Spritesheet::Region::TOP_LEFT) originString = LABEL_REGION_ORIGIN_TOP_LEFT;
                  auto originLabel = localize.get(originString);
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_ORIGIN), std::make_format_args(originLabel)).c_str());
                }
              }
              ImGui::EndChild();
              ImGui::EndTooltip();
            }
            ImGui::PopStyleVar(2);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
            if (ImGui::BeginDragDropSource())
            {
              static std::vector<int> dragDropSelection{};
              dragDropSelection.assign(selection.begin(), selection.end());
              ImGui::SetDragDropPayload("Region Drag Drop", dragDropSelection.data(),
                                        dragDropSelection.size() * sizeof(int));

              for (auto regionId : dragDropSelection)
              {
                auto dragIt = spritesheet->regions.find(regionId);
                if (dragIt == spritesheet->regions.end()) continue;
                ImGui::TextUnformatted(dragIt->second.name.c_str());
              }
              ImGui::EndDragDropSource();
            }

            ImGui::PopStyleVar(2);

            if (ImGui::BeginDragDropTarget())
            {
              if (auto payload = ImGui::AcceptDragDropPayload("Region Drag Drop"))
              {
                auto payloadIds = (int*)(payload->Data);
                int payloadCount = (int)(payload->DataSize / sizeof(int));
                std::vector<int> indices{};
                indices.reserve(payloadCount);
                for (int payloadIndex = 0; payloadIndex < payloadCount; payloadIndex++)
                {
                  int payloadId = payloadIds[payloadIndex];
                  int index = vector::find_index(spritesheet->regionOrder, payloadId);
                  if (index != -1) indices.push_back(index);
                }
                if (!indices.empty())
                {
                  std::sort(indices.begin(), indices.end());
                  DOCUMENT_EDIT(document, localize.get(EDIT_MOVE_REGIONS), Document::SPRITESHEETS,
                                vector::move_indices(spritesheet->regionOrder, indices, i));
                }
              }
              ImGui::EndDragDropTarget();
            }

            auto imageSize = to_imvec2(vec2(regionChildSize.y));
            auto aspectRatio = region.size.y != 0.0f ? (float)region.size.x / region.size.y : 1.0f;

            if (imageSize.x / imageSize.y > aspectRatio)
              imageSize.x = imageSize.y * aspectRatio;
            else
              imageSize.y = imageSize.x / aspectRatio;

            ImGui::SetCursorPos(cursorPos);
            ImGui::ImageWithBg(texture.id, imageSize, to_imvec2(uvMin), to_imvec2(uvMax), ImVec4(), tintColor);

            ImGui::SetCursorPos(ImVec2(regionChildSize.y + style.ItemSpacing.x,
                                       regionChildSize.y - regionChildSize.y / 2 - ImGui::GetTextLineHeight() / 2));

            if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
            ImGui::TextUnformatted(nameCStr);
            if (isReferenced) ImGui::PopFont();
          }

          ImGui::EndChild();

          if (isNewRegion)
          {
            ImGui::SetScrollHereY(0.5f);
            newRegionId = -1;
          }

          ImGui::PopID();
        }

        ImGui::PopStyleVar();
        selection.finish();

        if (shortcut(manager.chords[SHORTCUT_ADD], shortcut::FOCUSED)) add_open();
        if (shortcut(manager.chords[SHORTCUT_REMOVE], shortcut::FOCUSED)) remove_unused();
        if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
        if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste();
      }
      ImGui::EndChild();
      ImGui::PopStyleVar();

      context_menu();

      auto rowOneWidgetSize = widget_size_with_row_get(2);

      shortcut(manager.chords[SHORTCUT_ADD]);
      if (ImGui::Button(localize.get(BASIC_ADD), rowOneWidgetSize)) add_open();
      set_item_tooltip_shortcut(localize.get(TOOLTIP_ADD_REGION), settings.shortcutAdd);

      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_REMOVE]);
      if (ImGui::Button(localize.get(BASIC_REMOVE_UNUSED), rowOneWidgetSize)) remove_unused();
      set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_UNUSED_REGIONS), settings.shortcutAdd);
    }

    ImGui::End();

    propertiesPopup.trigger();

    if (ImGui::BeginPopupModal(propertiesPopup.label(), &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = child_size_get(5);
      auto& region = reference == -1 ? editRegion : spritesheet->regions.at(reference);

      if (propertiesPopup.isJustOpened) editRegion = anm2::Spritesheet::Region{};

      if (ImGui::BeginChild("##Child", childSize, ImGuiChildFlags_Borders))
      {
        const char* originOptions[] = {localize.get(LABEL_REGION_ORIGIN_TOP_LEFT),
                                       localize.get(LABEL_REGION_ORIGIN_CENTER),
                                       localize.get(LABEL_REGION_ORIGIN_CUSTOM)};

        if (propertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
        input_text_string(localize.get(BASIC_NAME), &region.name);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_NAME));
        ImGui::DragFloat2(localize.get(BASIC_CROP), value_ptr(region.crop), DRAG_SPEED, 0.0f, 0.0f,
                          math::vec2_format_get(region.crop));
        ImGui::DragFloat2(localize.get(BASIC_SIZE), value_ptr(region.size), DRAG_SPEED, 0.0f, 0.0f,
                          math::vec2_format_get(region.size));
        ImGui::BeginDisabled(region.origin != anm2::Spritesheet::Region::CUSTOM);
        ImGui::DragFloat2(localize.get(BASIC_PIVOT), value_ptr(region.pivot), DRAG_SPEED, 0.0f, 0.0f,
                          math::vec2_format_get(region.pivot));
        ImGui::EndDisabled();

        if (ImGui::Combo(localize.get(LABEL_REGION_PROPERTIES_ORIGIN), (int*)&region.origin, originOptions,
                         IM_ARRAYSIZE(originOptions)))
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REGION_PROPERTIES_ORIGIN));

        if (region.origin == anm2::Spritesheet::Region::TOP_LEFT)
          region.pivot = {};
        else if (region.origin == anm2::Spritesheet::Region::ORIGIN_CENTER)
          region.pivot = {(int)(region.size.x / 2.0f), (int)(region.size.y / 2.0f)};
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      shortcut(manager.chords[SHORTCUT_CONFIRM]);
      if (ImGui::Button(reference == -1 ? localize.get(BASIC_ADD) : localize.get(BASIC_CONFIRM), widgetSize))
      {
        if (reference == -1)
        {
          auto add = [&]()
          {
            auto id = map::next_id_get(spritesheet->regions);
            spritesheet->regions[id] = region;
            spritesheet->regionOrder.push_back(id);
            selection = {id};
            newRegionId = id;
          };

          DOCUMENT_EDIT(document, localize.get(EDIT_ADD_REGION), Document::SPRITESHEETS, add());
        }
        else
        {
          auto set = [&]()
          {
            spritesheet->regions.at(reference) = region;
            selection = {reference};
          };

          DOCUMENT_EDIT(document, localize.get(EDIT_SET_REGION_PROPERTIES), Document::SPRITESHEETS, set());
        }

        frame.reference = -1;
        frame.selection.clear();

        propertiesPopup.close();
      }

      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_CANCEL]);
      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) propertiesPopup.close();

      ImGui::EndPopup();
    }

    propertiesPopup.end();
  }
}
