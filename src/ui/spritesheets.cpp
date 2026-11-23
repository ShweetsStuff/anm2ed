#include "ui/panel.h"

#include <format>

#include "path.h"
#include "ui/input.h"
#include "ui/layout.h"
#include "working_directory.h"

constexpr int SPRITESHEET_CARD_LINES = 4;
constexpr int MERGE_OPTIONS_ROWS = 6;
constexpr int PADDING_MAX = 100;
// The merge region origin setting indexes this (top-left or center).
constexpr Origin MERGE_REGION_ORIGINS[] = {Origin::TOP_LEFT, Origin::CENTER};

static const Spritesheet* spritesheet_get(Document& document, int id)
{
    return item_get(document.model.content.spritesheets, id);
}

static bool is_spritesheet_regions(Document& document, int id)
{
    auto spritesheet = spritesheet_get(document, id);
    return spritesheet && !spritesheet->regions.empty();
}

static void spritesheets_save(Document& document, const std::set<int>& ids)
{
    for (auto id : ids)
    {
        auto spritesheet = spritesheet_get(document, id);
        auto texture = document.texture_get(id);
        if (!spritesheet || !texture)
            continue;
        auto pathString = path_to_utf8(spritesheet->path);
        WorkingDirectory workingDirectory(document.directory_get());
        path_ensure_directory(spritesheet->path.parent_path());
        auto isSaved = texture->write(spritesheet->path);
        if (isSaved)
            document.spritesheet_hash_set_saved(id);
        toast_log(isSaved ? Level::INFO : Level::ERROR,
                  isSaved ? TOAST_SAVE_SPRITESHEET : TOAST_SAVE_SPRITESHEET_FAILED, id, pathString);
    }
}

// Points a spritesheet at a new file: replacing loads it, setting the path writes the current texture there.
static void spritesheet_path_change(Document& document, int id, const std::filesystem::path& dialogPath,
                                    StringType edit, bool isWritten)
{
    auto texture = document.texture_get(id);
    if (!spritesheet_get(document, id) || (isWritten && !texture))
        return;
    document.edit_apply(edit, [&](Model& model) {
        auto spritesheet = item_get(model.content.spritesheets, id);
        auto newPath = asset_path_get(document, dialogPath);
        auto pathString = path_to_utf8(newPath);
        if (isWritten)
        {
            WorkingDirectory workingDirectory(document.directory_get());
            path_ensure_directory(newPath.parent_path());
            if (!texture->write(newPath))
                return toast_log(Level::ERROR, TOAST_SAVE_SPRITESHEET_FAILED, id, pathString);
        }
        spritesheet->path = newPath;
        if (isWritten)
            document.texturePaths[id] = newPath;
        else
            document.texture_reload(id);
        document.spritesheet_hash_set_saved(id);
        toast_log(Level::INFO, isWritten ? TOAST_SAVE_SPRITESHEET : TOAST_REPLACE_SPRITESHEET, id, pathString);
    });
}

static void spritesheet_textures_edit(Document& document, StringType edit, const std::function<bool()>& behavior,
                                      StringType successToast, StringType failToast)
{
    document.edit_begin(edit);
    auto isSuccess = behavior();
    toast_log(isSuccess ? Level::INFO : Level::ERROR, isSuccess ? successToast : failToast);
    document.change();
}

