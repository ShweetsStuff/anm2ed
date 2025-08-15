#pragma once

#include "clipboard.h"
#include "dialog.h"
#include "editor.h"
#include "ffmpeg.h"
#include "preview.h"
#include "generate_preview.h"
#include "resources.h"
#include "settings.h"
#include "snapshots.h"
#include "tool.h"
#include "window.h"

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#define IMGUI_ENABLE_DOCKING
#define IM_VEC2_CLASS_EXTRA                                                                                             \
    inline bool operator==(const ImVec2& rhs) const { return x == rhs.x && y == rhs.y; }                                \
    inline bool operator!=(const ImVec2& rhs) const { return !(*this == rhs); }                                         \
    inline ImVec2 operator+(const ImVec2& rhs) const { return ImVec2(x + rhs.x, y + rhs.y); }                           \
    inline ImVec2 operator-(const ImVec2& rhs) const { return ImVec2(x - rhs.x, y - rhs.y); }                           \
    inline ImVec2 operator*(const ImVec2& rhs) const { return ImVec2(x * rhs.x, y * rhs.y); }                           \
    inline ImVec2 operator*(float s) const { return ImVec2(x * s, y * s); }                                             \
    friend inline ImVec2 operator*(float s, const ImVec2& v) { return ImVec2(v.x * s, v.y * s); }                       \
    inline ImVec2& operator+=(const ImVec2& rhs) { x += rhs.x; y += rhs.y; return *this; }                               \
    inline ImVec2& operator-=(const ImVec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }                               \
    inline ImVec2& operator*=(float s) { x *= s; y *= s; return *this; }                                                \
    inline ImVec2(const vec2& v) : x(v.x), y(v.y) {}                                                                    \
    inline operator vec2() const { return vec2(x, y); }

#define IM_VEC4_CLASS_EXTRA                                                                                             \
    inline bool operator==(const ImVec4& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }    \
    inline bool operator!=(const ImVec4& rhs) const { return !(*this == rhs); }                                         \
    inline ImVec4 operator+(const ImVec4& rhs) const { return ImVec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }     \
    inline ImVec4 operator-(const ImVec4& rhs) const { return ImVec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }     \
    inline ImVec4 operator*(const ImVec4& rhs) const { return ImVec4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }     \
    inline ImVec4 operator*(float s) const { return ImVec4(x * s, y * s, z * s, w * s); }                               \
    friend inline ImVec4 operator*(float s, const ImVec4& v) { return ImVec4(v.x * s, v.y * s, v.z * s, v.w * s); }     \
    inline ImVec4& operator+=(const ImVec4& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }      \
    inline ImVec4& operator-=(const ImVec4& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }      \
    inline ImVec4& operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }                                \
    inline ImVec4(const vec4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}                                                    \
    inline operator vec4() const { return vec4(x, y, z, w); }
    
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#define IMGUI_CHORD_NONE (ImGuiMod_None)
#define IMGUI_EVENTS_FOOTER_HEIGHT 40
#define IMGUI_FRAME_BORDER 2.0f
#define IMGUI_LOG_DURATION 3.0f
#define IMGUI_LOG_PADDING 10.0f
#define IMGUI_PLAYHEAD_LINE_COLOR IM_COL32(255, 255, 255, 255)
#define IMGUI_TRIGGERS_EVENT_COLOR IM_COL32(255, 255, 255, 128)
#define IMGUI_PLAYHEAD_LINE_WIDTH 2.0f
#define IMGUI_SPRITESHEETS_FOOTER_HEIGHT 65
#define IMGUI_TIMELINE_FRAME_MULTIPLE 5
#define IMGUI_TIMELINE_MERGE
#define IMGUI_TOOL_COLOR_PICKER_DURATION 0.25f
#define IMGUI_OPTION_POPUP_ROW_COUNT 2

#define IMGUI_ACTION_FRAME_CROP "Frame Crop"
#define IMGUI_ACTION_FRAME_SWAP "Frame Swap"
#define IMGUI_ACTION_FRAME_TRANSFORM "Frame Transform"
#define IMGUI_ACTION_ANIMATION_SWAP "Animation Swap"
#define IMGUI_ACTION_TRIGGER_MOVE "Trigger AtFrame"
#define IMGUI_ACTION_MOVE_PLAYHEAD "Move Playhead"

#define IMGUI_POPUP_FLAGS ImGuiWindowFlags_NoMove
#define IMGUI_POPUP_MODAL_FLAGS ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize

#define IMGUI_LOG_FILE_OPEN_FORMAT "Opened anm2: {}" 
#define IMGUI_LOG_FILE_SAVE_FORMAT "Saved anm2 to: {}" 
#define IMGUI_LOG_RENDER_ANIMATION_FRAMES_SAVE_FORMAT "Saved rendered frames to: {}" 
#define IMGUI_LOG_RENDER_ANIMATION_SAVE_FORMAT "Saved rendered animation to: {}" 
#define IMGUI_LOG_RENDER_ANIMATION_NO_ANIMATION_ERROR "No animation selected; rendering cancelled."
#define IMGUI_LOG_RENDER_ANIMATION_NO_FRAMES_ERROR "No frames to render; rendering cancelled."
#define IMGUI_LOG_RENDER_ANIMATION_DIRECTORY_ERROR "Invalid directory! Make sure it exists and you have write permissions."
#define IMGUI_LOG_RENDER_ANIMATION_PATH_ERROR "Invalid path! Make sure it's valid and you have write permissions."
#define IMGUI_LOG_RENDER_ANIMATION_FFMPEG_PATH_ERROR "Invalid FFmpeg path! Make sure you have it installed and the path is correct."
#define IMGUI_LOG_RENDER_ANIMATION_FFMPEG_ERROR "FFmpeg could not render animation! Check paths or your FFmpeg installation."
#define IMGUI_LOG_SPRITESHEET_SAVE_FORMAT "Saved spritesheet #{} to: {}" 

#define IMGUI_NONE "None"
#define IMGUI_ANIMATION_DEFAULT_FORMAT "(*) {}"
#define IMGUI_BUFFER_MAX 255
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
#define IMGUI_TIMELINE_FRAME_LABEL_FORMAT "## {}"
#define IMGUI_SELECTABLE_INPUT_INT_FORMAT "#{}"
#define IMGUI_TIMELINE_ANIMATION_NONE "Select an animation to show timeline..."

#define IMGUI_LABEL_SHORTCUT_FORMAT "({})"
#define IMGUI_TOOLTIP_SHORTCUT_FORMAT "\n(Shortcut: {})"
#define IMGUI_INVISIBLE_FORMAT "## {}"

#define IMGUI_TRIGGERS_FONT_SCALE 2.0

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

const ImVec2 IMGUI_SPRITESHEET_PREVIEW_SIZE = {50.0, 50.0};
const ImVec2 IMGUI_TOOLTIP_OFFSET = {16, 8};
const vec2 IMGUI_SPRITESHEET_EDITOR_CROP_FORGIVENESS = {1, 1};

const ImGuiKey IMGUI_INPUT_DELETE = ImGuiKey_Delete;
const ImGuiKey IMGUI_INPUT_LEFT = ImGuiKey_LeftArrow;
const ImGuiKey IMGUI_INPUT_RIGHT = ImGuiKey_RightArrow;
const ImGuiKey IMGUI_INPUT_UP = ImGuiKey_UpArrow;
const ImGuiKey IMGUI_INPUT_DOWN = ImGuiKey_DownArrow;
const ImGuiKey IMGUI_INPUT_SHIFT = ImGuiMod_Shift;
const ImGuiKey IMGUI_INPUT_CTRL = ImGuiMod_Ctrl;
const ImGuiKey IMGUI_INPUT_ZOOM_IN = ImGuiKey_1;
const ImGuiKey IMGUI_INPUT_ZOOM_OUT = ImGuiKey_2;
const ImGuiKey IMGUI_INPUT_ENTER = ImGuiKey_Enter;
const ImGuiKey IMGUI_INPUT_RENAME = ImGuiKey_F2;
const ImGuiKey IMGUI_INPUT_DEFAULT = ImGuiKey_Home;
const ImGuiMouseButton IMGUI_MOUSE_DEFAULT = ImGuiMouseButton_Middle;

enum ImguiPopupType
{
    IMGUI_POPUP_NONE,
    IMGUI_POPUP_BY_ITEM,
    IMGUI_POPUP_CENTER_WINDOW
};

struct ImguiColorSet 
{
    ImVec4 normal{};
    ImVec4 active{};
    ImVec4 hovered{};
    ImVec4 border{};

    bool is_normal() const { return normal != ImVec4(); }
    bool is_active() const { return active != ImVec4(); }
    bool is_hovered() const { return hovered != ImVec4(); }
    bool is_border() const { return border != ImVec4(); }
};

struct ImguiLogItem
{
    std::string text;
    f32 timeRemaining;
};

struct Imgui
{  
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
    std::string pendingPopup{};
    ImguiPopupType pendingPopupType = IMGUI_POPUP_NONE;
    ImVec2 pendingPopupPosition{};
    std::vector<ImguiLogItem> log;
    SDL_SystemCursor cursor;
    SDL_SystemCursor pendingCursor;
    bool isCursorSet = false;
    bool isContextualActionsEnabled = true;
    bool isQuit = false;
    bool isTryQuit = false;
};

typedef void(*ImguiFunction)(Imgui*);

struct ImguiHotkey
{
    ImGuiKeyChord chord;
    ImguiFunction function;
    std::string focusWindow{};
    std::string undoAction{};

    bool is_focus_window() const { return !focusWindow.empty(); }
    bool is_undoable() const { return !undoAction.empty(); }
};

