#include "panel.hpp"

#include <format>

#include "path.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed::imgui
{
  constexpr int LIBRARY_CARD_LINES = 2;

  auto& libraries_get(model::Model& model) { return model.content.libraries; }

  void libraries_update(Panel& panel)
  {
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& libraries = document.model.content.libraries;
    auto selection = document.selected_ids_get(SelectionKind::LIBRARIES);

    if (dialog.is_selected(Dialog::LIBRARY_OPEN))
    {
      command_push(panel,
                   [&state, paths = dialog.paths](Document& document)
                   {
                     document.libraries_add(paths);
                     state.newId = document.focused_id_get(SelectionKind::LIBRARIES);
                   });
      dialog.reset();
    }

    if (ImGui::Begin(localize.get(LABEL_LIBRARIES_WINDOW), &settings.windowIsLibraries))
    {
      auto isOne = [selection]() { return selection.size() == 1; };
      auto loaded_get = [&](int id) { return document.library_get(document.model, id); };
      // Opens the library as its own tab, or switches to it when it already is one.
      auto open = [&](int id)
      {
        manager.command_push({.runManager = [path = document.library_path_get(id)](Manager& manager)
                              {
                                for (int i = 0; i < (int)manager.documents.size(); ++i)
                                {
                                  std::error_code ec{};
                                  if (!std::filesystem::equivalent(manager.documents[i].path, path, ec)) continue;
                                  manager.pendingSelected = i;
                                  return manager.set(i);
                                }
                                manager.open(path);
                              }});
      };

      Actions actions{};
      actions.add(ACTION_OPEN, isOne, [&]() { open(*selection.begin()); }, TOOLTIP_LIBRARY_OPEN);
      actions.add(ACTION_OPEN_DIRECTORY, isOne,
                  [&]() { directory_open(dialog, document, model::item_get(libraries, *selection.begin())->path); });
      actions.add(ACTION_ADD, {}, [&]() { dialog.file_open(Dialog::LIBRARY_OPEN, true); }, TOOLTIP_LIBRARY_ADD);
      actions.add(
          ACTION_REMOVE_UNUSED, {},
          [&]() { items_unused_remove(panel, ElementType::LIBRARY, EDIT_REMOVE_UNUSED_LIBRARIES, libraries_get); },
          TOOLTIP_REMOVE_UNUSED_LIBRARIES);
      actions.add(
          ACTION_RELOAD, [selection]() { return !selection.empty(); },
          [&]()
          {
            command_push(panel,
                         [selection](Document& document)
                         {
                           for (auto id : selection)
                             if (auto library = model::item_get(document.model.content.libraries, id))
                             {
                               document.library_reload(id);
                               toast_log(Level::INFO, TOAST_RELOAD_LIBRARY, id, path::to_utf8(library->path));
                             }
                         });
          },
          TOOLTIP_RELOAD_LIBRARIES);
      items_clipboard_actions_add<model::Library>(actions, panel, SelectionKind::LIBRARIES, EDIT_PASTE_LIBRARIES,
                                                  TOAST_DESERIALIZE_LIBRARIES_FAILED, libraries_get);

      ListRows rows{
          .label_get =
              [&](int id)
          {
            auto pathString = path::to_utf8(model::item_get(libraries, id)->path);
            return std::vformat(localize.get(FORMAT_SOUND), std::make_format_args(id, pathString));
          },
          .tooltip_draw =
              [&](int id)
          {
            auto loaded = loaded_get(id);
            tooltip_name_draw(resources, path::to_utf8(model::item_get(libraries, id)->path));
            ImGui::Text("%s: %d", localize.get(BASIC_ID), id);
            if (loaded && loaded->model.isValid)
            {
              auto animations = loaded->model.animations_get();
              auto count = animations.size();
              ImGui::SeparatorText(
                  std::vformat(localize.get(FORMAT_LIBRARY_ANIMATIONS), std::make_format_args(count)).c_str());
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
              [&](int id)
          {
            auto loaded = loaded_get(id);
            auto isValid = loaded && loaded->model.isValid;
            auto& texture = resources.icons[isValid ? icon::ANIMATION : icon::NONE];
            return CardImage{.texture = &texture, .size = glm::vec2(texture.size), .isValid = isValid};
          },
          .activate = open,
          .cardLines = LIBRARY_CARD_LINES};
      list_panel_draw(
          panel, actions, {{ACTION_ADD, ACTION_REMOVE_UNUSED, ACTION_RELOAD, ACTION_OPEN}},
          [&]() { content_list_draw(panel, SelectionKind::LIBRARIES, item_ids_get(libraries), rows); }, true);
    }
    ImGui::End();
  }
}
