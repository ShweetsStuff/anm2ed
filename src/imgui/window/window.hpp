#pragma once

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "clipboard.hpp"
#include "dialog.hpp"
#include "manager.hpp"
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
    resource::Image* texture{};
    glm::vec2 size{};
    glm::vec2 uvMin{};
    glm::vec2 uvMax{1.0f};
    bool isValid{};
  };

  constexpr bool window_flag_has(WindowFlags flags, WindowFlag flag) { return (flags & flag) != 0; }

  struct Window
  {
    using StorageGet = std::function<Storage&(Document&)>;
    using ContainerGet = std::function<Element*(Document&)>;
    using IndexGet = std::function<int(Document&)>;
    using ElementGet = std::function<Element*(Anm2&, int)>;
    using ElementKeyGet = std::function<int(const Element&, int)>;
    using RowLabelGet = std::function<std::string(Document&, const Element&)>;
    using RowFontGet = std::function<resource::font::Type(Document&, const Element&, int)>;
    using RowSelect = std::function<void(Window&, Document&, int)>;
    using RenameFinish = std::function<void(Document&, Element&, int, int)>;
    using TooltipDraw = std::function<void(Document&, Resources&, const Element&)>;
    using RowDragDropUpdate = std::function<bool(Window&, Manager&, Document&, const Element&, int)>;
    using CardImageGet = std::function<WindowCardImage(Document&, Resources&, const Element&)>;
    using PropertiesOpen = std::function<void(Manager&, int)>;
    using Command = std::function<void(Window&, Manager&, Settings&, Document&, Clipboard&)>;
    using IsEnabled = std::function<bool(Window&, Document&)>;
    using Update = std::function<void(Window&, Manager&, Settings&, Resources&, Clipboard&, Document&)>;

    StringType title{};
    bool Settings::* isOpen{};
    Document::ChangeType changeType{};
    ElementType containerType{ElementType::UNKNOWN};
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
    Element editElement{};
    Dialog* dialog{};
    WindowFlags flags{WINDOW_COPY | WINDOW_PASTE};
    bool isPreserveEditElementOnOpen{};
    ImVec2 tooltipWindowPadding{};
    ImVec2 tooltipItemSpacing{};
    StorageGet storage_get{};
    ContainerGet container_get{};
    IndexGet insert_index_get{};
    ElementGet element_get{};
    ElementKeyGet element_key_get{};
    RowLabelGet row_label_get{};
    RowFontGet row_font_get{};
    RowSelect row_select{};
    RenameFinish rename_finish{};
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
  Element* window_container_get(const Window&, Document&);
  Element* window_element_get(const Window&, Document&, int);
  void window_element_apply_push(Window&, Manager&, const Element&, int);
  void window_command_push(Window&, Manager&, Settings&, Clipboard&, const Window::Command&);
  int window_selection_start(Window&, Document&, const std::vector<int>&);
  void window_selection_finish(Window&, Manager&, Settings&, Clipboard&, Document&, int);
  bool window_row_draw(Window&, Manager&, Resources&, Document&, Element&, int, int, int);
  void window_cards_draw(Window&, Manager&, Settings&, Resources&, Clipboard&, Document&, Element*);
  void window_tooltip_name_draw(Resources&, const std::string&);
  void window_tooltip_image_draw(const WindowCardImage&, const std::function<void()>&);
  void window_directory_open(Dialog&, Document&, const std::filesystem::path&);
  std::filesystem::path window_asset_path_get(Document&, const std::filesystem::path&);
  PopupButton window_popup_buttons_draw(Manager&, const char*, bool = true, StringType = BASIC_CANCEL);
}