static void imgui_log_push(Imgui* self, const std::string& text)
{
    self->log.push_back({text, IMGUI_LOG_DURATION});
    std::println("[IMGUI] {}", text);
}

static std::vector<ImguiHotkey>& imgui_hotkey_registry()
{
    static std::vector<ImguiHotkey> registry;
    return registry;
}

static inline void imgui_file_new(Imgui* self)
{
    anm2_reference_clear(self->reference);
	anm2_new(self->anm2);
    resources_textures_free(self->resources);
}

static inline void imgui_file_open(Imgui* self)
{
    dialog_anm2_open(self->dialog);
}

static inline void imgui_file_save(Imgui* self)
{
    if (self->anm2->path.empty())
		dialog_anm2_save(self->dialog);
	else 
    {
		anm2_serialize(self->anm2, self->anm2->path);
        imgui_log_push(self, std::format(IMGUI_LOG_FILE_SAVE_FORMAT, self->anm2->path));
    }
}

static inline void imgui_file_save_as(Imgui* self)
{
	dialog_anm2_save(self->dialog);
}

static inline void imgui_quit(Imgui* self)
{
    if (!self->snapshots->undoStack.is_empty())
        self->isTryQuit = true;
    else
        self->isQuit = true;
}

static inline void imgui_explore(Imgui* self)
{
    std::filesystem::path filePath = self->anm2->path;
    std::filesystem::path parentPath = filePath.parent_path();
    dialog_explorer_open(parentPath.string());
}

static inline void imgui_undo_push(Imgui* self, const std::string& action = SNAPSHOT_ACTION)
{
    Snapshot snapshot = {*self->anm2, *self->reference, self->preview->time, action};
    snapshots_undo_push(self->snapshots, &snapshot);
}

static inline void imgui_tool_pan_set(Imgui* self)
{
    self->settings->tool = TOOL_PAN;
}

static inline void imgui_tool_move_set(Imgui* self)
{
    self->settings->tool = TOOL_MOVE;
}

static inline void imgui_tool_rotate_set(Imgui* self)
{
    self->settings->tool = TOOL_ROTATE;
}

static inline void imgui_tool_scale_set(Imgui* self)
{
    self->settings->tool = TOOL_SCALE;
}

static inline void imgui_tool_crop_set(Imgui* self)
{
    self->settings->tool = TOOL_CROP;
}

static inline void imgui_tool_draw_set(Imgui* self)
{
    self->settings->tool = TOOL_DRAW;
}

static inline void imgui_tool_erase_set(Imgui* self)
{
    self->settings->tool = TOOL_ERASE;
}

static inline void imgui_tool_color_picker_set(Imgui* self)
{
    self->settings->tool = TOOL_COLOR_PICKER;
}

static inline void imgui_undo(Imgui* self)
{
    if (self->snapshots->undoStack.top == 0) return;

    snapshots_undo(self->snapshots);
    imgui_log_push(self, std::format(IMGUI_LOG_UNDO_FORMAT, self->snapshots->action));
}

static inline void imgui_redo(Imgui* self)
{
    if (self->snapshots->redoStack.top == 0) return;
    
    std::string action = self->snapshots->action;
    snapshots_redo(self->snapshots);
    imgui_log_push(self, std::format(IMGUI_LOG_REDO_FORMAT, action));
}

static inline void imgui_cut(Imgui* self)
{
    clipboard_cut(self->clipboard);
}

static inline void imgui_copy(Imgui* self)
{
    clipboard_copy(self->clipboard);
}

static inline void imgui_paste(Imgui* self)
{
    clipboard_paste(self->clipboard);
}

static inline ImGuiKey imgui_key_from_char_get(char c) 
{
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    if (c >= 'A' && c <= 'Z') return (ImGuiKey)(ImGuiKey_A + (c - 'A'));
    return ImGuiKey_None;
}

static inline std::string imgui_string_from_chord_get(ImGuiKeyChord chord)
{
    std::string result;

    if (chord & ImGuiMod_Ctrl)   result += "Ctrl+";
    if (chord & ImGuiMod_Shift)  result += "Shift+";
    if (chord & ImGuiMod_Alt)    result += "Alt+";

    ImGuiKey key = (ImGuiKey)(chord & ~ImGuiMod_Mask_);

    if (key >= ImGuiKey_A && key <= ImGuiKey_Z)
        result.push_back('A' + (key - ImGuiKey_A));
    else if (key >= ImGuiKey_0 && key <= ImGuiKey_9)
        result.push_back('0' + (key - ImGuiKey_0));
    else
    {
        // Fallback to ImGui's built-in name for non-alphanumerics
        const char* name = ImGui::GetKeyName(key);
        if (name && *name)
            result += name;
        else
            result += "Unknown";
    }

    return result;
}

static void imgui_contextual_actions_enable(Imgui* self) { self->isContextualActionsEnabled = true; }
static void imgui_contextual_actions_disable(Imgui* self){ self->isContextualActionsEnabled = false; }
static inline bool imgui_is_popup_open(const std::string& label) { return ImGui::IsPopupOpen(label.c_str()); }
static inline void imgui_open_popup(const std::string& label) { ImGui::OpenPopup(label.c_str()); }
static inline void imgui_pending_popup_process(Imgui* self)
{
	if (self->pendingPopup.empty()) return;

	switch (self->pendingPopupType)
	{
		case IMGUI_POPUP_CENTER_WINDOW:
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			break;
		case IMGUI_POPUP_BY_ITEM:
		default:
			ImGui::SetNextWindowPos(self->pendingPopupPosition);
			break;
	}

	imgui_open_popup(self->pendingPopup.c_str());

	self->pendingPopup.clear();
	self->pendingPopupType = IMGUI_POPUP_NONE;
	self->pendingPopupPosition = ImVec2();
}

static inline bool imgui_begin_popup(const std::string& label, Imgui* imgui, ImVec2 size = ImVec2())
{
	imgui_pending_popup_process(imgui);
	if (size != ImVec2()) ImGui::SetNextWindowSizeConstraints(size, ImVec2(FLT_MAX, FLT_MAX));
	bool isActivated = ImGui::BeginPopup(label.c_str(), IMGUI_POPUP_FLAGS);
	return isActivated;
}

static inline bool imgui_begin_popup_modal(const std::string& label, Imgui* imgui, ImVec2 size = ImVec2())
{
	imgui_pending_popup_process(imgui);
	if (size != ImVec2()) ImGui::SetNextWindowSizeConstraints(size, ImVec2(FLT_MAX, FLT_MAX));
	bool isActivated = ImGui::BeginPopupModal(label.c_str(), nullptr, IMGUI_POPUP_MODAL_FLAGS);
	if (isActivated) imgui_contextual_actions_disable(imgui);
	return isActivated;
}

static inline void imgui_close_current_popup(Imgui* imgui)
{
	imgui_contextual_actions_enable(imgui);
	ImGui::CloseCurrentPopup();
}

static inline void imgui_end_popup(Imgui* imgui)
{
	ImGui::EndPopup();
	imgui_pending_popup_process(imgui);
}

enum ImguiItemType
{
    IMGUI_ITEM,
    IMGUI_TEXT,
    IMGUI_WINDOW,
    IMGUI_DOCKSPACE,
    IMGUI_CHILD,
    IMGUI_OPTION_POPUP,
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
    IMGUI_ATLAS_BUTTON
};

struct ImguiItemOverride
{
    bool isDisabled{};
    bool isSelected{};
    std::string label{};
    ImVec2 size{};
    s32 id{};
    s32 max{};
    AtlasType atlas = ATLAS_NONE;
    bool isMnemonicDisabled{};
    s32 value{};
};

// Item
struct ImguiItem
{
    std::string label{};
    std::string tooltip{};
    std::string& text = tooltip;
    std::string undoAction{};
    std::string popup{};
    std::string dragDrop{};
    std::string focusWindow{};
    std::vector<std::string> items{};
    AtlasType atlas;
    ImGuiKeyChord chord = IMGUI_CHORD_NONE;
    ImGuiKey mnemonicKey = ImGuiKey_None;
    s32 mnemonicIndex = INDEX_NONE;
    ImVec2 size{};
    ImVec2 popupSize{};
    ImguiColorSet color{};
    ImguiFunction function = nullptr;
    ImguiPopupType popupType = IMGUI_POPUP_CENTER_WINDOW;
    bool isDisabled = false;
    bool isMnemonicDisabled = false;
    bool isSelected = false;
    bool isUseItemActivated = false;
    bool isSizeToText = false;
    bool isShortcutInLabel = false;
    bool isSameLine = false;
    bool isSeparator = false;
    s32 id = 0;
    s32 idOffset{};
    f32 speed = 0.25f;
    s32 step = 1;
    s32 stepFast = 10;
    s32 min{};
    s32 max{};
    s32 value{};
    vec2 atlasOffset;
    s32 border{};
    s32 flags{};
    s32 windowFlags{};
    s32 rowCount = 0;

    void construct()
    {
        static s32 idNew = 0;
        id = idNew++;
  
        if (is_chord())
        {
            std::string chordString = imgui_string_from_chord_get(chord);
            if (isShortcutInLabel)
                label += std::format(IMGUI_LABEL_SHORTCUT_FORMAT, chordString);
            tooltip += std::format(IMGUI_TOOLTIP_SHORTCUT_FORMAT, chordString);
            if (function)
                imgui_hotkey_registry().push_back({chord, function, focusWindow, undoAction});
        }

        std::string labelNew{};

        for (s32 i = 0; i < (s32)label.size(); i++)
        {
            if (label[i] == '&')
            {
                if (label[i + 1] == '&')
                {
                    labelNew += '&';
                    i++;
                }
                else if (label[i + 1] != '\0')
                {
                    mnemonicKey = imgui_key_from_char_get(label[i + 1]);
                    mnemonicIndex = (s32)labelNew.size();
                    labelNew += label[i + 1];
                    i++;
                }
            }
            else 
                labelNew += label[i];
        }

        label = labelNew;
    }

