#pragma once

#include <functional>
#include <set>
#include <string>
#include <vector>

#include "actions.hpp"
#include "clipboard.hpp"
#include "dialog.hpp"
#include "manager.hpp"
#include "model/xml.hpp"
#include "resources.hpp"
#include "settings.hpp"
#include "toast.hpp"
#include "util/imgui/multiselect.hpp"
#include "util/imgui/popup.hpp"
#include "util/imgui/selectable.hpp"

// Panels are plain functions: each draws from the document and its selection, and queues edits as commands.
namespace anm2ed::imgui
{
  inline constexpr auto DRAG_DROP_SOURCE_FLAGS =
      ImGuiDragDropFlags_SourceNoPreviewTooltip | ImGuiDragDropFlags_SourceNoHoldToOpenOthers;
  inline constexpr ImVec4 CARD_TINT_INVALID = ImVec4(1.0f, 0.25f, 0.25f, 1.0f);
  inline constexpr ImVec4 CARD_TINT_VALID = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

  using Footer = std::vector<std::vector<ActionType>>;

  struct CardImage
  {
    const resource::Image* texture{};
    glm::vec2 size{};
    glm::vec2 uvMin{};
    glm::vec2 uvMax{1.0f};
    bool isValid{};
  };

  // What a list panel remembers between frames.
  struct PanelState
  {
    int newId{-1};
    int scrollId{-1};
    int renameId{-1};
    std::string renameText{};
    RenameState renameState{RENAME_SELECTABLE};
    std::vector<int> keys{};
    std::vector<int> dragIds{};
    MultiSelectStorage selection{};
    ImVec2 tooltipPadding{};
    ImVec2 tooltipSpacing{};
  };

  // Everything a panel draws with, for one frame. Commands get the document again when they run.
  struct Panel
  {
    Manager& manager;
    Settings& settings;
    Resources& resources;
    Dialog& dialog;
    Clipboard& clipboard;
    Document& document;
    PanelState& state;
  };

  // How a list shows its keys; only `label_get` is required. With `image_get` the rows are cards.
  struct ListRows
  {
    std::function<std::string(int)> label_get{};
    std::function<std::string(int)> name_get{};
    std::function<void(int, const std::string&)> rename{};
    std::function<resource::font::Type(int)> font_get{};
    std::function<void(int)> tooltip_draw{};
    std::function<CardImage(int)> image_get{};
    std::function<void(int)> select{};
    std::function<void(int)> activate{};
    std::function<bool(int, int)> drag_drop_update{};
    int cardLines{};
    bool isFocusItalic{};
  };

  enum class PopupButton
  {
    NONE,
    CONFIRM,
    CANCEL
  };

  bool is_drag_drop_active();
  void command_push(const Panel&, std::function<void(Document&)>);
  bool row_draw(Panel&, std::set<int>&, int&, const ListRows&, int, int, int);
  void list_draw(Panel&, const std::vector<int>&, std::set<int>&, int&, const ListRows&);
  void content_list_draw(Panel&, SelectionKind, const std::vector<int>&, const ListRows&);
  int list_select_begin(Panel&, std::set<int>&, int&, const ListRows&, const std::vector<int>&);
  void list_select_end(Panel&, std::set<int>&, int&, const ListRows&, int);
  void list_panel_draw(Panel&, Actions&, const Footer&, const std::function<void()>&, bool = false, Actions* = nullptr);
  bool tooltip_begin(const PanelState&, bool = true);
  void tooltip_end();
  void tooltip_name_draw(Resources&, const std::string&);
  void tooltip_image_draw(const CardImage&, const std::function<void()>&);
  void directory_open(Dialog&, Document&, const std::filesystem::path&);
  std::filesystem::path asset_path_get(Document&, const std::filesystem::path&);
  PopupButton popup_buttons_draw(Manager&, const char*, bool = true, StringType = BASIC_CANCEL);

  struct SpritesheetsPanel
  {
    PanelState state{};
    PopupHelper mergePopup{PopupHelper(LABEL_SPRITESHEETS_MERGE_POPUP, POPUP_SMALL_NO_HEIGHT)};
    PopupHelper packPopup{PopupHelper(LABEL_SPRITESHEETS_PACK_POPUP, POPUP_SMALL_NO_HEIGHT)};
    PopupHelper overwritePopup{PopupHelper(LABEL_TASKBAR_OVERWRITE_FILE, POPUP_SMALL_NO_HEIGHT)};
    std::set<int> mergeIds{};
    std::set<int> saveIds{};
    int packId{-1};
  };

