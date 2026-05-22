#pragma once

#include <functional>
#include <set>
#include <string>
#include <vector>

#include "clipboard.hpp"
#include "dialog.hpp"
#include "util/imgui/imgui.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"
#include "storage.hpp"

namespace anm2ed::imgui
{
  enum WindowFlag
  {
    WINDOW_ADD = 1 << 0,
    WINDOW_REMOVE = 1 << 1,
    WINDOW_REMOVE_UNUSED = 1 << 2,
    WINDOW_DUPLICATE = 1 << 3,
    WINDOW_MERGE = 1 << 4,
    WINDOW_DEFAULT = 1 << 5,
    WINDOW_CUT = 1 << 6,
    WINDOW_COPY = 1 << 7,
    WINDOW_PASTE = 1 << 8,
    WINDOW_RENAME = 1 << 9,
    WINDOW_PROPERTIES = 1 << 10,
    WINDOW_REFERENCE_ITALIC = 1 << 11
  };

  using WindowFlags = int;

  constexpr bool window_flag_has(WindowFlags flags, WindowFlag flag) { return (flags & flag) != 0; }

  struct Window
  {
    using StorageGet = std::function<Storage&(Document&)>;
    using ElementGet = std::function<Element*(Anm2&, int)>;
    using ElementKeyGet = std::function<int(const Element&, int)>;
    using RowLabelGet = std::function<std::string(Document&, const Element&)>;
    using RowFontGet = std::function<resource::font::Type(Document&, const Element&, int)>;
    using RowSelect = std::function<void(Window&, Document&, int)>;
    using RenameFinish = std::function<void(Document&, Element&, int, int)>;
    using TooltipDraw = std::function<void(Document&, Resources&, const Element&)>;
    using RowDragDropUpdate = std::function<bool(Window&, Manager&, Document&, const Element&, int)>;
    using PropertiesOpen = std::function<void(Manager&, int)>;
    using Command = std::function<void(Window&, Manager&, Settings&, Document&, Clipboard&)>;
    using IsAvailable = std::function<bool(Document&)>;
    using Update = std::function<void(Window&, Manager&, Settings&, Resources&, Clipboard&, Document&)>;
    using RowsUpdate = std::function<void(Window&, Manager&, Settings&, Resources&, Clipboard&, Document&, ImVec2)>;

    StringType title{};
    bool Settings::* isOpen{};
    Document::ChangeType changeType{};
    ElementType containerType{ElementType::UNKNOWN};
    ElementType elementType{ElementType::UNKNOWN};
    const char* childLabel{"##Window Child"};
    StringType addTooltip{};
    StringType duplicateTooltip{};
    StringType mergeTooltip{};
    StringType removeTooltip{};
    StringType removeUnusedTooltip{};
    StringType defaultTooltip{};
    StringType addEdit{};
    StringType renameEdit{};
    StringType pasteEdit{};
    StringType removeUnusedEdit{};
    StringType deserializeFailedToast{};
    StringType unavailableText{STRING_UNDEFINED};
    int newElementId{-1};
    int scrollQueued{-1};
    int renameQueued{-1};
    int renameId{-1};
    int editId{-1};
    int footerRows{-1};
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
    bool isChildPaddingZero{};
    bool isPreserveEditElementOnOpen{};
    ImVec2 tooltipWindowPadding{};
    ImVec2 tooltipItemSpacing{};
    StorageGet storage_get{};
    ElementGet element_get{};
    ElementKeyGet element_key_get{};
    RowLabelGet row_label_get{};
    RowFontGet row_font_get{};
    RowSelect row_select{};
    RenameFinish rename_finish{};
    TooltipDraw tooltip_draw{};
    RowDragDropUpdate row_drag_drop_update{};
    PropertiesOpen properties_open{};
    Command add{};
    Command remove{};
    Command duplicate{};
    Command merge{};
    Command merge_open{};
    Command default_set{};
    Command cut{};
    Command copy{};
    Command paste{};
    Command reload{};
    Command replace{};
    Command save{};
    Command pack{};
    Command trim{};
    Command properties{};
    Command open{};
    Command path_set{};
    IsAvailable is_available{};
    Update begin_update{};
    RowsUpdate rows_update{};
    Update context_update{};
    Update footer_update{};
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
}