    ImguiItem copy(const ImguiItemOverride& override) const
    {
        ImguiItem out = *this;
        if (override.isDisabled) out.isDisabled = override.isDisabled;
        if(override.isSelected) out.isSelected = override.isSelected;
        if (is_popup() && imgui_is_popup_open(popup)) out.isSelected = true;
        if (id != ID_NONE) out.id = id + idOffset + override.id;
        if (!override.label.empty()) out.label = override.label;
        if (override.size != ImVec2{}) out.size = override.size;
        if (override.max != 0) out.max = override.max;
        if (override.value != 0) out.value = override.value;
        if (override.atlas != ATLAS_NONE) out.atlas = override.atlas;
        if (override.isMnemonicDisabled) out.isMnemonicDisabled = override.isMnemonicDisabled;
        return out;
    }

    bool is_border() const { return border != 0; }
    bool is_row() const { return rowCount != 0; }
    bool is_chord() const { return chord != IMGUI_CHORD_NONE; }
    bool is_drag_drop() const { return !dragDrop.empty(); }
    bool is_focus_window() const { return !focusWindow.empty(); }
    bool is_popup() const { return !popup.empty(); }
    bool is_size() const { return size != ImVec2(); }
    bool is_popup_size() const { return popupSize != ImVec2(); }
    bool is_tooltip() const { return !tooltip.empty(); }
    bool is_undoable() const { return !undoAction.empty(); }
    bool is_mnemonic() const { return mnemonicKey != ImGuiKey_None; }
    bool is_range() const { return min != 0 || max != 0; }
    const char* label_get() const { return label.c_str(); }
    const char* drag_drop_get() const { return dragDrop.c_str(); }
    const char* tooltip_get() const { return tooltip.c_str(); }
    const char* text_get() const { return text.c_str(); }
};

#define IMGUI_ITEM(NAME, ...) const inline ImguiItem NAME = []{ ImguiItem self; __VA_ARGS__; self.construct(); return self; }()

IMGUI_ITEM(IMGUI_WINDOW_MAIN,
    self.label = "## Window",
    self.flags = ImGuiWindowFlags_NoTitleBar            | 
                 ImGuiWindowFlags_NoCollapse            | 
                 ImGuiWindowFlags_NoResize              | 
                 ImGuiWindowFlags_NoMove                | 
                 ImGuiWindowFlags_NoBringToFrontOnFocus | 
                 ImGuiWindowFlags_NoNavFocus
);

IMGUI_ITEM(IMGUI_DOCKSPACE_MAIN, 
    self.label = "## Dockspace",
    self.flags = ImGuiDockNodeFlags_PassthruCentralNode
);

IMGUI_ITEM(IMGUI_FOOTER_CHILD, 
    self.label = "## Footer Child", 
    self.size = {0, 36},
    self.flags = true
);

IMGUI_ITEM(IMGUI_TASKBAR,
    self.label = "## Taskbar",
    self.size = {0, 32},
    self.flags = ImGuiWindowFlags_NoTitleBar        |
                 ImGuiWindowFlags_NoResize          |
                 ImGuiWindowFlags_NoMove            |
                 ImGuiWindowFlags_NoScrollbar       |
                 ImGuiWindowFlags_NoScrollWithMouse |
                 ImGuiWindowFlags_NoSavedSettings
);

IMGUI_ITEM(IMGUI_FILE,
    self.label = "&File", 
    self.tooltip = "Opens the file menu, for reading/writing anm2 files.", 
    self.popup = "## File Popup",
    self.popupType = IMGUI_POPUP_BY_ITEM,
    self.isSizeToText = true,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_NEW,
    self.label = "&New           ",
    self.tooltip = "Load a blank .anm2 file to edit.",
    self.function = imgui_file_new,
    self.chord = ImGuiMod_Ctrl | ImGuiKey_N,
    self.isSizeToText = true,
    self.isShortcutInLabel = true
);

IMGUI_ITEM(IMGUI_OPEN,
    self.label = "&Open          ",
    self.tooltip = "Open an existing .anm2 file to edit.",
    self.function = imgui_file_open,
    self.chord = ImGuiMod_Ctrl | ImGuiKey_O,
    self.isSizeToText = true,
    self.isShortcutInLabel = true
);

IMGUI_ITEM(IMGUI_SAVE,
    self.label = "&Save          ",
    self.tooltip = "Saves the current .anm2 file to its path.\nIf no path exists, one can be chosen.",
    self.function = imgui_file_save,
    self.chord = ImGuiMod_Ctrl | ImGuiKey_S,
    self.isSizeToText = true,
    self.isShortcutInLabel = true
);

IMGUI_ITEM(IMGUI_SAVE_AS,
    self.label = "S&ave As ",
    self.tooltip = "Saves the current .anm2 file to a chosen path.",
    self.function = imgui_file_save_as,
    self.chord = ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S,
    self.isSizeToText = true,
    self.isShortcutInLabel = true
);

IMGUI_ITEM(IMGUI_EXPLORE_ANM2_LOCATION,
    self.label = "E&xplore Anm2 Location",
    self.tooltip = "Open the system's file explorer in the anm2's path.",
    self.function = imgui_explore,
    self.isSizeToText = true,
    self.isSeparator = true
);

IMGUI_ITEM(IMGUI_EXIT,
    self.label = "&Exit          ",
    self.tooltip = "Exits the program.",
    self.function = imgui_quit,
    self.chord = ImGuiMod_Alt | ImGuiKey_F4,
    self.isSizeToText  = true,
    self.isShortcutInLabel = true
);

IMGUI_ITEM(IMGUI_EXIT_CONFIRMATION,
    self.label = "Exit Confirmation",
    self.text = "Unsaved changes will be lost!\nAre you sure you want to exit?"
);

IMGUI_ITEM(IMGUI_WIZARD,
    self.label = "&Wizard",
    self.tooltip = "Opens the wizard menu, for neat functions related to the .anm2.",
    self.popup = "## Wizard Popup",
    self.popupType = IMGUI_POPUP_BY_ITEM,
    self.isSizeToText  = true,
    self.isSameLine = true
);

#define IMGUI_GENERATE_ANIMATION_FROM_GRID_PADDING 40
IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID,
    self.label = "&Generate Animation from Grid",
    self.tooltip = "Generate a new animation from grid values.",
    self.popup = "Generate Animation from Grid",
    self.popupType = IMGUI_POPUP_CENTER_WINDOW,
    self.popupSize = 
    {
        (GENERATE_PREVIEW_SIZE.x * 2) + IMGUI_GENERATE_ANIMATION_FROM_GRID_PADDING, 
        GENERATE_PREVIEW_SIZE.y + (IMGUI_FOOTER_CHILD.size.y * 2) + (IMGUI_GENERATE_ANIMATION_FROM_GRID_PADDING / 2)
    }
);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_OPTIONS_CHILD,
    self.label = "## Generate Animation From Grid Options Child",
    self.size = 
    {
        IMGUI_GENERATE_ANIMATION_FROM_GRID.popupSize.x / 2, 
        IMGUI_GENERATE_ANIMATION_FROM_GRID.popupSize.y - IMGUI_FOOTER_CHILD.size.y
    },
    self.flags = true
);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_START_POSITION,
    self.label = "Start Position",
    self.tooltip = "Set the starting position on the layer's spritesheet for the generated animation."
);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_SIZE,
    self.label = "Size",
    self.tooltip = "Set the size of each frame in the generated animation."
);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_PIVOT,
    self.label = "Pivot",
    self.tooltip = "Set the pivot of each frame in the generated animation."
);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_ROWS,
    self.label = "Rows",
    self.tooltip = "Set how many rows will be used in the generated animation.",
    self.max = 1000
);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_COLUMNS,
    self.label = "Columns",
    self.tooltip = "Set how many columns will be used in the generated animation.",
    self.max = 1000
);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_COUNT,
    self.label = "Count",
    self.tooltip = "Set how many frames will be made for the generated animation.",
    self.value = ANM2_FRAME_NUM_MIN
);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_DELAY,
    self.label = "Delay",
    self.tooltip = "Set the delay of each frame in the generated animation.",
    self.max = 1000
);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_PREVIEW_CHILD,
    self.label = "## Generate Animation From Grid Preview Child",
    self.size = 
    {
        IMGUI_GENERATE_ANIMATION_FROM_GRID.popupSize.x / 2, 
        IMGUI_GENERATE_ANIMATION_FROM_GRID.popupSize.y - IMGUI_FOOTER_CHILD.size.y
    },
    self.flags = true
);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_SLIDER_CHILD,
    self.label = "## Generate Animation From Grid Slider Child",
    self.size = 
    {
        (IMGUI_GENERATE_ANIMATION_FROM_GRID.popupSize.x / 2) - (IMGUI_GENERATE_ANIMATION_FROM_GRID_PADDING / 2), 
        IMGUI_FOOTER_CHILD.size.y
    },
    self.flags = true
);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_SLIDER,
    self.label = "## Generate Animation From Grid Slider",
    self.tooltip = "Change the time of the generated animation preview.",
    self.min = GENERATE_PREVIEW_TIME_MIN,
    self.max = GENERATE_PREVIEW_TIME_MAX,
    self.value = GENERATE_PREVIEW_TIME_MIN,
    self.rowCount = 1,
    self.flags = ImGuiSliderFlags_NoInput
);

