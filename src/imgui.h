#pragma once

#include "canvas.h"
#include "clipboard.h"
#include "dialog.h"
#include "editor.h"
#include "ffmpeg.h"
#include "generate_preview.h"
#include "preview.h"
#include "resources.h"
#include "settings.h"
#include "snapshots.h"
#include "tool.h"
#include "window.h"
#include <SDL3/SDL_mouse.h>

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#define IM_VEC2_CLASS_EXTRA                                                                                                                                    \
  inline bool operator==(const ImVec2& rhs) const { return x == rhs.x && y == rhs.y; }                                                                         \
  inline bool operator!=(const ImVec2& rhs) const { return !(*this == rhs); }                                                                                  \
  inline ImVec2 operator+(const ImVec2& rhs) const { return ImVec2(x + rhs.x, y + rhs.y); }                                                                    \
  inline ImVec2 operator-(const ImVec2& rhs) const { return ImVec2(x - rhs.x, y - rhs.y); }                                                                    \
  inline ImVec2 operator*(const ImVec2& rhs) const { return ImVec2(x * rhs.x, y * rhs.y); }                                                                    \
  inline ImVec2 operator*(float s) const { return ImVec2(x * s, y * s); }                                                                                      \
  friend inline ImVec2 operator*(float s, const ImVec2& v) { return ImVec2(v.x * s, v.y * s); }                                                                \
  inline ImVec2& operator+=(const ImVec2& rhs) {                                                                                                               \
    x += rhs.x;                                                                                                                                                \
    y += rhs.y;                                                                                                                                                \
    return *this;                                                                                                                                              \
  }                                                                                                                                                            \
  inline ImVec2& operator-=(const ImVec2& rhs) {                                                                                                               \
    x -= rhs.x;                                                                                                                                                \
    y -= rhs.y;                                                                                                                                                \
    return *this;                                                                                                                                              \
  }                                                                                                                                                            \
  inline ImVec2& operator*=(float s) {                                                                                                                         \
    x *= s;                                                                                                                                                    \
    y *= s;                                                                                                                                                    \
    return *this;                                                                                                                                              \
  }                                                                                                                                                            \
  inline ImVec2(const vec2& v) : x(v.x), y(v.y) {}                                                                                                             \
  inline operator vec2() const { return vec2(x, y); }

#define IM_VEC4_CLASS_EXTRA                                                                                                                                    \
  inline bool operator==(const ImVec4& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }                                             \
  inline bool operator!=(const ImVec4& rhs) const { return !(*this == rhs); }                                                                                  \
  inline ImVec4 operator+(const ImVec4& rhs) const { return ImVec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }                                              \
  inline ImVec4 operator-(const ImVec4& rhs) const { return ImVec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }                                              \
  inline ImVec4 operator*(const ImVec4& rhs) const { return ImVec4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }                                              \
  inline ImVec4 operator*(float s) const { return ImVec4(x * s, y * s, z * s, w * s); }                                                                        \
  friend inline ImVec4 operator*(float s, const ImVec4& v) { return ImVec4(v.x * s, v.y * s, v.z * s, v.w * s); }                                              \
  inline ImVec4& operator+=(const ImVec4& rhs) {                                                                                                               \
    x += rhs.x;                                                                                                                                                \
    y += rhs.y;                                                                                                                                                \
    z += rhs.z;                                                                                                                                                \
    w += rhs.w;                                                                                                                                                \
    return *this;                                                                                                                                              \
  }                                                                                                                                                            \
  inline ImVec4& operator-=(const ImVec4& rhs) {                                                                                                               \
    x -= rhs.x;                                                                                                                                                \
    y -= rhs.y;                                                                                                                                                \
    z -= rhs.z;                                                                                                                                                \
    w -= rhs.w;                                                                                                                                                \
    return *this;                                                                                                                                              \
  }                                                                                                                                                            \
  inline ImVec4& operator*=(float s) {                                                                                                                         \
    x *= s;                                                                                                                                                    \
    y *= s;                                                                                                                                                    \
    z *= s;                                                                                                                                                    \
    w *= s;                                                                                                                                                    \
    return *this;                                                                                                                                              \
  }                                                                                                                                                            \
  inline ImVec4(const vec4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}                                                                                             \
  inline operator vec4() const { return vec4(x, y, z, w); }

#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#define IMGUI_CHORD_NONE (ImGuiMod_None)
#define IMGUI_FRAME_BORDER 2.0f
#define IMGUI_LOG_DURATION 3.0f
#define IMGUI_TEXT_HEIGHT_PADDING 4.0f
#define IMGUI_PLAYHEAD_LINE_COLOR IM_COL32(UCHAR_MAX, UCHAR_MAX, UCHAR_MAX, UCHAR_MAX)
#define IMGUI_TRIGGERS_EVENT_COLOR IM_COL32(UCHAR_MAX, UCHAR_MAX, UCHAR_MAX, 128)
#define IMGUI_PLAYHEAD_LINE_WIDTH 2.0f
#define IMGUI_TIMELINE_FRAME_MULTIPLE 5
#define IMGUI_TIMELINE_MERGE
#define IMGUI_TOOL_COLOR_PICKER_DURATION 0.25f
#define IMGUI_CONFIRM_POPUP_ROW_COUNT 2
#define IMGUI_CHORD_REPEAT_TIME 0.25f

#define IMGUI_ACTION_FRAME_CROP "Frame Crop"
#define IMGUI_ACTION_FRAME_MOVE "Frame Move"

#define IMGUI_ACTION_ANIMATION_MOVE "Move Animation"
#define IMGUI_ACTION_TRIGGER_MOVE "Trigger At Frame"
#define IMGUI_ACTION_ITEM_SWAP "Item Swap"
#define IMGUI_ACTION_FRAME_DELAY "Frame Delay"
#define IMGUI_ACTION_DRAW "Draw"
#define IMGUI_ACTION_ERASE "Erase"
#define IMGUI_ACTION_MOVE "Move"
#define IMGUI_ACTION_SCALE "Scale"
#define IMGUI_ACTION_ROTATE "Rotate"
#define IMGUI_ACTION_CROP "Crop"
#define IMGUI_ACTION_ADD_SPRITESHEET "Add Spritesheet"
#define IMGUI_ACTION_REPLACE_SPRITESHEET "Replace Spritesheet"
#define IMGUI_ACTION_SPRITESHEET_SWAP "Spritesheet Swap"
#define IMGUI_ACTION_OPEN_FILE "Open File"

#define IMGUI_SET_ITEM_PROPERTIES_POPUP "Item Properties"
#define IMGUI_POPUP_FLAGS ImGuiWindowFlags_NoMove
#define IMGUI_POPUP_MODAL_FLAGS ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize

#define IMGUI_LOG_INIT "Initialized ImGui {}"
#define IMGUI_LOG_FREE "Freed ImGui"

#define IMGUI_LOG_NO_ANM2_PATH "Please save the .anm2 to a path first!"
#define IMGUI_LOG_FILE_OPEN_FORMAT "Opened anm2: {}"
#define IMGUI_LOG_FILE_SAVE_FORMAT "Saved anm2 to: {}"
#define IMGUI_LOG_SPRITESHEET_RELOAD "Reloaded selected spritesheets"
#define IMGUI_LOG_RENDER_ANIMATION_FRAMES_SAVE_FORMAT "Saved rendered frames to: {}"
#define IMGUI_LOG_RENDER_ANIMATION_SAVE_FORMAT "Saved rendered animation to: {}"
#define IMGUI_LOG_RENDER_ANIMATION_NO_ANIMATION_ERROR "No animation selected; rendering cancelled."
#define IMGUI_LOG_RENDER_ANIMATION_NO_FRAMES_ERROR "No frames to render; rendering cancelled."
#define IMGUI_LOG_RENDER_ANIMATION_DIRECTORY_ERROR "Invalid directory! Make sure it exists and you have write permissions."
#define IMGUI_LOG_RENDER_ANIMATION_PATH_ERROR "Invalid path! Make sure it's valid and you have write permissions."
#define IMGUI_LOG_RENDER_ANIMATION_FFMPEG_PATH_ERROR                                                                                                           \
  "Invalid FFmpeg path! Make sure you have it installed and the path is "                                                                                      \
  "correct."
#define IMGUI_LOG_RENDER_ANIMATION_FFMPEG_ERROR                                                                                                                \
  "FFmpeg could not render animation! Check paths or your FFmpeg "                                                                                             \
  "installation."
#define IMGUI_LOG_SPRITESHEET_SAVE_FORMAT "Saved spritesheet #{} to: {}"
#define IMGUI_LOG_DRAG_DROP_ERROR "Invalid file for dragging/dropping!"
#define IMGUI_LOG_ANIMATION_PASTE_ERROR "Failed to parse clipboard text as an animation."
#define IMGUI_LOG_FRAME_PASTE_ERROR "Failed to parse clipboard text as a frame."
#define IMGUI_LOG_RELOAD_SPRITESHEET "Reloaded spritesheet(s)."
#define IMGUI_LOG_REMOVE_UNUSED_SPRITESHEETS "Removed unused spritesheet(s)."
#define IMGUI_LOG_REMOVE_UNUSED_EVENTS "Removed unused event(s)."
#define IMGUI_LOG_ADD_SPRITESHEET_ERROR "Failed to add spritesheet: {}. Make sure it's a valid PNG file."
#define IMGUI_LOG_ADD_SPRITESHEET "Added spritesheet #{}: {}"
#define IMGUI_LOG_REPLACE_SPRITESHEET "Replaced spritesheet #{}: {}"

#define IMGUI_NONE "None"
#define IMGUI_ANIMATION_DEFAULT_FORMAT "(*) {}"
#define IMGUI_BUFFER_MAX UCHAR_MAX
#define IMGUI_INVISIBLE_LABEL_MARKER "##"
#define IMGUI_ITEM_SELECTABLE_EDITABLE_LABEL "## Editing"
#define IMGUI_LOG_FORMAT "## Log {}"
#define IMGUI_LOG_REDO_FORMAT "Redo: {}"
#define IMGUI_LOG_UNDO_FORMAT "Undo: {}"
#define IMGUI_OPENGL_VERSION "#version 330"
#define IMGUI_POSITION_FORMAT "Position: ({:8}, {:8})"
#define IMGUI_SPRITESHEET_FORMAT "#{} {}"
#define IMGUI_SPRITESHEET_ID_FORMAT "#{}"
#define IMGUI_TIMELINE_ITEM_CHILD_FORMAT "#{} {}"
#define IMGUI_LAYER_FORMAT "#{} {}"
#define IMGUI_NULL_FORMAT "#{} {}"
#define IMGUI_TIMELINE_FRAME_LABEL_FORMAT "## {}"
#define IMGUI_SELECTABLE_INPUT_INT_FORMAT "#{}"
#define IMGUI_TIMELINE_ANIMATION_NONE "Select an animation to show timeline..."
#define IMGUI_HOTKEY_CHANGE "Input new hotkey..."
#define IMGUI_LABEL_HOTKEY_FORMAT " ({})"
#define IMGUI_TOOLTIP_HOTKEY_FORMAT "\n(Hotkey: {})"
#define IMGUI_INVISIBLE_FORMAT "## {}"
#define IMGUI_RENDERING_FFMPEG_INFO_THRESHOLD 0.95f

#define IMGUI_TRIGGERS_FONT_SCALE 2.0

const ImVec2 IMGUI_TOOL_BUTTON_SIZE = {24, 24};
const ImVec2 IMGUI_TOOL_COLOR_SIZE = {24, 24};
const ImVec2 IMGUI_TIMELINE_FRAME_SIZE = {12, 36};
const ImVec2 IMGUI_TIMELINE_FRAME_ATLAS_OFFSET = {ATLAS_SIZE_SMALL.x * 0.25f, (IMGUI_TIMELINE_FRAME_SIZE.y * 0.5f) - (ATLAS_SIZE_SMALL.y * 0.5f)};
const ImVec2 IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE = {150, 0};
const ImVec2 IMGUI_TIMELINE_ITEM_SIZE = {300, IMGUI_TIMELINE_FRAME_SIZE.y};

const ImVec4 IMGUI_TIMELINE_FRAME_COLOR = {0.0f, 0.0f, 0.0f, 0.125};
const ImVec4 IMGUI_TIMELINE_FRAME_MULTIPLE_COLOR = {0.113, 0.184, 0.286, 0.125};
const ImVec4 IMGUI_TIMELINE_HEADER_FRAME_COLOR = {0.113, 0.184, 0.286, 0.5};
const ImVec4 IMGUI_TIMELINE_HEADER_FRAME_MULTIPLE_COLOR = {0.113, 0.184, 0.286, 1.0};
const ImVec4 IMGUI_TIMELINE_HEADER_FRAME_INACTIVE_COLOR = {0.113, 0.184, 0.286, 0.125};
const ImVec4 IMGUI_TIMELINE_HEADER_FRAME_MULTIPLE_INACTIVE_COLOR = {0.113, 0.184, 0.286, 0.25};
const ImVec4 IMGUI_ACTIVE_COLOR = {1.0, 1.0, 1.0, 1.0};
const ImVec4 IMGUI_INACTIVE_COLOR = {1.0, 1.0, 1.0, 0.25};

const ImVec2 IMGUI_TOOLTIP_OFFSET = {16, 8};
const vec2 IMGUI_SPRITESHEET_EDITOR_CROP_FORGIVENESS = {1, 1};

