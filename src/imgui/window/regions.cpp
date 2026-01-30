#include "regions.h"

#include <ranges>

#include <filesystem>
#include <format>

#include "document.h"
#include "log.h"
#include "map_.h"
#include "math_.h"
#include "path_.h"
#include "strings.h"
#include "toast.h"

#include "../../util/map_.h"

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
        }
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_REMOVE_UNUSED_REGIONS), Document::SPRITESHEETS, behavior());
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
        std::string errorString{};
        document.snapshot(localize.get(EDIT_PASTE_REGIONS));
        if (spritesheet->regions_deserialize(clipboard.get(), merge::APPEND, &errorString))
          document.change(Document::SPRITESHEETS);
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
        auto regionChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing() * 4);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());

        selection.start(spritesheet->regions.size());
        bool isValid = spritesheet->is_valid();
        auto& texture = isValid ? spritesheet->texture : resources.icons[icon::NONE];
        auto tintColor = !isValid ? ImVec4(1.0f, 0.25f, 0.25f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

        for (auto& [id, region] : spritesheet->regions)
        {
          auto nameCStr = region.name.c_str();
          auto isSelected = selection.contains(id);
          auto isReferenced = id == reference;

          ImGui::PushID(id);

          if (ImGui::BeginChild("##Region Child", regionChildSize, ImGuiChildFlags_Borders))
          {
            auto cursorPos = ImGui::GetCursorPos();

            ImGui::SetNextItemSelectionUserData(id);
            ImGui::SetNextItemStorageID(id);
            if (ImGui::Selectable("##Region Selectable", isSelected, 0, regionChildSize))
            {
              reference = id;
              document.reference = {document.reference.animationIndex};
              frame.reference = -1;
              frame.selection.clear();
            }
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
            auto uvMin = region.crop / vec2(texture.size);
            auto uvMax = (region.crop + region.size) / vec2(texture.size);

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
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_PIVOT), std::make_format_args(region.pivot.x, region.pivot.y))
                        .c_str());
              }
              ImGui::EndChild();
              ImGui::EndTooltip();
            }
            ImGui::PopStyleVar(2);

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
            ImGui::TextUnformatted(
                std::vformat(localize.get(FORMAT_SPRITESHEET), std::make_format_args(id, nameCStr)).c_str());
            if (isReferenced) ImGui::PopFont();
          }

          ImGui::EndChild();
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
      auto childSize = child_size_get(4);
      auto& region = reference == -1 ? editRegion : spritesheet->regions.at(reference);

      if (propertiesPopup.isJustOpened) editRegion = anm2::Spritesheet::Region{};

      if (ImGui::BeginChild("##Child", childSize, ImGuiChildFlags_Borders))
      {
        if (propertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
        input_text_string(localize.get(BASIC_NAME), &region.name);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_NAME));

        ImGui::DragFloat2(localize.get(BASIC_CROP), value_ptr(region.crop), DRAG_SPEED, 0.0f, 0.0f,
                          math::vec2_format_get(region.crop));
        ImGui::DragFloat2(localize.get(BASIC_SIZE), value_ptr(region.size), DRAG_SPEED, 0.0f, 0.0f,
                          math::vec2_format_get(region.size));
        ImGui::DragFloat2(localize.get(BASIC_PIVOT), value_ptr(region.pivot), DRAG_SPEED, 0.0f, 0.0f,
                          math::vec2_format_get(region.pivot));
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button(reference == -1 ? localize.get(BASIC_ADD) : localize.get(BASIC_CONFIRM), widgetSize))
      {
        if (reference == -1)
        {
          auto add = [&]()
          {
            auto id = map::next_id_get(spritesheet->regions);
            spritesheet->regions[id] = region;
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

      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) propertiesPopup.close();

      ImGui::EndPopup();
    }

    propertiesPopup.end();
  }
}