IMGUI_ITEM(IMGUI_GENERATE_ANIMATION_FROM_GRID_GENERATE,
    self.label = "Generate",
    self.tooltip = "Generate an animation with the used settings.",
    self.undoAction = "Generate Animation from Grid",
    self.rowCount = IMGUI_OPTION_POPUP_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES,
    self.label = "&Change All Frame Properties",
    self.tooltip = "Change all frame properties in the selected animation item (or selected frame).",
    self.popup = "Change All Frame Properties",
    self.popupType = IMGUI_POPUP_CENTER_WINDOW,
    self.popupSize = {500, 380}
);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_CHILD,
    self.label = "## Change All Frame Properties Child",
    self.size = 
    {
        IMGUI_CHANGE_ALL_FRAME_PROPERTIES.popupSize.x, 
        250
    },
    self.flags = true
);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_SETTINGS_CHILD,
    self.label = "## Change All Frame Properties Settings Child",
     self.size = 
    {
        IMGUI_CHANGE_ALL_FRAME_PROPERTIES.popupSize.x, 
        55
    },
    self.flags = true
);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_SETTINGS, self.label = "Settings");

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_FROM_SELECTED_FRAME,
    self.label = "From Selected Frames",
    self.tooltip = "The set frame properties will start from the selected frame.",
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_NUMBER_FRAMES,
    self.label = "# of Frames",
    self.tooltip = "Set the amount of frames that the set frame properties will apply to.",
    self.size = {200, 0},
    self.value = ANM2_FRAME_NUM_MIN,
    self.max = 1000
);

#define IMGUI_CHANGE_ALL_FRAME_PROPERTIES_OPTIONS_ROW_COUNT 4
IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_ADD,
    self.label = "Add",
    self.tooltip = "The specified values will be added to all specified frames.",
    self.undoAction = "Add Frame Properties",
    self.rowCount = IMGUI_CHANGE_ALL_FRAME_PROPERTIES_OPTIONS_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_SUBTRACT,
    self.label = "Subtract",
    self.tooltip = "The specified values will be added to all selected frames.",
    self.undoAction = "Subtract Frame Properties",
    self.rowCount = IMGUI_CHANGE_ALL_FRAME_PROPERTIES_OPTIONS_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_SET,
    self.label = "Set",
    self.tooltip = "The specified values will be set to the specified value in selected frames.",
    self.undoAction = "Set Frame Properties",
    self.rowCount = IMGUI_CHANGE_ALL_FRAME_PROPERTIES_OPTIONS_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_CHANGE_ALL_FRAME_PROPERTIES_CANCEL,
    self.label = "Cancel",
    self.tooltip = "Cancel changing all frame properties.",
    self.rowCount = IMGUI_CHANGE_ALL_FRAME_PROPERTIES_OPTIONS_ROW_COUNT
);

IMGUI_ITEM(IMGUI_SCALE_ANM2,
    self.label = "&Scale Anm2",
    self.tooltip = "Scale up all size and position-related frame properties in the anm2.",
    self.popup = "Scale Anm2",
    self.popupType = IMGUI_POPUP_CENTER_WINDOW,
    self.popupSize = {260, 72},
    self.isSizeToText = true,
    self.isSeparator = true
);

IMGUI_ITEM(IMGUI_SCALE_ANM2_OPTIONS_CHILD,
    self.label = "## Scale Anm2 Options Child",
    self.size = {IMGUI_SCALE_ANM2.popupSize.x, IMGUI_SCALE_ANM2.popupSize.y - IMGUI_FOOTER_CHILD.size.y},
    self.flags = true
);

IMGUI_ITEM(IMGUI_SCALE_ANM2_VALUE,
    self.label = "Value",
    self.tooltip = "The size and position-related frame properties in the anm2 will be scaled by this value.",
    self.value = 1,
    self.step = 0.25,
    self.stepFast = 1
);

IMGUI_ITEM(IMGUI_SCALE_ANM2_SCALE,
    self.label = "Scale",
    self.tooltip = "Scale the anm2 with the value specified.",
    self.undoAction = "Scale Anm2",
    self.rowCount = IMGUI_OPTION_POPUP_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION,
    self.label = "&Render Animation",
    self.tooltip = "Renders the current animation preview; output options can be customized.",
    self.popup = "Render Animation",
    self.popupSize = {600, 125}
);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_CHILD,
    self.label = "## Render Animation Child",
    self.size = {600, 125}
);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_LOCATION_BROWSE,
    self.label = "## Location Browse",
    self.tooltip = "Open file explorer to pick rendered animation location.",
    self.atlas = ATLAS_FOLDER,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_LOCATION,
    self.label = "Location",
    self.tooltip = "Select the location of the rendered animation.",
    self.max = 255
);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_FFMPEG_BROWSE,
    self.label = "## FFMpeg Browse",
    self.tooltip = "Open file explorer to pick the path of FFmpeg",
    self.atlas = ATLAS_FOLDER,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_FFMPEG_PATH,
    self.label = "FFmpeg Path",
    self.tooltip = "Sets the path FFmpeg currently resides in.\nFFmpeg is required for rendering animations.\nDownload it from https://ffmpeg.org/, your package manager, or wherever else.",
    self.max = 255
);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_OUTPUT,
    self.label = "Output",
    self.tooltip = "Select the rendered animation output.\nIt can either be one animated image or a sequence of frames.",
    self.items = {std::begin(RENDER_TYPE_STRINGS), std::end(RENDER_TYPE_STRINGS)},
    self.value = RENDER_PNG,
    self.isSeparator = true
);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_FORMAT,
    self.label = "Format",
    self.tooltip = "(PNG images only).\nSet the format of each output frame; i.e., its filename.\nThe format will only take one argument; that being the frame's index.\nFor example, a format like \"{}.png\" will export a frame of index 0 as \"0.png\".",
    self.max = 255
);

IMGUI_ITEM(IMGUI_RENDER_ANIMATION_CONFIRM,
    self.label = "Render",
    self.tooltip = "Render the animation, with the used settings.",
    self.popup = "Rendering Animation...",
    self.popupType = IMGUI_POPUP_CENTER_WINDOW,
    self.popupSize = {300, 60},
    self.isSameLine = true,
    self.rowCount = IMGUI_OPTION_POPUP_ROW_COUNT
);

IMGUI_ITEM(IMGUI_RENDERING_ANIMATION_CANCEL,
    self.label = "Cancel",
    self.tooltip = "Cancel rendering the animation.",
    self.rowCount = 1
);

IMGUI_ITEM(IMGUI_PLAYBACK,
    self.label = "&Playback",
    self.tooltip = "Opens the playback menu, for configuring playback settings.",
    self.popup = "## Playback Popup",
    self.popupType = IMGUI_POPUP_BY_ITEM,
    self.isSizeToText = true,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_ALWAYS_LOOP,
    self.label = "&Always Loop",
    self.tooltip = "Sets the animation playback to always loop, regardless of the animation's loop setting.",
    self.isSizeToText = true
);

IMGUI_ITEM(IMGUI_CLAMP_PLAYHEAD,
    self.label = "&Clamp Playhead",
    self.tooltip = "The playhead (draggable icon on timeline) won't be able to exceed the animation length.",
    self.isSizeToText = true
);

IMGUI_ITEM(IMGUI_SETTINGS,
    self.label = "&Settings",
    self.tooltip = "Opens the setting menu, for configuring general program settings.",
    self.popup = "## Settings Popup",
    self.popupType = IMGUI_POPUP_BY_ITEM,
    self.isSizeToText = true
);

IMGUI_ITEM(IMGUI_VSYNC,
    self.label = "&Vsync",
    self.tooltip = "Toggle vertical sync; synchronizes program framerate with your monitor's refresh rate.",
    self.isSizeToText = true
);

IMGUI_ITEM(IMGUI_ANIMATIONS, 
    self.label = "Animations",
    self.flags = ImGuiWindowFlags_NoScrollbar       |
                 ImGuiWindowFlags_NoScrollWithMouse
);
IMGUI_ITEM(IMGUI_ANIMATIONS_CHILD, self.label = "## Animations Child", self.flags = true);

IMGUI_ITEM(IMGUI_ANIMATION,
    self.label = "## Animation Item",
    self.undoAction = "Select Animation",
    self.dragDrop = "## Animation Drag Drop",
    self.atlas = ATLAS_ANIMATION,
    self.idOffset = 2000
);

#define IMGUI_ANIMATIONS_OPTIONS_ROW_COUNT 5
IMGUI_ITEM(IMGUI_ANIMATION_ADD,
    self.label = "Add",
    self.tooltip = "Adds a new animation.",
    self.undoAction = "Add Animation",
    self.rowCount = IMGUI_ANIMATIONS_OPTIONS_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_ANIMATION_DUPLICATE,
    self.label = "Duplicate",
    self.tooltip = "Duplicates the selected animation, placing it after.",
    self.undoAction = "Duplicate Animation",
    self.rowCount = IMGUI_ANIMATIONS_OPTIONS_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_ANIMATION_MERGE,
    self.label = "Merge",
    self.tooltip = "Open the animation merge popup, to merge animations together.",
    self.popup = "Merge Animations",
    self.popupSize = {300, 400},
    self.rowCount = IMGUI_ANIMATIONS_OPTIONS_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_MERGE_ANIMATIONS_CHILD, 
    self.label = "## Merge Animations",
    self.size = {IMGUI_ANIMATION_MERGE.popupSize.x, 250},
    self.flags = true
);

