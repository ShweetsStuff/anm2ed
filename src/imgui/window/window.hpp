#pragma once

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "clipboard.hpp"
#include "dialog.hpp"
#include "manager.hpp"
#include "model/xml.hpp"
#include "resources.hpp"
#include "settings.hpp"
#include "storage.hpp"
#include "util/imgui/popup.hpp"
#include "util/imgui/selectable.hpp"

namespace anm2ed::imgui
{
#define WINDOW_FLAGS                                                                                                   \
  X(PLAY, ACTION_PLAY, play)                                                                                           \
  X(RENAME, ACTION_RENAME, rename)                                                                                     \
  X(PROPERTIES, ACTION_PROPERTIES, properties)                                                                         \
  X(OPEN_DIRECTORY, ACTION_OPEN_DIRECTORY, open)                                                                       \
  X(SET_FILE_PATH, ACTION_SET_FILE_PATH, path_set)                                                                     \
  X(ADD, ACTION_ADD, add)                                                                                              \
  X(DUPLICATE, ACTION_DUPLICATE, duplicate)                                                                            \
  X(REMOVE_UNUSED, ACTION_REMOVE_UNUSED, remove_unused)                                                                \
  X(RELOAD, ACTION_RELOAD, reload)                                                                                     \
  X(REPLACE, ACTION_REPLACE, replace)                                                                                  \
  X(MERGE, ACTION_MERGE, merge)                                                                                        \
  X(GROUP, ACTION_GROUP, group)                                                                                        \
  X(REMOVE, ACTION_REMOVE, remove)                                                                                     \
  X(DEFAULT, ACTION_DEFAULT, default_set)                                                                              \
  X(TRIM, ACTION_TRIM, trim)                                                                                           \
  X(EXPORT, ACTION_EXPORT, export_open)                                                                                \
  X(PACK, ACTION_PACK, pack_open)                                                                                      \
  X(SAVE, ACTION_SAVE, save)                                                                                           \
  X(CUT, ACTION_CUT, cut)                                                                                              \
  X(COPY, ACTION_COPY, copy)                                                                                           \
  X(PASTE, ACTION_PASTE, paste)

  enum WindowFlagIndex
  {
#define X(symbol, action, command) WINDOW_INDEX_##symbol,
    WINDOW_FLAGS
#undef X
        WINDOW_INDEX_COUNT
  };

  enum WindowFlag
  {
#define X(symbol, action, command) WINDOW_##symbol = 1 << WINDOW_INDEX_##symbol,
    WINDOW_FLAGS
#undef X
        WINDOW_REFERENCE_ITALIC = 1 << WINDOW_INDEX_COUNT
  };

  using WindowFlags = int;

  inline constexpr int WINDOW_GROUP_KEY_OFFSET = 2;

  constexpr int window_group_key_get(int groupId) { return -groupId - WINDOW_GROUP_KEY_OFFSET; }
  constexpr int window_group_id_from_key_get(int key) { return -key - WINDOW_GROUP_KEY_OFFSET; }
  constexpr bool is_window_group_key(int key) { return key <= -WINDOW_GROUP_KEY_OFFSET; }

  struct WindowCardImage
  {
    const resource::Image* texture{};
    glm::vec2 size{};
    glm::vec2 uvMin{};
    glm::vec2 uvMax{1.0f};
    bool isValid{};
  };

  constexpr bool window_flag_has(WindowFlags flags, WindowFlag flag) { return (flags & flag) != 0; }