const ImGuiKey IMGUI_INPUT_DELETE = ImGuiKey_Delete;
const ImGuiKey IMGUI_INPUT_LEFT = ImGuiKey_LeftArrow;
const ImGuiKey IMGUI_INPUT_RIGHT = ImGuiKey_RightArrow;
const ImGuiKey IMGUI_INPUT_UP = ImGuiKey_UpArrow;
const ImGuiKey IMGUI_INPUT_DOWN = ImGuiKey_DownArrow;
const ImGuiKey IMGUI_INPUT_SHIFT = ImGuiMod_Shift;
const ImGuiKey IMGUI_INPUT_CTRL = ImGuiMod_Ctrl;
const ImGuiKey IMGUI_INPUT_ALT = ImGuiMod_Alt;
const ImGuiKey IMGUI_INPUT_ZOOM_IN = ImGuiKey_1;
const ImGuiKey IMGUI_INPUT_ZOOM_OUT = ImGuiKey_2;
const ImGuiKey IMGUI_INPUT_ENTER = ImGuiKey_Enter;
const ImGuiKey IMGUI_INPUT_RENAME = ImGuiKey_F2;
const ImGuiKey IMGUI_INPUT_DEFAULT = ImGuiKey_Home;
const ImGuiKeyChord IMGUI_CHORD_SELECT_ALL = ImGuiMod_Ctrl | ImGuiKey_A;
const ImGuiKeyChord IMGUI_CHORD_SELECT_NONE = ImGuiKey_Escape;
const ImGuiMouseButton IMGUI_MOUSE_DEFAULT = ImGuiMouseButton_Middle;

enum ImguiPopupType { IMGUI_POPUP_NONE, IMGUI_POPUP_BY_ITEM, IMGUI_POPUP_CENTER_WINDOW };
enum ImguiPopupState { IMGUI_POPUP_STATE_CLOSED, IMGUI_POPUP_STATE_OPEN, IMGUI_POPUP_STATE_CONFIRM, IMGUI_POPUP_STATE_CANCEL };

struct ImguiColorSet {
  std::optional<ImVec4> normal = {};
  std::optional<ImVec4> active = {};
  std::optional<ImVec4> hovered = {};
  std::optional<ImVec4> border = {};
};

struct ImguiLogItem {
  std::string text;
  float timeRemaining;
};

struct Imgui {
  Dialog* dialog = nullptr;
  Resources* resources = nullptr;
  Anm2* anm2 = nullptr;
  Anm2Reference* reference = nullptr;
  Editor* editor = nullptr;
  Preview* preview = nullptr;
  GeneratePreview* generatePreview = nullptr;
  Settings* settings = nullptr;
  Snapshots* snapshots = nullptr;
  Clipboard* clipboard = nullptr;
  SDL_Window* window = nullptr;
  SDL_GLContext* glContext = nullptr;
  Anm2 saveAnm2 = Anm2();
  ImFont* fonts[FONT_COUNT] = {};
  ImGuiStyle style;
  std::vector<ImguiLogItem> log{};
  std::string pendingPopup{};
  ImguiPopupType pendingPopupType = IMGUI_POPUP_NONE;
  ImVec2 pendingPopupPosition{};
  std::set<int> selectedAnimationIndices{};
  int lastAnimationIndex = ID_NONE;
  std::set<int> selectedSpritesheetIDs{};
  int lastSpritesheetID = ID_NONE;
  int selectedEventID = ID_NONE;
  int selectedLayerID = ID_NONE;
  int selectedNullID = ID_NONE;
  SDL_SystemCursor cursor = SDL_SYSTEM_CURSOR_DEFAULT;
  SDL_SystemCursor pendingCursor = SDL_SYSTEM_CURSOR_DEFAULT;
  bool isCursorSet = false;
  bool isContextualActionsEnabled = true;
  bool isQuit = false;
  bool isTryQuit = false;
};

static inline void imgui_snapshot(Imgui* self, const std::string& action = SNAPSHOT_ACTION) {
  self->snapshots->action = action;
  Snapshot snapshot = snapshot_get(self->snapshots);
  snapshots_undo_push(self->snapshots, &snapshot);
}

static void imgui_log_push(Imgui* self, const std::string& text) {
  self->log.push_back({text, IMGUI_LOG_DURATION});
  log_imgui(text);
}

static inline void imgui_anm2_new(Imgui* self) {
  anm2_free(self->anm2);
  anm2_new(self->anm2);

  *self->reference = Anm2Reference();
  self->selectedAnimationIndices = {};
  self->selectedSpritesheetIDs = {};
}

static inline void imgui_file_open(Imgui* self) { dialog_anm2_open(self->dialog); }

static inline void imgui_file_save(Imgui* self) {
  if (self->anm2->path.empty())
    dialog_anm2_save(self->dialog);
  else {
    anm2_serialize(self->anm2, self->anm2->path);
    imgui_log_push(self, std::format(IMGUI_LOG_FILE_SAVE_FORMAT, self->anm2->path));
  }

  self->saveAnm2 = *self->anm2;
}

static inline void imgui_file_new(Imgui* self) {
  std::string path = self->anm2->path;
  imgui_anm2_new(self);
  self->anm2->path = path;
  imgui_file_save(self);
}

static inline void imgui_file_save_as(Imgui* self) { dialog_anm2_save(self->dialog); }

static inline void imgui_quit(Imgui* self) {
  if (self->saveAnm2 != *self->anm2)
    self->isTryQuit = true;
  else
    self->isQuit = true;
}

static inline void imgui_explore(Imgui* self) {
  std::filesystem::path filePath = self->anm2->path;
  std::filesystem::path parentPath = filePath.parent_path();
  dialog_explorer_open(parentPath.string());
}

static inline void imgui_tool_pan_set(Imgui* self) { self->settings->tool = TOOL_PAN; }
static inline void imgui_tool_move_set(Imgui* self) { self->settings->tool = TOOL_MOVE; }
static inline void imgui_tool_rotate_set(Imgui* self) { self->settings->tool = TOOL_ROTATE; }
static inline void imgui_tool_scale_set(Imgui* self) { self->settings->tool = TOOL_SCALE; }
static inline void imgui_tool_crop_set(Imgui* self) { self->settings->tool = TOOL_CROP; }
static inline void imgui_tool_draw_set(Imgui* self) { self->settings->tool = TOOL_DRAW; }
static inline void imgui_tool_erase_set(Imgui* self) { self->settings->tool = TOOL_ERASE; }
static inline void imgui_tool_color_picker_set(Imgui* self) { self->settings->tool = TOOL_COLOR_PICKER; }

static inline void imgui_undo(Imgui* self) {
  if (self->snapshots->undoStack.top == 0)
    return;

  snapshots_undo(self->snapshots);
  imgui_log_push(self, std::format(IMGUI_LOG_UNDO_FORMAT, self->snapshots->action));
}

static inline void imgui_redo(Imgui* self) {
  if (self->snapshots->redoStack.top == 0)
    return;

  std::string action = self->snapshots->action;
  snapshots_redo(self->snapshots);
  imgui_log_push(self, std::format(IMGUI_LOG_REDO_FORMAT, action));
}

static inline void imgui_cut(Imgui* self) { clipboard_cut(self->clipboard); }

static inline void imgui_copy(Imgui* self) { clipboard_copy(self->clipboard); }

static inline void imgui_paste(Imgui* self) {
  if (!clipboard_paste(self->clipboard)) {
    switch (self->clipboard->type) {
    case CLIPBOARD_FRAME:
      imgui_log_push(self, IMGUI_LOG_FRAME_PASTE_ERROR);
      break;
    case CLIPBOARD_ANIMATION:
      imgui_log_push(self, IMGUI_LOG_ANIMATION_PASTE_ERROR);
      break;
    default:
      break;
    }
  }
}

static inline void imgui_onionskin_toggle(Imgui* self) { self->settings->onionskinIsEnabled = !self->settings->onionskinIsEnabled; }

static std::unordered_map<std::string, ImGuiKey> IMGUI_KEY_MAP = {{"A", ImGuiKey_A},
                                                                  {"B", ImGuiKey_B},
                                                                  {"C", ImGuiKey_C},
                                                                  {"D", ImGuiKey_D},
                                                                  {"E", ImGuiKey_E},
                                                                  {"F", ImGuiKey_F},
                                                                  {"G", ImGuiKey_G},
                                                                  {"H", ImGuiKey_H},
                                                                  {"I", ImGuiKey_I},
                                                                  {"J", ImGuiKey_J},
                                                                  {"K", ImGuiKey_K},
                                                                  {"L", ImGuiKey_L},
                                                                  {"M", ImGuiKey_M},
                                                                  {"N", ImGuiKey_N},
                                                                  {"O", ImGuiKey_O},
                                                                  {"P", ImGuiKey_P},
                                                                  {"Q", ImGuiKey_Q},
                                                                  {"R", ImGuiKey_R},
                                                                  {"S", ImGuiKey_S},
                                                                  {"T", ImGuiKey_T},
                                                                  {"U", ImGuiKey_U},
                                                                  {"V", ImGuiKey_V},
                                                                  {"W", ImGuiKey_W},
                                                                  {"X", ImGuiKey_X},
                                                                  {"Y", ImGuiKey_Y},
                                                                  {"Z", ImGuiKey_Z},

                                                                  {"0", ImGuiKey_0},
                                                                  {"1", ImGuiKey_1},
                                                                  {"2", ImGuiKey_2},
                                                                  {"3", ImGuiKey_3},
                                                                  {"4", ImGuiKey_4},
                                                                  {"5", ImGuiKey_5},
                                                                  {"6", ImGuiKey_6},
                                                                  {"7", ImGuiKey_7},
                                                                  {"8", ImGuiKey_8},
                                                                  {"9", ImGuiKey_9},

                                                                  {"Num0", ImGuiKey_Keypad0},
                                                                  {"Num1", ImGuiKey_Keypad1},
                                                                  {"Num2", ImGuiKey_Keypad2},
                                                                  {"Num3", ImGuiKey_Keypad3},
                                                                  {"Num4", ImGuiKey_Keypad4},
                                                                  {"Num5", ImGuiKey_Keypad5},
                                                                  {"Num6", ImGuiKey_Keypad6},
                                                                  {"Num7", ImGuiKey_Keypad7},
                                                                  {"Num8", ImGuiKey_Keypad8},
                                                                  {"Num9", ImGuiKey_Keypad9},
                                                                  {"NumAdd", ImGuiKey_KeypadAdd},
                                                                  {"NumSubtract", ImGuiKey_KeypadSubtract},
                                                                  {"NumMultiply", ImGuiKey_KeypadMultiply},
                                                                  {"NumDivide", ImGuiKey_KeypadDivide},
                                                                  {"NumEnter", ImGuiKey_KeypadEnter},
                                                                  {"NumDecimal", ImGuiKey_KeypadDecimal},

                                                                  {"F1", ImGuiKey_F1},
                                                                  {"F2", ImGuiKey_F2},
                                                                  {"F3", ImGuiKey_F3},
                                                                  {"F4", ImGuiKey_F4},
                                                                  {"F5", ImGuiKey_F5},
                                                                  {"F6", ImGuiKey_F6},
                                                                  {"F7", ImGuiKey_F7},
                                                                  {"F8", ImGuiKey_F8},
                                                                  {"F9", ImGuiKey_F9},
                                                                  {"F10", ImGuiKey_F10},
                                                                  {"F11", ImGuiKey_F11},
                                                                  {"F12", ImGuiKey_F12},

                                                                  {"Up", ImGuiKey_UpArrow},
                                                                  {"Down", ImGuiKey_DownArrow},
                                                                  {"Left", ImGuiKey_LeftArrow},
                                                                  {"Right", ImGuiKey_RightArrow},

                                                                  {"Space", ImGuiKey_Space},
                                                                  {"Enter", ImGuiKey_Enter},
                                                                  {"Escape", ImGuiKey_Escape},
                                                                  {"Tab", ImGuiKey_Tab},
                                                                  {"Backspace", ImGuiKey_Backspace},
                                                                  {"Delete", ImGuiKey_Delete},
                                                                  {"Insert", ImGuiKey_Insert},
                                                                  {"Home", ImGuiKey_Home},
                                                                  {"End", ImGuiKey_End},
                                                                  {"PageUp", ImGuiKey_PageUp},
                                                                  {"PageDown", ImGuiKey_PageDown},

                                                                  {"Minus", ImGuiKey_Minus},
                                                                  {"Equal", ImGuiKey_Equal},
                                                                  {"LeftBracket", ImGuiKey_LeftBracket},
                                                                  {"RightBracket", ImGuiKey_RightBracket},
                                                                  {"Semicolon", ImGuiKey_Semicolon},
                                                                  {"Apostrophe", ImGuiKey_Apostrophe},
                                                                  {"Comma", ImGuiKey_Comma},
                                                                  {"Period", ImGuiKey_Period},
                                                                  {"Slash", ImGuiKey_Slash},
                                                                  {"Backslash", ImGuiKey_Backslash},
                                                                  {"GraveAccent", ImGuiKey_GraveAccent},

                                                                  {"MouseLeft", ImGuiKey_MouseLeft},
                                                                  {"MouseRight", ImGuiKey_MouseRight},
                                                                  {"MouseMiddle", ImGuiKey_MouseMiddle},
                                                                  {"MouseX1", ImGuiKey_MouseX1},
                                                                  {"MouseX2", ImGuiKey_MouseX2}};