IMGUI_ITEM(IMGUI_MERGE_ON_CONFLICT_CHILD, 
    self.label = "## Merge On Conflict Child",
    self.size = {IMGUI_ANIMATION_MERGE.popupSize.x, 75},
    self.flags = true
);

IMGUI_ITEM(IMGUI_MERGE_ON_CONFLICT, self.label = "On Conflict");

IMGUI_ITEM(IMGUI_MERGE_APPEND_FRAMES,
    self.label = "Append Frames ",
    self.tooltip = "On frame conflict, the merged animation will have the selected animations' frames appended.",
    self.value = ANM2_MERGE_APPEND_FRAMES,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_MERGE_REPLACE_FRAMES,
    self.label = "Replace Frames",
    self.tooltip = "On frame conflict, the merged animation will have the latest selected animations' frames.",
    self.value = ANM2_MERGE_REPLACE_FRAMES
);

IMGUI_ITEM(IMGUI_MERGE_PREPEND_FRAMES,
    self.label = "Prepend Frames",
    self.tooltip = "On frame conflict, the merged animation will have the selected animations' frames prepended.",
    self.value = ANM2_MERGE_PREPEND_FRAMES,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_MERGE_IGNORE,
    self.label = "Ignore        ",
    self.tooltip = "On frame conflict, the merged animation will ignore the other selected animations' frames.",
    self.value = ANM2_MERGE_IGNORE
);

IMGUI_ITEM(IMGUI_MERGE_OPTIONS_CHILD, 
    self.label = "## Merge Options Child",
    self.size = {IMGUI_ANIMATION_MERGE.popupSize.x, 35},
    self.flags = true
);

IMGUI_ITEM(IMGUI_MERGE_DELETE_ANIMATIONS_AFTER, 
    self.label = "Delete Animations After Merging",
    self.tooltip = "After merging, the selected animations (besides the original) will be deleted."
);

IMGUI_ITEM(IMGUI_MERGE_CONFIRM,
    self.label = "Merge",
    self.tooltip = "Merge the selected animations with the options set.",
    self.undoAction = "Merge Animations",
    self.rowCount = IMGUI_OPTION_POPUP_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_ANIMATION_REMOVE,
    self.label = "Remove",
    self.tooltip = "Remove the selected animation.",
    self.undoAction = "Remove Animation",
    self.rowCount = IMGUI_ANIMATIONS_OPTIONS_ROW_COUNT,
    self.chord = ImGuiKey_Delete,
    self.focusWindow = IMGUI_ANIMATIONS.label,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_ANIMATION_DEFAULT,
    self.label = "Default",
    self.tooltip = "Set the selected animation as the default one.",
    self.undoAction = "Default Animation",
    self.rowCount = IMGUI_ANIMATIONS_OPTIONS_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_EVENTS, 
    self.label = "Events",
    self.flags = ImGuiWindowFlags_NoScrollbar       |
                 ImGuiWindowFlags_NoScrollWithMouse
);

IMGUI_ITEM(IMGUI_EVENTS_CHILD, self.label = "## Events Child", self.flags = true);

IMGUI_ITEM(IMGUI_EVENT,
    self.label = "## Event",
    self.atlas = ATLAS_EVENT,
    self.idOffset = 1000
);

#define IMGUI_EVENTS_OPTIONS_ROW_COUNT 2
IMGUI_ITEM(IMGUI_EVENTS_ADD,
    self.label = "Add",
    self.tooltip = "Adds a new event.",
    self.undoAction = "Add Event",
    self.rowCount = IMGUI_EVENTS_OPTIONS_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_EVENTS_REMOVE_UNUSED,
    self.label = "Remove Unused",
    self.tooltip = "Removes all unused events (i.e., not being used in any triggers in any animation).",
    self.undoAction = "Remove Unused Events",
    self.rowCount = IMGUI_EVENTS_OPTIONS_ROW_COUNT
);

IMGUI_ITEM(IMGUI_SPRITESHEETS, 
    self.label = "Spritesheets",
    self.flags = ImGuiWindowFlags_NoScrollbar       |
                 ImGuiWindowFlags_NoScrollWithMouse
);

IMGUI_ITEM(IMGUI_SPRITESHEETS_CHILD, self.label = "## Spritesheets Child", self.flags = true);

IMGUI_ITEM(IMGUI_SPRITESHEET_CHILD, 
    self.label = "## Spritesheet Child",
    self.rowCount = 1,
    self.size = {0, IMGUI_SPRITESHEET_PREVIEW_SIZE.y + 40},
    self.flags = true
);

IMGUI_ITEM(IMGUI_SPRITESHEET_SELECTED, self.label = "## Spritesheet Selected", self.isSameLine = true);

IMGUI_ITEM(IMGUI_SPRITESHEET,
    self.label = "## Spritesheet",
    self.dragDrop = "## Spritesheet Drag Drop",
    self.atlas = ATLAS_SPRITESHEET,
    self.isSizeToText = true
);

IMGUI_ITEM(IMGUI_SPRITESHEETS_FOOTER_CHILD,
    self.label = "## Spritesheets Footer Child",
    self.size = {0, 60},
    self.flags = true
);

#define IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_COUNT 4
#define IMGUI_SPRITESHEETS_OPTIONS_SECOND_ROW_COUNT 3
IMGUI_ITEM(IMGUI_SPRITESHEET_ADD,
    self.label = "Add",
    self.tooltip = "Select an image to add as a spritesheet.",
    self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_SPRITESHEETS_RELOAD,
    self.label = "Reload",
    self.tooltip = "Reload the selected spritesheet.",
    self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_SPRITESHEETS_REPLACE,
    self.label = "Replace",
    self.tooltip = "Replace the highlighted spritesheet with another.",
    self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_SPRITESHEETS_REMOVE_UNUSED,
    self.label = "Remove Unused",
    self.tooltip = "Remove all unused spritesheets in the anm2 (i.e., no layer in any animation uses the spritesheet).",
    self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_COUNT
);

IMGUI_ITEM(IMGUI_SPRITESHEETS_SELECT_ALL,
    self.label = "Select All",
    self.tooltip = "Select all spritesheets.",
    self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_SECOND_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_SPRITESHEETS_SELECT_NONE,
    self.label = "Select None",
    self.tooltip = "Unselect all spritesheets.",
    self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_SECOND_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_SPRITESHEET_SAVE,
    self.label = "Save",
    self.tooltip = "Save the selected spritesheets to their original locations.",
    self.rowCount = IMGUI_SPRITESHEETS_OPTIONS_SECOND_ROW_COUNT
);

const ImVec2 IMGUI_CANVAS_CHILD_SIZE = {230, 85};
IMGUI_ITEM(IMGUI_CANVAS_GRID_CHILD, 
    self.label = "## Canvas Grid Child",
    self.size = IMGUI_CANVAS_CHILD_SIZE,
    self.flags = true
);

IMGUI_ITEM(IMGUI_CANVAS_GRID,
    self.label = "Grid",
    self.tooltip = "Toggles the visiblity of the canvas' grid."
);

IMGUI_ITEM(IMGUI_CANVAS_GRID_SNAP,
    self.label = "Snap",
    self.tooltip = "Using the crop tool will snap the points to the nearest grid point."
);

IMGUI_ITEM(IMGUI_CANVAS_GRID_COLOR,
    self.label = "Color",
    self.tooltip = "Change the color of the canvas' grid.",
    self.flags = ImGuiColorEditFlags_NoInputs
);

IMGUI_ITEM(IMGUI_CANVAS_GRID_SIZE,
    self.label = "Size",
    self.tooltip = "Change the size of the canvas' grid.",
    self.min = CANVAS_GRID_MIN,
    self.max = CANVAS_GRID_MAX,
    self.value = CANVAS_GRID_DEFAULT
);

IMGUI_ITEM(IMGUI_CANVAS_GRID_OFFSET,
    self.label = "Offset",
    self.tooltip = "Change the offset of the canvas' grid, in pixels."
);

IMGUI_ITEM(IMGUI_CANVAS_VIEW_CHILD, 
    self.label = "## View Child",
    self.size = IMGUI_CANVAS_CHILD_SIZE,
    self.flags = true
);

IMGUI_ITEM(IMGUI_CANVAS_ZOOM, 
    self.label = "Zoom",
    self.tooltip = "Change the zoom of the canvas.",
    self.min = CANVAS_ZOOM_MIN,
    self.max = CANVAS_ZOOM_MAX,
    self.value = CANVAS_ZOOM_DEFAULT
);

IMGUI_ITEM(IMGUI_CANVAS_CENTER_VIEW,
    self.label = "Center View",
    self.tooltip = "Centers the current view on the canvas.",
    self.size = {-FLT_MIN, 0}
);

IMGUI_ITEM(IMGUI_CANVAS_VISUAL_CHILD, 
    self.label = "## Animation Preview Visual Child",
    self.size = IMGUI_CANVAS_CHILD_SIZE,
    self.flags = true
);

IMGUI_ITEM(IMGUI_CANVAS_BACKGROUND_COLOR,
    self.label = "Background Color",
    self.tooltip = "Change the background color of the canvas.",
    self.flags = ImGuiColorEditFlags_NoInputs
);

IMGUI_ITEM(IMGUI_CANVAS_ANIMATION_OVERLAY, 
    self.label = "Overlay",
    self.tooltip = "Choose an animation to overlay over the previewed animation, for reference."
);

IMGUI_ITEM(IMGUI_CANVAS_ANIMATION_OVERLAY_TRANSPARENCY,
    self.label = "Alpha",
    self.tooltip = "Set the transparency of the animation overlay.",
    self.value = 255,
    self.max = 255
);

