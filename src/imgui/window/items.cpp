#include "window.hpp"

#include <format>

#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"

using namespace anm2ed::types;

namespace anm2ed::imgui
{
  constexpr int ITEM_PROPERTIES_ROWS = 2;

  std::vector<model::Layer>* layers_get(Document&, model::Model& model) { return &model.content.layers; }
  std::vector<model::Null>* nulls_get(Document&, model::Model& model) { return &model.content.nulls; }

  void item_tooltip_draw(Resources& resources, const std::string& name, int id)
  {
    window_tooltip_name_draw(resources, name);
    ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());
  }

  void item_tooltip_set(Window& window)
  {
    window.tooltip_draw = [name_get = window.name_get](Document& document, Resources& resources, int id)
    { item_tooltip_draw(resources, name_get(document, id), id); };
  }

  template <class Item>
  void item_properties_popup_update(Window& window, Manager& manager, Document& document, StringType nameTooltip,
                                    std::function<std::vector<Item>*(Document&, model::Model&)> items_get,
                                    const std::function<void(Manager::ItemEdit&)>& fields_draw)
  {
    auto& popup = manager.itemPropertiesPopups[window.elementType == ElementType::LAYER_ELEMENT ? 0 : 1];
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
      if (result == PopupButton::CONFIRM)
      {
        Item item{.name = manager.itemEdit.name};
        if constexpr (requires { item.spritesheetId; }) item.spritesheetId = manager.itemEdit.spritesheetId;
        if constexpr (requires { item.isShowRect; }) item.isShowRect = manager.itemEdit.isShowRect;
        window_item_apply_push(window, manager, item, reference, items_get);
      }
      if (result != PopupButton::NONE) popup.close();
      ImGui::EndPopup();
    }
    popup.end();
  }

  Window item_window_make(StringType title, bool Settings::* isOpen, ElementType elementType, const char* childLabel,
                          StringType addTooltip, StringType removeUnusedTooltip)
  {
    Window window{};
    window.title = title;
    window.isOpen = isOpen;
    window.elementType = elementType;
    window.childLabel = childLabel;
    window.tooltips = {{WINDOW_ADD, addTooltip}, {WINDOW_REMOVE_UNUSED, removeUnusedTooltip}};
    window.footer = {{WINDOW_ADD, WINDOW_REMOVE_UNUSED}};
    window.flags = WINDOW_ADD | WINDOW_REMOVE_UNUSED | WINDOW_COPY | WINDOW_PASTE;
    window.properties_open = [elementType](Manager& manager, int id) { manager.item_properties_open(elementType, id); };
    return window;
  }

  Window layers_window_register()
  {
    auto window = item_window_make(LABEL_LAYERS_WINDOW, &Settings::windowIsLayers, ElementType::LAYER_ELEMENT,
                                   "##Layers Child", TOOLTIP_ADD_LAYER, TOOLTIP_REMOVE_UNUSED_LAYERS);
    window.addEdit = EDIT_ADD_LAYER;
    window.propertiesEdit = EDIT_SET_LAYER_PROPERTIES;
    window.pasteEdit = EDIT_PASTE_LAYERS;
    window.removeUnusedEdit = EDIT_REMOVE_UNUSED_LAYERS;
    window.deserializeFailedToast = TOAST_DESERIALIZE_LAYERS_FAILED;
    window.flags |= WINDOW_PROPERTIES;
    window.storage_get = [](Document& document) -> Storage& { return document.layer; };
    window_items_bind<model::Layer>(window, layers_get);
    window.row_label_get = [](Document& document, int id)
    {
      auto layer = model::item_get(document.model.content.layers, id);
      return std::vformat(localize.get(FORMAT_LAYER),
                          std::make_format_args(layer->id, layer->name, layer->spritesheetId));
    };
    window.tooltip_draw = [](Document& document, Resources& resources, int id)
    {
      auto layer = model::item_get(document.model.content.layers, id);
      item_tooltip_draw(resources, layer->name, id);
      ImGui::TextUnformatted(
          std::vformat(localize.get(FORMAT_SPRITESHEET_ID), std::make_format_args(layer->spritesheetId)).c_str());
    };
    window.popup_update = [](Window& window, Manager& manager, Settings&, Resources&, Clipboard&, Document& document)
    {
      item_properties_popup_update<model::Layer>(window, manager, document, TOOLTIP_ITEM_NAME, layers_get,
                                                 [&](Manager::ItemEdit& layer)
                                                 {
                                                   combo_id_mapped(localize.get(LABEL_SPRITESHEET),
                                                                   &layer.spritesheetId, document.spritesheet.ids,
                                                                   document.spritesheet.labels);
                                                   ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_SPRITESHEET));
                                                 });
    };
    return window;
  }

  Window nulls_window_register()
  {
    auto window = item_window_make(LABEL_NULLS_WINDOW, &Settings::windowIsNulls, ElementType::NULL_ELEMENT,
                                   "##Nulls Child", TOOLTIP_ADD_NULL, TOOLTIP_REMOVE_UNUSED_NULLS);
    window.addEdit = EDIT_ADD_NULL;
    window.propertiesEdit = EDIT_SET_NULL_PROPERTIES;
    window.pasteEdit = EDIT_PASTE_NULLS;
    window.removeUnusedEdit = EDIT_REMOVE_UNUSED_NULLS;
    window.deserializeFailedToast = TOAST_DESERIALIZE_NULLS_FAILED;
    window.flags |= WINDOW_PROPERTIES | WINDOW_REFERENCE_ITALIC;
    window.storage_get = [](Document& document) -> Storage& { return document.null; };
    window_items_bind<model::Null>(window, nulls_get);
    item_tooltip_set(window);
    window.row_label_get = [name_get = window.name_get](Document& document, int id)
    {
      auto name = name_get(document, id);
      return std::vformat(localize.get(FORMAT_NULL), std::make_format_args(id, name));
    };
    window.popup_update = [](Window& window, Manager& manager, Settings&, Resources&, Clipboard&, Document& document)
    {
      item_properties_popup_update<model::Null>(window, manager, document, TOOLTIP_NULL_NAME, nulls_get,
                                                [](Manager::ItemEdit& null)
                                                {
                                                  ImGui::Checkbox(localize.get(LABEL_RECT), &null.isShowRect);
                                                  ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NULL_RECT));
                                                });
    };
    return window;
  }

  Window events_window_register()
  {
    auto window = item_window_make(LABEL_EVENTS_WINDOW, &Settings::windowIsEvents, ElementType::EVENT_ELEMENT,
                                   "##Events Child", TOOLTIP_ADD_EVENT, TOOLTIP_REMOVE_UNUSED_EVENTS);
    window.addEdit = EDIT_ADD_EVENT;
    window.renameEdit = EDIT_RENAME_EVENT;
    window.pasteEdit = EDIT_PASTE_EVENTS;
    window.removeUnusedEdit = EDIT_REMOVE_UNUSED_EVENTS;
    window.deserializeFailedToast = TOAST_DESERIALIZE_EVENTS_FAILED;
    window.flags |= WINDOW_RENAME;
    window.properties_open = {};
    window.storage_get = [](Document& document) -> Storage& { return document.event; };
    window_items_bind<model::Event>(window, [](Document&, model::Model& model) { return &model.content.events; });
    item_tooltip_set(window);
    window.add = [](Window& window, Manager&, Settings&, Document& document, Clipboard&)
    {
      document.edit_apply(window.addEdit,
                          [&](model::Model& model)
                          {
                            auto& events = model.content.events;
                            auto& event = events.emplace_back(model::Event{.id = model::item_next_id_get(events),
                                                                           .name = localize.get(TEXT_NEW_EVENT)});
                            auto& storage = window.storage_get(document);
                            storage.selection = {event.id};
                            storage.reference = event.id;
                            window.newElementId = event.id;
                          });
    };
    return window;
  }
}