static std::unordered_map<std::string, ImGuiKey> IMGUI_MOD_MAP = {
    {"Ctrl", ImGuiMod_Ctrl},
    {"Shift", ImGuiMod_Shift},
    {"Alt", ImGuiMod_Alt},
    {"Super", ImGuiMod_Super},
};

static inline ImGuiKey imgui_key_from_char_get(char c) {
  if (c >= 'a' && c <= 'z')
    c -= 'a' - 'A';
  if (c >= 'A' && c <= 'Z')
    return (ImGuiKey)(ImGuiKey_A + (c - 'A'));
  return ImGuiKey_None;
}

static inline std::string imgui_string_from_chord_get(ImGuiKeyChord chord) {
  std::string result;

  if (chord & ImGuiMod_Ctrl)
    result += "Ctrl+";
  if (chord & ImGuiMod_Shift)
    result += "Shift+";
  if (chord & ImGuiMod_Alt)
    result += "Alt+";
  if (chord & ImGuiMod_Super)
    result += "Super+";

  ImGuiKey key = (ImGuiKey)(chord & ~ImGuiMod_Mask_);

  if (key != ImGuiKey_None) {
    const char* name = ImGui::GetKeyName(key);
    if (name && *name)
      result += name;
    else
      result += "Unknown";
  }

  if (!result.empty() && result.back() == '+')
    result.pop_back();

  return result;
}

static inline ImGuiKeyChord imgui_chord_from_string_get(const std::string& str) {
  ImGuiKeyChord chord = 0;
  ImGuiKey baseKey = ImGuiKey_None;

  std::stringstream ss(str);
  std::string token;
  while (std::getline(ss, token, '+')) {
    // trim
    token.erase(0, token.find_first_not_of(" \t\r\n"));
    token.erase(token.find_last_not_of(" \t\r\n") + 1);

    if (token.empty())
      continue;

    if (auto it = IMGUI_MOD_MAP.find(token); it != IMGUI_MOD_MAP.end()) {
      chord |= it->second;
    } else if (baseKey == ImGuiKey_None) {
      if (auto it2 = IMGUI_KEY_MAP.find(token); it2 != IMGUI_KEY_MAP.end())
        baseKey = it2->second;
    }
  }

  if (baseKey != ImGuiKey_None)
    chord |= baseKey;

  return chord;
}

static inline void imgui_keyboard_nav_enable(void) { ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; }
static inline void imgui_keyboard_nav_disable(void) { ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard; }
static inline void imgui_contextual_actions_enable(Imgui* self) { self->isContextualActionsEnabled = true; }
static inline void imgui_contextual_actions_disable(Imgui* self) { self->isContextualActionsEnabled = false; }
static inline bool imgui_is_popup_open(const std::string& label) { return ImGui::IsPopupOpen(label.c_str()); }
static inline bool imgui_is_any_popup_open(void) { return ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId); }
static inline void imgui_open_popup(const std::string& label) { ImGui::OpenPopup(label.c_str()); }
static inline void imgui_pending_popup_process(Imgui* self) {
  if (self->pendingPopup.empty())
    return;

  switch (self->pendingPopupType) {
  case IMGUI_POPUP_CENTER_WINDOW:
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    break;
  case IMGUI_POPUP_BY_ITEM:
  default:
    ImGui::SetNextWindowPos(self->pendingPopupPosition);
    break;
  }

  if (!imgui_is_any_popup_open())
    imgui_open_popup(self->pendingPopup.c_str());

  self->pendingPopup.clear();
  self->pendingPopupType = IMGUI_POPUP_NONE;
  self->pendingPopupPosition = ImVec2();
}

static inline bool imgui_begin_popup(const std::string& label, Imgui* imgui, ImVec2 size = ImVec2()) {
  imgui_pending_popup_process(imgui);
  if (size != ImVec2())
    ImGui::SetNextWindowSizeConstraints(size, ImVec2(FLT_MAX, FLT_MAX));
  bool isActivated = ImGui::BeginPopup(label.c_str(), IMGUI_POPUP_FLAGS);
  return isActivated;
}

static inline bool imgui_begin_popup_modal(const std::string& label, Imgui* imgui, ImVec2 size = ImVec2()) {
  imgui_pending_popup_process(imgui);
  if (size != ImVec2())
    ImGui::SetNextWindowSizeConstraints(size, ImVec2(FLT_MAX, FLT_MAX));
  bool isActivated = ImGui::BeginPopupModal(label.c_str(), nullptr, IMGUI_POPUP_MODAL_FLAGS);
  if (isActivated)
    imgui_contextual_actions_disable(imgui);
  return isActivated;
}

static inline void imgui_close_current_popup(Imgui* imgui) {
  imgui_contextual_actions_enable(imgui);
  ImGui::CloseCurrentPopup();
}

static inline void imgui_end_popup(Imgui* imgui) {
  ImGui::EndPopup();
  imgui_pending_popup_process(imgui);
}

enum ImguiItemType {
  IMGUI_ITEM,
  IMGUI_TEXT,
  IMGUI_IMAGE,
  IMGUI_BEGIN_WINDOW,
  IMGUI_END_WINDOW,
  IMGUI_DOCKSPACE,
  IMGUI_BEGIN_CHILD,
  IMGUI_END_CHILD,
  IMGUI_TABLE,
  IMGUI_CONFIRM_POPUP,
  IMGUI_SELECTABLE,
  IMGUI_BUTTON,
  IMGUI_RADIO_BUTTON,
  IMGUI_COLOR_BUTTON,
  IMGUI_CHECKBOX,
  IMGUI_INPUT_INT,
  IMGUI_INPUT_TEXT,
  IMGUI_INPUT_FLOAT,
  IMGUI_SLIDER_FLOAT,
  IMGUI_DRAG_FLOAT,
  IMGUI_COLOR_EDIT,
  IMGUI_COMBO,
  IMGUI_ATLAS_TEXT,
  IMGUI_ATLAS_BUTTON
};

struct ImguiItem;

static std::vector<ImguiItem*>& imgui_item_registry(void) {
  static std::vector<ImguiItem*> registry;
  return registry;
}

static ImGuiKeyChord* imgui_hotkey_chord_registry(void) {
  static ImGuiKeyChord registry[HOTKEY_COUNT];
  return registry;
}

typedef void (*ImguiFunction)(Imgui*);
using IntStringMap = std::map<int, std::string>;

#define IMGUI_ITEM_MEMBERS                                                                                                                                     \
  X(label, std::string, false, {})                                                                                                                             \
  X(tooltip, std::string, false, {})                                                                                                                           \
  X(snapshotAction, std::string, true, {})                                                                                                                     \
  X(popup, std::string, false, {})                                                                                                                             \
  X(dragDrop, std::string, true, {})                                                                                                                           \
  X(focusWindow, std::string, true, {})                                                                                                                        \
  X(items, IntStringMap, false, {})                                                                                                                            \
  X(atlas, AtlasType, false, ATLAS_NONE)                                                                                                                       \
  X(textureID, int, false, ID_NONE)                                                                                                                            \
  X(chord, ImGuiKeyChord, true, {})                                                                                                                            \
  X(hotkey, HotkeyType, true, {})                                                                                                                              \
  X(mnemonicKey, ImGuiKey, false, ImGuiKey_None)                                                                                                               \
  X(mnemonicIndex, int, false, INDEX_NONE)                                                                                                                     \
  X(position, vec2, true, {})                                                                                                                                  \
  X(size, vec2, true, {})                                                                                                                                      \
  X(scale, float, false, 1.0f)                                                                                                                                 \
  X(uvMin, vec2, false, vec2())                                                                                                                                \
  X(uvMax, vec2, false, vec2(1.0f))                                                                                                                            \
  X(popupSize, vec2, false, {})                                                                                                                                \
  X(color, ImguiColorSet, false, {})                                                                                                                           \
  X(function, ImguiFunction, false, {})                                                                                                                        \
  X(popupType, ImguiPopupType, false, IMGUI_POPUP_CENTER_WINDOW)                                                                                               \
  X(isDisabled, bool, false, false)                                                                                                                            \
  X(isSelected, bool, false, false)                                                                                                                            \
  X(isMnemonicDisabled, bool, false, false)                                                                                                                    \
  X(isEmptyFormat, bool, false, false)                                                                                                                         \
  X(isUseItemActivated, bool, false, false)                                                                                                                    \
  X(isWidthToText, bool, false, false)                                                                                                                         \
  X(isWidthToRegion, bool, false, false)                                                                                                                       \
  X(isHeightToRegion, bool, false, false)                                                                                                                      \
  X(isHotkeyInLabel, bool, false, false)                                                                                                                       \
  X(isAllowHotkeyWhenFocusWindow, bool, false, false)                                                                                                          \
  X(isAtlasStretch, bool, false, false)                                                                                                                        \
  X(isSameLine, bool, false, false)                                                                                                                            \
  X(isSeparator, bool, false, false)                                                                                                                           \
  X(id, int, false, 0)                                                                                                                                         \
  X(idOffset, int, false, {})                                                                                                                                  \
  X(speed, float, false, 0.25f)                                                                                                                                \
  X(step, float, false, 1.0f)                                                                                                                                  \
  X(stepFast, float, false, 10.0f)                                                                                                                             \
  X(min, int, true, {})                                                                                                                                        \
  X(max, int, true, {})                                                                                                                                        \
  X(value, int, false, {})                                                                                                                                     \
  X(atlasOffset, vec2, true, {})                                                                                                                               \
  X(cursorPosition, vec2, true, {})                                                                                                                            \
  X(cursorOffset, vec2, false, {})                                                                                                                             \
  X(textPosition, vec2, true, {})                                                                                                                              \
  X(textOffset, vec2, false, {})                                                                                                                               \
  X(itemSpacing, vec2, true, {})                                                                                                                               \
  X(windowPadding, vec2, true, {})                                                                                                                             \
  X(framePadding, vec2, true, {})                                                                                                                              \
  X(border, int, true, {})                                                                                                                                     \
  X(flags, int, false, {})                                                                                                                                     \
  X(windowFlags, int, false, {})                                                                                                                               \
  X(rowCount, int, true, {})                                                                                                                                   \
  X(font, ImFont*, true, {})

struct ImguiItemOverride {
#define X(name, type, isOptional, ...) std::optional<type> name = {};
  IMGUI_ITEM_MEMBERS
#undef X
};

struct ImguiItem {
#define X(name, type, isOptional, ...) std::conditional_t<isOptional, std::optional<type>, type> name = __VA_ARGS__;
  IMGUI_ITEM_MEMBERS
#undef X

  bool is_chord() const { return chord.has_value() || hotkey.has_value(); }
  bool is_mnemonic() const { return mnemonicKey != ImGuiKey_None; }
  bool is_range() const { return min != 0 || max != 0; }
  const char* drag_drop_get() const { return dragDrop->c_str(); }
  const char* text_get() const { return tooltip.c_str(); }

  void construct() {
    static int idNew = 0;
    id = idNew++;

    imgui_item_registry().push_back(this);

    std::string labelNew{};

    for (int i = 0; i < (int)label.size(); i++) {
      if (label[i] == '&') {
        if (label[i + 1] == '&') {
          labelNew += '&';
          i++;
        } else if (label[i + 1] != '\0') {
          mnemonicKey = imgui_key_from_char_get(label[i + 1]);
          mnemonicIndex = (int)labelNew.size();
          labelNew += label[i + 1];
          i++;
        }
      } else
        labelNew += label[i];
    }

    label = labelNew;
  }

  ImguiItem copy(const ImguiItemOverride& override) const {
    ImguiItem out = *this;

#define X(name, type, value, isOptional)                                                                                                                       \
  if (override.name.has_value())                                                                                                                               \
    out.name = *override.name;
    IMGUI_ITEM_MEMBERS
#undef X

    out.id += out.idOffset;

    return out;
  }

  ImGuiKeyChord chord_get() const {
    if (hotkey.has_value())
      return imgui_hotkey_chord_registry()[*hotkey];
    if (chord.has_value())
      return *chord;
    return ImGuiKey_None;
  }

  std::string label_get() const {
    std::string newLabel = label;
    if (isHotkeyInLabel)
      newLabel += std::format(IMGUI_LABEL_HOTKEY_FORMAT, imgui_string_from_chord_get(chord_get()));
    return newLabel;
  }

  std::string tooltip_get() const {
    std::string newTooltip = tooltip;
    if (is_chord())
      newTooltip += std::format(IMGUI_TOOLTIP_HOTKEY_FORMAT, imgui_string_from_chord_get(chord_get()));
    return newTooltip;
  }
};

#define IMGUI_ITEM(NAME, ...)                                                                                                                                  \
  const inline ImguiItem NAME = [] {                                                                                                                           \
    ImguiItem self;                                                                                                                                            \
    __VA_ARGS__;                                                                                                                                               \
    self.construct();                                                                                                                                          \
    return self;                                                                                                                                               \
  }()