IMGUI_ITEM(IMGUI_CANVAS_HELPER_CHILD, 
    self.label = "## Animation Preview Helper Child",
    self.size = IMGUI_CANVAS_CHILD_SIZE,
    self.flags = true
);

IMGUI_ITEM(IMGUI_CANVAS_AXES,
    self.label = "Axes",
    self.tooltip = "Toggle the display of the X/Y axes."
);

IMGUI_ITEM(IMGUI_CANVAS_AXES_COLOR,
    self.label   = "Color",
    self.tooltip = "Change the color of the axes.",
    self.flags = ImGuiColorEditFlags_NoInputs
);

IMGUI_ITEM(IMGUI_CANVAS_ROOT_TRANSFORM,
    self.label   = "Root Transform",
    self.tooltip = "Toggles the root frames's attributes transforming the other items in an animation.",
    self.value = true
);

IMGUI_ITEM(IMGUI_CANVAS_TRIGGERS,
    self.label   = "Triggers",
    self.tooltip = "Toggles activated triggers drawing their event name.",
    self.value = true
);

IMGUI_ITEM(IMGUI_CANVAS_PIVOTS,
    self.label   = "Pivots",
    self.tooltip = "Toggles drawing each layer's pivot."
);

IMGUI_ITEM(IMGUI_CANVAS_TARGETS,
    self.label   = "Targets",
    self.tooltip = "Toggles drawing the targets (i.e., the colored root/null icons)."
);

IMGUI_ITEM(IMGUI_CANVAS_BORDER,
    self.label   = "Border",
    self.tooltip = "Toggles the appearance of a border around the items."
);

IMGUI_ITEM(IMGUI_ANIMATION_PREVIEW,
    self.label = "Animation Preview",
    self.flags =  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
);

IMGUI_ITEM(IMGUI_SPRITESHEET_EDITOR, 
    self.label = "Spritesheet Editor",
    self.flags =  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES, self.label = "Frame Properties");

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_POSITION,
    self.label = "Position",
    self.tooltip = "Change the position of the selected frame.",
    self.undoAction = "Frame Position",
    self.isUseItemActivated = true
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_CROP,
    self.label = "Crop",
    self.tooltip = "Change the crop position of the selected frame.",
    self.undoAction = "Frame Crop",
    self.isUseItemActivated = true
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_SIZE,
    self.label = "Size",
    self.tooltip = "Change the size of the crop of the selected frame.",
    self.undoAction = "Frame Size",
    self.isUseItemActivated = true
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_PIVOT,
    self.label = "Pivot",
    self.tooltip = "Change the pivot of the selected frame.",
    self.undoAction = "Frame Pivot",
    self.isUseItemActivated = true
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_SCALE,
    self.label = "Scale",
    self.tooltip = "Change the scale of the selected frame.",
    self.undoAction = "Frame Scale",
    self.isUseItemActivated = true,
    self.value = 100
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_ROTATION,
    self.label = "Rotation",
    self.tooltip = "Change the rotation of the selected frame.",
    self.undoAction = "Frame Rotation",
    self.isUseItemActivated = true
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_DELAY,
    self.label = "Duration",
    self.tooltip = "Change the duration of the selected frame.",
    self.undoAction = "Frame Duration",
    self.isUseItemActivated = true,
    self.min = ANM2_FRAME_NUM_MIN,
    self.max = ANM2_FRAME_NUM_MAX,
    self.value = ANM2_FRAME_NUM_MIN
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_TINT,
    self.label = "Tint",
    self.tooltip = "Change the tint of the selected frame.",
    self.undoAction = "Frame Tint",
    self.isUseItemActivated = true,
    self.value = 1
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_COLOR_OFFSET,
    self.label = "Color Offset",
    self.tooltip = "Change the color offset of the selected frame.",
    self.undoAction = "Frame Color Offset",
    self.isUseItemActivated = true,
    self.value = 0
);

const ImVec2 IMGUI_FRAME_PROPERTIES_FLIP_BUTTON_SIZE = {75, 0};
IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_FLIP_X,
    self.label = "Flip X",
    self.tooltip = "Change the sign of the X scale, to cheat flipping the layer horizontally.\n(Anm2 doesn't support flipping directly.)",
    self.undoAction = "Frame Flip X",
    self.size = IMGUI_FRAME_PROPERTIES_FLIP_BUTTON_SIZE,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_FLIP_Y,
    self.label = "Flip Y",
    self.tooltip = "Change the sign of the Y scale, to cheat flipping the layer vertically.\n(Anm2 doesn't support flipping directly.)",
    self.undoAction = "Frame Flip Y",
    self.size = IMGUI_FRAME_PROPERTIES_FLIP_BUTTON_SIZE 
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_VISIBLE,
    self.label = "Visible",
    self.tooltip = "Toggles the visibility of the selected frame.",
    self.undoAction = "Frame Visibility",
    self.isSameLine = true,
    self.value = true
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_INTERPOLATED,
    self.label = "Interpolation",
    self.tooltip = "Toggles the interpolation of the selected frame.",
    self.undoAction = "Frame Interpolation",
    self.value = true
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_EVENT,
    self.label = "Event",
    self.tooltip = "Change the event the trigger uses.",
    self.undoAction = "Trigger Event"
);

IMGUI_ITEM(IMGUI_FRAME_PROPERTIES_AT_FRAME,
    self.label = "At Frame",
    self.tooltip = "Change the frame where the trigger occurs.",
    self.undoAction = "Trigger At Frame"
);

IMGUI_ITEM(IMGUI_TOOLS, self.label = "Tools");

IMGUI_ITEM(IMGUI_TOOL_PAN,
    self.label = "## Pan",
    self.tooltip = "Use the pan tool.\nWill shift the view as the cursor is dragged.\nYou can also use the middle mouse button to pan at any time.",
    self.function = imgui_tool_pan_set,
    self.chord = ImGuiKey_P,
    self.atlas = ATLAS_PAN
);

IMGUI_ITEM(IMGUI_TOOL_MOVE,
    self.label = "## Move",
    self.tooltip = "Use the move tool.\nWill move the selected item as the cursor is dragged, or directional keys are pressed.\n(Animation Preview only.)",
    self.function = imgui_tool_move_set,
    self.chord = ImGuiKey_M,
    self.atlas = ATLAS_MOVE
);

IMGUI_ITEM(IMGUI_TOOL_ROTATE,
    self.label = "## Rotate",
    self.tooltip = "Use the rotate tool.\nWill rotate the selected item as the cursor is dragged, or directional keys are pressed.\n(Animation Preview only.)",
    self.function = imgui_tool_rotate_set,
    self.chord = ImGuiKey_R,
    self.atlas = ATLAS_ROTATE
);

IMGUI_ITEM(IMGUI_TOOL_SCALE,
    self.label = "## Scale",
    self.tooltip = "Use the scale tool.\nWill scale the selected item as the cursor is dragged, or directional keys are pressed.\n(Animation Preview only.)",
    self.function = imgui_tool_scale_set,
    self.chord = ImGuiKey_S,
    self.atlas = ATLAS_SCALE
);

IMGUI_ITEM(IMGUI_TOOL_CROP,
    self.label = "## Crop",
    self.tooltip = "Use the crop tool.\nWill produce a crop rectangle based on how the cursor is dragged.\n(Spritesheet Editor only.)",
    self.function = imgui_tool_crop_set,
    self.chord = ImGuiKey_C,
    self.atlas = ATLAS_CROP
);

IMGUI_ITEM(IMGUI_TOOL_DRAW,
    self.label = "## Draw",
    self.tooltip = "Draws pixels onto the selected spritesheet, with the current color.\n(Spritesheet Editor only.)",
    self.function = imgui_tool_draw_set,
    self.chord = ImGuiKey_B,
    self.atlas = ATLAS_DRAW
);

IMGUI_ITEM(IMGUI_TOOL_ERASE,
    self.label = "## Erase",
    self.tooltip = "Erases pixels from the selected spritesheet.\n(Spritesheet Editor only.)",
    self.function = imgui_tool_erase_set,
    self.chord = ImGuiKey_E,
    self.atlas = ATLAS_ERASE
);

IMGUI_ITEM(IMGUI_TOOL_COLOR_PICKER,
    self.label = "## Color Picker",
    self.tooltip = "Selects a color from the canvas, to be used for drawing.\n(Spritesheet Editor only).",
    self.function = imgui_tool_color_picker_set,
    self.chord = ImGuiKey_W,
    self.atlas = ATLAS_COLOR_PICKER
);

IMGUI_ITEM(IMGUI_TOOL_UNDO,
    self.label = "## Undo",
    self.tooltip = "Undoes the last action.",
    self.function = imgui_undo,
    self.chord = ImGuiKey_Z,
    self.atlas = ATLAS_UNDO
);

IMGUI_ITEM(IMGUI_TOOL_REDO,
    self.label = "## Redo",
    self.tooltip = "Redoes the last action.",
    self.function = imgui_redo,
    self.chord = ImGuiMod_Shift + ImGuiKey_Z,
    self.atlas = ATLAS_REDO
);

IMGUI_ITEM(IMGUI_TOOL_COLOR,
    self.label = "## Color",
    self.tooltip = "Set the color, to be used by the draw tool.",
    self.flags = ImGuiColorEditFlags_NoInputs
);

