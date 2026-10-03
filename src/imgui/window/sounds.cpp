#include "panel.hpp"

#include <format>

#include "path.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed::imgui
{
  constexpr int SOUND_CARD_LINES = 2;

  auto& sounds_get(model::Model& model) { return model.content.sounds; }

  void sounds_update(Panel& panel)
  {
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& sounds = document.model.content.sounds;
    auto selection = document.selected_ids_get(SelectionKind::SOUNDS);

    if (dialog.is_selected(Dialog::SOUND_OPEN))
    {
      command_push(panel,
                   [&state, paths = dialog.paths](Document& document)
                   {
                     document.sounds_add(paths);
                     state.newId = document.focused_id_get(SelectionKind::SOUNDS);
                   });
      dialog.reset();
    }

    if (dialog.is_selected(Dialog::SOUND_REPLACE))
    {
      if (selection.size() == 1 && !dialog.path.empty())
        command_push(panel,
                     [id = *selection.begin(), dialogPath = dialog.path](Document& document)
                     {
                       if (!model::item_get(document.model.content.sounds, id)) return;
                       document.edit_apply(EDIT_REPLACE_SOUND,
                                           [&](model::Model& model)
                                           {
                                             auto sound = model::item_get(model.content.sounds, id);
                                             sound->path = asset_path_get(document, dialogPath);
                                             document.sound_reload(id);
                                             toast_log(Level::INFO, TOAST_REPLACE_SOUND, id,
                                                       path::to_utf8(sound->path));
                                           });
                     });
      dialog.reset();
    }

    if (ImGui::Begin(localize.get(LABEL_SOUNDS_WINDOW), &settings.windowIsSounds))
    {
      auto isOne = [selection]() { return selection.size() == 1; };
      auto play = [&](int id)
      {
        if (auto audio = document.sound_get(id)) resource::audio::play(*audio);
      };
      auto open = [&](int id)
      {
        if (auto sound = model::item_get(sounds, id)) directory_open(dialog, document, sound->path);
      };

      Actions actions{};
      actions.add(ACTION_PLAY, isOne, [&]() { play(*selection.begin()); }, STRING_UNDEFINED, -1);
      actions.add(ACTION_OPEN_DIRECTORY, isOne, [&]() { open(*selection.begin()); });
      actions.add(ACTION_ADD, {}, [&]() { dialog.file_open(Dialog::SOUND_OPEN, true); }, TOOLTIP_SOUND_ADD);
      actions.add(
          ACTION_REMOVE_UNUSED, {},
          [&]() { items_unused_remove(panel, ElementType::SOUND_ELEMENT, EDIT_REMOVE_UNUSED_SOUNDS, sounds_get); },
          TOOLTIP_REMOVE_UNUSED_SOUNDS);
      actions.add(
          ACTION_RELOAD, [selection]() { return !selection.empty(); },
          [&]()
          {
            command_push(panel,
                         [selection](Document& document)
                         {
                           document.edit_apply(EDIT_RELOAD_SOUNDS,
                                               [&](model::Model& model)
                                               {
                                                 for (auto id : selection)
                                                   if (auto sound = model::item_get(model.content.sounds, id))
                                                   {
                                                     document.sound_reload(id);
                                                     toast_log(Level::INFO, TOAST_RELOAD_SOUND, id,
                                                               path::to_utf8(sound->path));
                                                   }
                                               });
                         });
          },
          TOOLTIP_RELOAD_SOUNDS);
      actions.add(ACTION_REPLACE, isOne, [&]() { dialog.file_open(Dialog::SOUND_REPLACE); }, TOOLTIP_REPLACE_SOUND);
      items_clipboard_actions_add<model::Sound>(actions, panel, SelectionKind::SOUNDS, TOAST_SOUNDS_PASTE,
                                                TOAST_SOUNDS_DESERIALIZE_ERROR, sounds_get);

      ListRows rows{.label_get =
                        [&](int id)
                    {
                      auto pathString = path::to_utf8(model::item_get(sounds, id)->path);
                      return std::vformat(localize.get(FORMAT_SOUND), std::make_format_args(id, pathString));
                    },
                    .tooltip_draw =
                        [&](int id)
                    {
                      auto audio = document.sound_get(id);
                      tooltip_name_draw(resources, path::to_utf8(model::item_get(sounds, id)->path));
                      ImGui::Text("%s: %d", localize.get(BASIC_ID), id);
                      if (audio && audio->is_valid())
                        ImGui::TextUnformatted(localize.get(TEXT_OPEN_DIRECTORY));
                      else
                      {
                        ImGui::Spacing();
                        ImGui::TextWrapped("%s", localize.get(TOOLTIP_SOUND_INVALID));
                      }
                    },
                    .image_get =
                        [&](int id)
                    {
                      auto audio = document.sound_get(id);
                      auto isValid = audio && audio->is_valid();
                      auto& texture = resources.icons[isValid ? icon::SOUND : icon::NONE];
                      return CardImage{.texture = &texture, .size = glm::vec2(texture.size), .isValid = isValid};
                    },
                    .select =
                        [&](int id)
                    {
                      if (id != -1 && ImGui::IsItemClicked(ImGuiMouseButton_Left)) play(id);
                    },
                    .activate = open,
                    .cardLines = SOUND_CARD_LINES};
      list_panel_draw(panel, actions, {{ACTION_ADD, ACTION_REMOVE_UNUSED, ACTION_RELOAD, ACTION_REPLACE}},
                      [&]() { content_list_draw(panel, SelectionKind::SOUNDS, item_ids_get(sounds), rows); });
    }
    ImGui::End();
  }
}
