#include "ui/panel.h"

#include <algorithm>
#include <format>

#include "vec.h"

#include "math_util.h"
#include "path.h"
#include "ui/draw.h"
#include "ui/input.h"
#include "ui/layout.h"
#include "vector.h"
#include "working_directory.h"

constexpr int REGION_CARD_LINES = 2;
constexpr int REGION_POPUP_ROWS = 5;
constexpr const char* REGION_DRAG_DROP = "Region Drag Drop";
constexpr const char* REGION_EXPORT_NAME_DEFAULT = "region";
constexpr const char* REGION_EXPORT_EXTENSION = ".png";
constexpr ImVec2 DRAG_TOOLTIP_OFFSET = ImVec2(16.0f, 16.0f);

constexpr StringType ORIGIN_LABELS[] = {LABEL_REGION_ORIGIN_CUSTOM, LABEL_REGION_ORIGIN_TOP_LEFT,
                                        LABEL_REGION_ORIGIN_CENTER};
constexpr Origin ORIGIN_COMBO_ORDER[] = {Origin::TOP_LEFT, Origin::CENTER, Origin::CUSTOM};

struct RegionExportOptions
{
    int spritesheetId = -1;
    int regionId = -1;
    std::filesystem::path path;
    bool isMakeSpritesheet = false;
    bool isRemoveCurrent = false;
};

static void region_pivot_apply(Region& region)
{
    if (region.origin == Origin::TOP_LEFT)
        region.pivot = {};
    if (region.origin == Origin::CENTER)
        region.pivot = region.size * 0.5f;
}

static const Spritesheet* region_spritesheet_get(Document& document)
{
    return item_get(document.model.content.spritesheets, document.focused_id_get(SelectionKind::SPRITESHEETS));
}

// New regions go after the last selected (or focused) one.
static int region_insert_index_get(Document& document)
{
    auto spritesheet = region_spritesheet_get(document);
    if (!spritesheet)
        return 0;
    auto selection = document.selected_ids_get(SelectionKind::REGIONS);
    auto focus = document.focused_id_get(SelectionKind::REGIONS);
    int index = (int)spritesheet->regions.size();
    for (int i = 0; i < (int)spritesheet->regions.size(); i++)
    {
        auto id = spritesheet->regions[i].id;
        if (selection.contains(id) || (selection.empty() && focus == id))
            index = i + 1;
    }
    return index;
}

static void region_select(Document& document, int id)
{
    document.editTarget = Document::EditTarget::REGION;
    document.selected_ids_set(SelectionKind::REGIONS, {id});
    document.focused_id_set(SelectionKind::REGIONS, id);
    document.reference_set({document.reference_get().animationIndex});
    document.frame_references_clear();
}

static CardImage region_image_get(Document& document, Resources& resources, const Region& region)
{
    auto texture = document.texture_get(document.focused_id_get(SelectionKind::SPRITESHEETS));
    if (!texture || !texture->is_valid())
        return {.texture = &resources.icons[ICON_NONE], .size = region.size};
    return {.texture = texture,
            .size = region.size,
            .uvMin = region.crop / Vec2(texture->size),
            .uvMax = (region.crop + region.size) / Vec2(texture->size),
            .isValid = true};
}