  struct Window
  {
    using StorageGet = std::function<Storage&(Document&)>;
    using KeysGet = std::function<std::vector<int>(Document&)>;
    using NameGet = std::function<std::string(Document&, int)>;
    using RowFontGet = std::function<resource::font::Type(Document&, int)>;
    using RowSelect = std::function<void(Window&, Document&, int)>;
    using RenameApply = std::function<void(Document&, model::Model&, int, const std::string&)>;
    using PasteApply = std::function<int(Document&, model::Model&, const std::string&, std::string*)>;
    using UnusedRemove = std::function<void(Document&, model::Model&)>;
    using TooltipDraw = std::function<void(Document&, Resources&, int)>;
    using RowDragDropUpdate = std::function<bool(Window&, Manager&, Document&, int, int)>;
    using CardImageGet = std::function<WindowCardImage(Document&, Resources&, int)>;
    using PropertiesOpen = std::function<void(Manager&, int)>;
    using Command = std::function<void(Window&, Manager&, Settings&, Document&, Clipboard&)>;
    using IsEnabled = std::function<bool(Window&, Document&)>;
    using Update = std::function<void(Window&, Manager&, Settings&, Resources&, Clipboard&, Document&)>;

    StringType title{};
    bool Settings::* isOpen{};
    ElementType elementType{ElementType::UNKNOWN};
    const char* childLabel{"##Window Child"};
    std::map<WindowFlag, StringType> tooltips{};
    std::vector<std::vector<WindowFlag>> footer{};
    StringType addEdit{};
    StringType propertiesEdit{};
    StringType renameEdit{};
    StringType pasteEdit{};
    StringType removeUnusedEdit{};
    StringType deserializeFailedToast{};
    StringType unavailableText{STRING_UNDEFINED};
    int cardLines{};
    int newElementId{-1};
    int scrollQueued{-1};
    int renameQueued{-1};
    int renameId{-1};
    int editId{-1};
    RenameState renameState{RENAME_SELECTABLE};
    std::string renameText{};
    PopupHelper popup{STRING_UNDEFINED};
    PopupHelper popup2{STRING_UNDEFINED};
    PopupHelper popup3{STRING_UNDEFINED};
    std::set<int> selection{};
    std::set<int> selection2{};
    std::vector<int> dragSelection{};
    std::vector<int> order{};
    model::Region editRegion{};
    Dialog* dialog{};
    WindowFlags flags{WINDOW_COPY | WINDOW_PASTE};
    bool isPreserveEditElementOnOpen{};
    ImVec2 tooltipWindowPadding{};
    ImVec2 tooltipItemSpacing{};
    StorageGet storage_get{};
    KeysGet keys_get{};
    NameGet name_get{};
    NameGet row_label_get{};
    NameGet copy_get{};
    RowFontGet row_font_get{};
    RowSelect row_select{};
    RenameApply rename_apply{};
    PasteApply paste_apply{};
    UnusedRemove unused_remove{};
    TooltipDraw tooltip_draw{};
    RowDragDropUpdate row_drag_drop_update{};
    CardImageGet card_image_get{};
    PropertiesOpen properties_open{};
#define X(symbol, action, command) Command command{};
    WINDOW_FLAGS
#undef X
    Command merge_open{};
    std::map<WindowFlag, IsEnabled> enabled{};
    IsEnabled is_available{};
    Update rows_update{};
    Update body_update{};
    Update popup_update{};
    Update post_update{};
  };

  Window animations_window_register();
  Window regions_window_register();
  Window sounds_window_register();
  Window spritesheets_window_register();
  Window layers_window_register();
  Window nulls_window_register();
  Window events_window_register();
  void window_update(Window&, Manager&, Settings&, Resources&, Dialog&, Clipboard&);

  enum class PopupButton
  {
    NONE,
    CONFIRM,
    CANCEL
  };

  inline constexpr auto DRAG_DROP_SOURCE_FLAGS =
      ImGuiDragDropFlags_SourceNoPreviewTooltip | ImGuiDragDropFlags_SourceNoHoldToOpenOthers;
  inline constexpr ImVec4 CARD_TINT_INVALID = ImVec4(1.0f, 0.25f, 0.25f, 1.0f);
  inline constexpr ImVec4 CARD_TINT_VALID = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

