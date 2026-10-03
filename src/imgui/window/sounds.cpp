#include "window.hpp"

#include <format>

#include "path.hpp"
#include "toast.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed::imgui
{
  constexpr int SOUND_CARD_LINES = 2;

  Window sounds_window_register()
  {
    Window window{};
    window.title = LABEL_SOUNDS_WINDOW;
    window.isOpen = &Settings::windowIsSounds;
    window.changeType = Document::SOUNDS;
    window.containerType = ElementType::SOUNDS;
    window.elementType = ElementType::SOUND_ELEMENT;
    window.childLabel = "##Sounds Child";
    window.cardLines = SOUND_CARD_LINES;
    window.pasteEdit = TOAST_SOUNDS_PASTE;
    window.removeUnusedEdit = EDIT_REMOVE_UNUSED_SOUNDS;
    window.deserializeFailedToast = TOAST_SOUNDS_DESERIALIZE_ERROR;
    window.flags = WINDOW_PLAY | WINDOW_OPEN_DIRECTORY | WINDOW_ADD | WINDOW_REMOVE_UNUSED | WINDOW_RELOAD |
                   WINDOW_REPLACE | WINDOW_COPY | WINDOW_PASTE;
    window.footer = {{WINDOW_ADD, WINDOW_REMOVE_UNUSED, WINDOW_RELOAD, WINDOW_REPLACE}};
    window.tooltips = {{WINDOW_ADD, TOOLTIP_SOUND_ADD},
                       {WINDOW_REMOVE_UNUSED, TOOLTIP_REMOVE_UNUSED_SOUNDS},
                       {WINDOW_RELOAD, TOOLTIP_RELOAD_SOUNDS},
                       {WINDOW_REPLACE, TOOLTIP_REPLACE_SOUND}};
    window.storage_get = [](Document& document) -> Storage& { return document.sound; };
    window.row_label_get = [](Document&, const Element& sound)
    {
      auto pathString = path::to_utf8(sound.path);
      return std::vformat(localize.get(FORMAT_SOUND), std::make_format_args(sound.id, pathString));
    };
    window.row_select = [](Window&, Document& document, int id)
    {
      if (auto audio = document.sound_get(id); audio && ImGui::IsItemClicked(ImGuiMouseButton_Left))
        resource::audio::play(*audio);
    };
    window.card_image_get = [](Document& document, Resources& resources, const Element& sound)
    {
      auto audio = document.sound_get(sound.id);
      auto isValid = audio && audio->is_valid();
      auto& texture = resources.icons[isValid ? icon::SOUND : icon::NONE];
      return WindowCardImage{.texture = &texture, .size = glm::vec2(texture.size), .isValid = isValid};
    };
    window.tooltip_draw = [](Document& document, Resources& resources, const Element& sound)
    {
      auto audio = document.sound_get(sound.id);
      window_tooltip_name_draw(resources, path::to_utf8(sound.path));
      ImGui::Text("%s: %d", localize.get(BASIC_ID), sound.id);
      if (audio && audio->is_valid())
        ImGui::TextUnformatted(localize.get(TEXT_OPEN_DIRECTORY));
      else
      {
        ImGui::Spacing();
        ImGui::TextWrapped("%s", localize.get(TOOLTIP_SOUND_INVALID));
      }
    };
    window.add = [](Window& window, Manager&, Settings&, Document&, Clipboard&)
    {
      if (window.dialog) window.dialog->file_open(Dialog::SOUND_OPEN, true);
    };
    window.replace = [](Window& window, Manager&, Settings&, Document&, Clipboard&)
    {
      if (window.dialog) window.dialog->file_open(Dialog::SOUND_REPLACE);
    };
    window.play = [](Window&, Manager&, Settings&, Document& document, Clipboard&)
    {
      if (auto audio = document.sound_get(*document.sound.selection.begin())) resource::audio::play(*audio);
    };
    window.open = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto sound = window_element_get(window, document, document.sound.reference);
      if (sound && window.dialog) window_directory_open(*window.dialog, document, sound->path);
    };
    window.reload = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      window_edit(document, window.changeType, localize.get(EDIT_RELOAD_SOUNDS),
                  [&]()
                  {
                    for (auto id : document.sound.selection)
                      if (auto sound = window_element_get(window, document, id))
                      {
                        document.sound_reload(id);
                        toast_log(Level::INFO, TOAST_RELOAD_SOUND, id, path::to_utf8(sound->path));
                      }
                  });
    };
    window.body_update = [](Window& window, Manager& manager, Settings&, Resources&, Clipboard&, Document& document)
    {
      if (!window.dialog) return;
      if (window.dialog->is_selected(Dialog::SOUND_OPEN))
      {
        manager.command_push({manager.selected, [&window, paths = window.dialog->paths](Manager&, Document& document)
                              {
                                document.sounds_add(paths);
                                window.newElementId = document.sound.reference;
                              }});
        window.dialog->reset();
      }

      if (window.dialog->is_selected(Dialog::SOUND_REPLACE))
      {
        if (document.sound.selection.size() == 1 && !window.dialog->path.empty())
          manager.command_push({manager.selected, [&window, id = *document.sound.selection.begin(),
                                                   dialogPath = window.dialog->path](Manager&, Document& document)
                                {
                                  auto sound = window_element_get(window, document, id);
                                  if (!sound) return;
                                  window_edit(document, window.changeType, localize.get(EDIT_REPLACE_SOUND),
                                              [&]()
                                              {
                                                sound->path = window_asset_path_get(document, dialogPath);
                                                document.sound_reload(id);
                                                toast_log(Level::INFO, TOAST_REPLACE_SOUND, id,
                                                          path::to_utf8(sound->path));
                                              });
                                }});
        window.dialog->reset();
      }
    };
    return window;
  }
}
