#include "window.hpp"

#include <algorithm>
#include <format>

#include "math.hpp"
#include "path.hpp"
#include "toast.hpp"
#include "util/imgui/draw.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"
#include "vector.hpp"
#include "working_directory.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  constexpr int REGION_CARD_LINES = 2;
  constexpr int REGION_POPUP_ROWS = 5;
  constexpr const char* REGION_DRAG_DROP = "Region Drag Drop";
  constexpr const char* REGION_EXPORT_NAME_DEFAULT = "region";
  constexpr const char* PNG_EXTENSION = ".png";
  constexpr ImVec2 DRAG_TOOLTIP_OFFSET = ImVec2(16.0f, 16.0f);

  constexpr StringType ORIGIN_LABELS[] = {LABEL_REGION_ORIGIN_CUSTOM, LABEL_REGION_ORIGIN_TOP_LEFT,
                                          LABEL_REGION_ORIGIN_CENTER};
  constexpr Origin ORIGIN_COMBO_ORDER[] = {Origin::TOP_LEFT, Origin::CENTER, Origin::CUSTOM};

  struct RegionExportOptions
  {
    int spritesheetId{-1};
    int regionId{-1};
    std::filesystem::path path{};
    bool isMakeSpritesheet{};
    bool isRemoveCurrent{};
  };

  void region_pivot_apply(Element& region)
  {
    if (region.origin == Origin::TOP_LEFT) region.pivot = {};
    if (region.origin == Origin::CENTER) region.pivot = region.size * 0.5f;
  }

  Element* region_spritesheet_get(Document& document)
  {
    return document.anm2.element_get(ElementType::SPRITESHEET, document.spritesheet.reference);
  }

  int region_insert_index_get(Document& document)
  {
    auto spritesheet = region_spritesheet_get(document);
    auto& storage = document.region;
    int index = spritesheet ? (int)spritesheet->children.size() : 0;
    for (int i = 0; spritesheet && i < (int)spritesheet->children.size(); i++)
    {
      const auto& child = spritesheet->children[i];
      if (child.type == ElementType::REGION &&
          (storage.selection.contains(child.id) || (storage.selection.empty() && storage.reference == child.id)))
        index = i + 1;
    }
    return index;
  }

  WindowCardImage region_image_get(Document& document, Resources& resources, const Element& region)
  {
    auto texture = document.texture_get(document.spritesheet.reference);
    auto isValid = texture && texture->is_valid();
    if (!isValid) return {.texture = &resources.icons[icon::NONE], .size = region.size};
    return {.texture = texture,
            .size = region.size,
            .uvMin = region.crop / vec2(texture->size),
            .uvMax = (region.crop + region.size) / vec2(texture->size),
            .isValid = true};
  }

  bool region_pixels_get(const Element& region, const Image& texture, std::vector<uint8_t>& pixels, ivec2& size)
  {
    auto minPoint = ivec2(glm::min(region.crop, region.crop + region.size));
    size = ivec2(glm::max(region.crop, region.crop + region.size)) - minPoint;
    if (size.x <= 0 || size.y <= 0 || texture.size.x <= 0 || texture.size.y <= 0 || texture.pixels.empty())
      return false;

    pixels.assign((size_t)size.x * size.y * image::CHANNELS, 0);
    for (int y = 0; y < size.y; y++)
      for (int x = 0; x < size.x; x++)
      {
        auto source = minPoint + ivec2(x, y);
        if (source.x < 0 || source.y < 0 || source.x >= texture.size.x || source.y >= texture.size.y) continue;
        std::copy_n(texture.pixels.data() + ((size_t)source.y * texture.size.x + source.x) * image::CHANNELS,
                    image::CHANNELS, pixels.data() + ((size_t)y * size.x + x) * image::CHANNELS);
      }
    return true;
  }

  bool region_export(Document& document, const RegionExportOptions& options)
  {
    auto pathString = options.path.empty() ? std::string("in memory") : path::to_utf8(options.path);
    if (options.path.empty() && !options.isMakeSpritesheet)
    {
      toast_log(Level::ERROR, TOAST_EXPORT_REGION_PATH_EMPTY);
      return false;
    }

    auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, options.spritesheetId);
    auto region = spritesheet ? child_id_get(*spritesheet, ElementType::REGION, options.regionId) : nullptr;
    auto texture = document.texture_get(options.spritesheetId);
    std::vector<uint8_t> pixels{};
    ivec2 exportSize{};
    if (!region || !texture || !texture->is_valid() || !region_pixels_get(*region, *texture, pixels, exportSize))
    {
      toast_log(Level::ERROR, TOAST_EXPORT_REGION_FAILED, region ? region->name : std::string(), pathString);
      return false;
    }

    auto sourceRegion = *region;
    auto outputPath = options.path;
    if (!outputPath.empty())
    {
      outputPath = window_asset_path_get(document, outputPath.replace_extension(PNG_EXTENSION));
      pathString = path::to_utf8(outputPath);
      WorkingDirectory workingDirectory(document.directory_get());
      path::ensure_directory(outputPath.parent_path());
      if (!Image::write_pixels_png(outputPath, exportSize, pixels.data()))
      {
        toast_log(Level::ERROR, TOAST_EXPORT_REGION_FAILED, sourceRegion.name, pathString);
        return false;
      }
    }

    if (options.isMakeSpritesheet || options.isRemoveCurrent)
    {
      document.edit_begin(EDIT_EXPORT_REGION, true);

      if (options.isMakeSpritesheet)
      {
        auto spritesheets = document.anm2.element_get(ElementType::SPRITESHEETS);
        auto exported = element_make(ElementType::SPRITESHEET);
        exported.id = element_child_next_id_get(*spritesheets, ElementType::SPRITESHEET);
        exported.path = outputPath;

        auto exportedRegion = sourceRegion;
        exportedRegion.id = 0;
        exportedRegion.crop = {};
        exportedRegion.size = vec2(exportSize);
        region_pivot_apply(exportedRegion);
        exported.children.push_back(exportedRegion);
        spritesheets->children.push_back(exported);

        document.textures[exported.id] = Image(pixels.data(), exportSize);
        document.texturePaths[exported.id] = exported.path;
        document.spritesheet.reference = exported.id;
        document.spritesheet.selection = {exported.id};
        document.region.reference = exportedRegion.id;
        document.region.selection = {exportedRegion.id};
        if (!outputPath.empty())
          document.spritesheet_hash_set_saved(exported.id);
        else
          document.spritesheetSaveHashes[exported.id] = 0;
      }

      if (options.isRemoveCurrent)
      {
        if (auto source = document.anm2.element_get(ElementType::SPRITESHEET, options.spritesheetId))
          element_child_id_erase(*source, ElementType::REGION, options.regionId);
        document.anm2.region_frames_sync(true);
        if (!options.isMakeSpritesheet)
        {
          document.region.reference = -1;
          document.region.selection.clear();
        }
      }
      document.change(Document::SPRITESHEETS);
    }

    toast_log(Level::INFO, TOAST_EXPORT_REGION, sourceRegion.name, pathString);
    return true;
  }

  bool region_drag_drop_update(Window& window, Manager& manager, Document& document, const Element& region, int index)
  {
    auto& selection = document.region.selection;
    if (ImGui::BeginDragDropSource(DRAG_DROP_SOURCE_FLAGS))
    {
      window.dragSelection = selection.contains(region.id) ? std::vector<int>(selection.begin(), selection.end())
                                                           : std::vector<int>{region.id};
      ImGui::SetDragDropPayload(REGION_DRAG_DROP, window.dragSelection.data(),
                                window.dragSelection.size() * sizeof(int));
      ImGui::EndDragDropSource();
    }

    if (!ImGui::BeginDragDropTarget()) return false;
    auto isMoved = false;
    if (auto payload = ImGui::AcceptDragDropPayload(REGION_DRAG_DROP, ImGuiDragDropFlags_AcceptBeforeDelivery |
                                                                          ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
    {
      auto cardMin = ImGui::GetWindowPos();
      auto cardMax = ImVec2(cardMin.x + ImGui::GetWindowSize().x, cardMin.y + ImGui::GetWindowSize().y);
      auto isAfter = is_drop_after(cardMin, cardMax);
      drop_line_draw(ImGui::GetForegroundDrawList(), cardMin, cardMax, isAfter);

      std::vector<int> indices{};
      auto payloadIds = (const int*)payload->Data;
      for (int i = 0; i < payload->DataSize / (int)sizeof(int); i++)
        if (auto orderIndex = vector::find_index(window.order, payloadIds[i]); orderIndex != -1)
          indices.push_back(orderIndex);

      if (!indices.empty() && payload->IsDelivery())
      {
        std::sort(indices.begin(), indices.end());
        manager.command_push(
            {manager.selected,
             [&window, indices, movedIds = window.dragSelection, targetIndex = index + (isAfter ? 1 : 0),
              spritesheetId = document.spritesheet.reference](Manager&, Document& document) mutable
             {
               auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, spritesheetId);
               if (!spritesheet) return;
               document.edit_apply(EDIT_MOVE_REGIONS, window.changeType,
                                   [&](Anm2&)
                                   {
                                     vector::move_indices_to_position(spritesheet->children, indices, targetIndex);
                                     document.region.selection = std::set<int>(movedIds.begin(), movedIds.end());
                                   });
             }});
        isMoved = true;
      }
    }
    ImGui::EndDragDropTarget();
    return isMoved;
  }

  void region_drag_tooltip_update(Window& window, Document& document)
  {
    auto payload = ImGui::GetDragDropPayload();
    auto spritesheet = region_spritesheet_get(document);
    if (!spritesheet || !payload || !payload->IsDataType(REGION_DRAG_DROP) || payload->IsDelivery() ||
        window.dragSelection.empty())
      return;

    if (payload->DataFrameCount != ImGui::GetFrameCount() &&
        ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern | DRAG_DROP_SOURCE_FLAGS))
    {
      ImGui::SetDragDropPayload(REGION_DRAG_DROP, window.dragSelection.data(),
                                window.dragSelection.size() * sizeof(int));
      ImGui::EndDragDropSource();
    }

    auto mousePos = ImGui::GetIO().MousePos;
    ImGui::SetNextWindowPos(ImVec2(mousePos.x + DRAG_TOOLTIP_OFFSET.x, mousePos.y + DRAG_TOOLTIP_OFFSET.y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window.tooltipWindowPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, window.tooltipItemSpacing);
    if (ImGui::BeginTooltip())
    {
      for (auto regionId : window.dragSelection)
        if (auto region = child_id_get(*spritesheet, ElementType::REGION, regionId))
          ImGui::TextUnformatted(region->name.c_str());
      ImGui::EndTooltip();
    }
    ImGui::PopStyleVar(2);
  }

  void region_properties_popup_update(Window& window, Manager& manager, Document& document)
  {
    auto& reference = document.region.reference;
    auto spritesheet = region_spritesheet_get(document);
    auto target = spritesheet && reference != -1 ? child_id_get(*spritesheet, ElementType::REGION, reference) : nullptr;

    window.popup.trigger();
    if (ImGui::BeginPopupModal(window.popup.label(), &window.popup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (!spritesheet || (reference != -1 && !target)) window.popup.close();

      if (window.popup.isJustOpened && !window.isPreserveEditElementOnOpen)
        window.editElement = target ? *target : element_make(ElementType::REGION);
      if (window.popup.isJustOpened) window.isPreserveEditElementOnOpen = false;
      auto& region = window.editElement;

      if (ImGui::BeginChild("##Child", child_size_get(REGION_POPUP_ROWS), ImGuiChildFlags_Borders))
      {
        if (window.popup.isJustOpened) ImGui::SetKeyboardFocusHere();
        input_text_string(localize.get(BASIC_NAME), &region.name);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_NAME));
        ImGui::DragFloat2(localize.get(BASIC_CROP), value_ptr(region.crop), DRAG_SPEED, 0.0f, 0.0f,
                          math::vec2_format_get(region.crop));
        ImGui::DragFloat2(localize.get(BASIC_SIZE), value_ptr(region.size), DRAG_SPEED, 0.0f, 0.0f,
                          math::vec2_format_get(region.size));
        ImGui::BeginDisabled(region.origin != Origin::CUSTOM);
        ImGui::DragFloat2(localize.get(BASIC_PIVOT), value_ptr(region.pivot), DRAG_SPEED, 0.0f, 0.0f,
                          math::vec2_format_get(region.pivot));
        ImGui::EndDisabled();

        const char* originOptions[std::size(ORIGIN_COMBO_ORDER)]{};
        for (int i = 0; i < (int)std::size(ORIGIN_COMBO_ORDER); ++i)
          originOptions[i] = localize.get(ORIGIN_LABELS[(int)ORIGIN_COMBO_ORDER[i]]);
        auto originIndex = (int)(std::ranges::find(ORIGIN_COMBO_ORDER, region.origin) - std::begin(ORIGIN_COMBO_ORDER));
        if (ImGui::Combo(localize.get(LABEL_REGION_PROPERTIES_ORIGIN), &originIndex, originOptions,
                         (int)std::size(originOptions)))
          region.origin = ORIGIN_COMBO_ORDER[originIndex];
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_REGION_PROPERTIES_ORIGIN));
        region_pivot_apply(region);
      }
      ImGui::EndChild();

      auto result = window_popup_buttons_draw(manager, localize.get(reference == -1 ? BASIC_ADD : BASIC_CONFIRM));
      if (result == PopupButton::CONFIRM) window_element_apply_push(window, manager, region, reference);
      if (result != PopupButton::NONE) window.popup.close();
      ImGui::EndPopup();
    }
    window.popup.end();
  }

  void region_export_popup_update(Window& window, Manager& manager, Settings& settings, Resources& resources,
                                  Document& document)
  {
    auto spritesheet = region_spritesheet_get(document);
    auto regionId = window.editId != -1 ? window.editId : document.region.reference;
    auto region = spritesheet && regionId != -1 ? child_id_get(*spritesheet, ElementType::REGION, regionId) : nullptr;

    window.popup2.trigger();
    if (ImGui::BeginPopupModal(window.popup2.label(), &window.popup2.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (!region) window.popup2.close();
      if (region && window.popup2.isJustOpened)
      {
        auto filename = path::from_utf8(region->name).filename();
        settings.exportRegionPath =
            settings.exportRegionPath.parent_path() /
            (filename.empty() ? std::filesystem::path(REGION_EXPORT_NAME_DEFAULT) : filename).concat(PNG_EXTENSION);
      }
      if (window.dialog && window.dialog->is_selected(Dialog::REGION_EXPORT_PATH_SET))
      {
        settings.exportRegionPath = window.dialog->path;
        window.dialog->reset();
      }

      if (ImGui::BeginChild("##Export Region Child", child_size_get(REGION_POPUP_ROWS), ImGuiChildFlags_Borders))
      {
        if (ImGui::ImageButton("##Export Region Path Set", resources.icon_id_get(icon::FOLDER), icon_size_get()) &&
            window.dialog)
          window.dialog->file_save(Dialog::REGION_EXPORT_PATH_SET, settings.exportRegionPath);
        ImGui::SameLine();
        input_text_path(localize.get(LABEL_OUTPUT_PATH), &settings.exportRegionPath);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OUTPUT_PATH));
        ImGui::Checkbox(localize.get(LABEL_EXPORT_MAKE_SPRITESHEET), &settings.isExportRegionMakeSpritesheet);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_EXPORT_MAKE_SPRITESHEET));
        ImGui::Checkbox(localize.get(LABEL_EXPORT_REMOVE_CURRENT_REGION), &settings.isExportRegionRemoveCurrent);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_EXPORT_REMOVE_CURRENT_REGION));
      }
      ImGui::EndChild();

      auto texture = document.texture_get(document.spritesheet.reference);
      auto isValid = region && texture && texture->is_valid() && !texture->pixels.empty() &&
                     (settings.isExportRegionMakeSpritesheet || !settings.exportRegionPath.empty());
      auto result = window_popup_buttons_draw(manager, localize.get(BASIC_CONFIRM), isValid);
      if (result == PopupButton::CONFIRM)
      {
        if (!settings.exportRegionPath.empty()) settings.exportRegionPath.replace_extension(PNG_EXTENSION);
        RegionExportOptions options{.spritesheetId = document.spritesheet.reference,
                                    .regionId = regionId,
                                    .path = settings.exportRegionPath,
                                    .isMakeSpritesheet = settings.isExportRegionMakeSpritesheet,
                                    .isRemoveCurrent = settings.isExportRegionRemoveCurrent};
        manager.command_push({manager.selected, [&window, options](Manager&, Document& document)
                              {
                                if (region_export(document, options) && options.isMakeSpritesheet)
                                  window.newElementId = document.region.reference;
                              }});
      }
      if (result != PopupButton::NONE)
      {
        window.editId = -1;
        window.popup2.close();
      }
      ImGui::EndPopup();
    }
    window.popup2.end();
  }

  Window regions_window_register()
  {
    Window window{};
    window.title = LABEL_REGIONS_WINDOW;
    window.isOpen = &Settings::windowIsRegions;
    window.changeType = Document::SPRITESHEETS;
    window.elementType = ElementType::REGION;
    window.childLabel = "##Regions Child";
    window.cardLines = REGION_CARD_LINES;
    window.unavailableText = TEXT_SELECT_SPRITESHEET;
    window.addEdit = EDIT_ADD_REGION;
    window.propertiesEdit = EDIT_SET_REGION_PROPERTIES;
    window.flags = WINDOW_PROPERTIES | WINDOW_ADD | WINDOW_REMOVE_UNUSED | WINDOW_TRIM | WINDOW_EXPORT | WINDOW_COPY |
                   WINDOW_PASTE;
    window.footer = {{WINDOW_ADD, WINDOW_EXPORT, WINDOW_REMOVE_UNUSED}};
    window.tooltips = {{WINDOW_ADD, TOOLTIP_ADD_REGION},
                       {WINDOW_REMOVE_UNUSED, TOOLTIP_REMOVE_UNUSED_REGIONS},
                       {WINDOW_TRIM, TOOLTIP_TRIM_REGIONS}};
    window.editElement = element_make(ElementType::REGION);
    window.popup = PopupHelper(LABEL_REGION_PROPERTIES, POPUP_SMALL_NO_HEIGHT);
    window.popup2 = PopupHelper(LABEL_EXPORT_REGION, POPUP_SMALL_NO_HEIGHT);
    window.storage_get = [](Document& document) -> Storage& { return document.region; };
    window.container_get = region_spritesheet_get;
    window.insert_index_get = region_insert_index_get;
    window.is_available = [](Window&, Document& document) { return region_spritesheet_get(document) != nullptr; };
    window.card_image_get = region_image_get;
    window.row_drag_drop_update = region_drag_drop_update;
    window.row_select = [](Window&, Document& document, int)
    {
      document.editTarget = Document::EditTarget::REGION;
      document.reference = {document.reference.animationIndex};
      document.frames.reference = -1;
      document.frames.selection.clear();
    };
    window.tooltip_draw = [](Document& document, Resources& resources, const Element& region)
    {
      window_tooltip_image_draw(
          region_image_get(document, resources, region),
          [&]()
          {
            window_tooltip_name_draw(resources, region.name);
            ImGui::TextUnformatted(
                std::vformat(localize.get(FORMAT_CROP), std::make_format_args(region.crop.x, region.crop.y)).c_str());
            ImGui::TextUnformatted(
                std::vformat(localize.get(FORMAT_SIZE), std::make_format_args(region.size.x, region.size.y)).c_str());
            auto originLabel = localize.get(ORIGIN_LABELS[(int)region.origin]);
            ImGui::TextUnformatted(
                (region.origin == Origin::CUSTOM
                     ? std::vformat(localize.get(FORMAT_PIVOT), std::make_format_args(region.pivot.x, region.pivot.y))
                     : std::vformat(localize.get(FORMAT_ORIGIN), std::make_format_args(originLabel)))
                    .c_str());
          });
    };
    window.add = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      document.region.reference = -1;
      window.editElement = element_make(ElementType::REGION);
      window.popup.open();
    };
    window.properties = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& selection = document.region.selection;
      if (selection.size() != 1 || !window_element_get(window, document, *selection.begin())) return;
      document.region.reference = *selection.begin();
      window.popup.open();
    };
    window.export_open = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      window.editId = *document.region.selection.begin();
      window.popup2.open();
    };
    window.remove_unused = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      document.edit_apply(EDIT_REMOVE_UNUSED_REGIONS, window.changeType, [&](Anm2& anm2)
                          { return edit::regions_remove_unused(anm2, document.spritesheet.reference); });
    };
    window.trim = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto& region = document.region;
      document.edit_apply(EDIT_TRIM_REGIONS, window.changeType,
                          [&](Anm2&)
                          {
                            if (!document.regions_trim(document.spritesheet.reference, region.selection)) return;
                            if (region.reference != -1 && !region.selection.contains(region.reference))
                              region.reference = *region.selection.begin();
                            document.reference = {document.reference.animationIndex};
                            document.frames.reference = -1;
                            document.frames.selection.clear();
                          });
    };
    window.paste = [](Window& window, Manager&, Settings&, Document& document, Clipboard& clipboard)
    {
      auto spritesheet = region_spritesheet_get(document);
      if (!spritesheet || clipboard.is_empty()) return;
      auto maxIdBefore = element_child_max_id_get(*spritesheet, ElementType::REGION);
      std::string errorString{};
      document.edit_apply(EDIT_PASTE_REGIONS, window.changeType,
                          [&](Anm2& anm2)
                          {
                            edit::regions_paste(anm2, document.spritesheet.reference, clipboard.get(),
                                                region_insert_index_get(document), &errorString);
                            auto target = region_spritesheet_get(document);
                            auto maxIdAfter = target ? element_child_max_id_get(*target, ElementType::REGION) : -1;
                            if (maxIdAfter <= maxIdBefore) return;
                            window.newElementId = maxIdAfter;
                            document.region.selection = {maxIdAfter};
                            document.region.reference = maxIdAfter;
                          });
      if (!errorString.empty()) toast_log(Level::ERROR, TOAST_DESERIALIZE_REGIONS_FAILED, errorString);
    };
    window.rows_update = [](Window& window, Manager& manager, Settings& settings, Resources& resources,
                            Clipboard& clipboard, Document& document)
    {
      window_cards_draw(window, manager, settings, resources, clipboard, document, region_spritesheet_get(document));
      region_drag_tooltip_update(window, document);
    };
    window.popup_update =
        [](Window& window, Manager& manager, Settings& settings, Resources& resources, Clipboard&, Document& document)
    {
      if (manager.isMakeRegionRequested)
      {
        document.spritesheet.reference = manager.makeRegionSpritesheetId;
        document.region.reference = -1;
        window.editElement = manager.makeRegion;
        window.isPreserveEditElementOnOpen = true;
        window.popup.open();
        manager.isMakeRegionRequested = false;
      }
      region_properties_popup_update(window, manager, document);
      region_export_popup_update(window, manager, settings, resources, document);
    };
    return window;
  }
}