IMGUI_ITEM(IMGUI_WINDOW_MAIN, self.label = "## Window",
           self.flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

IMGUI_ITEM(IMGUI_DOCKSPACE_MAIN, self.label = "## Dockspace", self.flags = ImGuiDockNodeFlags_PassthruCentralNode);

IMGUI_ITEM(IMGUI_FOOTER_CHILD, self.label = "## Footer Child", self.size = {0, 42}, self.flags = true);

IMGUI_ITEM(IMGUI_TASKBAR, self.label = "Taskbar", self.size = {0, 32},
           self.flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings);

IMGUI_ITEM(IMGUI_FILE, self.label = "&File", self.tooltip = "Opens the file menu, for reading/writing anm2 files.", self.popup = "## File Popup",
           self.popupType = IMGUI_POPUP_BY_ITEM, self.isWidthToText = true, self.isHeightToRegion = true, self.isSameLine = true);

IMGUI_ITEM(IMGUI_NEW, self.label = "&New", self.tooltip = "Load a blank .anm2 file to edit.", self.function = imgui_file_new, self.hotkey = HOTKEY_NEW,
           self.isWidthToRegion = true, self.isHotkeyInLabel = true);

IMGUI_ITEM(IMGUI_OPEN, self.label = "&Open", self.tooltip = "Open an existing .anm2 file to edit.", self.function = imgui_file_open, self.hotkey = HOTKEY_OPEN,
           self.isWidthToRegion = true, self.isHotkeyInLabel = true);

IMGUI_ITEM(IMGUI_SAVE, self.label = "&Save",
           self.tooltip = "Saves the current .anm2 file to its path.\nIf no "
                          "path exists, one can be chosen.",
           self.function = imgui_file_save, self.hotkey = HOTKEY_SAVE, self.isWidthToText = true, self.isHotkeyInLabel = true);

IMGUI_ITEM(IMGUI_SAVE_AS, self.label = "S&ave As", self.tooltip = "Saves the current .anm2 file to a chosen path.", self.function = imgui_file_save_as,
           self.hotkey = HOTKEY_SAVE_AS, self.isWidthToText = true, self.isHotkeyInLabel = true);

IMGUI_ITEM(IMGUI_EXPLORE_ANM2_LOCATION, self.label = "E&xplore Anm2 Location", self.tooltip = "Open the system's file explorer in the anm2's path.",
           self.function = imgui_explore, self.isWidthToText = true, self.isSeparator = true);

IMGUI_ITEM(IMGUI_EXIT, self.label = "&Exit", self.tooltip = "Exits the program.", self.function = imgui_quit, self.hotkey = HOTKEY_EXIT,
           self.isWidthToText = true, self.isHotkeyInLabel = true);

IMGUI_ITEM(IMGUI_EXIT_CONFIRMATION, self.label = "Exit Confirmation", self.tooltip = "Unsaved changes will be lost!\nAre you sure you want to exit?");

IMGUI_ITEM(IMGUI_OPEN_CONFIRMATION, self.label = "Open Confirmation", self.tooltip = "Unsaved changes will be lost!\nAre you sure you open a new file?");

IMGUI_ITEM(IMGUI_NO_ANM2_PATH_CONFIRMATION, self.label = "No Anm2 Path", self.tooltip = "You will need to load or make a new .anm2 file first!\n");

IMGUI_ITEM(IMGUI_WIZARD, self.label = "&Wizard", self.tooltip = "Opens the wizard menu, for neat functions related to the .anm2.",
           self.popup = "## Wizard Popup", self.popupType = IMGUI_POPUP_BY_ITEM, self.isHeightToRegion = true, self.isWidthToText = true,
           self.isSameLine = true);

#define IMGUI_GENERATE_ANIMATION_FROM_GRID_PADDING 40
IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID, self.label = "&Generate Animation from Grid", self.tooltip = "Generate a new animation from grid values.",
           self.popup = "Generate Animation from Grid", self.popupType = IMGUI_POPUP_CENTER_WINDOW,
           self.popupSize = {(GENERATE_PREVIEW_SIZE.x * 2) + IMGUI_GENERATE_ANIMATION_FROM_GRID_PADDING,
                             GENERATE_PREVIEW_SIZE.y + (IMGUI_FOOTER_CHILD.size->y * 2) + (IMGUI_GENERATE_ANIMATION_FROM_GRID_PADDING * 0.5f)});

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_OPTIONS_CHILD, self.label = "## Generate Animation From Grid Options Child",
           self.size = {IMGUI_GENERATE_ANIMATION_FROM_GRID.popupSize.x * 0.5f, IMGUI_GENERATE_ANIMATION_FROM_GRID.popupSize.y - IMGUI_FOOTER_CHILD.size->y},
           self.flags = true);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_START_POSITION, self.label = "Start Position",
           self.tooltip = "Set the starting position on the layer's "
                          "spritesheet for the generated animation.");

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_SIZE, self.label = "Size", self.tooltip = "Set the size of each frame in the generated animation.");

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_PIVOT, self.label = "Pivot", self.tooltip = "Set the pivot of each frame in the generated animation.");

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_ROWS, self.label = "Rows", self.tooltip = "Set how many rows will be used in the generated animation.",
           self.min = 1, self.max = 1000);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_COLUMNS, self.label = "Columns", self.tooltip = "Set how many columns will be used in the generated animation.",
           self.min = 1, self.max = 1000);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_COUNT, self.label = "Count", self.tooltip = "Set how many frames will be made for the generated animation.",
           self.value = ANM2_FRAME_NUM_MIN);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_DELAY, self.label = "Delay", self.tooltip = "Set the delay of each frame in the generated animation.",
           self.max = 1000);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_PREVIEW_CHILD, self.label = "## Generate Animation From Grid Preview Child",
           self.size = {IMGUI_GENERATE_ANIMATION_FROM_GRID.popupSize.x * 0.5f, IMGUI_GENERATE_ANIMATION_FROM_GRID.popupSize.y - IMGUI_FOOTER_CHILD.size->y},
           self.flags = true);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_SLIDER_CHILD, self.label = "## Generate Animation From Grid Slider Child",
           self.size = {(IMGUI_GENERATE_ANIMATION_FROM_GRID.popupSize.x * 0.5f) - (IMGUI_GENERATE_ANIMATION_FROM_GRID_PADDING * 0.5f),
                        IMGUI_FOOTER_CHILD.size->y},
           self.flags = true);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_SLIDER, self.label = "## Generate Animation From Grid Slider",
           self.tooltip = "Change the time of the generated animation preview.", self.min = GENERATE_PREVIEW_TIME_MIN, self.max = GENERATE_PREVIEW_TIME_MAX,
           self.value = GENERATE_PREVIEW_TIME_MIN, self.rowCount = 1, self.isEmptyFormat = true, self.flags = ImGuiSliderFlags_NoInput);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_GENERATE, self.label = "Generate", self.tooltip = "Generate an animation with the used settings.",
           self.snapshotAction = "Generate Animation from Grid", self.rowCount = IMGUI_CONFIRM_POPUP_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES, self.label = "&Change All Frame Properties",
           self.tooltip = "Change all frame properties in the selected "
                          "animation item (or selected frame).",
           self.popup = "Change All Frame Properties", self.popupType = IMGUI_POPUP_CENTER_WINDOW, self.popupSize = {500, 405});

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_CHILD, self.label = "## Change All Frame Properties Child",
           self.size = {IMGUI_CHANGE_ALL_FRAME_PROPERTIES.popupSize.x, 275}, self.flags = true);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_SETTINGS_CHILD, self.label = "## Change All Frame Properties Settings Child",
           self.size = {IMGUI_CHANGE_ALL_FRAME_PROPERTIES.popupSize.x, 55}, self.flags = true);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_SETTINGS, self.label = "Settings");

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_FROM_SELECTED_FRAME, self.label = "From Selected Frames",
           self.tooltip = "The set frame properties will start from the selected frame.", self.isSameLine = true);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_NUMBER_FRAMES, self.label = "# of Frames",
           self.tooltip = "Set the amount of frames that the set frame properties will apply to.", self.size = {200, 0}, self.value = ANM2_FRAME_NUM_MIN,
           self.max = 1000);

#define IMGUI_CHANGE_ALL_FRAME_PROPERTIES_OPTIONS_ROW_COUNT 4
IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_ADD, self.label = "Add", self.tooltip = "The specified values will be added to all specified frames.",
           self.snapshotAction = "Add Frame Properties", self.rowCount = IMGUI_CHANGE_ALL_FRAME_PROPERTIES_OPTIONS_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_SUBTRACT, self.label = "Subtract",
           self.tooltip = "The specified values will be subtracted from all selected frames.", self.snapshotAction = "Subtract Frame Properties",
           self.rowCount = IMGUI_CHANGE_ALL_FRAME_PROPERTIES_OPTIONS_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_SET, self.label = "Set",
           self.tooltip = "The specified values will be set to the specified "
                          "value in selected frames.",
           self.snapshotAction = "Set Frame Properties", self.rowCount = IMGUI_CHANGE_ALL_FRAME_PROPERTIES_OPTIONS_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_CANCEL, self.label = "Cancel", self.tooltip = "Cancel changing all frame properties.",
           self.rowCount = IMGUI_CHANGE_ALL_FRAME_PROPERTIES_OPTIONS_ROW_COUNT);

IMGUI_ITEM(IMGUI_SCALE_ANM2, self.label = "S&cale Anm2", self.tooltip = "Scale up all size and position-related frame properties in the anm2.",
           self.popup = "Scale Anm2", self.popupType = IMGUI_POPUP_CENTER_WINDOW, self.popupSize = {260, 75}, self.isWidthToText = true,
           self.isSeparator = true);

IMGUI_ITEM(IMGUI_SCALE_ANM2_OPTIONS_CHILD, self.label = "## Scale Anm2 Options Child", self.flags = true);

IMGUI_ITEM(IMGUI_SCALE_ANM2_VALUE, self.label = "Value",
           self.tooltip = "The size and position-related frame properties in "
                          "the anm2 will be scaled by this value.",
           self.value = 1, self.step = 0.25, self.stepFast = 1);

IMGUI_ITEM(IMGUI_SCALE_ANM2_SCALE, self.label = "Scale", self.tooltip = "Scale the anm2 with the value specified.", self.snapshotAction = "Scale Anm2",
           self.rowCount = IMGUI_CONFIRM_POPUP_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION, self.label = "&Render Animation",
           self.tooltip = "Renders the current animation preview; output "
                          "options can be customized.",
           self.popup = "Render Animation", self.popupSize = {500, 170}, self.popupType = IMGUI_POPUP_CENTER_WINDOW);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_CHILD, self.label = "## Render Animation Child", self.flags = true);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_FOOTER_CHILD, self.label = "## Render Animation Footer Child", self.flags = true);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_LOCATION_BROWSE, self.label = "## Location Browse", self.tooltip = "Open file explorer to pick rendered animation location.",
           self.atlas = ATLAS_FOLDER, self.isSameLine = true);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_LOCATION, self.label = "Location",
           self.tooltip = "Select the location of the rendered animation.\nFor PNG "
                          "images, this should be a directory, otherwise, a filepath.",
           self.max = 1024);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_FFMPEG_BROWSE, self.label = "## FFMpeg Browse", self.tooltip = "Open file explorer to pick the path of FFmpeg",
           self.atlas = ATLAS_FOLDER, self.isSameLine = true);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_FFMPEG_PATH, self.label = "FFmpeg Path",
           self.tooltip = "Sets the path FFmpeg currently resides in.\nFFmpeg is required "
                          "for rendering animations.\nDownload it from "
                          "https://ffmpeg.org/, your package manager, or wherever else.",
           self.max = 1024);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_OUTPUT, self.label = "Output",
           self.tooltip = "Select the rendered animation output.\nIt can either be "
                          "one animated image or a sequence of frames.",
           self.items = {{RENDER_PNG, RENDER_TYPE_STRINGS[RENDER_PNG]},
                         {RENDER_GIF, RENDER_TYPE_STRINGS[RENDER_GIF]},
                         {RENDER_WEBM, RENDER_TYPE_STRINGS[RENDER_WEBM]},
                         {RENDER_MP4, RENDER_TYPE_STRINGS[RENDER_MP4]}},
           self.value = RENDER_PNG);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_FORMAT, self.label = "Format",
           self.tooltip = "(PNG images only).\nSet the format of each output frame; i.e., "
                          "its filename.\nThe format will only take one argument; that "
                          "being the frame's index.\nFor example, a format like "
                          "\"{}.png\" will export a frame of index 0 as \"0.png\".",
           self.max = UCHAR_MAX);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_IS_USE_ANIMATION_BOUNDS, self.label = "Use Animation Bounds",
           self.tooltip = "Instead of using the animation preview's bounds, the rendered "
                          "animation will use the animation's bounds.\nNOTE: If you're looking "
                          "to make a transparent animation, set the preview background to be "
                          "transparent\nand toggle off other drawn things.",
           self.value = SETTINGS_RENDER_IS_USE_ANIMATION_BOUNDS_DEFAULT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_SCALE, self.label = "Scale", self.tooltip = "Change the scale the animation will be rendered at.",
           self.value = SETTINGS_RENDER_SCALE_DEFAULT, self.size = {125, 0});

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_CONFIRM, self.label = "Render", self.tooltip = "Render the animation, with the used settings.",
           self.popup = "Rendering Animation...", self.popupType = IMGUI_POPUP_CENTER_WINDOW, self.popupSize = {300, 60}, self.isSameLine = true,
           self.rowCount = IMGUI_CONFIRM_POPUP_ROW_COUNT);

IMGUI_ITEM(IMGUI_RENDERING_ANIMATION_CHILD, self.label = "##Rendering Child", self.size = {400.0f, 65.0f}, self.flags = true);

IMGUI_ITEM(IMGUI_RENDERING_ANIMATION_INFO, self.label = "Recording frames. Once done, the program may halt\nas "
                                                        "FFmpeg renders the animation. Please be patient!");
IMGUI_ITEM(IMGUI_RENDERING_ANIMATION_CANCEL, self.label = "Cancel", self.tooltip = "Cancel rendering the animation.", self.rowCount = 1);

