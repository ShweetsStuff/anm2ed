#include "window.hpp"

#include <format>

#include "path.hpp"
#include "toast.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"
#include "working_directory.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed::imgui
{
  constexpr int SPRITESHEET_CARD_LINES = 4;
  constexpr int MERGE_OPTIONS_ROWS = 6;
  constexpr int PADDING_MAX = 100;

  bool is_spritesheet_regions(Document& document, int id)
  {
    auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, id);
    return spritesheet && child_first_get(*spritesheet, ElementType::REGION);
  }

  void spritesheets_save(Document& document, const std::set<int>& ids)
  {
    for (auto id : ids)
    {
      auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, id);
      auto texture = document.texture_get(id);
      if (!spritesheet || !texture) continue;
      auto pathString = path::to_utf8(spritesheet->path);
      WorkingDirectory workingDirectory(document.directory_get());
      path::ensure_directory(spritesheet->path.parent_path());
      auto isSaved = texture->write_png(spritesheet->path);
      if (isSaved) document.spritesheet_hash_set_saved(id);
      toast_log(isSaved ? Level::INFO : Level::ERROR, isSaved ? TOAST_SAVE_SPRITESHEET : TOAST_SAVE_SPRITESHEET_FAILED,
                id, pathString);
    }
  }

  void spritesheet_path_change(Window& window, Manager& manager, int id, const std::filesystem::path& dialogPath,
                               StringType edit, bool isWritten)
  {
    manager.command_push({manager.selected, [&window, id, dialogPath, edit, isWritten](Manager&, Document& document)
                          {
                            auto spritesheet = document.anm2.element_get(ElementType::SPRITESHEET, id);
                            auto texture = document.texture_get(id);
                            if (!spritesheet || (isWritten && !texture)) return;
                            window_edit(document, window.changeType, localize.get(edit),
                                        [&]()
                                        {
                                          auto newPath = window_asset_path_get(document, dialogPath);
                                          auto pathString = path::to_utf8(newPath);
                                          if (isWritten)
                                          {
                                            WorkingDirectory workingDirectory(document.directory_get());
                                            path::ensure_directory(newPath.parent_path());
                                            if (!texture->write_png(newPath))
                                            {
                                              toast_log(Level::ERROR, TOAST_SAVE_SPRITESHEET_FAILED, id, pathString);
                                              return;
                                            }
                                          }
                                          spritesheet->path = newPath;
                                          if (isWritten)
                                            document.texturePaths[id] = newPath;
                                          else
                                            document.texture_reload(id);
                                          document.spritesheet_hash_set_saved(id);
                                          toast_log(Level::INFO,
                                                    isWritten ? TOAST_SAVE_SPRITESHEET : TOAST_REPLACE_SPRITESHEET, id,
                                                    pathString);
                                        });
                          }});
  }

  void spritesheet_textures_edit(Document& document, StringType edit, Document::ChangeType changeType,
                                 const std::function<bool()>& behavior, StringType successToast, StringType failToast)
  {
    document.snapshots.anm2_textures_push(localize.get(edit));
    auto isSuccess = behavior();
    toast_log(isSuccess ? Level::INFO : Level::ERROR, isSuccess ? successToast : failToast);
    document.change(changeType);
  }

  Window spritesheets_window_register()
  {
    Window window{};
    window.title = LABEL_SPRITESHEETS_WINDOW;
    window.isOpen = &Settings::windowIsSpritesheets;
    window.changeType = Document::SPRITESHEETS;
    window.containerType = ElementType::SPRITESHEETS;
    window.elementType = ElementType::SPRITESHEET;
    window.childLabel = "##Spritesheets Child";
    window.cardLines = SPRITESHEET_CARD_LINES;
    window.pasteEdit = EDIT_PASTE_SPRITESHEETS;
    window.deserializeFailedToast = TOAST_DESERIALIZE_SPRITESHEETS_FAILED;
    window.flags = WINDOW_OPEN_DIRECTORY | WINDOW_SET_FILE_PATH | WINDOW_ADD | WINDOW_REMOVE_UNUSED | WINDOW_RELOAD |
                   WINDOW_REPLACE | WINDOW_MERGE | WINDOW_PACK | WINDOW_SAVE | WINDOW_COPY | WINDOW_PASTE;
    window.footer = {{WINDOW_ADD, WINDOW_RELOAD, WINDOW_REPLACE}, {WINDOW_REMOVE_UNUSED, WINDOW_SAVE}};
    window.tooltips = {
        {WINDOW_ADD, TOOLTIP_ADD_SPRITESHEET},         {WINDOW_RELOAD, TOOLTIP_RELOAD_SPRITESHEETS},
        {WINDOW_REPLACE, TOOLTIP_REPLACE_SPRITESHEET}, {WINDOW_REMOVE_UNUSED, TOOLTIP_REMOVE_UNUSED_SPRITESHEETS},
        {WINDOW_PACK, TOOLTIP_PACK_SPRITESHEET},       {WINDOW_SAVE, TOOLTIP_SAVE_SPRITESHEETS}};
    window.enabled = {{WINDOW_PACK, [](Window&, Document& document)
                       {
                         auto& selection = document.spritesheet.selection;
                         return selection.size() == 1 && is_spritesheet_regions(document, *selection.begin());
                       }}};
    window.popup = PopupHelper(LABEL_SPRITESHEETS_MERGE_POPUP, POPUP_SMALL_NO_HEIGHT);
    window.popup2 = PopupHelper(LABEL_SPRITESHEETS_PACK_POPUP, POPUP_SMALL_NO_HEIGHT);
    window.popup3 = PopupHelper(LABEL_TASKBAR_OVERWRITE_FILE, POPUP_SMALL_NO_HEIGHT);
    window.storage_get = [](Document& document) -> Storage& { return document.spritesheet; };
    window.row_label_get = [](Document& document, const Element& spritesheet)
    {
      auto pathString = path::to_utf8(spritesheet.path);
      auto label = std::vformat(localize.get(FORMAT_SPRITESHEET), std::make_format_args(spritesheet.id, pathString));
      return document.spritesheet_is_dirty(spritesheet.id)
                 ? std::vformat(localize.get(FORMAT_SPRITESHEET_NOT_SAVED), std::make_format_args(label))
                 : label;
    };
    window.row_select = [](Window&, Document& document, int)
    {
      document.editTarget = Document::EditTarget::SPRITESHEET;
      document.region.reference = -1;
      document.region.selection.clear();
    };
    window.card_image_get = [](Document& document, Resources& resources, const Element& spritesheet)
    {
      auto texture = document.texture_get(spritesheet.id);
      auto isValid = texture && texture->is_valid();
      auto& image = isValid ? *texture : resources.icons[icon::NONE];
      return WindowCardImage{.texture = &image, .size = glm::vec2(image.size), .isValid = isValid};
    };
    window.tooltip_draw = [](Document& document, Resources& resources, const Element& spritesheet)
    {
      auto texture = document.texture_get(spritesheet.id);
      auto isValid = texture && texture->is_valid();
      auto& image = isValid ? *texture : resources.icons[icon::NONE];
      window_tooltip_image_draw(
          {.texture = &image, .size = glm::vec2(image.size), .isValid = isValid},
          [&]()
          {
            window_tooltip_name_draw(resources, path::to_utf8(spritesheet.path));
            ImGui::TextUnformatted(
                std::vformat(localize.get(FORMAT_ID), std::make_format_args(spritesheet.id)).c_str());
            if (isValid)
              ImGui::TextUnformatted(
                  std::vformat(localize.get(FORMAT_TEXTURE_SIZE), std::make_format_args(image.size.x, image.size.y))
                      .c_str());
            else
              ImGui::TextUnformatted(localize.get(TOOLTIP_SPRITESHEET_INVALID));
            ImGui::TextUnformatted(localize.get(TEXT_OPEN_DIRECTORY));
          });
    };
    window.add = [](Window& window, Manager&, Settings&, Document&, Clipboard&)
    {
      if (window.dialog) window.dialog->file_open(Dialog::SPRITESHEET_OPEN, true);
    };
    window.replace = [](Window& window, Manager&, Settings&, Document&, Clipboard&)
    {
      if (window.dialog) window.dialog->file_open(Dialog::SPRITESHEET_REPLACE);
    };
    window.path_set = [](Window& window, Manager&, Settings&, Document&, Clipboard&)
    {
      if (window.dialog) window.dialog->file_save(Dialog::SPRITESHEET_PATH_SET);
    };
    window.open = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto spritesheet = window_element_get(window, document, document.spritesheet.reference);
      if (spritesheet && window.dialog) window_directory_open(*window.dialog, document, spritesheet->path);
    };
    window.remove_unused = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto unused = document.anm2.element_unused(ElementType::SPRITESHEET);
      auto spritesheets = window_container_get(window, document);
      if (unused.empty() || !spritesheets) return;
      window_edit(document, window.changeType, localize.get(EDIT_REMOVE_UNUSED_SPRITESHEETS),
                  [&]()
                  {
                    for (auto id : unused)
                      if (auto spritesheet = window_element_get(window, document, id))
                      {
                        toast_log(Level::INFO, TOAST_REMOVE_SPRITESHEET, id, path::to_utf8(spritesheet->path));
                        element_child_id_erase(*spritesheets, ElementType::SPRITESHEET, id);
                      }
                  });
    };
    window.reload = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto selected = document.spritesheet.selection;
      window_edit(document, window.changeType, localize.get(EDIT_RELOAD_SPRITESHEETS),
                  [&]()
                  {
                    for (auto id : selected)
                      document.texture_reload(id);
                  });
      for (auto id : selected)
        if (auto spritesheet = window_element_get(window, document, id))
        {
          document.spritesheet_hash_set_saved(id);
          toast_log(Level::INFO, TOAST_RELOAD_SPRITESHEET, id, path::to_utf8(spritesheet->path));
        }
    };
    window.save = [](Window& window, Manager&, Settings& settings, Document& document, Clipboard&)
    {
      if (!settings.fileIsWarnOverwrite) return spritesheets_save(document, document.spritesheet.selection);
      window.selection2 = document.spritesheet.selection;
      window.popup3.open();
    };
    window.merge_open = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      window.selection = document.spritesheet.selection;
      window.popup.open();
    };
    window.pack_open = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      window.editId = *document.spritesheet.selection.begin();
      window.popup2.open();
    };
    window.body_update = [](Window& window, Manager& manager, Settings&, Resources&, Clipboard&, Document& document)
    {
      if (!window.dialog) return;
      auto& selection = document.spritesheet.selection;
      if (window.dialog->is_selected(Dialog::SPRITESHEET_OPEN))
      {
        manager.command_push({manager.selected, [&window, paths = window.dialog->paths](Manager&, Document& document)
                              {
                                document.spritesheets_add(paths);
                                document.region.reference = -1;
                                document.region.selection.clear();
                                window.newElementId = document.spritesheet.reference;
                              }});
        window.dialog->reset();
      }

      for (auto [type, edit] : {std::pair{Dialog::SPRITESHEET_REPLACE, EDIT_REPLACE_SPRITESHEET},
                                std::pair{Dialog::SPRITESHEET_PATH_SET, EDIT_SET_SPRITESHEET_FILE_PATH}})
      {
        if (!window.dialog->is_selected(type)) continue;
        if (selection.size() == 1 && !window.dialog->path.empty())
          spritesheet_path_change(window, manager, *selection.begin(), window.dialog->path, edit,
                                  type == Dialog::SPRITESHEET_PATH_SET);
        window.dialog->reset();
      }
    };
    window.popup_update =
        [](Window& window, Manager& manager, Settings& settings, Resources&, Clipboard&, Document& document)
    {
      window.popup.trigger();
      if (ImGui::BeginPopupModal(window.popup.label(), &window.popup.isOpen, ImGuiWindowFlags_NoResize))
      {
        settings.mergeSpritesheetsRegionOrigin =
            glm::clamp(settings.mergeSpritesheetsRegionOrigin, (int)origin::TOP_LEFT, (int)origin::ORIGIN_CENTER);

        if (ImGui::BeginChild("##Merge Spritesheets Options", child_size_get(MERGE_OPTIONS_ROWS),
                              ImGuiChildFlags_Borders))
        {
          ImGui::SeparatorText(localize.get(LABEL_REGION_PROPERTIES_ORIGIN));
          ImGui::RadioButton(localize.get(LABEL_MERGE_SPRITESHEETS_APPEND_BOTTOM), &settings.mergeSpritesheetsOrigin,
                             APPEND_BOTTOM);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MERGE_SPRITESHEETS_BOTTOM_LEFT));
          ImGui::SameLine();
          ImGui::RadioButton(localize.get(LABEL_MERGE_SPRITESHEETS_APPEND_RIGHT), &settings.mergeSpritesheetsOrigin,
                             APPEND_RIGHT);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MERGE_SPRITESHEETS_TOP_RIGHT));

          ImGui::SeparatorText(localize.get(LABEL_OPTIONS));
          ImGui::Checkbox(localize.get(LABEL_MERGE_MAKE_SPRITESHEET_REGIONS), &settings.mergeSpritesheetsIsMakeRegions);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MERGE_MAKE_SPRITESHEET_REGIONS));

          ImGui::BeginDisabled(!settings.mergeSpritesheetsIsMakeRegions);
          ImGui::Checkbox(localize.get(LABEL_MERGE_MAKE_PRIMARY_SPRITESHEET_REGION),
                          &settings.mergeSpritesheetsIsMakePrimaryRegion);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_MERGE_MAKE_PRIMARY_SPRITESHEET_REGION));
          const char* regionOriginOptions[] = {localize.get(LABEL_REGION_ORIGIN_TOP_LEFT),
                                               localize.get(LABEL_REGION_ORIGIN_CENTER)};
          ImGui::Combo(localize.get(LABEL_REGION_PROPERTIES_ORIGIN), &settings.mergeSpritesheetsRegionOrigin,
                       regionOriginOptions, IM_ARRAYSIZE(regionOriginOptions));
          ImGui::EndDisabled();
        }
        ImGui::EndChild();

        auto result =
            window_popup_buttons_draw(manager, localize.get(BASIC_MERGE), window.selection.size() > 1, BASIC_CANCEL);
        if (result == PopupButton::CONFIRM)
          manager.command_push(
              {manager.selected,
               [ids = window.selection, isAppendRight = settings.mergeSpritesheetsOrigin == APPEND_RIGHT,
                isMakeRegions = settings.mergeSpritesheetsIsMakeRegions,
                isMakePrimaryRegion = settings.mergeSpritesheetsIsMakePrimaryRegion,
                regionOrigin = (origin::Type)settings.mergeSpritesheetsRegionOrigin](Manager&, Document& document)
               {
                 spritesheet_textures_edit(
                     document, EDIT_MERGE_SPRITESHEETS, Document::ALL,
                     [&]()
                     {
                       if (!document.spritesheets_merge(ids, isAppendRight, isMakeRegions, isMakePrimaryRegion,
                                                        regionOrigin))
                         return false;
                       document.spritesheet.selection = {*ids.begin()};
                       document.spritesheet.reference = *ids.begin();
                       document.region.reference = -1;
                       document.region.selection.clear();
                       return true;
                     },
                     TOAST_MERGE_SPRITESHEETS, TOAST_MERGE_SPRITESHEETS_FAILED);
               }});
        if (result != PopupButton::NONE)
        {
          window.selection.clear();
          window.popup.close();
        }
        ImGui::EndPopup();
      }
      window.popup.end();

      window.popup2.trigger();
      if (ImGui::BeginPopupModal(window.popup2.label(), &window.popup2.isOpen, ImGuiWindowFlags_NoResize))
      {
        settings.packPadding = std::max(0, settings.packPadding);
        if (ImGui::BeginChild("##Pack Spritesheet Options", child_size_get(), ImGuiChildFlags_Borders))
          ImGui::DragInt(localize.get(LABEL_PACK_PADDING), &settings.packPadding, DRAG_SPEED, 0, PADDING_MAX);
        ImGui::EndChild();

        auto result = window_popup_buttons_draw(manager, localize.get(BASIC_PACK),
                                                is_spritesheet_regions(document, window.editId), BASIC_CANCEL);
        if (result == PopupButton::CONFIRM)
          manager.command_push({manager.selected,
                                [id = window.editId, padding = settings.packPadding](Manager&, Document& document)
                                {
                                  spritesheet_textures_edit(
                                      document, EDIT_PACK_SPRITESHEET, Document::SPRITESHEETS,
                                      [&]() { return document.spritesheet_pack(id, padding); }, TOAST_PACK_SPRITESHEET,
                                      TOAST_PACK_SPRITESHEET_FAILED);
                                }});
        if (result != PopupButton::NONE)
        {
          window.editId = -1;
          window.popup2.close();
        }
        ImGui::EndPopup();
      }
      window.popup2.end();

      window.popup3.trigger();
      if (ImGui::BeginPopupModal(window.popup3.label(), &window.popup3.isOpen, ImGuiWindowFlags_NoResize))
      {
        ImGui::TextUnformatted(localize.get(LABEL_OVERWRITE_CONFIRMATION));
        auto result = window_popup_buttons_draw(manager, localize.get(BASIC_YES), true, BASIC_NO);
        if (result == PopupButton::CONFIRM)
          manager.command_push({manager.selected, [ids = window.selection2](Manager&, Document& document)
                                { spritesheets_save(document, ids); }});
        if (result != PopupButton::NONE)
        {
          window.selection2.clear();
          window.popup3.close();
        }
        ImGui::EndPopup();
      }
      window.popup3.end();
    };
    return window;
  }
}