static bool region_export(Document& document, const RegionExportOptions& options)
{
    auto pathString = options.path.empty() ? std::string("in memory") : path_to_utf8(options.path);
    if (options.path.empty() && !options.isMakeSpritesheet)
    {
        toast_log(Level::ERROR, TOAST_EXPORT_REGION_PATH_EMPTY);
        return false;
    }

    auto spritesheet = item_get(document.model.content.spritesheets, options.spritesheetId);
    auto region = spritesheet ? item_get(spritesheet->regions, options.regionId) : nullptr;
    auto texture = document.texture_get(options.spritesheetId);
    auto minPoint = region ? IVec2(vec_min(region->crop, region->crop + region->size)) : IVec2();
    auto exportSize = region ? IVec2(vec_max(region->crop, region->crop + region->size)) - minPoint : IVec2();
    if (!region || !texture || !texture->is_valid() || exportSize.x <= 0 || exportSize.y <= 0)
    {
        toast_log(Level::ERROR, TOAST_EXPORT_REGION_FAILED, region ? region->name : std::string(), pathString);
        return false;
    }

    auto exportedImage = texture->region_get(minPoint, exportSize);
    auto sourceRegion = *region;
    auto outputPath = options.path;
    if (!outputPath.empty())
    {
        outputPath = asset_path_get(document, outputPath.replace_extension(REGION_EXPORT_EXTENSION));
        pathString = path_to_utf8(outputPath);
        WorkingDirectory workingDirectory(document.directory_get());
        path_ensure_directory(outputPath.parent_path());
        if (!exportedImage.write_png(outputPath))
        {
            toast_log(Level::ERROR, TOAST_EXPORT_REGION_FAILED, sourceRegion.name, pathString);
            return false;
        }
    }

    if (options.isMakeSpritesheet || options.isRemoveCurrent)
    {
        document.edit_begin(EDIT_EXPORT_REGION);

        if (options.isRemoveCurrent)
        {
            if (auto source = item_get(document.model.content.spritesheets, options.spritesheetId))
                std::erase_if(source->regions, [&](const Region& region) { return region.id == options.regionId; });
            region_frames_sync(document.model);
            document.selected_clear(SelectionKind::REGIONS);
        }

        if (options.isMakeSpritesheet)
        {
            auto& spritesheets = document.model.content.spritesheets;
            Spritesheet exported = {.id = item_next_id_get(spritesheets), .path = outputPath};

            auto exportedRegion = sourceRegion;
            exportedRegion.uid = uid_next();
            exportedRegion.id = 0;
            exportedRegion.crop = {};
            exportedRegion.size = Vec2(exportSize);
            region_pivot_apply(exportedRegion);
            exported.regions.push_back(exportedRegion);
            spritesheets.push_back(exported);

            document.texture_set(exported.id, exportedImage);
            document.texturePaths[exported.id] = exported.path;
            document.selected_ids_set(SelectionKind::SPRITESHEETS, {exported.id});
            document.focused_id_set(SelectionKind::SPRITESHEETS, exported.id);
            document.selected_ids_set(SelectionKind::REGIONS, {exportedRegion.id});
            document.focused_id_set(SelectionKind::REGIONS, exportedRegion.id);
            if (!outputPath.empty())
                document.spritesheet_hash_set_saved(exported.id);
            else
                document.spritesheetSaveHashes[exported.id] = 0;
        }
        document.change();
    }

    toast_log(Level::INFO, TOAST_EXPORT_REGION, sourceRegion.name, pathString);
    return true;
}

