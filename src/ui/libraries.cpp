#include "ui/panel.h"

#include <format>

#include "path.h"

constexpr int LIBRARY_CARD_LINES = 2;

void libraries_update(Panel& panel)
{
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& libraries = document.model.content.libraries;
    auto selection = document.selected_ids_get(SelectionKind::LIBRARIES);

    if (dialog.is_selected(Dialog::LIBRARY_OPEN))
    {
        command_push(panel, [&state, paths = dialog.paths](Document& document) {
            document.libraries_add(paths);
            state.newId = document.focused_id_get(SelectionKind::LIBRARIES);
        });
        dialog.reset();
    }

    if (ImGui::Begin(localize.get(LABEL_LIBRARIES_WINDOW), &settings.windowIsLibraries))
    {
        auto isOne = [selection]() {
            return selection.size() == 1;
        };
        auto loaded_get = [&](int id) {
            return document.library_get(document.model, id);
        };
        // Opens the library as its own tab, or switches to it when it already is one.
        auto open = [&](int id) {
            manager.command_push({.runManager = [path = document.library_path_get(id)](Manager& manager) {
                if (auto index = manager.path_index_get(path); index != -1)
                    return manager.tab_select(index);
                manager.open(path);
            }});
        };

        Actions actions = {};
        actions.add(ACTION_OPEN, isOne, [&]() { open(*selection.begin()); }, TOOLTIP_LIBRARY_OPEN);
        actions.add(ACTION_OPEN_DIRECTORY, isOne,
                    [&]() { directory_open(dialog, document, item_get(libraries, *selection.begin())->path); });
        actions.add(ACTION_ADD, {}, [&]() { dialog.file_open(Dialog::LIBRARY_OPEN); }, TOOLTIP_LIBRARY_ADD);
        content_remove_action_add(actions, panel, ElementType::LIBRARY, SelectionKind::LIBRARIES);
        actions.add(
            ACTION_RELOAD, [selection]() { return !selection.empty(); },
            [&]() {
                command_push(panel, [selection](Document& document) {
                    for (auto id : selection)
                        if (auto library = item_get(document.model.content.libraries, id))
                        {
                            document.library_reload(id);
                            toast_log(Level::INFO, TOAST_RELOAD_LIBRARY, id, path_to_utf8(library->path));
                        }
                });
            },
            TOOLTIP_RELOAD_LIBRARIES);
        items_clipboard_actions_add<Library>(actions, panel, SelectionKind::LIBRARIES, EDIT_PASTE_LIBRARIES,
                                             TOAST_DESERIALIZE_LIBRARIES_FAILED, &Content::libraries);

        ListRows rows{.label_get =
                          [&](int id) {
                              auto pathString = path_to_utf8(item_get(libraries, id)->path);
                              return localize_format(FORMAT_ID_PATH, id, pathString);
                          },
                      .tooltip_draw =
                          [&](int id) {
                              auto loaded = loaded_get(id);
                              tooltip_name_draw(resources, path_to_utf8(item_get(libraries, id)->path));
                              ImGui::Text("%s: %d", localize.get(BASIC_ID), id);
                              if (loaded && loaded->model.isValid)
                              {
                                  auto animations = loaded->model.animations_get();
                                  auto count = animations.size();
                                  ImGui::SeparatorText(std::vformat(localize.get(FORMAT_LIBRARY_ANIMATIONS),
                                                                    std::make_format_args(count))
                                                           .c_str());
                                  for (auto [index, animation] : animations)
                                      ImGui::BulletText("%s", animation->name.c_str());
                                  ImGui::Separator();
                                  ImGui::TextUnformatted(localize.get(TOOLTIP_LIBRARY_OPEN));
                              }
                              else
                              {
                                  ImGui::Spacing();
                                  ImGui::TextWrapped("%s", localize.get(TOOLTIP_LIBRARY_INVALID));
                              }
                          },
                      .image_get =
                          [&](int id) {
                              auto loaded = loaded_get(id);
                              auto isValid = loaded && loaded->model.isValid;
                              return icon_card_get(resources, ICON_ANIMATION, isValid);
                          },
                      .activate = open,
                      .cardLines = LIBRARY_CARD_LINES};
        list_panel_draw(
            panel, actions, {{ACTION_ADD, ACTION_REMOVE, ACTION_RELOAD, ACTION_OPEN}},
            [&]() { content_list_draw(panel, SelectionKind::LIBRARIES, item_ids_get(libraries), rows); }, LIST_CARDS);
    }
    ImGui::End();
}