  bool is_drag_drop_active();
  void window_command_push(Window&, Manager&, Settings&, Clipboard&, const Window::Command&);
  int window_selection_start(Window&, Document&, const std::vector<int>&);
  void window_selection_finish(Window&, Manager&, Settings&, Clipboard&, Document&, int);
  bool window_row_draw(Window&, Manager&, Resources&, Document&, int, int, int);
  void window_cards_draw(Window&, Manager&, Settings&, Resources&, Clipboard&, Document&);
  void window_tooltip_name_draw(Resources&, const std::string&);
  void window_tooltip_image_draw(const WindowCardImage&, const std::function<void()>&);
  void window_directory_open(Dialog&, Document&, const std::filesystem::path&);
  std::filesystem::path window_asset_path_get(Document&, const std::filesystem::path&);
  PopupButton window_popup_buttons_draw(Manager&, const char*, bool = true, StringType = BASIC_CANCEL);

  // Binds a window's generic list behavior (keys, names, copy/paste, remove unused) to a list of content items.
  template <class Item>
  void window_items_bind(Window& window, std::function<std::vector<Item>*(Document&, model::Model&)> items_get)
  {
    auto items_const_get = [items_get](Document& document) { return items_get(document, document.model); };
    window.keys_get = [items_const_get](Document& document)
    {
      std::vector<int> keys{};
      if (auto items = items_const_get(document))
        for (const auto& item : *items)
          keys.push_back(item.id);
      return keys;
    };
    window.copy_get = [items_const_get](Document& document, int key)
    {
      auto items = items_const_get(document);
      auto item = items ? model::item_get(*items, key) : nullptr;
      return item ? model::item_to_string(*item) : std::string{};
    };
    window.paste_apply =
        [items_get](Document& document, model::Model& model, const std::string& text, std::string* errorString)
    {
      auto items = items_get(document, model);
      auto pasted = model::items_from_string<Item>(text, errorString);
      if (!items || pasted.empty()) return -1;
      for (auto& item : pasted)
      {
        item.id = model::item_next_id_get(*items);
        items->push_back(std::move(item));
      }
      return items->back().id;
    };
    if constexpr (requires(Item item) { item.name; })
    {
      window.name_get = [items_const_get](Document& document, int key)
      {
        auto items = items_const_get(document);
        auto item = items ? model::item_get(*items, key) : nullptr;
        return item ? item->name : std::string{};
      };
      window.rename_apply = [items_get](Document& document, model::Model& model, int key, const std::string& name)
      {
        auto items = items_get(document, model);
        if (auto item = items ? model::item_get(*items, key) : nullptr) item->name = name;
      };
    }
    window.unused_remove = [items_get, type = window.elementType](Document& document, model::Model& model)
    {
      auto unused = model.unused_get(type);
      if (auto items = items_get(document, model))
        std::erase_if(*items, [&](const Item& item) { return unused.contains(item.id); });
    };
  }

  // Adds `edited` as a new item (reference -1) or replaces the item with that id, then selects it.
  template <class Item>
  void window_item_apply_push(Window& window, Manager& manager, Item edited, int reference,
                              std::function<std::vector<Item>*(Document&, model::Model&)> items_get,
                              int insertIndex = -1)
  {
    manager.command_push(
        {manager.selected, [&window, edited, reference, items_get, insertIndex](Manager&, Document& document)
         {
           document.edit_apply(reference == -1 ? window.addEdit : window.propertiesEdit,
                               [&](model::Model& model)
                               {
                                 auto items = items_get(document, model);
                                 if (!items) return;
                                 auto target = reference == -1 ? nullptr : model::item_get(*items, reference);
                                 if (reference != -1 && !target) return;
                                 auto changed = edited;
                                 changed.id = reference == -1 ? model::item_next_id_get(*items) : reference;
                                 if (target)
                                 {
                                   changed.uid = target->uid;
                                   *target = changed;
                                 }
                                 else
                                 {
                                   changed.uid = util::uid_next();
                                   auto position =
                                       insertIndex < 0 ? (int)items->size() : std::min(insertIndex, (int)items->size());
                                   items->insert(items->begin() + position, changed);
                                   window.newElementId = changed.id;
                                 }
                                 auto& storage = window.storage_get(document);
                                 storage.selection = {changed.id};
                                 storage.reference = changed.id;
                                 if (window.row_select) window.row_select(window, document, changed.id);
                               });
         }});
  }
}