static bool region_drag_drop_update(Panel& panel, int id, int index)
{
    auto& state = panel.state;
    auto selection = panel.document.selected_ids_get(SelectionKind::REGIONS);
    if (ImGui::BeginDragDropSource(DRAG_DROP_SOURCE_FLAGS))
    {
        state.dragIds =
            selection.contains(id) ? std::vector<int>(selection.begin(), selection.end()) : std::vector<int>{id};
        ImGui::SetDragDropPayload(REGION_DRAG_DROP, state.dragIds.data(), state.dragIds.size() * sizeof(int));
        ImGui::EndDragDropSource();
    }

    if (!ImGui::BeginDragDropTarget())
        return false;
    auto isMoved = false;
    if (auto payload = ImGui::AcceptDragDropPayload(REGION_DRAG_DROP, ImGuiDragDropFlags_AcceptBeforeDelivery |
                                                                          ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
    {
        auto cardMin = ImGui::GetWindowPos();
        auto cardMax = ImVec2(cardMin.x + ImGui::GetWindowSize().x, cardMin.y + ImGui::GetWindowSize().y);
        auto isAfter = is_drop_after(cardMin, cardMax);
        drop_line_draw(ImGui::GetForegroundDrawList(), cardMin, cardMax, isAfter);

        std::vector<int> indices;
        auto payloadIds = (const int*)payload->Data;
        for (int i = 0; i < payload->DataSize / (int)sizeof(int); i++)
            if (auto keyIndex = vector_index_find(state.keys, payloadIds[i]); keyIndex != -1)
                indices.push_back(keyIndex);

        if (!indices.empty() && payload->IsDelivery())
        {
            std::sort(indices.begin(), indices.end());
            command_push(panel, [indices, movedIds = state.dragIds, targetIndex = index + (isAfter ? 1 : 0),
                                 spritesheetId =
                                     panel.document.focused_id_get(SelectionKind::SPRITESHEETS)](Document& document) {
                if (!item_get(document.model.content.spritesheets, spritesheetId))
                    return;
                document.edit_apply(EDIT_MOVE_REGIONS, [&](Model& model) {
                    auto spritesheet = item_get(model.content.spritesheets, spritesheetId);
                    vector_indices_move(spritesheet->regions, indices, targetIndex);
                });
                document.selected_ids_set(SelectionKind::REGIONS, {movedIds.begin(), movedIds.end()});
            });
            isMoved = true;
        }
    }
    ImGui::EndDragDropTarget();
    return isMoved;
}

static void region_drag_tooltip_update(Panel& panel)
{
    auto& state = panel.state;
    auto payload = ImGui::GetDragDropPayload();
    auto spritesheet = region_spritesheet_get(panel.document);
    if (!spritesheet || !payload || !payload->IsDataType(REGION_DRAG_DROP) || payload->IsDelivery() ||
        state.dragIds.empty())
        return;

    if (payload->DataFrameCount != ImGui::GetFrameCount() &&
        ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern | DRAG_DROP_SOURCE_FLAGS))
    {
        ImGui::SetDragDropPayload(REGION_DRAG_DROP, state.dragIds.data(), state.dragIds.size() * sizeof(int));
        ImGui::EndDragDropSource();
    }

    auto mousePos = ImGui::GetIO().MousePos;
    ImGui::SetNextWindowPos(ImVec2(mousePos.x + DRAG_TOOLTIP_OFFSET.x, mousePos.y + DRAG_TOOLTIP_OFFSET.y));
    if (!tooltip_begin(state, false))
        return;
    for (auto regionId : state.dragIds)
        if (auto region = item_get(spritesheet->regions, regionId))
            ImGui::TextUnformatted(region->name.c_str());
    tooltip_end();
}

static void region_properties_open(RegionsPanel& regions, Document& document, int id)
{
    auto spritesheet = region_spritesheet_get(document);
    auto region = spritesheet ? item_get(spritesheet->regions, id) : nullptr;
    if (id != -1 && !region)
        return;
    regions.editId = id;
    regions.editRegion = region ? *region : Region{};
    regions.propertiesPopup.open();
}

static void region_properties_popup_update(Panel& panel, RegionsPanel& regions)
{
    auto& popup = regions.propertiesPopup;
    popup.trigger();
    if (!ImGui::BeginPopupModal(popup.label(), &popup.isOpen, ImGuiWindowFlags_NoResize))
        return popup.end();

    auto& region = regions.editRegion;
    if (ImGui::BeginChild("##Child", child_size_get(REGION_POPUP_ROWS), ImGuiChildFlags_Borders))
    {
        if (popup.isJustOpened)
            ImGui::SetKeyboardFocusHere();
        input_text_string(localize.get(BASIC_NAME), &region.name);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_NAME));
        ImGui::DragFloat2(localize.get(BASIC_CROP), values_get(region.crop), DRAG_SPEED, 0.0f, 0.0f,
                          vec2_format_get(region.crop));
        ImGui::DragFloat2(localize.get(BASIC_SIZE), values_get(region.size), DRAG_SPEED, 0.0f, 0.0f,
                          vec2_format_get(region.size));
        ImGui::BeginDisabled(region.origin != Origin::CUSTOM);
        ImGui::DragFloat2(localize.get(BASIC_PIVOT), values_get(region.pivot), DRAG_SPEED, 0.0f, 0.0f,
                          vec2_format_get(region.pivot));
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

    auto result = popup_buttons_draw(panel.manager, localize.get(regions.editId == -1 ? BASIC_ADD : BASIC_CONFIRM));
    if (result == PopupButton::CONFIRM)
    {
        auto& state = panel.state;
        command_push(
            panel, [&state, edited = region, id = regions.editId, insertIndex = region_insert_index_get(panel.document),
                    spritesheetId = panel.document.focused_id_get(SelectionKind::SPRITESHEETS)](Document& document) {
                auto changedId = id;
                document.edit_apply(id == -1 ? EDIT_ADD_REGION : EDIT_SET_REGION_PROPERTIES, [&](Model& model) {
                    auto spritesheet = item_get(model.content.spritesheets, spritesheetId);
                    if (!spritesheet)
                        return;
                    auto& items = spritesheet->regions;
                    if (auto target = item_get(items, id))
                    {
                        auto uid = target->uid;
                        *target = edited;
                        target->uid = uid;
                        target->id = id;
                        return;
                    }
                    if (id != -1)
                        return;
                    changedId = item_next_id_get(items);
                    auto& added = *items.insert(items.begin() + std::min(insertIndex, (int)items.size()), edited);
                    added.uid = uid_next();
                    added.id = changedId;
                    state.newId = changedId;
                });
                if (changedId != -1)
                    region_select(document, changedId);
            });
    }
    if (result != PopupButton::NONE)
        popup.close();
    ImGui::EndPopup();
    popup.end();
}

