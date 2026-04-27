#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <imgui/imgui.h>

#include "document_new.hpp"

namespace anm2ed::ui_new
{
#define CONTEXT_MENU_OPTION_LIST                                                                                        \
  X(ADD, "Add", 0)                                                                                                      \
  X(COPY, "Copy", 1)                                                                                                    \
  X(CUT, "Cut", 2)                                                                                                      \
  X(PASTE, "Paste", 3)                                                                                                  \
  X(DEFAULT, "Default", 4)                                                                                              \
  X(DUPLICATE, "Duplicate", 5)                                                                                          \
  X(MERGE, "Merge", 6)                                                                                                  \
  X(REDO, "Redo", 7)                                                                                                    \
  X(REMOVE, "Remove", 8)                                                                                                \
  X(UNDO, "Undo", 9)                                                                                                    \
  X(OPEN_DIRECTORY, "Open Directory", 10)                                                                               \
  X(SET_FILE_PATH, "Set File Path", 11)                                                                                 \
  X(PACK, "Pack", 12)                                                                                                   \
  X(PLAY, "Play", 13)                                                                                                   \
  X(INSERT, "Insert", 14)                                                                                               \
  X(BAKE, "Bake", 15)                                                                                                   \
  X(FIT_ANIMATION_LENGTH, "Fit Animation Length", 16)                                                                   \
  X(SPLIT, "Split", 17)                                                                                                 \
  X(MAKE_REGION, "Make Region", 18)

#define BUTTON_OPTION_LIST                                                                                              \
  X(ADD, "Add", 0)                                                                                                      \
  X(DUPLICATE, "Duplicate", 1)                                                                                          \
  X(MERGE, "Merge", 2)                                                                                                  \
  X(REMOVE, "Remove", 3)                                                                                                \
  X(DEFAULT, "Default", 4)                                                                                              \
  X(RELOAD, "Reload", 5)                                                                                                \
  X(REPLACE, "Replace", 6)                                                                                              \
  X(REMOVE_UNUSED, "Remove Unused", 7)                                                                                  \
  X(SAVE, "Save", 8)

  enum ContextMenuOptions : uint32_t
  {
    CONTEXT_MENU_NONE = 0,
#define X(name, label, bit) CONTEXT_MENU_##name = 1u << bit,
    CONTEXT_MENU_OPTION_LIST
#undef X
  };

  enum ButtonOptions : uint32_t
  {
    BUTTON_NONE = 0,
#define X(name, label, bit) BUTTON_##name = 1u << bit,
    BUTTON_OPTION_LIST
#undef X
  };

  struct ContextMenuOptionEntry
  {
    ContextMenuOptions option;
    const char* label;
  };

  struct ButtonOptionEntry
  {
    ButtonOptions option;
    const char* label;
  };

  constexpr ContextMenuOptions operator|(ContextMenuOptions left, ContextMenuOptions right)
  {
    return static_cast<ContextMenuOptions>(static_cast<uint32_t>(left) | static_cast<uint32_t>(right));
  }

  constexpr ButtonOptions operator|(ButtonOptions left, ButtonOptions right)
  {
    return static_cast<ButtonOptions>(static_cast<uint32_t>(left) | static_cast<uint32_t>(right));
  }

#define X(name, label, bit) {CONTEXT_MENU_##name, label},
  inline constexpr std::array<ContextMenuOptionEntry, 19> CONTEXT_MENU_OPTION_ENTRIES = {{
      CONTEXT_MENU_OPTION_LIST
  }};
#undef X

#define X(name, label, bit) {BUTTON_##name, label},
  inline constexpr std::array<ButtonOptionEntry, 9> BUTTON_OPTION_ENTRIES = {{
      BUTTON_OPTION_LIST
  }};
#undef X

  bool is_flag_set(ContextMenuOptions, ContextMenuOptions);
  bool is_flag_set(ButtonOptions, ButtonOptions);
  std::vector<ContextMenuOptionEntry> options_get(ContextMenuOptions);
  std::vector<ButtonOptionEntry> options_get(ButtonOptions);

  class Window
  {
  public:
    enum Type
    {
      ELEMENT,
      ANIMATION_PREVIEW,
      SPRITESHEET_EDITOR,
      TIMELINE,
      FRAME_PROPERTIES,
      ONIONSKIN
    };

    std::string label{};
    Type windowType{ELEMENT};
    ButtonOptions buttonOptions{BUTTON_NONE};
    ContextMenuOptions contextMenuOptions{CONTEXT_MENU_NONE};
    anm2_new::Anm2::Element::Type elementType{anm2_new::Anm2::Element::UNKNOWN};

    Window() = default;
    Window(std::string, Type, ButtonOptions = BUTTON_NONE, ContextMenuOptions = CONTEXT_MENU_NONE,
           anm2_new::Anm2::Element::Type = anm2_new::Anm2::Element::UNKNOWN);

    void update(DocumentNew*);

  private:
    void update_footer();
    void update_element(DocumentNew*);
    std::vector<ButtonOptionEntry> button_entries_get() const;
  };

#undef CONTEXT_MENU_OPTION_LIST
#undef BUTTON_OPTION_LIST
}
