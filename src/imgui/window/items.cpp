#include "window.hpp"

#include <format>

#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"

using namespace anm2ed::types;

namespace anm2ed::imgui
{
  constexpr int ITEM_PROPERTIES_ROWS = 2;

  void item_tooltip_draw(Resources& resources, const Element& element)
  {
    window_tooltip_name_draw(resources, element.name);
    ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(element.id)).c_str());
  }

  void item_properties_popup_update(Window& window, Manager& manager, Document& document, StringType nameTooltip,
                                    const std::function<void(Element&)>& fields_draw)
  {
    auto& popup = manager.itemPropertiesPopups[track_container_get(window.elementType) - TRACK_CONTAINERS];
    auto reference = window.storage_get(document).reference;

    popup.trigger();
    if (ImGui::BeginPopupModal(popup.label(), &popup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (ImGui::BeginChild("##Child", child_size_get(ITEM_PROPERTIES_ROWS), ImGuiChildFlags_Borders))
      {
        if (popup.isJustOpened) ImGui::SetKeyboardFocusHere();
        input_text_string(localize.get(BASIC_NAME), &manager.itemEdit.name);
        ImGui::SetItemTooltip("%s", localize.get(nameTooltip));
        fields_draw(manager.itemEdit);
      }
      ImGui::EndChild();

      auto result = window_popup_buttons_draw(manager, localize.get(reference == -1 ? BASIC_ADD : BASIC_CONFIRM));
      if (result == PopupButton::CONFIRM) window_element_apply_push(window, manager, manager.itemEdit, reference);
      if (result != PopupButton::NONE) popup.close();
      ImGui::EndPopup();
    }
    popup.end();
  }

  Window item_window_make(StringType title, bool Settings::* isOpen, Document::ChangeType changeType,
                          ElementType elementType, const char* childLabel, StringType addTooltip,
                          StringType removeUnusedTooltip)
  {
    Window window{};
    window.title = title;
    window.isOpen = isOpen;
    window.changeType = changeType;
    window.containerType = ELEMENT_CONTAINERS[(int)elementType];
    window.elementType = elementType;
    window.childLabel = childLabel;
    window.tooltips = {{WINDOW_ADD, addTooltip}, {WINDOW_REMOVE_UNUSED, removeUnusedTooltip}};
    window.footer = {{WINDOW_ADD, WINDOW_REMOVE_UNUSED}};
    window.flags = WINDOW_ADD | WINDOW_REMOVE_UNUSED | WINDOW_COPY | WINDOW_PASTE;
    window.tooltip_draw = [](Document&, Resources& resources, const Element& element)
    { item_tooltip_draw(resources, element); };
    window.properties_open = [elementType](Manager& manager, int id) { manager.item_properties_open(elementType, id); };
    return window;
  }

  Window layers_window_register()
  {
    auto window =
        item_window_make(LABEL_LAYERS_WINDOW, &Settings::windowIsLayers, Document::LAYERS, ElementType::LAYER_ELEMENT,
                         "##Layers Child", TOOLTIP_ADD_LAYER, TOOLTIP_REMOVE_UNUSED_LAYERS);
    window.addEdit = EDIT_ADD_LAYER;
    window.propertiesEdit = EDIT_SET_LAYER_PROPERTIES;
    window.pasteEdit = EDIT_PASTE_LAYERS;
    window.removeUnusedEdit = EDIT_REMOVE_UNUSED_LAYERS;
    window.deserializeFailedToast = TOAST_DESERIALIZE_LAYERS_FAILED;
    window.flags |= WINDOW_PROPERTIES;
    window.storage_get = [](Document& document) -> Storage& { return document.layer; };
    window.row_label_get = [](Document&, const Element& layer)
    {
      return std::vformat(localize.get(FORMAT_LAYER), std::make_format_args(layer.id, layer.name, layer.spritesheetId));
    };
    window.tooltip_draw = [](Document&, Resources& resources, const Element& layer)
    {
      item_tooltip_draw(resources, layer);
      ImGui::TextUnformatted(
          std::vformat(localize.get(FORMAT_SPRITESHEET_ID), std::make_format_args(layer.spritesheetId)).c_str());
    };
    window.popup_update = [](Window& window, Manager& manager, Settings&, Resources&, Clipboard&, Document& document)
    {
      item_properties_popup_update(window, manager, document, TOOLTIP_ITEM_NAME,
                                   [&](Element& layer)
                                   {
                                     combo_id_mapped(localize.get(LABEL_SPRITESHEET), &layer.spritesheetId,
                                                     document.spritesheet.ids, document.spritesheet.labels);
                                     ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_SPRITESHEET));
                                   });
    };
    return window;
  }

  Window nulls_window_register()
  {
    auto window =
        item_window_make(LABEL_NULLS_WINDOW, &Settings::windowIsNulls, Document::NULLS, ElementType::NULL_ELEMENT,
                         "##Nulls Child", TOOLTIP_ADD_NULL, TOOLTIP_REMOVE_UNUSED_NULLS);
    window.addEdit = EDIT_ADD_NULL;
    window.propertiesEdit = EDIT_SET_NULL_PROPERTIES;
    window.pasteEdit = EDIT_PASTE_NULLS;
    window.removeUnusedEdit = EDIT_REMOVE_UNUSED_NULLS;
    window.deserializeFailedToast = TOAST_DESERIALIZE_NULLS_FAILED;
    window.flags |= WINDOW_PROPERTIES | WINDOW_REFERENCE_ITALIC;
    window.storage_get = [](Document& document) -> Storage& { return document.null; };
    window.row_label_get = [](Document&, const Element& null)
    { return std::vformat(localize.get(FORMAT_NULL), std::make_format_args(null.id, null.name)); };
    window.popup_update = [](Window& window, Manager& manager, Settings&, Resources&, Clipboard&, Document& document)
    {
      item_properties_popup_update(window, manager, document, TOOLTIP_NULL_NAME,
                                   [](Element& null)
                                   {
                                     ImGui::Checkbox(localize.get(LABEL_RECT), &null.isShowRect);
                                     ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NULL_RECT));
                                   });
    };
    return window;
  }

  Window events_window_register()
  {
    auto window =
        item_window_make(LABEL_EVENTS_WINDOW, &Settings::windowIsEvents, Document::EVENTS, ElementType::EVENT_ELEMENT,
                         "##Events Child", TOOLTIP_ADD_EVENT, TOOLTIP_REMOVE_UNUSED_EVENTS);
    window.addEdit = EDIT_ADD_EVENT;
    window.renameEdit = EDIT_RENAME_EVENT;
    window.pasteEdit = EDIT_PASTE_EVENTS;
    window.removeUnusedEdit = EDIT_REMOVE_UNUSED_EVENTS;
    window.deserializeFailedToast = TOAST_DESERIALIZE_EVENTS_FAILED;
    window.flags |= WINDOW_RENAME;
    window.properties_open = {};
    window.storage_get = [](Document& document) -> Storage& { return document.event; };
    window.add = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      auto event = element_make(ElementType::EVENT_ELEMENT);
      event.name = localize.get(TEXT_NEW_EVENT);
      auto events = window_container_get(window, document);
      if (!events) return;
      document.edit_apply(window.addEdit, window.changeType,
                          [&](Anm2&)
                          {
                            event.id = element_child_next_id_get(*events, ElementType::EVENT_ELEMENT);
                            events->children.push_back(event);
                            auto& storage = window.storage_get(document);
                            storage.selection = {event.id};
                            storage.reference = event.id;
                            window.newElementId = event.id;
                          });
    };
    return window;
  }
}