static void region_export_popup_update(Panel& panel, RegionsPanel& regions)
{
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto spritesheet = region_spritesheet_get(document);
    auto region = spritesheet ? item_get(spritesheet->regions, regions.exportId) : nullptr;
    auto& popup = regions.exportPopup;

    popup.trigger();
    if (!ImGui::BeginPopupModal(popup.label(), &popup.isOpen, ImGuiWindowFlags_NoResize))
        return popup.end();

    if (!region)
        popup.close();
    if (region && popup.isJustOpened)
    {
        auto filename = path_from_utf8(region->name).filename();
        settings.exportRegionPath = settings.exportRegionPath.parent_path() /
                                    (filename.empty() ? std::filesystem::path(REGION_EXPORT_NAME_DEFAULT) : filename)
                                        .concat(REGION_EXPORT_EXTENSION);
    }
    if (dialog.is_selected(Dialog::REGION_EXPORT_PATH_SET))
    {
        settings.exportRegionPath = dialog.path;
        dialog.reset();
    }

    if (ImGui::BeginChild("##Export Region Child", child_size_get(REGION_POPUP_ROWS), ImGuiChildFlags_Borders))
    {
        if (ImGui::ImageButton("##Export Region Path Set", resources.icon_id_get(ICON_FOLDER), icon_size_get()))
            dialog.file_save(Dialog::REGION_EXPORT_PATH_SET, settings.exportRegionPath);
        ImGui::SameLine();
        input_text_path(localize.get(LABEL_OUTPUT_PATH), &settings.exportRegionPath);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OUTPUT_PATH));
        ImGui::Checkbox(localize.get(LABEL_EXPORT_MAKE_SPRITESHEET), &settings.isExportRegionMakeSpritesheet);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_EXPORT_MAKE_SPRITESHEET));
        ImGui::Checkbox(localize.get(LABEL_EXPORT_REMOVE_CURRENT_REGION), &settings.isExportRegionRemoveCurrent);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_EXPORT_REMOVE_CURRENT_REGION));
    }
    ImGui::EndChild();

    auto spritesheetId = document.focused_id_get(SelectionKind::SPRITESHEETS);
    auto texture = document.texture_get(spritesheetId);
    auto isValid = region && texture && texture->is_valid() && !texture->pixels.empty() &&
                   (settings.isExportRegionMakeSpritesheet || !settings.exportRegionPath.empty());
    auto result = popup_buttons_draw(manager, localize.get(BASIC_CONFIRM), isValid);
    if (result == PopupButton::CONFIRM)
    {
        if (!settings.exportRegionPath.empty())
            settings.exportRegionPath.replace_extension(REGION_EXPORT_EXTENSION);
        RegionExportOptions options{.spritesheetId = spritesheetId,
                                    .regionId = regions.exportId,
                                    .path = settings.exportRegionPath,
                                    .isMakeSpritesheet = settings.isExportRegionMakeSpritesheet,
                                    .isRemoveCurrent = settings.isExportRegionRemoveCurrent};
        command_push(panel, [&state, options](Document& document) {
            if (region_export(document, options) && options.isMakeSpritesheet)
                state.newId = document.focused_id_get(SelectionKind::REGIONS);
        });
    }
    if (result != PopupButton::NONE)
        popup.close();
    ImGui::EndPopup();
    popup.end();
}

