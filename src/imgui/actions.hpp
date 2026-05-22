#pragma once

#include <functional>
#include <vector>

#include "util/imgui/imgui.hpp"
#include "manager.hpp"
#include "settings.hpp"

namespace anm2ed::imgui
{
#define ACTIONS(X)                                                                                                     \
  X(ACTION_UNDO, SHORTCUT_STRING_UNDO, SHORTCUT_UNDO)                                                                  \
  X(ACTION_REDO, SHORTCUT_STRING_REDO, SHORTCUT_REDO)                                                                  \
  X(ACTION_ADD, BASIC_ADD, SHORTCUT_ADD)                                                                               \
  X(ACTION_REMOVE, BASIC_REMOVE, SHORTCUT_REMOVE)                                                                      \
  X(ACTION_REMOVE_UNUSED, BASIC_REMOVE_UNUSED, SHORTCUT_REMOVE)                                                        \
  X(ACTION_DUPLICATE, BASIC_DUPLICATE, SHORTCUT_DUPLICATE)                                                             \
  X(ACTION_MERGE, BASIC_MERGE, SHORTCUT_MERGE)                                                                         \
  X(ACTION_GROUP, BASIC_GROUP, SHORTCUT_GROUP)                                                                         \
  X(ACTION_DEFAULT, BASIC_DEFAULT, SHORTCUT_DEFAULT)                                                                   \
  X(ACTION_RENAME, BASIC_RENAME, SHORTCUT_RENAME)                                                                      \
  X(ACTION_PROPERTIES, BASIC_PROPERTIES, -1)                                                                           \
  X(ACTION_CUT, BASIC_CUT, SHORTCUT_CUT)                                                                               \
  X(ACTION_COPY, BASIC_COPY, SHORTCUT_COPY)                                                                            \
  X(ACTION_PASTE, BASIC_PASTE, SHORTCUT_PASTE)                                                                         \
  X(ACTION_RELOAD, BASIC_RELOAD, -1)                                                                                   \
  X(ACTION_REPLACE, BASIC_REPLACE, -1)                                                                                 \
  X(ACTION_SAVE, BASIC_SAVE, -1)                                                                                       \
  X(ACTION_OPEN_DIRECTORY, BASIC_OPEN_DIRECTORY, -1)                                                                   \
  X(ACTION_SET_FILE_PATH, BASIC_SET_FILE_PATH, -1)                                                                     \
  X(ACTION_PACK, BASIC_PACK, -1)                                                                                       \
  X(ACTION_TRIM, BASIC_TRIM, -1)                                                                                       \
  X(ACTION_PLAY, LABEL_PLAY, SHORTCUT_PLAY_PAUSE)                                                                      \
  X(ACTION_CENTER_VIEW, LABEL_CENTER_VIEW, SHORTCUT_CENTER_VIEW)                                                       \
  X(ACTION_FIT_VIEW, LABEL_FIT, SHORTCUT_FIT)                                                                          \
  X(ACTION_ZOOM_IN, SHORTCUT_STRING_ZOOM_IN, SHORTCUT_ZOOM_IN)                                                         \
  X(ACTION_ZOOM_OUT, SHORTCUT_STRING_ZOOM_OUT, SHORTCUT_ZOOM_OUT)

  enum ActionType
  {
#define X(name, label, shortcut) name,
    ACTIONS(X)
#undef X
        ACTION_COUNT
  };

  struct ActionInfo
  {
    StringType label{};
    int shortcut{-1};
  };

  constexpr ActionInfo ACTION_INFOS[] = {
#define X(name, label, shortcut) {label, shortcut},
      ACTIONS(X)
#undef X
  };

  struct Action
  {
    ActionType type{};
    StringType label{STRING_UNDEFINED};
    StringType tooltip{STRING_UNDEFINED};
    int shortcut{-1};
    bool isSeparator{};
    std::function<bool()> isEnabled{};
    std::function<void()> run{};
  };

  struct Actions
  {
    std::vector<Action> items{};

    void add(Action);
    void add(ActionType, std::function<bool()>, std::function<void()>, StringType = STRING_UNDEFINED,
             int = ACTION_COUNT);
    void separator();
  };

  bool is_action_enabled(const Action&);
  Action action_make(ActionType, std::function<bool()>, std::function<void()>, StringType = STRING_UNDEFINED,
                     int = ACTION_COUNT);
  void actions_undo_redo_add(Actions&, Manager&, Document&);
  void actions_menu_draw(Actions&, Settings&);
  bool actions_context_window_draw(const char*, Actions&, Settings&, ImGuiPopupFlags = ImGuiPopupFlags_MouseButtonRight);
  bool actions_popup_draw(const char*, Actions&, Settings&);
  void actions_shortcuts_update(Actions&, Manager&, types::shortcut::Type = types::shortcut::FOCUSED);
  void action_button_draw(Action&, Manager&, Settings&, ImVec2, bool&);
}
