#include "panel.hpp"

#include <format>

#include "path.hpp"
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
  // The merge region origin setting indexes this (top-left or center).
  constexpr Origin MERGE_REGION_ORIGINS[] = {Origin::TOP_LEFT, Origin::CENTER};

  auto& spritesheets_get(model::Model& model) { return model.content.spritesheets; }

  const model::Spritesheet* spritesheet_get(Document& document, int id)
  {
    return model::item_get(document.model.content.spritesheets, id);
  }

  bool is_spritesheet_regions(Document& document, int id)
  {
    auto spritesheet = spritesheet_get(document, id);
    return spritesheet && !spritesheet->regions.empty();
  }

  void spritesheets_save(Document& document, const std::set<int>& ids)
  {
    for (auto id : ids)
    {
      auto spritesheet = spritesheet_get(document, id);
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

  // Points a spritesheet at a new file: replacing loads it, setting the path writes the current texture there.
  void spritesheet_path_change(Document& document, int id, const std::filesystem::path& dialogPath, StringType edit,
                               bool isWritten)
  {
    auto texture = document.texture_get(id);
    if (!spritesheet_get(document, id) || (isWritten && !texture)) return;
    document.edit_apply(edit,
                        [&](model::Model& model)
                        {
                          auto spritesheet = model::item_get(model.content.spritesheets, id);
                          auto newPath = asset_path_get(document, dialogPath);
                          auto pathString = path::to_utf8(newPath);
                          if (isWritten)
                          {
                            WorkingDirectory workingDirectory(document.directory_get());
                            path::ensure_directory(newPath.parent_path());
                            if (!texture->write_png(newPath))
                              return toast_log(Level::ERROR, TOAST_SAVE_SPRITESHEET_FAILED, id, pathString);
                          }
                          spritesheet->path = newPath;
                          if (isWritten)
                            document.texturePaths[id] = newPath;
                          else
                            document.texture_reload(id);
                          document.spritesheet_hash_set_saved(id);
                          toast_log(Level::INFO, isWritten ? TOAST_SAVE_SPRITESHEET : TOAST_REPLACE_SPRITESHEET, id,
                                    pathString);
                        });
  }

  void spritesheet_textures_edit(Document& document, StringType edit, const std::function<bool()>& behavior,
                                 StringType successToast, StringType failToast)
  {
    document.edit_begin(edit);
    auto isSuccess = behavior();
    toast_log(isSuccess ? Level::INFO : Level::ERROR, isSuccess ? successToast : failToast);
    document.change();
  }

  void spritesheets_dialog_update(Panel& panel)
  {
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    if (dialog.is_selected(Dialog::SPRITESHEET_OPEN))
    {
      command_push(panel,
                   [&state, paths = dialog.paths](Document& document)
                   {
                     document.spritesheets_add(paths);
                     document.selected_clear(SelectionKind::REGIONS);
                     state.newId = document.focused_id_get(SelectionKind::SPRITESHEETS);
                   });
      dialog.reset();
    }

    auto selection = document.selected_ids_get(SelectionKind::SPRITESHEETS);
    for (auto [type, edit] : {std::pair{Dialog::SPRITESHEET_REPLACE, EDIT_REPLACE_SPRITESHEET},
                              std::pair{Dialog::SPRITESHEET_PATH_SET, EDIT_SET_SPRITESHEET_FILE_PATH}})
    {
      if (!dialog.is_selected(type)) continue;
      if (selection.size() == 1 && !dialog.path.empty())
        command_push(panel, [id = *selection.begin(), path = dialog.path, edit,
                             isWritten = type == Dialog::SPRITESHEET_PATH_SET](Document& document)
                     { spritesheet_path_change(document, id, path, edit, isWritten); });
      dialog.reset();
    }
  }

  void spritesheets_merge_popup_update(Panel& panel, SpritesheetsPanel& spritesheets)
  {
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& popup = spritesheets.mergePopup;
    popup.trigger();
    if (!ImGui::BeginPopupModal(popup.label(), &popup.isOpen, ImGuiWindowFlags_NoResize)) return popup.end();

    settings.mergeSpritesheetsRegionOrigin =
        glm::clamp(settings.mergeSpritesheetsRegionOrigin, 0, (int)std::size(MERGE_REGION_ORIGINS) - 1);
    if (ImGui::BeginChild("##Merge Spritesheets Options", child_size_get(MERGE_OPTIONS_ROWS), ImGuiChildFlags_Borders))
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

    auto result = popup_buttons_draw(manager, localize.get(BASIC_MERGE), spritesheets.mergeIds.size() > 1);
    if (result == PopupButton::CONFIRM)
      command_push(panel,
                   [ids = spritesheets.mergeIds, isAppendRight = settings.mergeSpritesheetsOrigin == APPEND_RIGHT,
                    isMakeRegions = settings.mergeSpritesheetsIsMakeRegions,
                    isMakePrimaryRegion = settings.mergeSpritesheetsIsMakePrimaryRegion,
                    regionOrigin = MERGE_REGION_ORIGINS[settings.mergeSpritesheetsRegionOrigin]](Document& document)
                   {
                     spritesheet_textures_edit(
                         document, EDIT_MERGE_SPRITESHEETS,
                         [&]()
                         {
                           if (!document.spritesheets_merge(ids, isAppendRight, isMakeRegions, isMakePrimaryRegion,
                                                            regionOrigin))
                             return false;
                           document.selected_ids_set(SelectionKind::SPRITESHEETS, {*ids.begin()});
                           document.focused_id_set(SelectionKind::SPRITESHEETS, *ids.begin());
                           document.selected_clear(SelectionKind::REGIONS);
                           return true;
                         },
                         TOAST_MERGE_SPRITESHEETS, TOAST_MERGE_SPRITESHEETS_FAILED);
                   });
    if (result != PopupButton::NONE) popup.close();
    ImGui::EndPopup();
    popup.end();
  }

  void spritesheets_pack_popup_update(Panel& panel, SpritesheetsPanel& spritesheets)
  {
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& popup = spritesheets.packPopup;
    popup.trigger();
    if (!ImGui::BeginPopupModal(popup.label(), &popup.isOpen, ImGuiWindowFlags_NoResize)) return popup.end();

    settings.packPadding = std::max(0, settings.packPadding);
    if (ImGui::BeginChild("##Pack Spritesheet Options", child_size_get(), ImGuiChildFlags_Borders))
      ImGui::DragInt(localize.get(LABEL_PACK_PADDING), &settings.packPadding, DRAG_SPEED, 0, PADDING_MAX);
    ImGui::EndChild();

    auto result =
        popup_buttons_draw(manager, localize.get(BASIC_PACK), is_spritesheet_regions(document, spritesheets.packId));
    if (result == PopupButton::CONFIRM)
      command_push(panel,
                   [id = spritesheets.packId, padding = settings.packPadding](Document& document)
                   {
                     spritesheet_textures_edit(
                         document, EDIT_PACK_SPRITESHEET, [&]() { return document.spritesheet_pack(id, padding); },
                         TOAST_PACK_SPRITESHEET, TOAST_PACK_SPRITESHEET_FAILED);
                   });
    if (result != PopupButton::NONE) popup.close();
    ImGui::EndPopup();
    popup.end();
  }

  void spritesheets_overwrite_popup_update(Panel& panel, SpritesheetsPanel& spritesheets)
  {
    auto& popup = spritesheets.overwritePopup;
    popup.trigger();
    if (!ImGui::BeginPopupModal(popup.label(), &popup.isOpen, ImGuiWindowFlags_NoResize)) return popup.end();

    ImGui::TextUnformatted(localize.get(LABEL_OVERWRITE_CONFIRMATION));
    auto result = popup_buttons_draw(panel.manager, localize.get(BASIC_YES), true, BASIC_NO);
    if (result == PopupButton::CONFIRM)
      command_push(panel, [ids = spritesheets.saveIds](Document& document) { spritesheets_save(document, ids); });
    if (result != PopupButton::NONE) popup.close();
    ImGui::EndPopup();
    popup.end();
  }

  void spritesheets_update(Panel& panel, SpritesheetsPanel& spritesheets)
  {
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& items = document.model.content.spritesheets;
    spritesheets_dialog_update(panel);

    if (ImGui::Begin(localize.get(LABEL_SPRITESHEETS_WINDOW), &settings.windowIsSpritesheets))
    {
      auto selection = document.selected_ids_get(SelectionKind::SPRITESHEETS);
      auto isOne = [selection]() { return selection.size() == 1; };
      auto isAny = [selection]() { return !selection.empty(); };
      auto open = [&](int id)
      {
        if (auto spritesheet = spritesheet_get(document, id)) directory_open(dialog, document, spritesheet->path);
      };

      Actions actions{};
      actions.add(ACTION_OPEN_DIRECTORY, isOne, [&]() { open(*selection.begin()); });
      actions.add(ACTION_SET_FILE_PATH, isOne, [&]() { dialog.file_save(Dialog::SPRITESHEET_PATH_SET); });
      actions.add(ACTION_ADD, {}, [&]() { dialog.file_open(Dialog::SPRITESHEET_OPEN, true); }, TOOLTIP_ADD_SPRITESHEET);
      actions.add(
          ACTION_REMOVE_UNUSED, {},
          [&]()
          {
            command_push(panel,
                         [](Document& document)
                         {
                           auto unused = document.model.unused_get(ElementType::SPRITESHEET);
                           if (unused.empty()) return;
                           document.edit_apply(EDIT_REMOVE_UNUSED_SPRITESHEETS,
                                               [&](model::Model& model)
                                               {
                                                 std::erase_if(model.content.spritesheets,
                                                               [&](const model::Spritesheet& spritesheet)
                                                               {
                                                                 if (!unused.contains(spritesheet.id)) return false;
                                                                 toast_log(Level::INFO, TOAST_REMOVE_SPRITESHEET,
                                                                           spritesheet.id,
                                                                           path::to_utf8(spritesheet.path));
                                                                 return true;
                                                               });
                                               });
                         });
          },
          TOOLTIP_REMOVE_UNUSED_SPRITESHEETS);
      actions.add(
          ACTION_RELOAD, isAny,
          [&]()
          {
            command_push(panel,
                         [selection](Document& document)
                         {
                           document.edit_apply(EDIT_RELOAD_SPRITESHEETS,
                                               [&](model::Model&)
                                               {
                                                 for (auto id : selection)
                                                   document.texture_reload(id);
                                               });
                           for (auto id : selection)
                             if (auto spritesheet = spritesheet_get(document, id))
                             {
                               document.spritesheet_hash_set_saved(id);
                               toast_log(Level::INFO, TOAST_RELOAD_SPRITESHEET, id, path::to_utf8(spritesheet->path));
                             }
                         });
          },
          TOOLTIP_RELOAD_SPRITESHEETS);
      actions.add(
          ACTION_REPLACE, isOne, [&]() { dialog.file_open(Dialog::SPRITESHEET_REPLACE); }, TOOLTIP_REPLACE_SPRITESHEET);
      actions.add(
          ACTION_MERGE, [selection]() { return selection.size() > 1; },
          [&]()
          {
            spritesheets.mergeIds = selection;
            spritesheets.mergePopup.open();
          });
      actions.add(
          ACTION_PACK, [&]() { return selection.size() == 1 && is_spritesheet_regions(document, *selection.begin()); },
          [&]()
          {
            spritesheets.packId = *selection.begin();
            spritesheets.packPopup.open();
          },
          TOOLTIP_PACK_SPRITESHEET);
      actions.add(
          ACTION_SAVE, isAny,
          [&]()
          {
            if (!settings.fileIsWarnOverwrite)
              return command_push(panel, [selection](Document& document) { spritesheets_save(document, selection); });
            spritesheets.saveIds = selection;
            spritesheets.overwritePopup.open();
          },
          TOOLTIP_SAVE_SPRITESHEETS);
      items_clipboard_actions_add<model::Spritesheet>(actions, panel, SelectionKind::SPRITESHEETS,
                                                      EDIT_PASTE_SPRITESHEETS, TOAST_DESERIALIZE_SPRITESHEETS_FAILED,
                                                      spritesheets_get);

      auto image_get = [&](int id)
      {
        auto texture = document.texture_get(id);
        auto isValid = texture && texture->is_valid();
        auto& image = isValid ? *texture : resources.icons[icon::NONE];
        return CardImage{.texture = &image, .size = glm::vec2(image.size), .isValid = isValid};
      };
      ListRows rows{
          .label_get =
              [&](int id)
          {
            auto pathString = path::to_utf8(spritesheet_get(document, id)->path);
            auto label = std::vformat(localize.get(FORMAT_SPRITESHEET), std::make_format_args(id, pathString));
            return document.spritesheet_is_dirty(id)
                       ? std::vformat(localize.get(FORMAT_SPRITESHEET_NOT_SAVED), std::make_format_args(label))
                       : label;
          },
          .tooltip_draw =
              [&](int id)
          {
            auto image = image_get(id);
            tooltip_image_draw(image,
                               [&]()
                               {
                                 tooltip_name_draw(resources, path::to_utf8(spritesheet_get(document, id)->path));
                                 ImGui::TextUnformatted(
                                     std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());
                                 if (image.isValid)
                                   ImGui::TextUnformatted(
                                       std::vformat(localize.get(FORMAT_TEXTURE_SIZE),
                                                    std::make_format_args(image.texture->size.x, image.texture->size.y))
                                           .c_str());
                                 else
                                   ImGui::TextUnformatted(localize.get(TOOLTIP_SPRITESHEET_INVALID));
                                 ImGui::TextUnformatted(localize.get(TEXT_OPEN_DIRECTORY));
                               });
          },
          .image_get = image_get,
          .select =
              [&](int)
          {
            document.editTarget = Document::EditTarget::SPRITESHEET;
            document.selected_clear(SelectionKind::REGIONS);
          },
          .activate = open,
          .cardLines = SPRITESHEET_CARD_LINES};
      list_panel_draw(
          panel, actions, {{ACTION_ADD, ACTION_RELOAD, ACTION_REPLACE}, {ACTION_REMOVE_UNUSED, ACTION_SAVE}},
          [&]() { content_list_draw(panel, SelectionKind::SPRITESHEETS, item_ids_get(items), rows); }, true);
    }
    ImGui::End();

    spritesheets_merge_popup_update(panel, spritesheets);
    spritesheets_pack_popup_update(panel, spritesheets);
    spritesheets_overwrite_popup_update(panel, spritesheets);
  }
}