void regions_update(Panel& panel, RegionsPanel& regions)
{
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    if (manager.isMakeRegionRequested)
    {
        document.focused_id_set(SelectionKind::SPRITESHEETS, manager.makeRegionSpritesheetId);
        regions.editId = -1;
        regions.editRegion = manager.makeRegion;
        regions.propertiesPopup.open();
        manager.isMakeRegionRequested = false;
    }

    if (ImGui::Begin(localize.get(LABEL_REGIONS_WINDOW), &settings.windowIsRegions))
    {
        auto spritesheet = region_spritesheet_get(document);
        if (!spritesheet)
            ImGui::TextUnformatted(localize.get(TEXT_SELECT_SPRITESHEET));
        else
        {
            auto spritesheetId = spritesheet->id;
            auto selection = document.selected_ids_get(SelectionKind::REGIONS);
            auto isOne = [selection]() {
                return selection.size() == 1;
            };

            Actions actions = {};
            actions.add(ACTION_PROPERTIES, isOne,
                        [&]() { region_properties_open(regions, document, *selection.begin()); });
            actions.add(ACTION_ADD, {}, [&]() { region_properties_open(regions, document, -1); }, TOOLTIP_ADD_REGION);
            content_remove_action_add(actions, panel, ElementType::REGION, SelectionKind::REGIONS, spritesheetId);
            actions.add(
                ACTION_TRIM, [selection]() { return !selection.empty(); },
                [&]() {
                    command_push(panel, [spritesheetId, selection](Document& document) {
                        document.edit_apply(EDIT_TRIM_REGIONS, [&](Model&) {
                            if (!document.regions_trim(spritesheetId, selection))
                                return;
                            document.reference_set({document.reference_get().animationIndex});
                            document.frame_references_clear();
                        });
                    });
                },
                TOOLTIP_TRIM_REGIONS);
            actions.add(ACTION_EXPORT, isOne, [&]() {
                regions.exportId = *selection.begin();
                regions.exportPopup.open();
            });
            actions.separator();
            actions.add(
                ACTION_COPY, [selection]() { return !selection.empty(); },
                [&]() { items_copy(clipboard, spritesheet->regions, selection); });
            actions.add(
                ACTION_PASTE, [&]() { return !clipboard.is_empty(); },
                [&]() {
                    command_push(panel, [&state, spritesheetId, text = clipboard.get(),
                                         insertIndex = region_insert_index_get(document)](Document& document) {
                        auto spritesheet_regions_get = [&]() -> const std::vector<Region>* {
                            auto spritesheet = item_get(document.model.content.spritesheets, spritesheetId);
                            return spritesheet ? &spritesheet->regions : nullptr;
                        };
                        if (!spritesheet_regions_get())
                            return;
                        auto nextId = item_next_id_get(*spritesheet_regions_get());
                        std::string errorString;
                        document.edit_apply(EDIT_PASTE_REGIONS, [&](Model& model) {
                            return regions_paste(model, spritesheetId, text, insertIndex, &errorString);
                        });
                        if (!errorString.empty())
                            toast_log(Level::ERROR, TOAST_DESERIALIZE_REGIONS_FAILED, errorString);
                        auto lastId = item_next_id_get(*spritesheet_regions_get()) - 1;
                        if (lastId < nextId)
                            return;
                        state.newId = lastId;
                        region_select(document, lastId);
                    });
                });

            ListRows rows{
                .label_get = [&](int id) { return item_get(spritesheet->regions, id)->name; },
                .tooltip_draw =
                    [&](int id) {
                        auto& region = *item_get(spritesheet->regions, id);
                        tooltip_image_draw(region_image_get(document, resources, region), [&]() {
                            tooltip_name_draw(resources, region.name);
                            ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_CROP),
                                                                std::make_format_args(region.crop.x, region.crop.y))
                                                       .c_str());
                            ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_SIZE),
                                                                std::make_format_args(region.size.x, region.size.y))
                                                       .c_str());
                            auto originLabel = localize.get(ORIGIN_LABELS[(int)region.origin]);
                            ImGui::TextUnformatted(
                                (region.origin == Origin::CUSTOM
                                     ? std::vformat(localize.get(FORMAT_PIVOT),
                                                    std::make_format_args(region.pivot.x, region.pivot.y))
                                     : std::vformat(localize.get(FORMAT_ORIGIN), std::make_format_args(originLabel)))
                                    .c_str());
                        });
                    },
                .image_get =
                    [&](int id) { return region_image_get(document, resources, *item_get(spritesheet->regions, id)); },
                .select =
                    [&](int) {
                        document.editTarget = Document::EditTarget::REGION;
                        document.reference_set({document.reference_get().animationIndex});
                        document.frame_references_clear();
                    },
                .activate = [&](int id) { region_properties_open(regions, document, id); },
                .drag_drop_update = [&](int id, int index) { return region_drag_drop_update(panel, id, index); },
                .cardLines = REGION_CARD_LINES};
            list_panel_draw(
                panel, actions, {{ACTION_ADD, ACTION_EXPORT, ACTION_REMOVE}},
                [&]() {
                    content_list_draw(panel, SelectionKind::REGIONS, item_ids_get(spritesheet->regions), rows);
                    region_drag_tooltip_update(panel);
                },
                true);
        }
    }
    ImGui::End();

    region_properties_popup_update(panel, regions);
    region_export_popup_update(panel, regions);
}