const inline ImguiItem* IMGUI_TOOL_ITEMS[TOOL_COUNT] = 
{
    &IMGUI_TOOL_PAN,
    &IMGUI_TOOL_MOVE,
    &IMGUI_TOOL_ROTATE,
    &IMGUI_TOOL_SCALE,
    &IMGUI_TOOL_CROP,
    &IMGUI_TOOL_DRAW,
    &IMGUI_TOOL_ERASE,
    &IMGUI_TOOL_COLOR_PICKER,
    &IMGUI_TOOL_UNDO,
    &IMGUI_TOOL_REDO,
    &IMGUI_TOOL_COLOR
};

IMGUI_ITEM(IMGUI_COLOR_PICKER_BUTTON, self.label = "## Color Picker Button");

IMGUI_ITEM(IMGUI_TIMELINE,
    self.label = "Timeline",
    self.flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
);

IMGUI_ITEM(IMGUI_TIMELINE_CHILD, 
    self.label = "## Timeline Child",
    self.flags = true
);

IMGUI_ITEM(IMGUI_TIMELINE_HEADER_CHILD,
    self.label = "## Timeline Header Child",
    self.size = {0, IMGUI_TIMELINE_FRAME_SIZE.y},
    self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
);

IMGUI_ITEM(IMGUI_PLAYHEAD,
    self.label = "## Playhead",
    self.flags = ImGuiWindowFlags_NoTitleBar       | 
                 ImGuiWindowFlags_NoResize         |
		         ImGuiWindowFlags_NoMove           | 
                 ImGuiWindowFlags_NoBackground     |
		         ImGuiWindowFlags_NoInputs
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEMS_CHILD,
    self.label = "## Timeline Items",
    self.size = {IMGUI_TIMELINE_ITEM_SIZE.x, 0},
    self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_CHILD,
    self.label = "## Timeline Item Child",
    self.size = IMGUI_TIMELINE_ITEM_SIZE,
    self.flags = true,
    self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_ROOT_CHILD,
    self.label = "## Root Item Child",
    self.color = {{0.045f, 0.08f, 0.11f, 1.0f}},
    self.size = IMGUI_TIMELINE_ITEM_SIZE,
    self.flags = true,
    self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_LAYER_CHILD,
    self.label = "## Layer Item Child",
    self.color = {{0.0875f, 0.05f, 0.015f, 1.0f}},
    self.size = IMGUI_TIMELINE_ITEM_SIZE,
    self.flags = true,
    self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_NULL_CHILD,
    self.label = "## Null Item Child",
    self.color = {{0.055f, 0.10f, 0.055f, 1.0f}},
    self.size = IMGUI_TIMELINE_ITEM_SIZE,
    self.flags = true,
    self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_TRIGGERS_CHILD,
    self.label = "## Triggers Item Child",
    self.color = {{0.10f, 0.0375f, 0.07f, 1.0f}},
    self.size = IMGUI_TIMELINE_ITEM_SIZE,
    self.flags = true,
    self.windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
);

const inline ImguiItem* IMGUI_TIMELINE_ITEM_CHILDS[ANM2_COUNT]
{
    &IMGUI_TIMELINE_ITEM_CHILD,
    &IMGUI_TIMELINE_ITEM_ROOT_CHILD,
    &IMGUI_TIMELINE_ITEM_LAYER_CHILD,
    &IMGUI_TIMELINE_ITEM_NULL_CHILD,
    &IMGUI_TIMELINE_ITEM_TRIGGERS_CHILD
};

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_SELECTABLE,
    self.label = "## Selectable",
    self.size = IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_ROOT_SELECTABLE,
    self.label = "Root",
    self.tooltip = "The root item of an animation.\nChanging its properties will transform the rest of the animation.",
    self.undoAction = "Root Item Select",
    self.atlas = ATLAS_ROOT,
    self.size = IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_LAYER_SELECTABLE,
    self.label = "## Layer Selectable",
    self.tooltip = "A layer item.\nA graphical item within the animation.",
    self.undoAction = "Layer Item Select",
    self.dragDrop = "## Layer Drag Drop",
    self.atlas = ATLAS_LAYER,
    self.size = IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_NULL_SELECTABLE,
    self.label = "## Null Selectable",
    self.tooltip = "A null item.\nAn invisible item within the animation that is accessible via a game engine.",
    self.undoAction = "Null Item Select",
    self.dragDrop = "## Null Drag Drop",
    self.atlas = ATLAS_NULL,
    self.size = IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_TRIGGERS_SELECTABLE,
    self.label = "Triggers",
    self.tooltip = "The animation's triggers.\nWill fire based on an event.",
    self.undoAction = "Triggers Item Select",
    self.atlas = ATLAS_TRIGGERS,
    self.size = IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE
);

const inline ImguiItem* IMGUI_TIMELINE_ITEM_SELECTABLES[ANM2_COUNT]
{
    &IMGUI_TIMELINE_ITEM_SELECTABLE,
    &IMGUI_TIMELINE_ITEM_ROOT_SELECTABLE,
    &IMGUI_TIMELINE_ITEM_LAYER_SELECTABLE,
    &IMGUI_TIMELINE_ITEM_NULL_SELECTABLE,
    &IMGUI_TIMELINE_ITEM_TRIGGERS_SELECTABLE
};

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_VISIBLE,
    self.label = "## Visible",
    self.tooltip = "The item is visible.\nPress to set to invisible.",
    self.undoAction = "Item Invisible",
    self.atlas = ATLAS_VISIBLE
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_INVISIBLE,
    self.label = "## Invisible",
    self.tooltip = "The item is invisible.\nPress to set to visible.",
    self.undoAction = "Item Visible",
    self.atlas = ATLAS_INVISIBLE
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_SHOW_RECT,
    self.label = "## Show Rect",
    self.tooltip = "The rect is shown.\nPress to hide rect.",
    self.undoAction = "Hide Rect",
    self.atlas = ATLAS_SHOW_RECT
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_HIDE_RECT,
    self.label = "## Hide Rect",
    self.tooltip = "The rect is hidden.\nPress to show rect.",
    self.undoAction = "Show Rect",
    self.atlas = ATLAS_HIDE_RECT
);

IMGUI_ITEM(IMGUI_TIMELINE_SPRITESHEET_ID,
    self.label = "## Spritesheet ID",
    self.tooltip = "Change the spritesheet ID this item uses.",
    self.atlas = ATLAS_SPRITESHEET,
    self.size = {32, 0}
);

IMGUI_ITEM(IMGUI_TIMELINE_FRAMES_CHILD, 
    self.label = "## Timeline Frames Child",
    self.windowFlags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_HorizontalScrollbar
);

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_FRAMES_CHILD,
    self.label = "## Timeline Item Frames Child",
    self.size = {0, IMGUI_TIMELINE_FRAME_SIZE.y}
);

IMGUI_ITEM(IMGUI_TIMELINE_FRAME, self.label = "## Frame");

#define IMGUI_TIMELINE_FRAME_BORDER 5
static const vec4 IMGUI_FRAME_BORDER_COLOR = {1.0f, 1.0f, 1.0f, 0.25f};
IMGUI_ITEM(IMGUI_TIMELINE_ROOT_FRAME,
    self.label = "## Root Frame",
    self.undoAction = "Root Frame Select",
    self.color = {{0.14f, 0.27f, 0.39f, 1.0f}, {0.28f, 0.54f, 0.78f, 1.0f}, {0.36f, 0.70f, 0.95f, 1.0f}, IMGUI_FRAME_BORDER_COLOR},
    self.size = IMGUI_TIMELINE_FRAME_SIZE,
    self.atlasOffset = IMGUI_TIMELINE_FRAME_ATLAS_OFFSET,
    self.border = IMGUI_FRAME_BORDER
);

IMGUI_ITEM(IMGUI_TIMELINE_LAYER_FRAME,
    self.label = "## Layer Frame",
    self.undoAction = "Layer Frame Select",
    self.dragDrop = "## Layer Frame Drag Drop",
    self.color = {{0.45f, 0.18f, 0.07f, 1.0f}, {0.78f, 0.32f, 0.12f, 1.0f}, {0.95f, 0.40f, 0.15f, 1.0f}, IMGUI_FRAME_BORDER_COLOR},
    self.size = IMGUI_TIMELINE_FRAME_SIZE,
    self.atlasOffset = IMGUI_TIMELINE_FRAME_ATLAS_OFFSET,
    self.border = IMGUI_FRAME_BORDER
);

IMGUI_ITEM(IMGUI_TIMELINE_NULL_FRAME,
    self.label = "## Null Frame",
    self.undoAction = "Null Frame Select",
    self.dragDrop = "## Null Frame Drag Drop",
    self.color = {{0.17f, 0.33f, 0.17f, 1.0f}, {0.34f, 0.68f, 0.34f, 1.0f}, {0.44f, 0.88f, 0.44f, 1.0f}, IMGUI_FRAME_BORDER_COLOR},
    self.size = IMGUI_TIMELINE_FRAME_SIZE,
    self.atlasOffset = IMGUI_TIMELINE_FRAME_ATLAS_OFFSET,
    self.border = IMGUI_FRAME_BORDER
);

IMGUI_ITEM(IMGUI_TIMELINE_TRIGGERS_FRAME,
    self.label = "## Triggers Frame",
    self.undoAction = "Trigger Select",
    self.color = {{0.36f, 0.14f, 0.24f, 1.0f}, {0.72f, 0.28f, 0.48f, 1.0f}, {0.92f, 0.36f, 0.60f, 1.0f}, IMGUI_FRAME_BORDER_COLOR},
    self.size = IMGUI_TIMELINE_FRAME_SIZE,
    self.atlasOffset = IMGUI_TIMELINE_FRAME_ATLAS_OFFSET,
    self.border = IMGUI_FRAME_BORDER
);