  struct RegionsPanel
  {
    PanelState state{};
    PopupHelper propertiesPopup{PopupHelper(LABEL_REGION_PROPERTIES, POPUP_SMALL_NO_HEIGHT)};
    PopupHelper exportPopup{PopupHelper(LABEL_EXPORT_REGION, POPUP_SMALL_NO_HEIGHT)};
    model::Region editRegion{};
    int editId{-1};
    int exportId{-1};
  };

  struct AnimationsPanel
  {
    PanelState state{};
    PopupHelper mergePopup{PopupHelper(LABEL_ANIMATIONS_MERGE_POPUP)};
    MultiSelectStorage mergeSelection{};
    std::vector<int> mergeKeys{};
    int mergeReference{-1};
  };

  void animations_update(Panel&, AnimationsPanel&);
  void layers_update(Panel&);
  void nulls_update(Panel&);
  void events_update(Panel&);
  void sounds_update(Panel&);
  void spritesheets_update(Panel&, SpritesheetsPanel&);
  void regions_update(Panel&, RegionsPanel&);

  template <class Items> std::vector<int> item_ids_get(const Items& items)
  {
    std::vector<int> ids{};
    for (const auto& item : items)
      ids.push_back(item.id);
    return ids;
  }

  template <class Items> void items_copy(Clipboard& clipboard, const Items& items, const std::set<int>& ids)
  {
    std::string text{};
    for (auto id : ids)
      if (auto item = model::item_get(items, id)) text += model::item_to_string(*item);
    if (!text.empty()) clipboard.set(text);
  }

  // Pastes clipboard items into a content list with fresh ids and selects the last one.
  template <class Item, class ItemsGet>
  void items_paste(const Panel& panel, SelectionKind kind, StringType edit, StringType failToast, ItemsGet items_get)
  {
    auto text = panel.clipboard.get();
    auto& state = panel.state;
    command_push(panel,
                 [&state, kind, edit, failToast, text, items_get](Document& document)
                 {
                   std::string errorString{};
                   auto pasted = model::items_from_string<Item>(text, &errorString);
                   if (pasted.empty()) return toast_log(Level::ERROR, failToast, errorString);
                   document.edit_apply(edit,
                                       [&](model::Model& model)
                                       {
                                         auto& items = items_get(model);
                                         for (auto& item : pasted)
                                         {
                                           item.id = model::item_next_id_get(items);
                                           items.push_back(std::move(item));
                                         }
                                       });
                   auto id = items_get(document.model).back().id;
                   document.selected_ids_set(kind, {id});
                   document.focused_id_set(kind, id);
                   state.newId = id;
                 });
  }

  // Copy and paste for a content list, after a separator.
  template <class Item, class ItemsGet>
  void items_clipboard_actions_add(Actions& actions, Panel& panel, SelectionKind kind, StringType pasteEdit,
                                   StringType failToast, ItemsGet items_get)
  {
    auto selection = panel.document.selected_ids_get(kind);
    actions.separator();
    actions.add(
        ACTION_COPY, [selection]() { return !selection.empty(); },
        [&panel, selection, items_get]() { items_copy(panel.clipboard, items_get(panel.document.model), selection); });
    actions.add(
        ACTION_PASTE, [&panel]() { return !panel.clipboard.is_empty(); },
        [&panel, kind, pasteEdit, failToast, items_get]()
        { items_paste<Item>(panel, kind, pasteEdit, failToast, items_get); });
  }

  // Removes content items no animation (or layer, for spritesheets) uses.
  template <class ItemsGet>
  void items_unused_remove(const Panel& panel, ElementType type, StringType edit, ItemsGet items_get)
  {
    command_push(
        panel,
        [type, edit, items_get](Document& document)
        {
          auto unused = document.model.unused_get(type);
          if (unused.empty()) return;
          document.edit_apply(
              edit, [&](model::Model& model)
              { std::erase_if(items_get(model), [&](const auto& item) { return unused.contains(item.id); }); });
        });
  }
}
