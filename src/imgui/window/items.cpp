#include "panel.hpp"

#include <format>

#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;

namespace anm2ed::imgui
{
  constexpr int NULL_PROPERTIES_ROWS = 2;
  constexpr int LAYER_PROPERTIES_ROWS = 6;

  auto& layers_get(model::Model& model) { return model.content.layers; }
  auto& nulls_get(model::Model& model) { return model.content.nulls; }
  auto& events_get(model::Model& model) { return model.content.events; }

  void item_tooltip_draw(Resources& resources, const std::string& name, int id)
  {
    tooltip_name_draw(resources, name);
    ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());
  }

  // "#id name", then what the layer shows: its spritesheet or its animation.
  std::string layer_label_get(const model::Layer& layer)
  {
    if (layer.is_animation())
      return std::vformat(localize.get(FORMAT_LAYER_ANIMATION),
                          std::make_format_args(layer.id, layer.name, layer.animation));
    return std::vformat(localize.get(FORMAT_LAYER), std::make_format_args(layer.id, layer.name, layer.spritesheetId));
  }

  model::Layer item_edit_layer_get(const Manager::ItemEdit& edit)
  {
    return {.name = edit.name,
            .spritesheetId = edit.spritesheetId,
            .libraryId = edit.isAnimation ? edit.libraryId : -1,
            .animation = edit.isAnimation ? edit.animation : std::string{}};
  }

  model::Null item_edit_null_get(const Manager::ItemEdit& edit)
  {
    return {.name = edit.name, .isShowRect = edit.isShowRect};
  }

  // A layer's source: a spritesheet, or an animation of this file ("Self") or a library.
  void layer_source_draw(Document& document, Manager::ItemEdit& layer)
  {
    ImGui::PushID("##Source");
    for (auto [label, isAnimation] : {std::pair{LABEL_SPRITESHEET, false}, std::pair{LABEL_ANIMATION, true}})
    {
      if (isAnimation) ImGui::SameLine();
      if (ImGui::RadioButton(localize.get(label), layer.isAnimation == isAnimation)) layer.isAnimation = isAnimation;
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_SOURCE));
    }
    ImGui::PopID();
    ImGui::Separator();

    if (!layer.isAnimation)
    {
      auto spritesheets = document.choices_get(SelectionKind::SPRITESHEETS);
      combo_id_mapped(localize.get(LABEL_SPRITESHEET), &layer.spritesheetId, spritesheets.ids, spritesheets.labels);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_SPRITESHEET));
      return;
    }

    auto libraries = document.choices_get(SelectionKind::LIBRARIES);
    libraries.labels.front() = localize.get(TEXT_SELF);
    combo_id_mapped(localize.get(LABEL_LIBRARY), &layer.libraryId, libraries.ids, libraries.labels);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_LIBRARY));
    auto names = document.animation_names_get(layer.libraryId);
    if (layer.animation.empty() && !names.empty()) layer.animation = names.front();
    combo_string_mapped(localize.get(LABEL_ANIMATION), &layer.animation, names, names);
    ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_ANIMATION));
  }

  // The layer/null properties popup: adds a new item (id -1) or replaces the edited one, then selects it.
  template <class Item, class ItemsGet, class FieldsDraw>
  void item_properties_update(Panel& panel, int popupIndex, int rows, SelectionKind kind, StringType nameTooltip,
                              StringType addEdit, StringType propertiesEdit, ItemsGet items_get,
                              FieldsDraw&& fields_draw)
  {
    auto& manager = panel.manager;
    auto& popup = manager.itemPropertiesPopups[popupIndex];
    auto& edit = manager.itemEdit;

    popup.trigger();
    if (ImGui::BeginPopupModal(popup.label(), &popup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (ImGui::BeginChild("##Child", child_size_get(rows), ImGuiChildFlags_Borders))
      {
        if (popup.isJustOpened) ImGui::SetKeyboardFocusHere();
        input_text_string(localize.get(BASIC_NAME), &edit.name);
        ImGui::SetItemTooltip("%s", localize.get(nameTooltip));
        fields_draw(edit);
      }
      ImGui::EndChild();

      auto result = popup_buttons_draw(manager, localize.get(edit.id == -1 ? BASIC_ADD : BASIC_CONFIRM));
      if (result == PopupButton::CONFIRM)
      {
        Item item{};
        if constexpr (std::is_same_v<Item, model::Layer>)
          item = item_edit_layer_get(edit);
        else
          item = item_edit_null_get(edit);
        auto& state = panel.state;
        command_push(panel,
                     [&state, item, id = edit.id, kind, addEdit, propertiesEdit, items_get](Document& document)
                     {
                       auto changedId = id;
                       document.edit_apply(id == -1 ? addEdit : propertiesEdit,
                                           [&](model::Model& model)
                                           {
                                             auto& items = items_get(model);
                                             if (auto target = model::item_get(items, id))
                                             {
                                               auto uid = target->uid;
                                               *target = item;
                                               target->uid = uid;
                                               target->id = id;
                                               return;
                                             }
                                             if (id != -1) return;
                                             changedId = model::item_next_id_get(items);
                                             items.push_back(item);
                                             items.back().id = changedId;
                                             state.newId = changedId;
                                           });
                       document.selected_ids_set(kind, {changedId});
                       document.focused_id_set(kind, changedId);
                     });
      }
      if (result != PopupButton::NONE) popup.close();
      ImGui::EndPopup();
    }
    popup.end();
  }

  // Layers and nulls: properties popup, add, remove unused, copy and paste.
  template <class Item, class ItemsGet>
  void track_items_actions_add(Actions& actions, Panel& panel, SelectionKind kind, ElementType type,
                               StringType addTooltip, StringType removeUnusedTooltip, StringType removeUnusedEdit,
                               StringType pasteEdit, StringType pasteFailToast, ItemsGet items_get)
  {
    auto selection = panel.document.selected_ids_get(kind);
    auto& manager = panel.manager;
    actions.add(
        ACTION_PROPERTIES, [selection]() { return selection.size() == 1; },
        [&manager, type, selection]() { manager.item_properties_open(type, *selection.begin()); });
    actions.add(ACTION_ADD, {}, [&manager, type]() { manager.item_properties_open(type, -1); }, addTooltip);
    actions.add(
        ACTION_REMOVE_UNUSED, {}, [&panel, type, removeUnusedEdit, items_get]()
        { items_unused_remove(panel, type, removeUnusedEdit, items_get); }, removeUnusedTooltip);
    items_clipboard_actions_add<Item>(actions, panel, kind, pasteEdit, pasteFailToast, items_get);
  }

  void layers_update(Panel& panel)
  {
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& layers = document.model.content.layers;
    if (ImGui::Begin(localize.get(LABEL_LAYERS_WINDOW), &settings.windowIsLayers))
    {
      Actions actions{};
      track_items_actions_add<model::Layer>(actions, panel, SelectionKind::LAYERS, ElementType::LAYER_ELEMENT,
                                            TOOLTIP_ADD_LAYER, TOOLTIP_REMOVE_UNUSED_LAYERS, EDIT_REMOVE_UNUSED_LAYERS,
                                            EDIT_PASTE_LAYERS, TOAST_DESERIALIZE_LAYERS_FAILED, layers_get);
      ListRows rows{.label_get = [&](int id) { return layer_label_get(*model::item_get(layers, id)); },
                    .is_invalid = [&](int id)
                    { return document.is_layer_animation_missing(*model::item_get(layers, id)); },
                    .tooltip_draw =
                        [&](int id)
                    {
                      auto layer = model::item_get(layers, id);
                      item_tooltip_draw(resources, layer->name, id);
                      if (document.is_layer_animation_missing(*layer))
                        ImGui::TextUnformatted(localize.get(TOOLTIP_LAYER_ANIMATION_MISSING));
                      else if (!layer->is_animation())
                        ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_SPRITESHEET_ID),
                                                            std::make_format_args(layer->spritesheetId))
                                                   .c_str());
                    },
                    .activate = [&](int id) { manager.item_properties_open(ElementType::LAYER_ELEMENT, id); }};
      list_panel_draw(panel, actions, {{ACTION_ADD, ACTION_REMOVE_UNUSED}},
                      [&]() { content_list_draw(panel, SelectionKind::LAYERS, item_ids_get(layers), rows); });
    }
    ImGui::End();

    item_properties_update<model::Layer>(panel, 0, LAYER_PROPERTIES_ROWS, SelectionKind::LAYERS, TOOLTIP_ITEM_NAME,
                                         EDIT_ADD_LAYER, EDIT_SET_LAYER_PROPERTIES, layers_get,
                                         [&](Manager::ItemEdit& layer)
                                         {
                                           ImGui::SeparatorText(localize.get(LABEL_LAYER_SOURCE));
                                           layer_source_draw(document, layer);
                                         });
  }

  void nulls_update(Panel& panel)
  {
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& nulls = document.model.content.nulls;
    if (ImGui::Begin(localize.get(LABEL_NULLS_WINDOW), &settings.windowIsNulls))
    {
      Actions actions{};
      track_items_actions_add<model::Null>(actions, panel, SelectionKind::NULLS, ElementType::NULL_ELEMENT,
                                           TOOLTIP_ADD_NULL, TOOLTIP_REMOVE_UNUSED_NULLS, EDIT_REMOVE_UNUSED_NULLS,
                                           EDIT_PASTE_NULLS, TOAST_DESERIALIZE_NULLS_FAILED, nulls_get);
      ListRows rows{.label_get =
                        [&](int id)
                    {
                      auto name = model::item_get(nulls, id)->name;
                      return std::vformat(localize.get(FORMAT_NULL), std::make_format_args(id, name));
                    },
                    .tooltip_draw = [&](int id) { item_tooltip_draw(resources, model::item_get(nulls, id)->name, id); },
                    .activate = [&](int id) { manager.item_properties_open(ElementType::NULL_ELEMENT, id); },
                    .isFocusItalic = true};
      list_panel_draw(panel, actions, {{ACTION_ADD, ACTION_REMOVE_UNUSED}},
                      [&]() { content_list_draw(panel, SelectionKind::NULLS, item_ids_get(nulls), rows); });
    }
    ImGui::End();

    item_properties_update<model::Null>(panel, 1, NULL_PROPERTIES_ROWS, SelectionKind::NULLS, TOOLTIP_NULL_NAME,
                                        EDIT_ADD_NULL, EDIT_SET_NULL_PROPERTIES, nulls_get,
                                        [](Manager::ItemEdit& null)
                                        {
                                          ImGui::Checkbox(localize.get(LABEL_RECT), &null.isShowRect);
                                          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NULL_RECT));
                                        });
  }

  void events_update(Panel& panel)
  {
    auto& [manager, settings, resources, dialog, clipboard, document, state] = panel;
    auto& events = document.model.content.events;
    if (ImGui::Begin(localize.get(LABEL_EVENTS_WINDOW), &settings.windowIsEvents))
    {
      auto selection = document.selected_ids_get(SelectionKind::EVENTS);
      auto is_built_in = [&](int id) { return model::is_event_built_in(model::item_get(events, id)->name); };
      Actions actions{};
      actions.add(
          ACTION_RENAME, [=]() { return selection.size() == 1 && !is_built_in(*selection.begin()); },
          [&state, selection]() { state.newId = *selection.begin(); });
      actions.add(
          ACTION_ADD, {},
          [&panel]()
          {
            auto& state = panel.state;
            command_push(panel,
                         [&state](Document& document)
                         {
                           int id{};
                           document.edit_apply(EDIT_ADD_EVENT,
                                               [&](model::Model& model)
                                               {
                                                 auto& events = model.content.events;
                                                 id = model::item_next_id_get(events);
                                                 events.push_back({.id = id, .name = localize.get(TEXT_NEW_EVENT)});
                                               });
                           document.selected_ids_set(SelectionKind::EVENTS, {id});
                           document.focused_id_set(SelectionKind::EVENTS, id);
                           state.newId = id;
                         });
          },
          TOOLTIP_ADD_EVENT);
      actions.add(
          ACTION_REMOVE_UNUSED, {},
          [&panel]() { items_unused_remove(panel, ElementType::EVENT_ELEMENT, EDIT_REMOVE_UNUSED_EVENTS, events_get); },
          TOOLTIP_REMOVE_UNUSED_EVENTS);
      items_clipboard_actions_add<model::Event>(actions, panel, SelectionKind::EVENTS, EDIT_PASTE_EVENTS,
                                                TOAST_DESERIALIZE_EVENTS_FAILED, events_get);
      ListRows rows{.label_get = [&](int id) { return model::item_get(events, id)->name; },
                    .name_get = [&](int id) { return model::item_get(events, id)->name; },
                    .rename =
                        [&](int id, const std::string& name)
                    {
                      command_push(panel,
                                   [id, name](Document& document)
                                   {
                                     document.edit_apply(EDIT_RENAME_EVENT,
                                                         [&](model::Model& model)
                                                         {
                                                           if (auto event = model::item_get(model.content.events, id))
                                                             event->name = name;
                                                         });
                                   });
                    },
                    .is_locked = is_built_in,
                    .font_get = [&](int id) { return is_built_in(id) ? font::ITALICS : font::REGULAR; },
                    .tooltip_draw =
                        [&](int id)
                    {
                      item_tooltip_draw(resources, model::item_get(events, id)->name, id);
                      if (is_built_in(id)) ImGui::TextUnformatted(localize.get(TOOLTIP_EVENT_BUILT_IN));
                    }};
      // Built-in events come first; an extended document lists the ones it doesn't have yet above them.
      auto ids = item_ids_get(events);
      std::ranges::stable_partition(ids, is_built_in);
      list_panel_draw(panel, actions, {{ACTION_ADD, ACTION_REMOVE_UNUSED}},
                      [&]()
                      {
                        ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
                        for (auto name : model::EVENT_BUILT_INS)
                          if (document.model.isExtendedFormat &&
                              std::ranges::find(events, name, &model::Event::name) == events.end())
                          {
                            ImGui::TextDisabled("%s", std::string(name).c_str());
                            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_EVENT_BUILT_IN));
                          }
                        ImGui::PopFont();
                        content_list_draw(panel, SelectionKind::EVENTS, ids, rows);
                      });
    }
    ImGui::End();
  }
}