const inline ImguiItem* IMGUI_TIMELINE_FRAMES[ANM2_COUNT]
{
    &IMGUI_TIMELINE_FRAME,
    &IMGUI_TIMELINE_ROOT_FRAME,
    &IMGUI_TIMELINE_LAYER_FRAME,
    &IMGUI_TIMELINE_NULL_FRAME,
    &IMGUI_TIMELINE_TRIGGERS_FRAME
};

IMGUI_ITEM(IMGUI_TIMELINE_ITEM_FOOTER_CHILD, 
    self.label = "## Item Footer Child", 
    self.size = {IMGUI_TIMELINE_ITEM_CHILD.size.x, IMGUI_FOOTER_CHILD.size.y},
    self.flags = true
);
IMGUI_ITEM(IMGUI_TIMELINE_OPTIONS_FOOTER_CHILD, 
    self.label = "## Options Footer Child",
    self.size = {0, IMGUI_FOOTER_CHILD.size.y},
    self.flags = true
);

#define IMGUI_TIMELINE_FOOTER_ITEM_CHILD_ITEM_COUNT 2
IMGUI_ITEM(IMGUI_TIMELINE_ADD_ITEM,
    self.label = "Add",
    self.tooltip = "Adds an item (layer or null) to the animation.",
    self.popup = "## Add Item Popup",
    self.popupType = IMGUI_POPUP_BY_ITEM,
    self.rowCount = IMGUI_TIMELINE_FOOTER_ITEM_CHILD_ITEM_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_TIMELINE_ADD_ITEM_LAYER,
    self.label = "Layer",
    self.tooltip = "Adds a layer item.\nA layer item is a primary graphical item, using a spritesheet.",
    self.undoAction = "Add Layer",
    self.isSizeToText = true
);

IMGUI_ITEM(IMGUI_TIMELINE_ADD_ITEM_NULL,
    self.label = "Null",
    self.tooltip = "Adds a null item.\nA null item is an invisible item, often accessed by the game engine.",
    self.undoAction = "Add Null",
    self.isSizeToText = true
);

IMGUI_ITEM(IMGUI_TIMELINE_REMOVE_ITEM,
    self.label = "Remove",
    self.tooltip = "Removes the selected item (layer or null) from the animation.",
    self.undoAction = "Remove Item",
    self.focusWindow = IMGUI_TIMELINE.label,
    self.rowCount = IMGUI_TIMELINE_FOOTER_ITEM_CHILD_ITEM_COUNT
);

#define IMGUI_TIMELINE_OPTIONS_ROW_COUNT 10
IMGUI_ITEM(IMGUI_PLAY,
    self.label = "|>  Play",
    self.tooltip = "Play the current animation, if paused.",
    self.focusWindow = IMGUI_TIMELINE.label,
    self.chord = ImGuiKey_Space,
    self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_PAUSE,
    self.label = "|| Pause",
    self.tooltip = "Pause the current animation, if playing.",
    self.focusWindow = IMGUI_TIMELINE.label,
    self.chord = ImGuiKey_Space,
    self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_ADD_FRAME,
    self.label = "+ Insert Frame",
    self.tooltip = "Inserts a frame in the selected animation item, based on the preview time.",
    self.undoAction = "Insert Frame",
    self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_REMOVE_FRAME,
    self.label = "- Delete Frame",
    self.tooltip = "Removes the selected frame from the selected animation item.",
    self.undoAction = "Delete Frame",
    self.focusWindow = IMGUI_TIMELINE.label,
    self.chord = ImGuiKey_Delete,
    self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_BAKE,
    self.label = "Bake",
    self.tooltip = "Opens the bake popup menu, if a frame is selected.\nBaking a frame takes the currently interpolated values at the time between it and the next frame and separates them based on the interval.",
    self.popup = "Bake Frames",
    self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT,
    self.popupSize = {260, 145},
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_BAKE_CHILD,
    self.label = "## Bake Child",
    self.flags = true
);

IMGUI_ITEM(IMGUI_BAKE_INTERVAL,
    self.label = "Interval",
    self.tooltip = "Sets the delay of the baked frames the selected frame will be separated out into.",
    self.min = ANM2_FRAME_DELAY_MIN,
    self.value = ANM2_FRAME_DELAY_MIN
);

IMGUI_ITEM(IMGUI_BAKE_ROUND_SCALE,
    self.label = "Round Scale",
    self.tooltip = "The scale of the baked frames will be rounded to the nearest integer.",
    self.value = true
);

IMGUI_ITEM(IMGUI_BAKE_ROUND_ROTATION,
    self.label = "Round Rotation",
    self.tooltip = "The rotation of the baked frames will be rounded to the nearest integer.",
    self.value = true,
    self.isSeparator = true
);

IMGUI_ITEM(IMGUI_BAKE_CONFIRM,
    self.label = "Bake",
    self.tooltip = "Bake the selected frame with the options selected.",
    self.undoAction = "Bake Frames",
    self.rowCount = IMGUI_OPTION_POPUP_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_FIT_ANIMATION_LENGTH, 
    self.label = "Fit Animation Length",
    self.tooltip = "Sets the animation's length to the latest frame.",
    self.undoAction = "Fit Animation Length",
    self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_ANIMATION_LENGTH, 
    self.label = "Length",
    self.tooltip = "Sets the animation length.\n(Will not change frames.)",
    self.undoAction = "Set Animation Length",
    self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT,
    self.min = ANM2_FRAME_NUM_MIN,
    self.max = ANM2_FRAME_NUM_MAX,
    self.value = ANM2_FRAME_NUM_MIN,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_FPS, 
    self.label = "FPS",
    self.tooltip = "Sets the animation's frames per second (its speed).",
    self.undoAction = "Set FPS",
    self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT,
    self.min = ANM2_FPS_MIN,
    self.max = ANM2_FPS_MAX,
    self.value = ANM2_FPS_DEFAULT,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_LOOP,
    self.label = "Loop",
    self.tooltip = "Toggles the animation looping.",
    self.undoAction = "Set Loop",
    self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT,
    self.value = true,
    self.isSameLine = true
);

IMGUI_ITEM(IMGUI_CREATED_BY,
    self.label = "Author",
    self.tooltip = "Sets the author of the animation.",
    self.rowCount = IMGUI_TIMELINE_OPTIONS_ROW_COUNT,
    self.max = 255
);

IMGUI_ITEM(IMGUI_CONTEXT_MENU, self.label = "## Context Menu");

IMGUI_ITEM(IMGUI_CUT,
    self.label = "Cut",
    self.tooltip = "Cuts the currently selected contextual element; removing it and putting it to the clipboard.",
    self.undoAction = "Cut",
    self.function = imgui_cut,
    self.chord = ImGuiMod_Ctrl | ImGuiKey_X,
    self.isSizeToText = true
);

IMGUI_ITEM(IMGUI_COPY,
    self.label = "Copy",
    self.tooltip = "Copies the currently selected contextual element to the clipboard.",
    self.undoAction = "Copy",
    self.function = imgui_copy,
    self.chord = ImGuiMod_Ctrl | ImGuiKey_C,
    self.isSizeToText = true
);

IMGUI_ITEM(IMGUI_PASTE,
    self.label = "Paste",
    self.tooltip = "Pastes the currently selection contextual element from the clipboard.",
    self.undoAction = "Paste",
    self.function = imgui_paste,
    self.chord = ImGuiMod_Ctrl | ImGuiKey_V,
    self.isSizeToText = true
);

IMGUI_ITEM(IMGUI_CHANGE_INPUT_TEXT,
    self.label = "## Input Text",
    self.tooltip = "Rename the selected item.",
    self.undoAction = "Rename Item",
    self.flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue,
    self.max = 255
);

IMGUI_ITEM(IMGUI_CHANGE_INPUT_INT,
    self.label = "## Input Int",
    self.tooltip = "Change the selected item's value.",
    self.undoAction = "Change Value",
    self.step = 0
);



#define IMGUI_OPTION_POPUP_ROW_COUNT 2
IMGUI_ITEM(IMGUI_POPUP_OK,
    self.label = "OK",
    self.tooltip = "Confirm the action.",
    self.rowCount = IMGUI_OPTION_POPUP_ROW_COUNT
);

IMGUI_ITEM(IMGUI_POPUP_CANCEL,
    self.label = "Cancel",
    self.tooltip = "Cancel the action.",
    self.rowCount = IMGUI_OPTION_POPUP_ROW_COUNT
);

IMGUI_ITEM(IMGUI_LOG_WINDOW, 
    self.label = "## Log Window",
    self.flags = ImGuiWindowFlags_NoTitleBar 		 |
                 ImGuiWindowFlags_NoResize 			 |
                 ImGuiWindowFlags_NoMove 			 |
                 ImGuiWindowFlags_NoScrollbar 		 |
                 ImGuiWindowFlags_NoSavedSettings 	 |
                 ImGuiWindowFlags_AlwaysAutoResize   |
                 ImGuiWindowFlags_NoFocusOnAppearing |
                 ImGuiWindowFlags_NoNav 			 |
                 ImGuiWindowFlags_NoInputs
);

void imgui_init
(
    Imgui* self,
    Dialog* dialog,
    Resources* resources,
    Anm2* anm2,
    Anm2Reference* reference,
    Editor* editor,
    Preview* preview,
    GeneratePreview* generatePreview,
    Settings* settings,
    Snapshots* snapshots,
    Clipboard* clipboard,
    SDL_Window* window,
    SDL_GLContext* glContext
);

void imgui_update(Imgui* self);
void imgui_draw();
void imgui_free();