IMGUI_ITEM(IMGUI_PLAYBACK, self.label = "&Playback", self.tooltip = "Opens the playback menu, for configuring playback settings.",
           self.popup = "## Playback Popup", self.popupType = IMGUI_POPUP_BY_ITEM, self.isWidthToText = true, self.isHeightToRegion = true,
           self.isSameLine = true);

IMGUI_ITEM(IMGUI_ALWAYS_LOOP, self.label = "&Always Loop",
           self.tooltip = "Sets the animation playback to always loop, "
                          "regardless of the animation's loop setting.",
           self.isWidthToText = true);

IMGUI_ITEM(IMGUI_CLAMP_PLAYHEAD, self.label = "&Clamp Playhead",
           self.tooltip = "The playhead (draggable icon on timeline) won't be "
                          "able to exceed the animation length.",
           self.isWidthToText = true);

IMGUI_ITEM(IMGUI_SETTINGS, self.label = "&Settings", self.tooltip = "Opens the setting menu, for configuring general program settings.",
           self.popup = "## Settings Popup", self.popupType = IMGUI_POPUP_BY_ITEM, self.isWidthToText = true, self.isHeightToRegion = true);

IMGUI_ITEM(IMGUI_VSYNC, self.label = "&Vsync",
           self.tooltip = "Toggle vertical sync; synchronizes program "
                          "framerate with your monitor's refresh rate.",
           self.isWidthToText = true, self.isSeparator = true);

IMGUI_ITEM(IMGUI_HOTKEYS, self.label = "&Hotkeys", self.tooltip = "Change the program's hotkeys.", self.popup = "Hotkeys", self.popupSize = {500, 405},
           self.isWidthToText = true, self.isSeparator = true);

IMGUI_ITEM(IMGUI_HOTKEYS_CHILD, self.label = "## Hotkeys Child", self.size = {IMGUI_HOTKEYS.popupSize.x, IMGUI_HOTKEYS.popupSize.y - 35}, self.flags = true);

#define IMGUI_HOTKEYS_FUNCTION "Function"
#define IMGUI_HOTKEYS_HOTKEY "Hotkey"
IMGUI_ITEM(IMGUI_HOTKEYS_TABLE, self.label = "## Hotkeys Table", self.value = 2, self.flags = ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY);

IMGUI_ITEM(IMGUI_HOTKEYS_OPTIONS_CHILD, self.label = "## Merge Options Child", self.size = {IMGUI_HOTKEYS.popupSize.x, 35}, self.flags = true);

IMGUI_ITEM(IMGUI_HOTKEYS_CONFIRM, self.label = "Confirm", self.tooltip = "Use these hotkeys.", self.rowCount = 1);

IMGUI_ITEM(IMGUI_DEFAULT_SETTINGS, self.label = "&Reset to Default Settings", self.tooltip = "Reset the program's settings to their default state.",
           self.isWidthToText = true);

IMGUI_ITEM(IMGUI_ANM2S, self.label = "## Anm2s", self.size = {0, 32},
           self.flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings);

IMGUI_ITEM(IMGUI_ANM2, self.label = "## Anm2");