static void spritesheets_dialog_update(Panel& panel)
{
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    if (dialog.is_selected(Dialog::SPRITESHEET_OPEN))
    {
        command_push(panel, [&state, paths = dialog.paths](Document& document) {
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
        if (!dialog.is_selected(type))
            continue;
        if (selection.size() == 1 && !dialog.path.empty())
            command_push(panel, [id = *selection.begin(), path = dialog.path, edit,
                                 isWritten = type == Dialog::SPRITESHEET_PATH_SET](Document& document) {
                spritesheet_path_change(document, id, path, edit, isWritten);
            });
        dialog.reset();
    }
}

static void spritesheets_merge_popup_update(Panel& panel, SpritesheetsPanel& spritesheets)
{
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& popup = spritesheets.mergePopup;
    if (!popup.begin())
        return popup.end();

    settings.mergeSpritesheetsRegionOrigin =
        vec_clamp(settings.mergeSpritesheetsRegionOrigin, 0, (int)std::size(MERGE_REGION_ORIGINS) - 1);
    if (ImGui::BeginChild("##Merge Spritesheets Options", child_size_get(MERGE_OPTIONS_ROWS), ImGuiChildFlags_Borders))
    {
        ImGui::SeparatorText(localize.get(LABEL_REGION_PROPERTIES_ORIGIN));
        ImGui::RadioButton(localize.get(LABEL_MERGE_SPRITESHEETS_APPEND_BOTTOM), &settings.mergeSpritesheetsOrigin,
                           APPEND_BOTTOM);
        tooltip_set(TOOLTIP_MERGE_SPRITESHEETS_BOTTOM_LEFT);
        ImGui::SameLine();
        ImGui::RadioButton(localize.get(LABEL_MERGE_SPRITESHEETS_APPEND_RIGHT), &settings.mergeSpritesheetsOrigin,
                           APPEND_RIGHT);
        tooltip_set(TOOLTIP_MERGE_SPRITESHEETS_TOP_RIGHT);

        ImGui::SeparatorText(localize.get(LABEL_OPTIONS));
        checkbox_draw(LABEL_MERGE_MAKE_SPRITESHEET_REGIONS, settings.mergeSpritesheetsIsMakeRegions,
                      TOOLTIP_MERGE_MAKE_SPRITESHEET_REGIONS);

        ImGui::BeginDisabled(!settings.mergeSpritesheetsIsMakeRegions);
        ImGui::Checkbox(localize.get(LABEL_MERGE_MAKE_PRIMARY_SPRITESHEET_REGION),
                        &settings.mergeSpritesheetsIsMakePrimaryRegion);
        tooltip_set(TOOLTIP_MERGE_MAKE_PRIMARY_SPRITESHEET_REGION);
        const char* regionOriginOptions[] = {localize.get(LABEL_REGION_ORIGIN_TOP_LEFT),
                                             localize.get(LABEL_REGION_ORIGIN_CENTER)};
        ImGui::Combo(localize.get(LABEL_REGION_PROPERTIES_ORIGIN), &settings.mergeSpritesheetsRegionOrigin,
                     regionOriginOptions, IM_ARRAYSIZE(regionOriginOptions));
        ImGui::EndDisabled();
    }
    ImGui::EndChild();

    auto result =
        popup_buttons_draw(manager, {.confirm = BASIC_MERGE, .isConfirmEnabled = spritesheets.mergeIds.size() > 1});
    if (result == PopupButton::CONFIRM)
        command_push(panel,
                     [ids = spritesheets.mergeIds,
                      options = SpritesheetMergeOptions{
                          .isAppendRight = settings.mergeSpritesheetsOrigin == APPEND_RIGHT,
                          .isMakeRegions = settings.mergeSpritesheetsIsMakeRegions,
                          .isMakePrimaryRegion = settings.mergeSpritesheetsIsMakePrimaryRegion,
                          .origin = MERGE_REGION_ORIGINS[settings.mergeSpritesheetsRegionOrigin]}](Document& document) {
                         spritesheet_textures_edit(
                             document, EDIT_MERGE_SPRITESHEETS,
                             [&]() {
                                 if (!document.spritesheets_merge(ids, options))
                                     return false;
                                 document.selected_ids_set(SelectionKind::SPRITESHEETS, {*ids.begin()});
                                 document.focused_id_set(SelectionKind::SPRITESHEETS, *ids.begin());
                                 document.selected_clear(SelectionKind::REGIONS);
                                 return true;
                             },
                             TOAST_MERGE_SPRITESHEETS, TOAST_MERGE_SPRITESHEETS_FAILED);
                     });
    if (result != PopupButton::NONE)
        popup.close();
    ImGui::EndPopup();
    popup.end();
}

static void spritesheets_pack_popup_update(Panel& panel, SpritesheetsPanel& spritesheets)
{
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& popup = spritesheets.packPopup;
    if (!popup.begin())
        return popup.end();

    settings.packPadding = std::max(0, settings.packPadding);
    if (ImGui::BeginChild("##Pack Spritesheet Options", child_size_get(), ImGuiChildFlags_Borders))
        ImGui::DragInt(localize.get(LABEL_PACK_PADDING), &settings.packPadding, DRAG_SPEED, 0, PADDING_MAX);
    ImGui::EndChild();

    auto result = popup_buttons_draw(
        manager, {.confirm = BASIC_PACK, .isConfirmEnabled = is_spritesheet_regions(document, spritesheets.packId)});
    if (result == PopupButton::CONFIRM)
        command_push(panel, [id = spritesheets.packId, padding = settings.packPadding](Document& document) {
            spritesheet_textures_edit(
                document, EDIT_PACK_SPRITESHEET, [&]() { return document.spritesheet_pack(id, padding); },
                TOAST_PACK_SPRITESHEET, TOAST_PACK_SPRITESHEET_FAILED);
        });
    if (result != PopupButton::NONE)
        popup.close();
    ImGui::EndPopup();
    popup.end();
}

static void spritesheets_overwrite_popup_update(Panel& panel, SpritesheetsPanel& spritesheets)
{
    auto& popup = spritesheets.overwritePopup;
    if (!popup.begin())
        return popup.end();

    ImGui::TextUnformatted(localize.get(LABEL_OVERWRITE_CONFIRMATION));
    auto result = popup_buttons_draw(panel.manager, {.confirm = BASIC_YES, .cancel = BASIC_NO});
    if (result == PopupButton::CONFIRM)
        command_push(panel, [ids = spritesheets.saveIds](Document& document) { spritesheets_save(document, ids); });
    if (result != PopupButton::NONE)
        popup.close();
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
        auto isOne = [selection]() {
            return selection.size() == 1;
        };
        auto isAny = [selection]() {
            return !selection.empty();
        };
        auto open = [&](int id) {
            if (auto spritesheet = spritesheet_get(document, id))
                directory_open(dialog, document, spritesheet->path);
        };

        Actions actions = {};
        actions.add(ACTION_OPEN_DIRECTORY, isOne, [&]() { open(*selection.begin()); });
        actions.add(ACTION_SET_FILE_PATH, isOne, [&]() { dialog.file_save(Dialog::SPRITESHEET_PATH_SET); });
        actions.add(ACTION_ADD, {}, [&]() { dialog.file_open(Dialog::SPRITESHEET_OPEN); }, TOOLTIP_ADD_SPRITESHEET);
        content_remove_action_add(actions, panel, ElementType::SPRITESHEET, SelectionKind::SPRITESHEETS);
        actions.add(
            ACTION_RELOAD, isAny,
            [&]() {
                command_push(panel, [selection](Document& document) { document.spritesheets_reload(selection); });
            },
            TOOLTIP_RELOAD_SPRITESHEETS);
        actions.add(
            ACTION_REPLACE, isOne, [&]() { dialog.file_open(Dialog::SPRITESHEET_REPLACE); },
            TOOLTIP_REPLACE_SPRITESHEET);
        actions.add(
            ACTION_MERGE, [selection]() { return selection.size() > 1; },
            [&]() {
                spritesheets.mergeIds = selection;
                spritesheets.mergePopup.open();
            });
        actions.add(
            ACTION_PACK,
            [&]() { return selection.size() == 1 && is_spritesheet_regions(document, *selection.begin()); },
            [&]() {
                spritesheets.packId = *selection.begin();
                spritesheets.packPopup.open();
            },
            TOOLTIP_PACK_SPRITESHEET);
        // SVG sheets become PNGs beside them. Greyed when none of the selection is an SVG.
        actions.add(
            ACTION_RASTERIZE,
            [&]() {
                return std::ranges::any_of(selection, [&](int id) {
                    auto texture = document.texture_get(id);
                    return texture && texture->is_vector();
                });
            },
            [&]() {
                command_push(panel,
                             [selection](Document& document) { document.spritesheets_vector_convert(selection); });
            },
            TOOLTIP_RASTERIZE_SPRITESHEETS);
        actions.add(
            ACTION_SAVE, isAny,
            [&]() {
                if (!settings.fileIsWarnOverwrite)
                    return command_push(panel,
                                        [selection](Document& document) { spritesheets_save(document, selection); });
                spritesheets.saveIds = selection;
                spritesheets.overwritePopup.open();
            },
            TOOLTIP_SAVE_SPRITESHEETS);
        items_clipboard_actions_add<Spritesheet>(actions, panel, SelectionKind::SPRITESHEETS, EDIT_PASTE_SPRITESHEETS,
                                                 TOAST_DESERIALIZE_SPRITESHEETS_FAILED, &Content::spritesheets);

        auto image_get = [&](int id) {
            auto texture = document.texture_get(id);
            auto isValid = texture && texture->is_valid();
            auto& image = isValid ? *texture : resources.icons[ICON_NONE];
            return CardImage{.texture = &image, .size = Vec2(image.size), .isValid = isValid};
        };
        ListRows rows{.label_get =
                          [&](int id) {
                              auto pathString = path_to_utf8(spritesheet_get(document, id)->path);
                              auto label = localize_format(FORMAT_SPRITESHEET, id, pathString);
                              return document.spritesheet_is_dirty(id)
                                         ? localize_format(FORMAT_SPRITESHEET_NOT_SAVED, label)
                                         : label;
                          },
                      .tooltip_draw =
                          [&](int id) {
                              auto image = image_get(id);
                              tooltip_image_draw(image, [&]() {
                                  tooltip_name_draw(resources, path_to_utf8(spritesheet_get(document, id)->path));
                                  ImGui::TextUnformatted(localize_format(FORMAT_ID, id).c_str());
                                  if (image.isValid)
                                      ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_TEXTURE_SIZE),
                                                                          std::make_format_args(image.texture->size.x,
                                                                                                image.texture->size.y))
                                                                 .c_str());
                                  else
                                      ImGui::TextUnformatted(localize.get(TOOLTIP_SPRITESHEET_INVALID));
                                  ImGui::TextUnformatted(localize.get(TEXT_OPEN_DIRECTORY));
                              });
                          },
                      .image_get = image_get,
                      .select =
                          [&](int) {
                              document.editTarget = Document::EditTarget::SPRITESHEET;
                              document.selected_clear(SelectionKind::REGIONS);
                          },
                      .activate = open,
                      .cardLines = SPRITESHEET_CARD_LINES};
        list_panel_draw(
            panel, actions, {{ACTION_ADD, ACTION_RELOAD, ACTION_REPLACE}, {ACTION_REMOVE, ACTION_SAVE}},
            [&]() { content_list_draw(panel, SelectionKind::SPRITESHEETS, item_ids_get(items), rows); }, true);
    }
    ImGui::End();

    spritesheets_merge_popup_update(panel, spritesheets);
    spritesheets_pack_popup_update(panel, spritesheets);
    spritesheets_overwrite_popup_update(panel, spritesheets);
}