IMGUI_ITEM(IMGUI_LAYERS, self.label = "Layers", self.flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
IMGUI_ITEM(IMGUI_LAYERS_CHILD, self.label = "## Layers Child", self.flags = true);

IMGUI_ITEM(IMGUI_LAYER, self.label = "## Layer Item", self.dragDrop = "## Layer Drag Drop", self.atlas = ATLAS_LAYER, self.isWidthToRegion = true,
           self.idOffset = 3000);

IMGUI_ITEM(IMGUI_LAYER_SPRITESHEET_ID, self.label = "## Spritesheet ID", self.tooltip = "Change the spritesheet ID this layer uses.",
           self.atlas = ATLAS_SPRITESHEET, self.size = {50, 0});

#define IMGUI_LAYERS_OPTIONS_ROW_COUNT 2
IMGUI_ITEM(IMGUI_LAYER_ADD, self.label = "Add", self.tooltip = "Adds a new layer.", self.snapshotAction = "Add Layer",
           self.rowCount = IMGUI_LAYERS_OPTIONS_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_LAYER_REMOVE, self.label = "Remove",
           self.tooltip = "Removes the selected layer.\nThis will remove all layer "
                          "animations that use this layer from all animations.",
           self.snapshotAction = "Remove Layer", self.chord = ImGuiKey_Delete, self.focusWindow = IMGUI_LAYERS.label,
           self.rowCount = IMGUI_LAYERS_OPTIONS_ROW_COUNT);

IMGUI_ITEM(IMGUI_NULLS, self.label = "Nulls", self.flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
IMGUI_ITEM(IMGUI_NULLS_CHILD, self.label = "## Nulls Child", self.flags = true);

IMGUI_ITEM(IMGUI_NULL, self.label = "## Null Item", self.dragDrop = "## Null Drag Drop", self.atlas = ATLAS_NULL, self.idOffset = 4000);

#define IMGUI_NULLS_OPTIONS_ROW_COUNT 2
IMGUI_ITEM(IMGUI_NULL_ADD, self.label = "Add", self.tooltip = "Adds a null layer.", self.snapshotAction = "Add Null",
           self.rowCount = IMGUI_NULLS_OPTIONS_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_NULL_REMOVE, self.label = "Remove",
           self.tooltip = "Removes the selected null.\nThis will remove all null "
                          "animations that use this null from all animations.",
           self.snapshotAction = "Remove Null", self.chord = ImGuiKey_Delete, self.focusWindow = IMGUI_NULLS.label,
           self.rowCount = IMGUI_NULLS_OPTIONS_ROW_COUNT);

IMGUI_ITEM(IMGUI_ANIMATIONS, self.label = "Animations", self.flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
IMGUI_ITEM(IMGUI_ANIMATIONS_CHILD, self.label = "## Animations Child", self.flags = true);

IMGUI_ITEM(IMGUI_ANIMATION, self.label = "## Animation Item", self.dragDrop = "## Animation Drag Drop", self.atlas = ATLAS_ANIMATION,
           self.isWidthToRegion = true, self.idOffset = 2000);

#define IMGUI_ANIMATIONS_OPTIONS_ROW_COUNT 5
IMGUI_ITEM(IMGUI_ANIMATION_ADD, self.label = "Add", self.tooltip = "Adds a new animation.", self.snapshotAction = "Add Animation",
           self.rowCount = IMGUI_ANIMATIONS_OPTIONS_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_ANIMATION_DUPLICATE, self.label = "Duplicate", self.tooltip = "Duplicates the selected animation, placing it after.",
           self.snapshotAction = "Duplicate Animation", self.rowCount = IMGUI_ANIMATIONS_OPTIONS_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_ANIMATION_MERGE, self.label = "Merge", self.tooltip = "Open the animation merge popup, to merge animations together.",
           self.popup = "Merge Animations", self.popupSize = {300, 400}, self.rowCount = IMGUI_ANIMATIONS_OPTIONS_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_MERGE_ANIMATIONS_CHILD, self.label = "## Merge Animations", self.size = {IMGUI_ANIMATION_MERGE.popupSize.x, 250}, self.flags = true);

IMGUI_ITEM(IMGUI_MERGE_ON_CONFLICT_CHILD, self.label = "## Merge On Conflict Child", self.size = {IMGUI_ANIMATION_MERGE.popupSize.x, 75}, self.flags = true);

IMGUI_ITEM(IMGUI_MERGE_ON_CONFLICT, self.label = "On Conflict");

IMGUI_ITEM(IMGUI_MERGE_APPEND_FRAMES, self.label = "Append Frames ",
           self.tooltip = "On frame conflict, the merged animation will have "
                          "the selected animations' frames appended.",
           self.value = ANM2_MERGE_APPEND, self.isSameLine = true);

IMGUI_ITEM(IMGUI_MERGE_REPLACE_FRAMES, self.label = "Replace Frames",
           self.tooltip = "On frame conflict, the merged animation will have "
                          "the latest selected animations' frames.",
           self.value = ANM2_MERGE_REPLACE);

IMGUI_ITEM(IMGUI_MERGE_PREPEND_FRAMES, self.label = "Prepend Frames",
           self.tooltip = "On frame conflict, the merged animation will have "
                          "the selected animations' frames prepended.",
           self.value = ANM2_MERGE_PREPEND, self.isSameLine = true);

IMGUI_ITEM(IMGUI_MERGE_IGNORE, self.label = "Ignore        ",
           self.tooltip = "On frame conflict, the merged animation will ignore "
                          "the other selected animations' frames.",
           self.value = ANM2_MERGE_IGNORE);

IMGUI_ITEM(IMGUI_MERGE_OPTIONS_CHILD, self.label = "## Merge Options Child", self.size = {IMGUI_ANIMATION_MERGE.popupSize.x, 35}, self.flags = true);

IMGUI_ITEM(IMGUI_MERGE_DELETE_ANIMATIONS_AFTER, self.label = "Delete Animations After Merging",
           self.tooltip = "After merging, the selected animations (besides the "
                          "original) will be deleted.");

IMGUI_ITEM(IMGUI_MERGE_CONFIRM, self.label = "Merge", self.tooltip = "Merge the selected animations with the options set.",
           self.snapshotAction = "Merge Animations", self.rowCount = IMGUI_CONFIRM_POPUP_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_ANIMATION_REMOVE, self.label = "Remove", self.tooltip = "Remove the selected animation(s).", self.snapshotAction = "Remove Animation(s)",
           self.rowCount = IMGUI_ANIMATIONS_OPTIONS_ROW_COUNT, self.chord = ImGuiKey_Delete, self.focusWindow = IMGUI_ANIMATIONS.label, self.isSameLine = true);

IMGUI_ITEM(IMGUI_ANIMATION_DEFAULT, self.label = "Default", self.tooltip = "Set the referenced animation as the default one.",
           self.snapshotAction = "Default Animation", self.rowCount = IMGUI_ANIMATIONS_OPTIONS_ROW_COUNT, self.isSameLine = true);

static inline void imgui_animations_select_all(Imgui* self) {
  for (int i = 0; i < (int)self->anm2->animations.size(); i++)
    self->selectedAnimationIndices.insert(i);
}

IMGUI_ITEM(IMGUI_ANIMATION_SELECT_ALL, self.label = "## Select None", self.hotkey = HOTKEY_SELECT_ALL, self.focusWindow = IMGUI_ANIMATIONS.label,
           self.function = imgui_animations_select_all, self.isAllowHotkeyWhenFocusWindow = true);

static inline void imgui_animations_select_none(Imgui* self) {
  self->selectedAnimationIndices.clear();
  *self->reference = Anm2Reference();
}

IMGUI_ITEM(IMGUI_ANIMATION_SELECT_NONE, self.label = "## Select None", self.hotkey = HOTKEY_SELECT_NONE, self.focusWindow = IMGUI_ANIMATIONS.label,
           self.function = imgui_animations_select_none, self.isAllowHotkeyWhenFocusWindow = true);

IMGUI_ITEM(IMGUI_EVENTS, self.label = "Events", self.flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

IMGUI_ITEM(IMGUI_EVENTS_CHILD, self.label = "## Events Child", self.flags = true);

IMGUI_ITEM(IMGUI_EVENT, self.label = "## Event", self.atlas = ATLAS_EVENT, self.idOffset = 1000);

#define IMGUI_EVENTS_OPTIONS_ROW_COUNT 2
IMGUI_ITEM(IMGUI_EVENTS_ADD, self.label = "Add", self.tooltip = "Adds a new event.", self.snapshotAction = "Add Event",
           self.rowCount = IMGUI_EVENTS_OPTIONS_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_EVENTS_REMOVE_UNUSED, self.label = "Remove Unused",
           self.tooltip = "Removes all unused events (i.e., not being used in "
                          "any triggers in any animation).",
           self.snapshotAction = "Remove Unused Events", self.rowCount = IMGUI_EVENTS_OPTIONS_ROW_COUNT);

// Spritesheets
IMGUI_ITEM(IMGUI_SPRITESHEET_PREVIEW, self.label = "## Spritesheet Preview", self.size = {65, 65});
IMGUI_ITEM(IMGUI_SPRITESHEETS, self.label = "Spritesheets", self.flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
IMGUI_ITEM(IMGUI_SPRITESHEETS_CHILD, self.label = "## Spritesheets Child", self.flags = true, self.windowPadding = vec2());
IMGUI_ITEM(IMGUI_SPRITESHEET_CHILD, self.label = "## Spritesheet Child", self.flags = true, self.itemSpacing = vec2(), self.windowPadding = vec2());
IMGUI_ITEM(IMGUI_SPRITESHEET, self.label = "## Spritesheet", self.dragDrop = "## Spritesheet Drag Drop");
IMGUI_ITEM(IMGUI_SPRITESHEET_TEXT, self.label = "## Spritesheet Text", self.atlas = ATLAS_SPRITESHEET, self.itemSpacing = vec2(8, 0));

IMGUI_ITEM(IMGUI_SPRITESHEETS_FOOTER_CHILD, self.label = "## Spritesheets Footer Child", self.flags = true);

#define IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_COUNT 4
#define IMGUI_SPRITESHEETS_OPTIONS_SECOND_ROW_COUNT 3
IMGUI_ITEM(IMGUI_SPRITESHEET_ADD, self.label = "Add", self.tooltip = "Select a .png image to add as a spritesheet.",
           self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_SPRITESHEETS_RELOAD, self.label = "Reload", self.tooltip = "Reload the selected spritesheet(s).",
           self.snapshotAction = "Reload Spritesheet(s)", self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_SPRITESHEETS_REPLACE, self.label = "Replace", self.tooltip = "Replace the highlighted spritesheet with another.",
           self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_SPRITESHEETS_REMOVE_UNUSED, self.label = "Remove Unused",
           self.tooltip = "Remove all unused spritesheets in the anm2 (i.e., "
                          "the spritesheet isn't used in any layer animations).",
           self.snapshotAction = "Remove Unused Spritesheets", self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_COUNT);

IMGUI_ITEM(IMGUI_SPRITESHEETS_SELECT_ALL, self.label = "Select All", self.tooltip = "Select all spritesheets.", self.hotkey = HOTKEY_SELECT_ALL,
           self.focusWindow = IMGUI_SPRITESHEETS.label, self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_SECOND_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_SPRITESHEETS_SELECT_NONE, self.label = "Select None", self.tooltip = "Unselect all spritesheets.", self.hotkey = HOTKEY_SELECT_NONE,
           self.focusWindow = IMGUI_SPRITESHEETS.label, self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_SECOND_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_SPRITESHEET_SAVE, self.label = "Save", self.tooltip = "Save the selected spritesheets to their original locations.",
           self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_SECOND_ROW_COUNT);

const ImVec2 IMGUI_CANVAS_CHILD_SIZE = {230, 85};
IMGUI_ITEM(IMGUI_CANVAS_GRID_CHILD, self.label = "## Canvas Grid Child", self.size = IMGUI_CANVAS_CHILD_SIZE, self.flags = true,
           self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

IMGUI_ITEM(IMGUI_CANVAS_GRID, self.label = "Grid", self.tooltip = "Toggles the visiblity of the canvas' grid.");

IMGUI_ITEM(IMGUI_CANVAS_GRID_SNAP, self.label = "Snap", self.tooltip = "Using the crop tool will snap the points to the nearest grid point.");

IMGUI_ITEM(IMGUI_CANVAS_GRID_COLOR, self.label = "Color", self.tooltip = "Change the color of the canvas' grid.", self.flags = ImGuiColorEditFlags_NoInputs);

IMGUI_ITEM(IMGUI_CANVAS_GRID_SIZE, self.label = "Size", self.tooltip = "Change the size of the canvas' grid.", self.min = CANVAS_GRID_MIN,
           self.max = CANVAS_GRID_MAX, self.value = CANVAS_GRID_DEFAULT);

IMGUI_ITEM(IMGUI_CANVAS_GRID_OFFSET, self.label = "Offset", self.tooltip = "Change the offset of the canvas' grid, in pixels.");

IMGUI_ITEM(IMGUI_CANVAS_VIEW_CHILD, self.label = "## View Child", self.size = IMGUI_CANVAS_CHILD_SIZE, self.flags = true,
           self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

IMGUI_ITEM(IMGUI_CANVAS_ZOOM, self.label = "Zoom", self.tooltip = "Change the zoom of the canvas.", self.min = CANVAS_ZOOM_MIN, self.max = CANVAS_ZOOM_MAX,
           self.speed = 1.0f, self.value = CANVAS_ZOOM_DEFAULT);

IMGUI_ITEM(IMGUI_CANVAS_VISUAL_CHILD, self.label = "## Animation Preview Visual Child", self.size = IMGUI_CANVAS_CHILD_SIZE, self.flags = true,
           self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

IMGUI_ITEM(IMGUI_CANVAS_BACKGROUND_COLOR, self.label = "Background Color", self.tooltip = "Change the background color of the canvas.",
           self.flags = ImGuiColorEditFlags_NoInputs);

IMGUI_ITEM(IMGUI_CANVAS_ANIMATION_OVERLAY, self.label = "Overlay",
           self.tooltip = "Choose an animation to overlay over the previewed "
                          "animation, for reference.");

IMGUI_ITEM(IMGUI_CANVAS_ANIMATION_OVERLAY_TRANSPARENCY, self.label = "Alpha", self.tooltip = "Set the transparency of the animation overlay.",
           self.value = SETTINGS_PREVIEW_OVERLAY_TRANSPARENCY_DEFAULT, self.max = UCHAR_MAX);

IMGUI_ITEM(IMGUI_CANVAS_HELPER_CHILD, self.label = "## Animation Preview Helper Child", self.size = IMGUI_CANVAS_CHILD_SIZE, self.flags = true,
           self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

IMGUI_ITEM(IMGUI_CANVAS_AXES, self.label = "Axes", self.tooltip = "Toggle the display of the X/Y axes.");

IMGUI_ITEM(IMGUI_CANVAS_AXES_COLOR, self.label = "Color", self.tooltip = "Change the color of the axes.", self.flags = ImGuiColorEditFlags_NoInputs);

IMGUI_ITEM(IMGUI_CANVAS_ROOT_TRANSFORM, self.label = "Root Transform",
           self.tooltip = "Toggles the root frames's attributes transforming "
                          "the other items in an animation.",
           self.value = SETTINGS_PREVIEW_IS_ROOT_TRANSFORM_DEFAULT);

IMGUI_ITEM(IMGUI_CANVAS_TRIGGERS, self.label = "Triggers", self.tooltip = "Toggles activated triggers drawing their event name.",
           self.value = SETTINGS_PREVIEW_IS_TRIGGERS_DEFAULT);

IMGUI_ITEM(IMGUI_CANVAS_PIVOTS, self.label = "Pivots", self.tooltip = "Toggles drawing each layer's pivot.", self.value = SETTINGS_PREVIEW_IS_PIVOTS_DEFAULT);

IMGUI_ITEM(IMGUI_CANVAS_ICONS, self.label = "Icons", self.tooltip = "Toggles drawing the the colored root/null icons.",
           self.value = SETTINGS_PREVIEW_IS_ICONS_DEFAULT);

IMGUI_ITEM(IMGUI_CANVAS_ALT_ICONS, self.label = "Alt Icons",
           self.tooltip = "Toggles the use of alternate icons for the targets "
                          "(the colored root/null icons).",
           self.value = SETTINGS_PREVIEW_IS_ALT_ICONS_DEFAULT);

IMGUI_ITEM(IMGUI_CANVAS_BORDER, self.label = "Border", self.tooltip = "Toggles the appearance of a border around the items.",
           self.value = SETTINGS_PREVIEW_IS_BORDER_DEFAULT);

IMGUI_ITEM(IMGUI_ANIMATION_PREVIEW, self.label = "Animation Preview", self.flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

#define IMGUI_ANIMATION_PREVIEW_VIEW_ROW_COUNT 2
IMGUI_ITEM(IMGUI_ANIMATION_PREVIEW_CENTER_VIEW, self.label = "Center View", self.tooltip = "Centers the current view on the animation preview.",
           self.hotkey = HOTKEY_CENTER_VIEW, self.focusWindow = IMGUI_ANIMATION_PREVIEW.label, self.rowCount = IMGUI_ANIMATION_PREVIEW_VIEW_ROW_COUNT,
           self.isSameLine = true);

IMGUI_ITEM(IMGUI_ANIMATION_PREVIEW_FIT, self.label = "Fit",
           self.tooltip = "Adjust the view/pan based on the size of the "
                          "animation, to fit the canvas' size.",
           self.hotkey = HOTKEY_FIT, self.focusWindow = IMGUI_ANIMATION_PREVIEW.label, self.rowCount = IMGUI_ANIMATION_PREVIEW_VIEW_ROW_COUNT);

IMGUI_ITEM(IMGUI_SPRITESHEET_EDITOR, self.label = "Spritesheet Editor", self.flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

#define IMGUI_SPRITESHEET_EDITOR_VIEW_ROW_COUNT 2
IMGUI_ITEM(IMGUI_SPRITESHEET_EDITOR_CENTER_VIEW, self.label = "Center View", self.tooltip = "Centers the current view on the spritesheet editor.",
           self.hotkey = HOTKEY_CENTER_VIEW, self.focusWindow = IMGUI_SPRITESHEET_EDITOR.label, self.rowCount = IMGUI_SPRITESHEET_EDITOR_VIEW_ROW_COUNT,
           self.isSameLine = true);

IMGUI_ITEM(IMGUI_SPRITESHEET_EDITOR_FIT, self.label = "Fit",
           self.tooltip = "Adjust the view/pan based on the size of the "
                          "spritesheet, to fit the canvas' size.",
           self.hotkey = HOTKEY_FIT, self.focusWindow = IMGUI_SPRITESHEET_EDITOR.label, self.rowCount = IMGUI_SPRITESHEET_EDITOR_VIEW_ROW_COUNT);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES, self.label = "Frame Properties");

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_POSITION, self.label = "Position", self.tooltip = "Change the position of the selected frame.",
           self.snapshotAction = "Frame Position", self.isUseItemActivated = true);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_CROP, self.label = "Crop", self.tooltip = "Change the crop position of the selected frame.",
           self.snapshotAction = "Frame Crop", self.isUseItemActivated = true);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_SIZE, self.label = "Size", self.tooltip = "Change the size of the crop of the selected frame.",
           self.snapshotAction = "Frame Size", self.isUseItemActivated = true);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_PIVOT, self.label = "Pivot", self.tooltip = "Change the pivot of the selected frame.", self.snapshotAction = "Frame Pivot",
           self.isUseItemActivated = true);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_SCALE, self.label = "Scale", self.tooltip = "Change the scale of the selected frame.", self.snapshotAction = "Frame Scale",
           self.isUseItemActivated = true);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_ROTATION, self.label = "Rotation", self.tooltip = "Change the rotation of the selected frame.",
           self.snapshotAction = "Frame Rotation", self.isUseItemActivated = true);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_DELAY, self.label = "Duration", self.tooltip = "Change the duration of the selected frame.",
           self.snapshotAction = "Frame Duration", self.isUseItemActivated = true, self.min = ANM2_FRAME_NUM_MIN, self.max = ANM2_FRAME_NUM_MAX,
           self.value = ANM2_FRAME_NUM_MIN);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_TINT, self.label = "Tint", self.tooltip = "Change the tint of the selected frame.", self.snapshotAction = "Frame Tint",
           self.isUseItemActivated = true, self.value = 1);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_COLOR_OFFSET, self.label = "Color Offset", self.tooltip = "Change the color offset of the selected frame.",
           self.snapshotAction = "Frame Color Offset", self.isUseItemActivated = true, self.value = 0);

#define IMGUI_FRAME_PROPERTIES_FLIP_ROW_COUNT 2
IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_FLIP_X, self.label = "Flip X",
           self.tooltip = "Change the sign of the X scale, to cheat flipping the layer "
                          "horizontally.\n(Anm2 doesn't support flipping directly.)",
           self.snapshotAction = "Frame Flip X", self.rowCount = IMGUI_FRAME_PROPERTIES_FLIP_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_FLIP_Y, self.label = "Flip Y",
           self.tooltip = "Change the sign of the Y scale, to cheat flipping the layer "
                          "vertically.\n(Anm2 doesn't support flipping directly.)",
           self.snapshotAction = "Frame Flip Y", self.rowCount = IMGUI_FRAME_PROPERTIES_FLIP_ROW_COUNT);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_VISIBLE, self.label = "Visible", self.tooltip = "Toggles the visibility of the selected frame.",
           self.snapshotAction = "Frame Visibility", self.isSameLine = true, self.value = true);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_INTERPOLATED, self.label = "Interpolation", self.tooltip = "Toggles the interpolation of the selected frame.",
           self.snapshotAction = "Frame Interpolation", self.isSameLine = true, self.value = true);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_ROUND, self.label = "Round", self.tooltip = "Values will be rounded to the nearest integer.",
           self.value = SETTINGS_PROPERTIES_IS_ROUND_DEFAULT);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_EVENT, self.label = "Event", self.tooltip = "Change the event the trigger uses.", self.snapshotAction = "Trigger Event");

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_AT_FRAME, self.label = "At Frame", self.tooltip = "Change the frame where the trigger occurs.",
           self.snapshotAction = "Trigger At Frame");

IMGUI_ITEM(IMGUI_TOOLS, self.label = "Tools");

IMGUI_ITEM(IMGUI_TOOL_PAN, self.label = "## Pan",
           self.tooltip = "Use the pan tool.\nWill shift the view as the cursor is dragged.\nYou "
                          "can also use the middle mouse button to pan at any time.",
           self.function = imgui_tool_pan_set, self.hotkey = HOTKEY_PAN, self.atlas = ATLAS_PAN, self.isAtlasStretch = true);

IMGUI_ITEM(IMGUI_TOOL_MOVE, self.label = "## Move",
           self.tooltip = "Use the move tool.\nAnimation Preview: Will move the position "
                          "of the frame."
                          "\nSpritesheet Editor: Will move the pivot, and holding right "
                          "click will use the Crop functionality instead."
                          "\nUse mouse or directional keys to change the value.",
           self.function = imgui_tool_move_set, self.hotkey = HOTKEY_MOVE, self.atlas = ATLAS_MOVE);

IMGUI_ITEM(IMGUI_TOOL_ROTATE, self.label = "## Rotate",
           self.tooltip = "Use the rotate tool.\nWill rotate the selected item as the cursor is "
                          "dragged, or directional keys are pressed.\n(Animation Preview only.)",
           self.function = imgui_tool_rotate_set, self.hotkey = HOTKEY_ROTATE, self.atlas = ATLAS_ROTATE);

IMGUI_ITEM(IMGUI_TOOL_SCALE, self.label = "## Scale",
           self.tooltip = "Use the scale tool.\nWill scale the selected item as the cursor is "
                          "dragged, or directional keys are pressed.\n(Animation Preview only.)",
           self.function = imgui_tool_scale_set, self.hotkey = HOTKEY_SCALE, self.atlas = ATLAS_SCALE);

IMGUI_ITEM(IMGUI_TOOL_CROP, self.label = "## Crop",
           self.tooltip = "Use the crop tool.\nWill produce a crop rectangle based on how "
                          "the cursor is dragged."
                          "\nAlternatively, you can use the arrow keys and Ctrl/Shift to "
                          "move the size/position, respectively."
                          "\nHolding right click will use the Move tool's functionality."
                          "\n(Spritesheet Editor only.)",
           self.function = imgui_tool_crop_set, self.hotkey = HOTKEY_CROP, self.atlas = ATLAS_CROP);

IMGUI_ITEM(IMGUI_TOOL_DRAW, self.label = "## Draw",
           self.tooltip = "Draws pixels onto the selected spritesheet, with "
                          "the current color.\n(Spritesheet Editor only.)",
           self.function = imgui_tool_draw_set, self.hotkey = HOTKEY_DRAW, self.atlas = ATLAS_DRAW);

IMGUI_ITEM(IMGUI_TOOL_ERASE, self.label = "## Erase",
           self.tooltip = "Erases pixels from the selected "
                          "spritesheet.\n(Spritesheet Editor only.)",
           self.function = imgui_tool_erase_set, self.hotkey = HOTKEY_ERASE, self.atlas = ATLAS_ERASE);

IMGUI_ITEM(IMGUI_TOOL_COLOR_PICKER, self.label = "## Color Picker",
           self.tooltip = "Selects a color from the canvas, to be used for "
                          "drawing.\n(Spritesheet Editor only).",
           self.function = imgui_tool_color_picker_set, self.hotkey = HOTKEY_COLOR_PICKER, self.atlas = ATLAS_COLOR_PICKER);

IMGUI_ITEM(IMGUI_TOOL_UNDO, self.label = "## Undo", self.tooltip = "Undo the last action.", self.function = imgui_undo, self.hotkey = HOTKEY_UNDO,
           self.atlas = ATLAS_UNDO);

IMGUI_ITEM(IMGUI_TOOL_REDO, self.label = "## Redo", self.tooltip = "Redo the last action.", self.function = imgui_redo, self.hotkey = HOTKEY_REDO,
           self.atlas = ATLAS_REDO);

IMGUI_ITEM(IMGUI_TOOL_COLOR, self.label = "## Color", self.tooltip = "Set the color, to be used by the draw tool.", self.flags = ImGuiColorEditFlags_NoInputs);

const inline ImguiItem* IMGUI_TOOL_ITEMS[TOOL_COUNT] = {&IMGUI_TOOL_PAN,  &IMGUI_TOOL_MOVE, &IMGUI_TOOL_ROTATE, &IMGUI_TOOL_SCALE,
                                                        &IMGUI_TOOL_CROP, &IMGUI_TOOL_DRAW, &IMGUI_TOOL_ERASE,  &IMGUI_TOOL_COLOR_PICKER,
                                                        &IMGUI_TOOL_UNDO, &IMGUI_TOOL_REDO, &IMGUI_TOOL_COLOR};

IMGUI_ITEM(IMGUI_COLOR_PICKER_BUTTON, self.label = "## Color Picker Button");

IMGUI_ITEM(IMGUI_TIMELINE, self.label = "Timeline", self.flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

IMGUI_ITEM(IMGUI_TIMELINE_CHILD, self.label = "## Timeline Child", self.flags = true);

IMGUI_ITEM(IMGUI_TIMELINE_HEADER_CHILD, self.label = "## Timeline Header Child", self.size = {0, IMGUI_TIMELINE_FRAME_SIZE.y},
           self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

IMGUI_ITEM(IMGUI_PLAYHEAD, self.label = "## Playhead",
           self.flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
                        ImGuiWindowFlags_NoInputs);

IMGUI_ITEM(IMGUI_TIMELINE_ITEMS_CHILD, self.label = "## Timeline Items", self.size = {IMGUI_TIMELINE_ITEM_SIZE.x, 0},
           self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_CHILD, self.label = "## Timeline Item Child", self.size = IMGUI_TIMELINE_ITEM_SIZE, self.flags = true,
           self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_ROOT_CHILD, self.label = "## Root Item Child", self.color.normal = {0.045f, 0.08f, 0.11f, 1.0f},
           self.size = IMGUI_TIMELINE_ITEM_SIZE, self.flags = true, self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_LAYER_CHILD, self.label = "## Layer Item Child", self.color.normal = {0.0875f, 0.05f, 0.015f, 1.0f},
           self.size = IMGUI_TIMELINE_ITEM_SIZE, self.flags = true, self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_NULL_CHILD, self.label = "## Null Item Child", self.color.normal = {0.055f, 0.10f, 0.055f, 1.0f},
           self.size = IMGUI_TIMELINE_ITEM_SIZE, self.flags = true, self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_TRIGGERS_CHILD, self.label = "## Triggers Item Child", self.color.normal = {0.10f, 0.0375f, 0.07f, 1.0f},
           self.size = IMGUI_TIMELINE_ITEM_SIZE, self.flags = true, self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

const inline ImguiItem* IMGUI_TIMELINE_ITEM_CHILDS[ANM2_COUNT]{&IMGUI_TIMELINE_ITEM_CHILD, &IMGUI_TIMELINE_ITEM_ROOT_CHILD, &IMGUI_TIMELINE_ITEM_LAYER_CHILD,
                                                               &IMGUI_TIMELINE_ITEM_NULL_CHILD, &IMGUI_TIMELINE_ITEM_TRIGGERS_CHILD};

#define IMGUI_POPUP_ITEM_PROPERTIES "Item Properties"
#define IMGUI_POPUP_ITEM_PROPERTIES_TYPE IMGUI_POPUP_CENTER_WINDOW
const ImVec2 IMGUI_POPUP_ITEM_PROPERTIES_SIZE = {300, 350};

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_PROPERTIES_TYPE_CHILD, self.label = "## Item Properties Type Child", self.size = {IMGUI_POPUP_ITEM_PROPERTIES_SIZE.x, 35},
           self.flags = true);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_PROPERTIES_LAYER, self.label = "Layer",
           self.tooltip = "The item will be a layer item.\nA layer item is a "
                          "primary graphical item, using a spritesheet.",
           self.isWidthToText = true, self.value = ANM2_LAYER, self.isSameLine = true);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_PROPERTIES_NULL, self.label = "Null",
           self.tooltip = "The item will be a null item.\nA null item is an "
                          "invisible item, often accessed by a game engine.",
           self.isWidthToText = true, self.value = ANM2_NULL);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_PROPERTIES_ITEMS_CHILD, self.label = "## Item Properties Items", self.size = {IMGUI_POPUP_ITEM_PROPERTIES_SIZE.x, 250},
           self.flags = true);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_PROPERTIES_OPTIONS_CHILD, self.label = "## Item Properties Options Child", self.size = {IMGUI_POPUP_ITEM_PROPERTIES_SIZE.x, 35},
           self.flags = true);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_PROPERTIES_CONFIRM, self.label = "Confirm", self.tooltip = "Set the timeline item's properties.",
           self.snapshotAction = "Timeline Item Change", self.rowCount = IMGUI_CONFIRM_POPUP_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_SELECTABLE, self.label = "## Selectable", self.size = IMGUI_TIMELINE_ITEM_SIZE);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_ROOT_SELECTABLE, self.label = "Root",
           self.tooltip = "The root item of an animation.\nChanging its properties "
                          "will transform the rest of the animation.",
           self.size = IMGUI_TIMELINE_ITEM_SIZE);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_LAYER_SELECTABLE, self.label = "## Layer Selectable", self.tooltip = "A layer item.\nA graphical item within the animation.",
           self.dragDrop = "## Layer Drag Drop", self.color.active = {0.45f, 0.18f, 0.07f, 1.0f}, self.popup = IMGUI_POPUP_ITEM_PROPERTIES,
           self.popupType = IMGUI_POPUP_ITEM_PROPERTIES_TYPE, self.size = IMGUI_TIMELINE_ITEM_SIZE);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_NULL_SELECTABLE, self.label = "## Null Selectable",
           self.tooltip = "A null item.\nAn invisible item within the "
                          "animation that is accessible via a game engine.",
           self.dragDrop = "## Null Drag Drop", self.popup = IMGUI_POPUP_ITEM_PROPERTIES, self.popupType = IMGUI_POPUP_ITEM_PROPERTIES_TYPE,
           self.size = IMGUI_TIMELINE_ITEM_SIZE);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_TRIGGERS_SELECTABLE, self.label = "Triggers", self.tooltip = "The animation's triggers.\nWill fire based on an event.",
           self.size = IMGUI_TIMELINE_ITEM_SIZE);

const inline ImguiItem* IMGUI_TIMELINE_ITEM_SELECTABLES[ANM2_COUNT]{&IMGUI_TIMELINE_ITEM_SELECTABLE, &IMGUI_TIMELINE_ITEM_ROOT_SELECTABLE,
                                                                    &IMGUI_TIMELINE_ITEM_LAYER_SELECTABLE, &IMGUI_TIMELINE_ITEM_NULL_SELECTABLE,
                                                                    &IMGUI_TIMELINE_ITEM_TRIGGERS_SELECTABLE};

IMGUI_ITEM(IMGUI_TIMELINE_SHOW_UNUSED, self.label = "## Show Unused", self.tooltip = "Layers/nulls without any frames will be hidden.",
           self.snapshotAction = "Hide Unused", self.atlas = ATLAS_SHOW_UNUSED);

IMGUI_ITEM(IMGUI_TIMELINE_HIDE_UNUSED, self.label = "## Hide Unused", self.tooltip = "Layers/nulls without any frames will be shown.",
           self.snapshotAction = "Show Unused", self.atlas = ATLAS_HIDE_UNUSED);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_VISIBLE, self.label = "## Visible", self.tooltip = "The item is visible.\nPress to set to invisible.",
           self.snapshotAction = "Item Invisible", self.atlas = ATLAS_VISIBLE);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_INVISIBLE, self.label = "## Invisible", self.tooltip = "The item is invisible.\nPress to set to visible.",
           self.snapshotAction = "Item Visible", self.atlas = ATLAS_INVISIBLE);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_SHOW_RECT, self.label = "## Show Rect", self.tooltip = "The rect is shown.\nPress to hide rect.",
           self.snapshotAction = "Hide Rect", self.atlas = ATLAS_SHOW_RECT);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_HIDE_RECT, self.label = "## Hide Rect", self.tooltip = "The rect is hidden.\nPress to show rect.",
           self.snapshotAction = "Show Rect", self.atlas = ATLAS_HIDE_RECT);

IMGUI_ITEM(IMGUI_TIMELINE_FRAMES_CHILD, self.label = "## Timeline Frames Child",
           self.windowFlags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_HorizontalScrollbar);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_FRAMES_CHILD, self.label = "## Timeline Item Frames Child", self.size = {0, IMGUI_TIMELINE_FRAME_SIZE.y});

IMGUI_ITEM(IMGUI_TIMELINE_FRAME, self.label = "## Frame");

#define IMGUI_TIMELINE_FRAME_BORDER 5
static const vec4 IMGUI_FRAME_BORDER_COLOR = {1.0f, 1.0f, 1.0f, 0.25f};
IMGUI_ITEM(IMGUI_TIMELINE_ROOT_FRAME, self.label = "## Root Frame", self.snapshotAction = "Root Frame",
           self.color = {vec4(0.14f, 0.27f, 0.39f, 1.0f), vec4(0.28f, 0.54f, 0.78f, 1.0f), vec4(0.36f, 0.70f, 0.95f, 1.0f), IMGUI_FRAME_BORDER_COLOR},
           self.size = IMGUI_TIMELINE_FRAME_SIZE, self.atlasOffset = IMGUI_TIMELINE_FRAME_ATLAS_OFFSET, self.border = IMGUI_FRAME_BORDER);

IMGUI_ITEM(IMGUI_TIMELINE_LAYER_FRAME, self.label = "## Layer Frame", self.dragDrop = "## Layer Frame Drag Drop", self.snapshotAction = "Layer Frame",
           self.color = {vec4(0.45f, 0.18f, 0.07f, 1.0f), vec4(0.78f, 0.32f, 0.12f, 1.0f), vec4(0.95f, 0.40f, 0.15f, 1.0f), IMGUI_FRAME_BORDER_COLOR},
           self.size = IMGUI_TIMELINE_FRAME_SIZE, self.atlasOffset = IMGUI_TIMELINE_FRAME_ATLAS_OFFSET, self.border = IMGUI_FRAME_BORDER);

IMGUI_ITEM(IMGUI_TIMELINE_NULL_FRAME, self.label = "## Null Frame", self.dragDrop = "## Null Frame Drag Drop", self.snapshotAction = "Null Frame",
           self.color = {vec4(0.17f, 0.33f, 0.17f, 1.0f), vec4(0.34f, 0.68f, 0.34f, 1.0f), vec4(0.44f, 0.88f, 0.44f, 1.0f), IMGUI_FRAME_BORDER_COLOR},
           self.size = IMGUI_TIMELINE_FRAME_SIZE, self.atlasOffset = IMGUI_TIMELINE_FRAME_ATLAS_OFFSET, self.border = IMGUI_FRAME_BORDER);

IMGUI_ITEM(IMGUI_TIMELINE_TRIGGERS_FRAME, self.label = "## Triggers Frame", self.snapshotAction = "Trigger",
           self.color = {vec4(0.36f, 0.14f, 0.24f, 1.0f), vec4(0.72f, 0.28f, 0.48f, 1.0f), vec4(0.92f, 0.36f, 0.60f, 1.0f), IMGUI_FRAME_BORDER_COLOR},
           self.size = IMGUI_TIMELINE_FRAME_SIZE, self.atlasOffset = IMGUI_TIMELINE_FRAME_ATLAS_OFFSET, self.border = IMGUI_FRAME_BORDER);

const inline ImguiItem* IMGUI_TIMELINE_FRAMES[ANM2_COUNT]{&IMGUI_TIMELINE_FRAME, &IMGUI_TIMELINE_ROOT_FRAME, &IMGUI_TIMELINE_LAYER_FRAME,
                                                          &IMGUI_TIMELINE_NULL_FRAME, &IMGUI_TIMELINE_TRIGGERS_FRAME};

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_FOOTER_CHILD, self.label = "## Item Footer Child", self.size = {IMGUI_TIMELINE_ITEM_CHILD.size->x, IMGUI_FOOTER_CHILD.size->y},
           self.flags = true, self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
IMGUI_ITEM(IMGUI_TIMELINE_OPTIONS_FOOTER_CHILD, self.label = "## Options Footer Child", self.size = {0, IMGUI_FOOTER_CHILD.size->y}, self.flags = true,
           self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

#define IMGUI_TIMELINE_FOOTER_ITEM_CHILD_ITEM_COUNT 2
IMGUI_ITEM(IMGUI_TIMELINE_ADD_ITEM, self.label = "Add",
           self.tooltip = "Adds an item (layer or null) to the animation.\nMake sure "
                          "to add a Layer/Null first in the Layers or Nulls windows.",
           self.popup = "Add Item", self.popupType = IMGUI_POPUP_ITEM_PROPERTIES_TYPE, self.popupSize = {300, 350},
           self.rowCount = IMGUI_TIMELINE_FOOTER_ITEM_CHILD_ITEM_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_TIMELINE_REMOVE_ITEM, self.label = "Remove", self.tooltip = "Removes the selected item (layer or null) from the animation.",
           self.snapshotAction = "Remove Item", self.chord = ImGuiKey_Delete, self.focusWindow = IMGUI_TIMELINE_ITEMS_CHILD.label,
           self.rowCount = IMGUI_TIMELINE_FOOTER_ITEM_CHILD_ITEM_COUNT);

#define IMGUI_TIMELINE_OPTIONS_ROW_COUNT 10
IMGUI_ITEM(IMGUI_PLAY, self.label = "|>  Play", self.tooltip = "Play the current animation, if paused.", self.focusWindow = IMGUI_TIMELINE.label,
           self.hotkey = HOTKEY_PLAY_PAUSE, self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_PAUSE, self.label = "|| Pause", self.tooltip = "Pause the current animation, if playing.", self.focusWindow = IMGUI_TIMELINE.label,
           self.hotkey = HOTKEY_PLAY_PAUSE, self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_INSERT_FRAME, self.label = "+ Insert Frame",
           self.tooltip = "Inserts a frame in the selected animation item, "
                          "based on the preview time.",
           self.snapshotAction = "Insert Frame", self.hotkey = HOTKEY_INSERT_FRAME, self.focusWindow = IMGUI_TIMELINE.label,
           self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_REMOVE_FRAME, self.label = "- Delete Frame", self.tooltip = "Removes the selected frame from the selected animation item.",
           self.snapshotAction = "Delete Frame", self.focusWindow = IMGUI_TIMELINE.label, self.chord = ImGuiKey_Delete,
           self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT, self.isSameLine = true);

static inline void imgui_shorten_frame(Imgui* self) {
  if (self->reference->itemType == ANM2_TRIGGER)
    return;
  if (Anm2Frame* frame = anm2_frame_from_reference(self->anm2, *self->reference))
    frame->delay = std::max(ANM2_FRAME_DELAY_MIN, frame->delay - 1);
}

IMGUI_ITEM(IMGUI_SHORTEN_FRAME, self.label = "## Shorten Frame", self.snapshotAction = "Shorten Frame", self.hotkey = HOTKEY_SHORTEN_FRAME,
           self.function = imgui_shorten_frame);

static inline void imgui_extend_frame(Imgui* self) {
  if (self->reference->itemType == ANM2_TRIGGER)
    return;
  if (Anm2Frame* frame = anm2_frame_from_reference(self->anm2, *self->reference))
    frame->delay++;
}

IMGUI_ITEM(IMGUI_EXTEND_FRAME, self.label = "## Extend Frame", self.snapshotAction = "Extend Frame", self.hotkey = HOTKEY_EXTEND_FRAME,
           self.function = imgui_extend_frame);

static inline void imgui_next_frame(Imgui* self) {
  if (Anm2Item* item = anm2_item_from_reference(self->anm2, *self->reference)) {
    self->reference->frameIndex = std::min((int)(item->frames.size() - 1), self->reference->frameIndex + 1);
    self->preview->time = anm2_time_from_reference(self->anm2, *self->reference);
  }
}

IMGUI_ITEM(IMGUI_NEXT_FRAME, self.label = "## Next Frame", self.hotkey = HOTKEY_NEXT_FRAME, self.function = imgui_next_frame);

static inline void imgui_previous_frame(Imgui* self) {
  if (anm2_item_from_reference(self->anm2, *self->reference)) {
    self->reference->frameIndex = std::max(0, self->reference->frameIndex - 1);
    self->preview->time = anm2_time_from_reference(self->anm2, *self->reference);
  }
}

IMGUI_ITEM(IMGUI_PREVIOUS_FRAME, self.label = "## Previous Frame", self.hotkey = HOTKEY_PREVIOUS_FRAME, self.function = imgui_previous_frame);

IMGUI_ITEM(IMGUI_BAKE, self.label = "Bake",
           self.tooltip = "Opens the bake popup menu, if a frame is selected.\nBaking a frame "
                          "takes the currently interpolated values at the time between it and "
                          "the next frame and separates them based on the interval.",
           self.popup = "Bake Frames", self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT, self.popupSize = {260, 145}, self.isSameLine = true);

IMGUI_ITEM(IMGUI_BAKE_CHILD, self.label = "## Bake Child", self.flags = true);

IMGUI_ITEM(IMGUI_BAKE_INTERVAL, self.label = "Interval",
           self.tooltip = "Sets the delay of the baked frames the selected "
                          "frame will be separated out into.",
           self.min = ANM2_FRAME_DELAY_MIN, self.value = ANM2_FRAME_DELAY_MIN);

IMGUI_ITEM(IMGUI_BAKE_ROUND_SCALE, self.label = "Round Scale", self.tooltip = "The scale of the baked frames will be rounded to the nearest integer.",
           self.value = true);

IMGUI_ITEM(IMGUI_BAKE_ROUND_ROTATION, self.label = "Round Rotation",
           self.tooltip = "The rotation of the baked frames will be rounded to "
                          "the nearest integer.",
           self.value = true, self.isSeparator = true);

IMGUI_ITEM(IMGUI_BAKE_CONFIRM, self.label = "Bake", self.tooltip = "Bake the selected frame with the options selected.", self.snapshotAction = "Bake Frames",
           self.rowCount = IMGUI_CONFIRM_POPUP_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_FIT_ANIMATION_LENGTH, self.label = "Fit Animation Length", self.tooltip = "Sets the animation's length to the latest frame.",
           self.snapshotAction = "Fit Animation Length", self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_ANIMATION_LENGTH, self.label = "Length", self.tooltip = "Sets the animation length.\n(Will not change frames.)",
           self.snapshotAction = "Set Animation Length", self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT, self.min = ANM2_FRAME_NUM_MIN,
           self.max = ANM2_FRAME_NUM_MAX, self.value = ANM2_FRAME_NUM_MIN, self.isSameLine = true);

IMGUI_ITEM(IMGUI_FPS, self.label = "FPS", self.tooltip = "Sets the animation's frames per second (its speed).", self.snapshotAction = "Set FPS",
           self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT, self.min = ANM2_FPS_MIN, self.max = ANM2_FPS_MAX, self.value = ANM2_FPS_DEFAULT,
           self.isSameLine = true);

IMGUI_ITEM(IMGUI_LOOP, self.label = "Loop", self.tooltip = "Toggles the animation looping.", self.snapshotAction = "Set Loop",
           self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT, self.value = true, self.isSameLine = true);

IMGUI_ITEM(IMGUI_CREATED_BY, self.label = "Author", self.tooltip = "Sets the author of the animation.", self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT,
           self.max = UCHAR_MAX);

#define IMGUI_ONIONSKIN_ROW_COUNT 3
IMGUI_ITEM(IMGUI_ONIONSKIN, self.label = "Onionskin");
IMGUI_ITEM(IMGUI_ONIONSKIN_ENABLED, self.label = "Enabled", self.tooltip = "Toggle onionskin (previews of frames before/after the current one.)",
           self.function = imgui_onionskin_toggle, self.hotkey = HOTKEY_ONIONSKIN, self.isSeparator = true);

IMGUI_ITEM(IMGUI_ONIONSKIN_BEFORE, self.label = "-- Before -- ");
IMGUI_ITEM(IMGUI_ONIONSKIN_AFTER, self.label = "-- After -- ");

IMGUI_ITEM(IMGUI_ONIONSKIN_COUNT, self.label = "Count", self.tooltip = "Set the number of previewed frames appearing.", self.min = 0, self.max = 100,
           self.value = SETTINGS_ONIONSKIN_BEFORE_COUNT_DEFAULT, self.rowCount = IMGUI_ONIONSKIN_ROW_COUNT, self.isSameLine = true);

IMGUI_ITEM(IMGUI_ONIONSKIN_COLOR_OFFSET, self.label = "Color Offset", self.tooltip = "Set the color offset of the previewed frames.",
           self.flags = ImGuiColorEditFlags_NoInputs, self.rowCount = IMGUI_ONIONSKIN_ROW_COUNT);

IMGUI_ITEM(IMGUI_ONIONSKIN_DRAW_ORDER, self.label = "Draw Order", self.isSameLine = true);

IMGUI_ITEM(IMGUI_ONIONSKIN_BELOW, self.label = "Below", self.tooltip = "The onionskin frames will draw below the base frame.", self.value = ONIONSKIN_BELOW,
           self.isSameLine = true);

IMGUI_ITEM(IMGUI_ONIONSKIN_ABOVE, self.label = "Above", self.tooltip = "The onionskin frames will draw above the base frame.", self.value = ONIONSKIN_ABOVE);

IMGUI_ITEM(IMGUI_CONTEXT_MENU, self.label = "## Context Menu");

IMGUI_ITEM(IMGUI_CUT, self.label = "Cut",
           self.tooltip = "Cuts the currently selected contextual element; "
                          "removing it and putting it to the clipboard.",
           self.snapshotAction = "Cut", self.function = imgui_cut, self.hotkey = HOTKEY_CUT, self.isWidthToText = true);

IMGUI_ITEM(IMGUI_COPY, self.label = "Copy", self.tooltip = "Copies the currently selected contextual element to the clipboard.", self.function = imgui_copy,
           self.hotkey = HOTKEY_COPY, self.isWidthToText = true);

IMGUI_ITEM(IMGUI_PASTE, self.label = "Paste", self.tooltip = "Pastes the currently selection contextual element from the clipboard.",
           self.snapshotAction = "Paste", self.function = imgui_paste, self.hotkey = HOTKEY_PASTE, self.isWidthToText = true);

IMGUI_ITEM(IMGUI_CHANGE_INPUT_TEXT, self.label = "## Input Text", self.tooltip = "Rename the selected item.", self.snapshotAction = "Rename",
           self.flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue, self.max = 0xFFFF);

IMGUI_ITEM(IMGUI_CHANGE_INPUT_INT, self.label = "## Input Int", self.tooltip = "Change the selected item's value.", self.snapshotAction = "Change Value",
           self.step = 0);

#define IMGUI_CONFIRM_POPUP_ROW_COUNT 2
IMGUI_ITEM(IMGUI_POPUP_OK, self.label = "OK", self.tooltip = "Confirm the action.", self.rowCount = IMGUI_CONFIRM_POPUP_ROW_COUNT);

IMGUI_ITEM(IMGUI_POPUP_CANCEL, self.label = "Cancel", self.tooltip = "Cancel the action.", self.rowCount = IMGUI_CONFIRM_POPUP_ROW_COUNT);

IMGUI_ITEM(IMGUI_LOG_WINDOW, self.label = "## Log Window",
           self.flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                        ImGuiWindowFlags_NoInputs);

IMGUI_ITEM(IMGUI_ATLAS, self.label = "## Atlas Image", self.atlas = ATLAS_NONE);

void imgui_init(Imgui* self, Dialog* dialog, Resources* resources, Anm2* anm2, Anm2Reference* reference, Editor* editor, Preview* preview,
                GeneratePreview* generatePreview, Settings* settings, Snapshots* snapshots, Clipboard* clipboard, SDL_Window* window, SDL_GLContext* glContext);

void imgui_update(Imgui* self);
void imgui_draw();
void imgui_free();