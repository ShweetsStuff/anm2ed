#pragma once

#include "clipboard.h"
#include "dialog.h"
#include "editor.h"
#include "preview.h"
#include "resources.h"
#include "settings.h"
#include "snapshots.h"
#include "tool.h"
#include "window.h"

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#define IMGUI_ENABLE_DOCKING
#define IM_VEC2_CLASS_EXTRA                                                                   \
    inline bool operator==(const ImVec2& rhs) const { return x == rhs.x && y == rhs.y; }      \
    inline bool operator!=(const ImVec2& rhs) const { return !(*this == rhs); }               \
    inline ImVec2 operator+(const ImVec2& rhs) const { return ImVec2(x + rhs.x, y + rhs.y); } \
    inline ImVec2 operator-(const ImVec2& rhs) const { return ImVec2(x - rhs.x, y - rhs.y); } \
    inline ImVec2 operator*(const ImVec2& rhs) const { return ImVec2(x * rhs.x, y * rhs.y); } \
    inline ImVec2(const vec2& v) : x(v.x), y(v.y) {}                                          \
    inline operator vec2() const { return vec2(x, y); }
    
#define IM_VEC4_CLASS_EXTRA                                                                                          \
    inline bool operator==(const ImVec4& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; } \
    inline bool operator!=(const ImVec4& rhs) const { return !(*this == rhs); }                                      \
    inline ImVec4 operator+(const ImVec4& rhs) const { return ImVec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }  \
    inline ImVec4 operator-(const ImVec4& rhs) const { return ImVec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }  \
    inline ImVec4 operator*(const ImVec4& rhs) const { return ImVec4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }  \
    inline ImVec4(const vec4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}                                                 \
    inline operator vec4() const { return vec4(x, y, z, w); }
    
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#define IMGUI_ANIMATION_DEFAULT_FORMAT "(*) {}"
#define IMGUI_CHORD_NONE (ImGuiMod_None)
#define IMGUI_EVENT_NONE "None"
#define IMGUI_FRAME_BORDER 2.0f
#define IMGUI_FRAME_PROPERTIES_NO_FRAME "Select a frame to show properties..."
#define IMGUI_ITEM_SELECTABLE_EDITABLE_LABEL "## Editing"
#define IMGUI_OPENGL_VERSION "#version 330"
#define IMGUI_PICKER_LINE_COLOR IM_COL32(255, 255, 255, 255)
#define IMGUI_POSITION_FORMAT "Position: {{{:6}, {:6}}}"
#define IMGUI_SPRITESHEET_FORMAT "#{} {}"
#define IMGUI_TIMELINE_CHILD_ID_LABEL "#{} {}"
#define IMGUI_TIMELINE_FOOTER_HEIGHT 40
#define IMGUI_ANIMATIONS_FOOTER_HEIGHT 40
#define IMGUI_EVENTS_FOOTER_HEIGHT 40
#define IMGUI_SPRITESHEETS_FOOTER_HEIGHT 65
#define IMGUI_TIMELINE_FRAME_BORDER 2
#define IMGUI_TIMELINE_FRAME_LABEL_FORMAT "## {}"
#define IMGUI_TIMELINE_FRAME_MULTIPLE 5
#define IMGUI_TIMELINE_MERGE
#define IMGUI_TIMELINE_NO_ANIMATION "Select an animation to show timeline..."
#define IMGUI_TIMELINE_PICKER_LINE_WIDTH 2.0f
#define IMGUI_TIMELINE_SPRITESHEET_ID_FORMAT "#{}"

#define IMGUI_INVISIBLE_LABEL_MARKER "##"
#define IMGUI_ANIMATIONS_OPTIONS_ROW_ITEM_COUNT 5
#define IMGUI_EVENTS_OPTIONS_ROW_ITEM_COUNT 2
#define IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_ITEM_COUNT 4
#define IMGUI_SPRITESHEETS_OPTIONS_SECOND_ROW_ITEM_COUNT 3
#define IMGUI_TIMELINE_FOOTER_ITEM_CHILD_ITEM_COUNT 2
#define IMGUI_TIMELINE_BAKE_OPTIONS_CHILD_ROW_ITEM_COUNT 2
#define IMGUI_TIMELINE_MERGE_OPTIONS_ROW_ITEM_COUNT 2
#define IMGUI_MESSAGE_DURATION 3.0f
#define IMGUI_MESSAGE_PADDING 10.0f
#define IMGUI_MESSAGE_FORMAT "## Message {}"
#define IMGUI_MESSAGE_UNDO_FORMAT "Undo: {}"
#define IMGUI_MESSAGE_REDO_FORMAT "Redo: {}"

#define IMGUI_ACTION_FILE_SAVE_FORMAT "Saved anm2 to: {}" 
#define IMGUI_ACTION_FILE_OPEN_FORMAT "Opened anm2: {}" 
#define IMGUI_ACTION_SPRITESHEET_SAVE_FORMAT "Saved spritesheet #{} to: {}" 
#define IMGUI_ACTION_TRIGGER_MOVE "Trigger AtFrame"
#define IMGUI_ACTION_FRAME_SWAP "Frame Swap"
#define IMGUI_ACTION_ANIMATION_SWAP "Animation Swap"
#define IMGUI_ACTION_FRAME_TRANSFORM "Frame Transform"
#define IMGUI_ACTION_FRAME_CROP "Frame Crop"

#define IMGUI_SPACING 4

const ImVec2 IMGUI_TIMELINE_FRAME_SIZE = {16, 40};
const ImVec2 IMGUI_TIMELINE_FRAME_CONTENT_OFFSET = {TEXTURE_SIZE_SMALL.x * 0.25f, (IMGUI_TIMELINE_FRAME_SIZE.y * 0.5f) - (TEXTURE_SIZE_SMALL.y * 0.5f)};
const ImVec2 IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE = {150, 0};
const ImVec2 IMGUI_TIMELINE_ITEM_SIZE = {300, 40};

const ImVec4 IMGUI_TIMELINE_FRAME_COLOR = {0.0f, 0.0f, 0.0f, 0.125};
const ImVec4 IMGUI_TIMELINE_FRAME_MULTIPLE_COLOR = {0.113, 0.184, 0.286, 0.125};
const ImVec4 IMGUI_TIMELINE_HEADER_FRAME_COLOR = {0.113, 0.184, 0.286, 0.5};
const ImVec4 IMGUI_TIMELINE_HEADER_FRAME_MULTIPLE_COLOR = {0.113, 0.184, 0.286, 1.0};
const ImVec4 IMGUI_TIMELINE_HEADER_FRAME_INACTIVE_COLOR = {0.113, 0.184, 0.286, 0.125};
const ImVec4 IMGUI_TIMELINE_HEADER_FRAME_MULTIPLE_INACTIVE_COLOR = {0.113, 0.184, 0.286, 0.25};
const ImVec4 IMGUI_INACTIVE_COLOR = {0.5, 0.5, 0.5, 1.0};
const ImVec4 IMGUI_ACTIVE_COLOR = {1.0, 1.0, 1.0, 1.0};

const ImVec2 IMGUI_FRAME_PROPERTIES_FLIP_BUTTON_SIZE = {100, 0};

const ImVec2 IMGUI_SPRITESHEET_PREVIEW_SIZE = {125.0, 125.0};
const ImVec2 IMGUI_TOOLTIP_OFFSET = {16, 8};
const vec2 IMGUI_SPRITESHEET_EDITOR_CROP_FORGIVENESS = {1, 1};
const ImVec2 IMGUI_CANVAS_CHILD_SIZE = {200, 85};

const ImGuiKey IMGUI_INPUT_DELETE = ImGuiKey_Delete;
const ImGuiKey IMGUI_INPUT_LEFT = ImGuiKey_LeftArrow;
const ImGuiKey IMGUI_INPUT_RIGHT = ImGuiKey_RightArrow;
const ImGuiKey IMGUI_INPUT_UP = ImGuiKey_UpArrow;
const ImGuiKey IMGUI_INPUT_DOWN = ImGuiKey_DownArrow;
const ImGuiKey IMGUI_INPUT_SHIFT = ImGuiMod_Shift;
const ImGuiKey IMGUI_INPUT_CTRL = ImGuiMod_Ctrl;
const ImGuiKey IMGUI_INPUT_ZOOM_IN = ImGuiKey_1;
const ImGuiKey IMGUI_INPUT_ZOOM_OUT = ImGuiKey_2;

inline const std::string IMGUI_FRAME_PROPERTIES_TITLE[ANM2_COUNT] =
{
    "",
    "-- Root --",
    "-- Layer --",
    "-- Null --",
    "-- Trigger --",
};

static inline ImGuiKey imgui_key_get(char c) 
{
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    if (c >= 'A' && c <= 'Z') return static_cast<ImGuiKey>(ImGuiKey_A + (c - 'A'));
    return ImGuiKey_None;
}

struct ImguiMessage
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
    Settings* settings = nullptr;
    Snapshots* snapshots = nullptr;
    Clipboard* clipboard = nullptr;
    SDL_Window* window = nullptr;
    SDL_GLContext* glContext = nullptr;
    std::vector<ImguiMessage> messageQueue;
    bool isHotkeysEnabled = true;
    bool isRename = false;
    bool isChangeValue = false;
    bool isQuit = false;
};

typedef void(*ImguiFunction)(Imgui*);

struct ImguiHotkey
{
    ImGuiKeyChord chord;
    ImguiFunction function;
    std::string focusWindow{};

    bool is_focus_window() const { return !focusWindow.empty(); }
};

static std::vector<ImguiHotkey>& imgui_hotkey_registry()
{
    static std::vector<ImguiHotkey> registry;
    return registry;
}

enum PopupType
{
    IMGUI_POPUP_BY_ITEM,
    IMGUI_POPUP_CENTER_SCREEN
};

struct ImguiColorSet 
{
    ImVec4 normal{};
    ImVec4 active{};
    ImVec4 hovered{};

    bool is_normal() const { return normal != ImVec4(); }
    bool is_active() const { return active != ImVec4(); }
    bool is_hovered() const { return hovered != ImVec4(); }
};

struct ImguiItemBuilder
{
    std::string label{};
    std::string tooltip{};
    std::string action = SNAPSHOT_ACTION;
    std::string popup{};
    std::string dragDrop{};
    std::string format = "%.1f";
    std::string focusWindow{};
    ImguiFunction function = nullptr;
    ImGuiKeyChord chord = IMGUI_CHORD_NONE;
    TextureType texture = TEXTURE_NONE;
    PopupType popupType = IMGUI_POPUP_BY_ITEM;
    bool isUndoable = false;
    bool isSizeToText = true;
    bool isSizeToChild = false;
    ImguiColorSet color{};
    ImVec2 size{};
    ImVec2 contentOffset{};
    s32 childRowItemCount{};
    f64 speed{};
    f64 min{};
    f64 max{};
    s32 border{};
    s32 step = 1;
    s32 stepFast = 1;
    s32 value = ID_NONE;
    s32 id = ID_NONE;
    s32 flags{};
    s32 flagsAlt{};
};

struct ImguiItem
{
    std::string label{};
    std::string tooltip{};
    std::string action = SNAPSHOT_ACTION;
    std::string popup{};
    std::string dragDrop{};
    std::string format = "%.1f";
    std::string focusWindow{};
    ImguiFunction function = nullptr;
    ImGuiKeyChord chord = IMGUI_CHORD_NONE;
    TextureType texture = TEXTURE_NONE;
    PopupType popupType = IMGUI_POPUP_BY_ITEM;
    bool isUndoable = false;
    bool isInactive = false;
    bool isSizeToText = true;
    bool isSizeToChild = true;
    bool isSelected = false;
    f64 speed{};
    f64 min{};
    f64 max{};
    s32 border{};
    s32 childRowItemCount = 1;
    s32 step = 1;
    s32 stepFast = 1;
    s32 value = ID_NONE;
    s32 id{};
    s32 flags{};
    s32 flagsAlt{};
    ImguiColorSet color{};
    ImVec2 size{};
    ImVec2 contentOffset{};
    ImGuiKey mnemonicKey = ImGuiKey_None;
    s32 mnemonicIndex = -1;
 
    bool is_tooltip() const { return !tooltip.empty(); }
    bool is_popup() const { return !popup.empty(); }
    bool is_action() const { return !action.empty(); }
    bool is_size() const { return size != ImVec2(); }
    bool is_border() const { return border != 0; }
    bool is_mnemonic() const { return mnemonicIndex != -1 && mnemonicKey != ImGuiKey_None; }
    bool is_chord() const { return chord != IMGUI_CHORD_NONE;}
    bool is_focus_window() const { return !focusWindow.empty(); }
    
    ImguiItem(const ImguiItemBuilder& builder)
    {
        static s32 newID = 0;

        tooltip       = builder.tooltip;
        popup         = builder.popup;
        action        = builder.action;
        dragDrop      = builder.dragDrop;
        format        = builder.format;
        focusWindow   = builder.focusWindow;
        function      = builder.function;
        chord         = builder.chord;
        popupType     = builder.popupType;
        texture       = builder.texture;
        isUndoable    = builder.isUndoable;
        isSizeToText  = builder.isSizeToText;
        isSizeToChild = builder.isSizeToChild;
        color         = builder.color;
        size          = builder.size;
        contentOffset = builder.contentOffset;
        speed         = builder.speed;
        min           = builder.min;
        max           = builder.max;
        border        = builder.border;
        childRowItemCount = builder.childRowItemCount;
        step          = builder.step;
        stepFast      = builder.stepFast;
        value         = builder.value;
        flags         = builder.flags;
        flagsAlt      = builder.flagsAlt;

        id = newID;
        newID++;

        if (is_chord() && function) 
            imgui_hotkey_registry().push_back({chord, function, focusWindow});
        
        mnemonicIndex = -1;

        for (s32 i = 0; i < (s32)builder.label.size(); i++)
        {
            if (builder.label[i] == '&')
            {
                if (builder.label[i + 1] == '&')
                {
                    label += '&';
                    i++;
                }
                else if (builder.label[i + 1] != '\0')
                {
                    mnemonicKey = imgui_key_get(builder.label[i + 1]);
                    mnemonicIndex = (s32)label.size();
                    label += builder.label[i + 1];
                    i++;
                }
            }
            else 
                label += builder.label[i];
        }
    }
};

static void imgui_message_queue_push(Imgui* self, const std::string& text)
{
    self->messageQueue.push_back({text, IMGUI_MESSAGE_DURATION});
}

static inline std::string_view imgui_nav_window_root_get()
{
    ImGuiWindow* navWindow = ImGui::GetCurrentContext()->NavWindow;
    if (!navWindow)
        return {};

    std::string_view name(navWindow->Name);
    size_t slash = name.find('/');
    return (slash == std::string_view::npos) ? name : name.substr(0, slash);
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
        imgui_message_queue_push(self, std::format(IMGUI_ACTION_FILE_SAVE_FORMAT, self->anm2->path));
    }
}

static inline void imgui_file_save_as(Imgui* self)
{
	dialog_anm2_save(self->dialog);
}

static inline void imgui_png_open(Imgui* self)
{
    dialog_png_open(self->dialog);
}

static inline void imgui_generate_gif_animation(Imgui* self)
{
    if (anm2_animation_from_reference(self->anm2, self->reference))
    {
        self->preview->isRecording = true;
        self->preview->time = 0;
    }
}

static inline void imgui_undo_stack_push(Imgui* self, const std::string& action = SNAPSHOT_ACTION)
{
    Snapshot snapshot = {*self->anm2, *self->reference, self->preview->time, action};
    snapshots_undo_stack_push(self->snapshots, &snapshot);
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
    imgui_message_queue_push(self, std::format(IMGUI_MESSAGE_UNDO_FORMAT, self->snapshots->action));
}

static inline void imgui_redo(Imgui* self)
{
    if (self->snapshots->redoStack.top == 0) return;
    
    std::string action = self->snapshots->action;
    snapshots_redo(self->snapshots);
    imgui_message_queue_push(self, std::format(IMGUI_MESSAGE_REDO_FORMAT, action));
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

const inline ImguiItem IMGUI_WINDOW = ImguiItem
({
    .label = "## Window",
    .flags = ImGuiWindowFlags_NoTitleBar            | 
             ImGuiWindowFlags_NoCollapse            | 
             ImGuiWindowFlags_NoResize              | 
             ImGuiWindowFlags_NoMove                | 
             ImGuiWindowFlags_NoBringToFrontOnFocus | 
             ImGuiWindowFlags_NoNavFocus
});

const inline ImguiItem IMGUI_DOCKSPACE = ImguiItem
({
    .label = "## Dockspace",
    .flags = ImGuiDockNodeFlags_PassthruCentralNode
});

const inline ImguiItem IMGUI_TASKBAR = ImguiItem
({
    .label = "## Taskbar",
    .size = {0, 32},
    .flags = ImGuiWindowFlags_NoTitleBar        |
             ImGuiWindowFlags_NoResize          |
             ImGuiWindowFlags_NoMove            |
             ImGuiWindowFlags_NoScrollbar       |
             ImGuiWindowFlags_NoScrollWithMouse |
             ImGuiWindowFlags_NoSavedSettings
});

const inline ImguiItem IMGUI_TASKBAR_FILE = ImguiItem
({
    .label = "&File", 
    .tooltip = "Opens the file menu, for reading/writing anm2 files.", 
    .popup = "## File Popup",
    .isSizeToText = true
});

const inline ImguiItem IMGUI_FILE_NEW = ImguiItem
({
    .label = "&New",
    .tooltip = "Load a blank .anm2 file to edit.",
    .function = imgui_file_new,
    .chord = ImGuiMod_Ctrl | ImGuiKey_N
});

const inline ImguiItem IMGUI_FILE_OPEN = ImguiItem
({
    .label = "&Open",
    .tooltip = "Open an existing .anm2 file to edit.",
    .function = imgui_file_open,
    .chord = ImGuiMod_Ctrl | ImGuiKey_O
});

const inline ImguiItem IMGUI_FILE_SAVE = ImguiItem
({
    .label = "&Save",
    .tooltip = "Saves the current .anm2 file to its path.\nIf no path exists, one can be chosen.",
    .function = imgui_file_save,
    .chord = ImGuiMod_Ctrl | ImGuiKey_S
});

const inline ImguiItem IMGUI_FILE_SAVE_AS = ImguiItem
({
    .label = "S&ave As",
    .tooltip = "Saves the current .anm2 file to a chosen path.",
    .function = imgui_file_save_as,
    .chord = ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S
});

const inline ImguiItem IMGUI_TASKBAR_WIZARD = ImguiItem
({
    .label = "&Wizard",
    .tooltip = "Opens the wizard menu, for neat functions related to the .anm2.",
    .popup = "## Wizard Popup",
    .isSizeToText  = true
});

const inline ImguiItem IMGUI_TASKBAR_WIZARD_GENERATE_ANIMATION_FROM_GRID = ImguiItem
({
    .label = "&Generate Animation from Grid",
    .tooltip = "Generate a new animation from grid values.",
    .popup = "Generate Animation from Grid",
    .popupType = IMGUI_POPUP_CENTER_SCREEN
});

const inline ImguiItem IMGUI_TASKBAR_WIZARD_RECORD_GIF_ANIMATION = ImguiItem
({
    .label = "G&enerate GIF Animation",
    .tooltip = "Generates a GIF animation from the current animation.",
    .function = imgui_generate_gif_animation
});

const inline ImguiItem IMGUI_TASKBAR_PLAYBACK = ImguiItem
({
    .label = "&Playback",
    .tooltip = "Opens the playback menu, for configuring playback settings.",
    .popup = "## Playback Popup",
    .isSizeToText = true
});

const inline ImguiItem IMGUI_PLAYBACK_ALWAYS_LOOP = ImguiItem
({
    .label = "&Always Loop",
    .tooltip = "Sets the animation playback to always loop, regardless of the animation's loop setting."
});

const inline ImguiItem IMGUI_ANIMATIONS = ImguiItem({"Animations"});

const inline ImguiItem IMGUI_ANIMATIONS_CHILD = ImguiItem
({
    .label = "## Animations Child",
    .flags = true,
});

const inline ImguiItem IMGUI_ANIMATION = ImguiItem
({
    .label = "## Animation Item",
    .action = "Select Animation",
    .dragDrop = "## Animation Drag Drop",
    .texture = TEXTURE_ANIMATION,
    .isUndoable = true,
    .isSizeToText = false
});

const inline ImguiItem IMGUI_ANIMATIONS_OPTIONS_CHILD = ImguiItem
({
    .label = "## Animations Options Child",
    .flags = true,
});

const inline ImguiItem IMGUI_ANIMATION_ADD = ImguiItem
({
    .label = "Add",
    .tooltip = "Adds a new animation.",
    .action = "Add Animation",
    .isUndoable = true,
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_ANIMATIONS_OPTIONS_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_ANIMATION_DUPLICATE = ImguiItem
({
    .label = "Duplicate",
    .tooltip = "Duplicates the selected animation, placing it after.",
    .action = "Duplicate Animation",
    .isUndoable = true,
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_ANIMATIONS_OPTIONS_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_ANIMATIONS_MERGE = ImguiItem
({
    .label = "Merge",
    .tooltip = "Open the animation merge popup, to merge animations together.",
    .popup = "Merge Animations",
    .popupType = IMGUI_POPUP_CENTER_SCREEN,
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_ANIMATIONS_OPTIONS_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_ANIMATIONS_MERGE_CHILD = ImguiItem
({
    .label = "## Merge Animations",
    .size = {300, 250},
    .flags = true
});

const inline ImguiItem IMGUI_ANIMATIONS_MERGE_ON_CONFLICT_CHILD = ImguiItem
({
    .label = "## Merge On Conflict Child",
    .size = {300, 75},
    .flags = true
});

const inline ImguiItem IMGUI_ANIMATIONS_MERGE_ON_CONFLICT = ImguiItem({"On Conflict"});

const inline ImguiItem IMGUI_ANIMATIONS_MERGE_APPEND_FRAMES = ImguiItem
({
    .label = "Append Frames ",
    .tooltip = "On frame conflict, the merged animation will have the selected animations' frames appended, in top-to-bottom order.",
    .value = ANM2_MERGE_APPEND_FRAMES
});

const inline ImguiItem IMGUI_ANIMATIONS_MERGE_REPLACE_FRAMES = ImguiItem
({
    .label = "Replace Frames",
    .tooltip = "On frame conflict, the merged animation will have the latest selected animations' frames.",
    .value = ANM2_MERGE_REPLACE_FRAMES
});

const inline ImguiItem IMGUI_ANIMATIONS_MERGE_PREPEND_FRAMES = ImguiItem
({
    .label = "Prepend Frames",
    .tooltip = "On frame conflict, the merged animation will have the selected animations' frames prepend, in top-to-bottom order.",
    .value = ANM2_MERGE_PREPEND_FRAMES
});

const inline ImguiItem IMGUI_ANIMATIONS_MERGE_IGNORE = ImguiItem
({
    .label = "Ignore        ",
    .tooltip = "On frame conflict, the merged animation will ignore the other selected animations' frames.",
    .value = ANM2_MERGE_IGNORE
});

const inline ImguiItem IMGUI_ANIMATIONS_MERGE_OPTIONS_CHILD = ImguiItem
({
    .label = "## Merge Options Child",
    .size = {300, 35},
    .flags = true
});

const inline ImguiItem IMGUI_ANIMATIONS_MERGE_DELETE_ANIMATIONS_AFTER = ImguiItem
({
    .label = "Delete Animations After Merging",
    .tooltip = "After merging, the selected animations (besides the original) will be deleted."
});

const inline ImguiItem IMGUI_ANIMATIONS_MERGE_CONFIRM = ImguiItem
({
    .label = "Merge",
    .tooltip = "Merge the selected animations with the options set.",
    .action = "Merge Animations",
    .isUndoable = true,
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_TIMELINE_MERGE_OPTIONS_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_ANIMATIONS_MERGE_CANCEL = ImguiItem
({
    .label = "Cancel",
    .tooltip = "Cancel merging.",
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_TIMELINE_MERGE_OPTIONS_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_ANIMATION_DEFAULT = ImguiItem
({
    .label = "Default",
    .tooltip = "Sets the selected animation to be the default.",
    .isUndoable = true,
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_ANIMATIONS_OPTIONS_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_ANIMATION_REMOVE = ImguiItem
({
    .label = "Remove",
    .tooltip = "Removes the selected animation.",
    .action = "Remove Animation",
    .focusWindow = IMGUI_ANIMATIONS.label,
    .chord = ImGuiKey_Delete,
    .isUndoable = true,
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_ANIMATIONS_OPTIONS_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_EVENTS = ImguiItem({"Events"});

const inline ImguiItem IMGUI_EVENTS_CHILD = ImguiItem
({
    .label = "## Events Child",
    .flags = true
});

const inline ImguiItem IMGUI_EVENT = ImguiItem
({
    .label = "## Event Item",
    .texture = TEXTURE_EVENT,
    .isUndoable = true,
    .isSizeToText = false
});

const inline ImguiItem IMGUI_EVENTS_OPTIONS_CHILD = ImguiItem
({
    .label = "## Events Options Child",
    .flags = true,
});

const inline ImguiItem IMGUI_EVENT_ADD = ImguiItem
({
    .label = "Add",
    .tooltip = "Adds a new event.",
    .action = "Add Event",
    .isUndoable = true,
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_EVENTS_OPTIONS_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_EVENT_REMOVE_UNUSED = ImguiItem
({
    .label = "Remove Unused",
    .tooltip = "Removes all unused events (i.e., not being used in any triggers in any animation).",
    .action = "Remove Unused Events",
    .isUndoable = true,
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_EVENTS_OPTIONS_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_SPRITESHEETS = ImguiItem({"Spritesheets"});

const inline ImguiItem IMGUI_SPRITESHEETS_CHILD = ImguiItem
({
    .label = "## Spritesheets Child",
    .flags = true
});

const inline ImguiItem IMGUI_SPRITESHEET_CHILD = ImguiItem
({
    .label = "## Spritesheet Child",
    .size = {0, 175},
    .flags = true
});

const inline ImguiItem IMGUI_SPRITESHEET = ImguiItem
({
    .label = "## Spritesheet",
    .dragDrop = "## Spritesheet Drag Drop",
    .texture = TEXTURE_SPRITESHEET,
    .flags = true
});

const inline ImguiItem IMGUI_SPRITESHEETS_OPTIONS_CHILD = ImguiItem
({
    .label = "## Spritesheets Options Child",
    .flags = true,
});

const inline ImguiItem IMGUI_SPRITESHEETS_ADD = ImguiItem
({
    .label = "Add",
    .tooltip = "Select an image to add as a spritesheet.",
    .function = imgui_png_open,
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_SPRITESHEETS_RELOAD = ImguiItem
({
    .label = "Reload",
    .tooltip = "Reload the selected spritesheet.",
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_SPRITESHEETS_REPLACE = ImguiItem
({
    .label = "Replace",
    .tooltip = "Replace the highlighted spritesheet with another.",
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_SPRITESHEETS_REMOVE_UNUSED = ImguiItem
({
    .label = "Remove Unused",
    .tooltip = "Remove all unused spritesheets in the anm2 (i.e., no layer in any animation uses the spritesheet).",
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_SPRITESHEETS_OPTIONS_FIRST_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_SPRITESHEETS_SELECT_ALL = ImguiItem
({
    .label = "Select All",
    .tooltip = "Select all spritesheets.",
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_SPRITESHEETS_OPTIONS_SECOND_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_SPRITESHEETS_SELECT_NONE = ImguiItem
({
    .label = "Select None",
    .tooltip = "Unselect all spritesheets.",
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_SPRITESHEETS_OPTIONS_SECOND_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_SPRITESHEETS_SAVE = ImguiItem
({
    .label = "Save",
    .tooltip = "Save the selected spritesheet(s) to their original path.",
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_SPRITESHEETS_OPTIONS_SECOND_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW = ImguiItem
({
    .label = "Animation Preview",
    .flags =  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_GRID_SETTINGS = ImguiItem
({
    .label = "## Grid Settings",
    .size = IMGUI_CANVAS_CHILD_SIZE,
    .flags = true
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_GRID = ImguiItem
({
    .label = "Grid",
    .tooltip = "Toggles the visiblity of a grid over the animation preview."
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_GRID_COLOR = ImguiItem
({
    .label = "Color",
    .tooltip = "Change the color of the animation preview grid.",
    .flags = ImGuiColorEditFlags_NoInputs
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_GRID_SIZE = ImguiItem
({
    .label = "Size",
    .tooltip = "Change the size of the animation preview grid, in pixels.",
    .min = CANVAS_GRID_MIN,
    .max = CANVAS_GRID_MAX
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_GRID_OFFSET = ImguiItem
({
    .label = "Offset",
    .tooltip = "Change the offset of the animation preview grid, in pixels."
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_VIEW_SETTINGS = ImguiItem 
({
    .label = "## View Settings",
    .size = IMGUI_CANVAS_CHILD_SIZE,
    .flags = true
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_ZOOM = ImguiItem 
({
    .label = "Zoom",
    .tooltip = "Change the animation preview's zoom.",
    .format = "%.0f",
    .min = CANVAS_ZOOM_MIN,
    .max = CANVAS_ZOOM_MAX
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_CENTER_VIEW = ImguiItem 
({
    .label = "Center View",
    .tooltip = "Centers the current view.",
    .size = {-FLT_MIN, 0}
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_BACKGROUND_SETTINGS = ImguiItem 
({
    .label = "## Background Settings",
    .size = IMGUI_CANVAS_CHILD_SIZE,
    .flags = true
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_BACKGROUND_COLOR = ImguiItem
({
    .label = "Background Color",
    .tooltip = "Change the background color of the animation preview.",
    .flags = ImGuiColorEditFlags_NoInputs
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_OVERLAY = ImguiItem
({
    .label = "Overlay",
    .tooltip = "Choose an animation to overlay over the previewed animation, for reference."
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_OVERLAY_TRANSPARENCY = ImguiItem
({
    .label = "Alpha",
    .tooltip = "Set the transparency of the animation overlay.",
    .format = "%.0f",
    .isSizeToChild = true,
    .min = 0,
    .max = 255
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_HELPER_SETTINGS = ImguiItem 
({
    .label = "## Helper Settings",
    .size = IMGUI_CANVAS_CHILD_SIZE,
    .flags = true
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_AXIS = ImguiItem
({
    .label   = "Axis",
    .tooltip = "Toggle the display of the X/Y axes on the animation preview."
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_AXIS_COLOR = ImguiItem
({
    .label   = "Color",
    .tooltip = "Change the color of the animation preview's axes.",
    .flags = ImGuiColorEditFlags_NoInputs
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_ROOT_TRANSFORM = ImguiItem
({
    .label   = "Root Transform",
    .tooltip = "Toggles the root frames's attributes transforming the other items in an animation."
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_SHOW_PIVOT = ImguiItem
({
    .label   = "Show Pivot",
    .tooltip = "Toggles the appearance of an icon for each animation item's pivot."
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_BORDER = ImguiItem
({
    .label   = "Border",
    .tooltip = "Toggles the appearance of a border around each layer."
});

const inline ImguiItem IMGUI_SPRITESHEET_EDITOR = ImguiItem
({
    .label = "Spritesheet Editor",
    .flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
});

const inline ImguiItem IMGUI_SPRITESHEET_EDITOR_GRID_SETTINGS = ImguiItem
({
    .label = "## Grid Settings",
    .size = IMGUI_CANVAS_CHILD_SIZE,
    .flags = true
});

const inline ImguiItem IMGUI_SPRITESHEET_EDITOR_GRID = ImguiItem
({
    .label = "Grid",
    .tooltip = "Toggles the visiblity of a grid over the spritesheet editor."
});

const inline ImguiItem IMGUI_SPRITESHEET_EDITOR_GRID_SNAP = ImguiItem
({
    .label = "Snap",
    .tooltip = "Using the crop tool will snap the points to the nearest grid point."
});

const inline ImguiItem IMGUI_SPRITESHEET_EDITOR_GRID_COLOR = ImguiItem
({
    .label = "Color",
    .tooltip = "Change the color of the spritesheet editor grid.",
    .flags = ImGuiColorEditFlags_NoInputs
});

const inline ImguiItem IMGUI_SPRITESHEET_EDITOR_GRID_SIZE = ImguiItem
({
    .label = "Size",
    .tooltip = "Change the size of the spritesheet editor grid, in pixels.",
    .min = CANVAS_GRID_MIN,
    .max = CANVAS_GRID_MAX
});

const inline ImguiItem IMGUI_SPRITESHEET_EDITOR_GRID_OFFSET = ImguiItem
({
    .label      = "Offset",
    .tooltip    = "Change the offset of the spritesheet editor grid, in pixels."
});

const inline ImguiItem IMGUI_SPRITESHEET_EDITOR_VIEW_SETTINGS = ImguiItem 
({
    .label = "## View Settings",
    .size = IMGUI_CANVAS_CHILD_SIZE,
    .flags = true
});

const inline ImguiItem IMGUI_SPRITESHEET_EDITOR_ZOOM = ImguiItem 
({
    .label = "Zoom",
    .tooltip = "Change the spritesheet editor's zoom.",
    .format = "%.0f",
    .min = CANVAS_ZOOM_MIN,
    .max = CANVAS_ZOOM_MAX
});

const inline ImguiItem IMGUI_SPRITESHEET_EDITOR_CENTER_VIEW = ImguiItem 
({
    .label = "Center View",
    .tooltip = "Centers the current view.",
    .size = {-FLT_MIN, 0},
});

const inline ImguiItem IMGUI_SPRITESHEET_EDITOR_BACKGROUND_SETTINGS = ImguiItem 
({
    .label = "## Background Settings",
    .size = IMGUI_CANVAS_CHILD_SIZE,
    .flags = true
});

const inline ImguiItem IMGUI_SPRITESHEET_EDITOR_BACKGROUND_COLOR = ImguiItem
({
    .label = "Background Color",
    .tooltip = "Change the background color of the spritesheet editor.",
    .flags = ImGuiColorEditFlags_NoInputs
});

const inline ImguiItem IMGUI_SPRITESHEET_EDITOR_BORDER = ImguiItem 
({
    .label = "Border",
    .tooltip = "Toggle a border appearing around the selected spritesheet."
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES = ImguiItem({"Frame Properties"});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_POSITION = ImguiItem
({
    .label = "Position",
    .tooltip = "Change the position of the selected frame.",
    .action = "Frame Position",
    .format = "%.0f",
    .isUndoable = true,
    .speed = 0.25f,
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_CROP = ImguiItem
({
    .label = "Crop",
    .tooltip = "Change the crop position of the selected frame.",
    .action = "Frame Crop",
    .format = "%.0f",
    .isUndoable = true,
    .speed = 0.25f,
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_SIZE = ImguiItem
({
    .label = "Size",
    .tooltip = "Change the size of the crop of the selected frame.",
    .action = "Frame Size",
    .format = "%.0f",
    .isUndoable = true,
    .speed = 0.25f
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_PIVOT = ImguiItem
({
    .label = "Pivot",
    .tooltip = "Change the pivot of the selected frame.",
    .action = "Frame Pivot",
    .format = "%.0f",
    .isUndoable = true,
    .speed = 0.25f
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_SCALE = ImguiItem
({
    .label = "Scale",
    .tooltip = "Change the scale of the selected frame.",
    .action = "Frame Scale",
    .isUndoable = true,
    .speed = 0.25f
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_ROTATION = ImguiItem
({
    .label = "Rotation",
    .tooltip = "Change the rotation of the selected frame.",
    .action = "Frame Rotation",
    .isUndoable = true,
    .speed = 0.25f
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_DURATION = ImguiItem
({
    .label = "Duration",
    .tooltip = "Change the duration of the selected frame.",
    .action = "Frame Duration",
    .isUndoable = true,
    .min = ANM2_FRAME_NUM_MIN,
    .max = ANM2_FRAME_NUM_MAX
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_TINT = ImguiItem
({
    .label = "Tint",
    .tooltip = "Change the tint of the selected frame.",
    .action = "Frame Tint",
    .isUndoable = true,
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_COLOR_OFFSET = ImguiItem
({
    .label = "Color Offset",
    .tooltip = "Change the color offset of the selected frame.",
    .action = "Frame Color Offset",
    .isUndoable = true,
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_VISIBLE = ImguiItem
({
    .label = "Visible",
    .tooltip = "Toggles the visibility of the selected frame.",
    .action = "Frame Visibility",
    .isUndoable = true,
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_INTERPOLATED = ImguiItem
({
    .label = "Interpolation",
    .tooltip = "Toggles the interpolation of the selected frame.",
    .action = "Frame Interpolation",
    .isUndoable = true,
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_FLIP_X = ImguiItem
({
    .label = "Flip X",
    .tooltip = "Change the sign of the X scale, to cheat flipping the layer horizontally.\n(Anm2 doesn't support flipping directly)",
    .action = "Frame Flip X",
    .isUndoable = true,
    .size = IMGUI_FRAME_PROPERTIES_FLIP_BUTTON_SIZE 
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_FLIP_Y = ImguiItem
({
    .label = "Flip Y",
    .tooltip = "Change the sign of the Y scale, to cheat flipping the layer vertically.\n(Anm2 doesn't support flipping directly)",
    .action = "Frame Flip Y",
    .isUndoable = true,
    .size = IMGUI_FRAME_PROPERTIES_FLIP_BUTTON_SIZE 
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_EVENT = ImguiItem
({
    .label = "Event",
    .tooltip = "Change the event the trigger uses.\nNOTE: This sets the event ID, not the event. If the events change IDs, then this will need to be changed.",
    .action = "Trigger Event",
    .isUndoable = true
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_AT_FRAME = ImguiItem
({
    .label = "At Frame",
    .tooltip = "Change the frame where the trigger occurs.",
    .action = "Trigger At Frame",
    .isUndoable = true
});

const inline ImguiItem IMGUI_TOOLS = ImguiItem({"Tools"});

const inline ImguiItem IMGUI_TOOL_PAN = ImguiItem
({
    .label = "## Pan",
    .tooltip = "Use the pan tool.\nWill shift the view as the cursor is dragged.\nYou can also use the middle mouse button to pan at any time.",
    .function = imgui_tool_pan_set,
    .chord = ImGuiKey_P,
    .texture = TEXTURE_PAN,
});

const inline ImguiItem IMGUI_TOOL_MOVE = ImguiItem
({
    .label = "## Move",
    .tooltip = "Use the move tool.\nWill move the selected item as the cursor is dragged, or directional keys are pressed.\n(Animation Preview only.)",
    .function = imgui_tool_move_set,
    .chord = ImGuiKey_M,
    .texture = TEXTURE_MOVE,
});

const inline ImguiItem IMGUI_TOOL_ROTATE = ImguiItem
({
    .label = "## Rotate",
    .tooltip = "Use the rotate tool.\nWill rotate the selected item as the cursor is dragged, or directional keys are pressed.\n(Animation Preview only.)",
    .function = imgui_tool_rotate_set,
    .chord = ImGuiKey_R,
    .texture = TEXTURE_ROTATE,
});

const inline ImguiItem IMGUI_TOOL_SCALE = ImguiItem
({
    .label = "## Scale",
    .tooltip = "Use the scale tool.\nWill scale the selected item as the cursor is dragged, or directional keys are pressed.\n(Animation Preview only.)",
    .function = imgui_tool_scale_set,
    .chord = ImGuiKey_S,
    .texture = TEXTURE_SCALE,
});

const inline ImguiItem IMGUI_TOOL_CROP = ImguiItem
({
    .label = "## Crop",
    .tooltip = "Use the crop tool.\nWill produce a crop rectangle based on how the cursor is dragged.\n(Spritesheet Editor only.)",
    .function = imgui_tool_crop_set,
    .chord = ImGuiKey_C,
    .texture = TEXTURE_CROP,
});

const inline ImguiItem IMGUI_TOOL_DRAW = ImguiItem
({
    .label = "## Draw",
    .tooltip = "Draws pixels onto the selected spritesheet, with the current color.\n(Spritesheet Editor only.)",
    .function = imgui_tool_draw_set,
    .chord = ImGuiKey_B,
    .texture = TEXTURE_DRAW,
});

const inline ImguiItem IMGUI_TOOL_ERASE = ImguiItem
({
    .label = "## Erase",
    .tooltip = "Erases pixels from the selected spritesheet.\n(Spritesheet Editor only.)",
    .function = imgui_tool_erase_set,
    .chord = ImGuiKey_E,
    .texture = TEXTURE_ERASE,
});

const inline ImguiItem IMGUI_TOOL_COLOR_PICKER = ImguiItem
({
    .label = "## Color Picker",
    .tooltip = "Selects a color from anywhere on the screen, to be used for drawing.",
    .function = imgui_tool_color_picker_set,
    .chord = ImGuiKey_W,
    .texture = TEXTURE_COLOR_PICKER,
});

const inline ImguiItem IMGUI_TOOL_UNDO = ImguiItem
({
    .label = "## Undo",
    .tooltip = "Undoes the last action.",
    .function = imgui_undo,
    .chord = ImGuiKey_Z,
    .texture = TEXTURE_UNDO
});

const inline ImguiItem IMGUI_TOOL_REDO = ImguiItem
({
    .label = "## Redo",
    .tooltip = "Redoes the last action.",
    .function = imgui_redo,
    .chord = ImGuiMod_Shift + ImGuiKey_Z,
    .texture = TEXTURE_REDO
});

const inline ImguiItem IMGUI_TOOL_COLOR = ImguiItem
({
    .label = "## Color",
    .tooltip = "Set the color, to be used by the draw tool.",
    .flags = ImGuiColorEditFlags_NoInputs
});

const inline ImguiItem IMGUI_TOOL_COLOR_PICKER_TOOLTIP_COLOR = ImguiItem
({
    .label = "## Color Picker Tooltip Color",
    .flags = ImGuiColorEditFlags_NoTooltip
});

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
    &IMGUI_TOOL_COLOR,
};

const inline ImguiItem IMGUI_TIMELINE = ImguiItem
({
    .label = "Timeline",
    .flags =  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
});

const inline ImguiItem IMGUI_TIMELINE_CHILD = ImguiItem
({
    .label = "## Timeline Child",
    .flags = true
});

const inline ImguiItem IMGUI_TIMELINE_HEADER = ImguiItem
({
    .label = "## Timeline Header",
    .size = {0, IMGUI_TIMELINE_FRAME_SIZE.y},
    .flagsAlt = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
});

const inline ImguiItem IMGUI_TIMELINE_ITEMS_CHILD = ImguiItem
({
    .label = "## Timeline Items",
    .size = {IMGUI_TIMELINE_ITEM_SIZE.x, 0},
    .flagsAlt = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_FRAMES_CHILD = ImguiItem
({
    .label = "## Timeline Item Frames",
    .size = {0, IMGUI_TIMELINE_FRAME_SIZE.y},
    .flags = true
});

const inline ImguiItem IMGUI_TIMELINE_FRAMES_CHILD = ImguiItem
({
    .label = "## Timeline Frames",
    .size = {-1, -1},
    .flagsAlt = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse
});

const inline ImguiItem IMGUI_TIMELINE_ITEM = ImguiItem
({
    .label = "## Timeline Item",
    .size = IMGUI_TIMELINE_ITEM_SIZE,
    .flags = true
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_ROOT = ImguiItem
({
    .label = "## Root Item",
    .color = {{0.010, 0.049, 0.078, 1.0f}},
    .size = IMGUI_TIMELINE_ITEM_SIZE,
    .flags = true
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_LAYER = ImguiItem
({
    .label = "## Layer Item",
    .color = {{0.098, 0.039, 0.020, 1.0f}},
    .size = IMGUI_TIMELINE_ITEM_SIZE,
    .flags = true
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_NULL = ImguiItem
({
    .label = "## Null Item",
    .color = {{0.020, 0.049, 0.000, 1.0f}},
    .size = IMGUI_TIMELINE_ITEM_SIZE,
    .flags = true
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_TRIGGERS = ImguiItem
({
    .label = "## Triggers Item",
    .color = {{0.078, 0.020, 0.029, 1.0f}},
    .size = IMGUI_TIMELINE_ITEM_SIZE,
    .flags = true
});

const inline ImguiItem* IMGUI_TIMELINE_ITEMS[ANM2_COUNT]
{
    &IMGUI_TIMELINE_ITEM,
    &IMGUI_TIMELINE_ITEM_ROOT,
    &IMGUI_TIMELINE_ITEM_LAYER,
    &IMGUI_TIMELINE_ITEM_NULL,
    &IMGUI_TIMELINE_ITEM_TRIGGERS
};

const inline ImguiItem IMGUI_TIMELINE_ITEM_SELECTABLE = ImguiItem
({
    .label = "## Selectable",
    .size = IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_ROOT_SELECTABLE = ImguiItem
({
    .label = "Root",
    .tooltip = "The root item of an animation.\nChanging its properties will transform the rest of the animation.",
    .action = "Root Item Select",
    .texture = TEXTURE_ROOT,
    .isUndoable = true,
    .size = IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_LAYER_SELECTABLE = ImguiItem
({
    .label = "## Layer Selectable",
    .tooltip = "A layer item.\nA graphical item within the animation.",
    .action = "Layer Item Select",
    .dragDrop = "## Layer Drag Drop",
    .texture = TEXTURE_LAYER,
    .isUndoable = true,
    .size = IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_NULL_SELECTABLE = ImguiItem
({
    .label = "## Null Selectable",
    .tooltip = "A null item.\nAn invisible item within the animation that is accessible via a game engine.",
    .action = "Null Item Select",
    .dragDrop = "## Null Drag Drop",
    .texture = TEXTURE_NULL,
    .isUndoable = true,
    .size = IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_TRIGGERS_SELECTABLE = ImguiItem
({
    .label = "Triggers",
    .tooltip = "The animation's triggers.\nWill fire based on an event.",
    .action = "Triggers Item Select",
    .texture = TEXTURE_TRIGGERS,
    .isUndoable = true,
    .size = IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE
});

const inline ImguiItem* IMGUI_TIMELINE_ITEM_SELECTABLES[ANM2_COUNT]
{
    &IMGUI_TIMELINE_ITEM_SELECTABLE,
    &IMGUI_TIMELINE_ITEM_ROOT_SELECTABLE,
    &IMGUI_TIMELINE_ITEM_LAYER_SELECTABLE,
    &IMGUI_TIMELINE_ITEM_NULL_SELECTABLE,
    &IMGUI_TIMELINE_ITEM_TRIGGERS_SELECTABLE
};

const inline ImguiItem IMGUI_TIMELINE_ITEM_VISIBLE = ImguiItem
({
    .label = "## Visible",
    .tooltip = "The item is visible.\nPress to set to invisible.",
    .texture = TEXTURE_VISIBLE,
    .isUndoable = true
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_INVISIBLE = ImguiItem
({
    .label = "## Invisible",
    .tooltip = "The item is invisible.\nPress to set to visible.",
    .texture = TEXTURE_INVISIBLE,
    .isUndoable = true
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_SHOW_RECT = ImguiItem
({
    .label = "## Show Rect",
    .tooltip = "The rect is shown.\nPress to hide rect.",
    .texture = TEXTURE_SHOW_RECT,
    .isUndoable = true
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_HIDE_RECT = ImguiItem
({
    .label = "## Hide Rect",
    .tooltip = "The rect is hidden.\nPress to show rect.",
    .texture = TEXTURE_HIDE_RECT,
    .isUndoable = true
});

const inline ImguiItem IMGUI_TIMELINE_SPRITESHEET_ID = ImguiItem
({
    .label = "## Spritesheet ID",
    .tooltip = "Change the spritesheet ID this item uses.",
    .texture = TEXTURE_SPRITESHEET,
    .isUndoable = true,
    .size = {32, 0},
});

const inline ImguiItem IMGUI_TIMELINE_FRAME = ImguiItem({"## Frame"});

const inline ImguiItem IMGUI_TIMELINE_ROOT_FRAME = ImguiItem
({
    .label = "## Root Frame",
    .action = "Root Frame Select",
    .isUndoable = true,
    .color = {{0.020, 0.294, 0.569, 0.5}, {0.471, 0.882, 1.000, 0.75}, {0.314, 0.588, 0.843, 0.75}},
    .size = IMGUI_TIMELINE_FRAME_SIZE,
    .contentOffset = IMGUI_TIMELINE_FRAME_CONTENT_OFFSET,
    .border = IMGUI_TIMELINE_FRAME_BORDER,
});

const inline ImguiItem IMGUI_TIMELINE_LAYER_FRAME = ImguiItem
({
    .label = "## Layer Frame",
    .action = "Layer Frame Select",
    .dragDrop = "## Layer Frame Drag Drop",
    .isUndoable = true,
    .color = {{0.529, 0.157, 0.000, 0.5}, {1.000, 0.618, 0.324, 0.75}, {0.882, 0.412, 0.216, 0.75}},
    .size = IMGUI_TIMELINE_FRAME_SIZE,
    .contentOffset = IMGUI_TIMELINE_FRAME_CONTENT_OFFSET,
    .border = IMGUI_TIMELINE_FRAME_BORDER,
});

const inline ImguiItem IMGUI_TIMELINE_NULL_FRAME = ImguiItem
({
    .label = "## Null Frame",
    .action = "Null Frame Select",
    .dragDrop = "## Null Frame Drag Drop",
    .isUndoable = true,
    .color = {{0.137, 0.353, 0.000, 0.5}, {0.646, 0.971, 0.441, 0.75}, {0.431, 0.647, 0.294, 0.75}},
    .size = IMGUI_TIMELINE_FRAME_SIZE,
    .contentOffset = IMGUI_TIMELINE_FRAME_CONTENT_OFFSET,
    .border = IMGUI_TIMELINE_FRAME_BORDER,
});

const inline ImguiItem IMGUI_TIMELINE_TRIGGERS_FRAME = ImguiItem
({
    .label = "## Triggers Frame",
    .action = "Trigger Select",
    .isUndoable = true,
    .color = {{0.529, 0.118, 0.196, 0.5}, {1.000, 0.618, 0.735, 0.75}, {0.804, 0.412, 0.490, 0.75}},
    .size = IMGUI_TIMELINE_FRAME_SIZE,
    .contentOffset = IMGUI_TIMELINE_FRAME_CONTENT_OFFSET,
    .border = IMGUI_TIMELINE_FRAME_BORDER,
});

const inline ImguiItem* IMGUI_TIMELINE_FRAMES[ANM2_COUNT]
{
    &IMGUI_TIMELINE_FRAME,
    &IMGUI_TIMELINE_ROOT_FRAME,
    &IMGUI_TIMELINE_LAYER_FRAME,
    &IMGUI_TIMELINE_NULL_FRAME,
    &IMGUI_TIMELINE_TRIGGERS_FRAME
};

const inline ImguiItem IMGUI_TIMELINE_PICKER = ImguiItem
({
    .label = "## Timeline Picker",
    .flags = ImGuiWindowFlags_NoTitleBar   | 
             ImGuiWindowFlags_NoResize     |
		     ImGuiWindowFlags_NoMove       | 
             ImGuiWindowFlags_NoBackground |
		     ImGuiWindowFlags_NoInputs
});

const inline ImguiItem IMGUI_TIMELINE_FOOTER_ITEM_CHILD = ImguiItem
({
    .label = "## Timeline Footer Item Child",
    .size = {IMGUI_TIMELINE_ITEM_SIZE.x, 0},
    .flags = true
});

const inline ImguiItem IMGUI_TIMELINE_ADD_ITEM = ImguiItem
({
    .label = "Add",
    .tooltip = "Adds an item (layer or null) to the animation.",
    .popup = "##Add Item Popup",
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_TIMELINE_FOOTER_ITEM_CHILD_ITEM_COUNT
});

const inline ImguiItem IMGUI_TIMELINE_ADD_ITEM_LAYER = ImguiItem
({
    .label = "Layer",
    .tooltip = "Adds a layer item.\nA layer item is a primary graphical item, using a spritesheet.",
    .action = "Add Layer",
    .isUndoable = true,

});

const inline ImguiItem IMGUI_TIMELINE_ADD_ITEM_NULL = ImguiItem
({
    .label = "Null",
    .tooltip = "Adds a null item.\nA null item is an invisible item, often accessed by the game engine.",
    .action = "Add Null",
    .isUndoable = true
});

const inline ImguiItem IMGUI_TIMELINE_REMOVE_ITEM = ImguiItem
({
    .label = "Remove",
    .tooltip = "Removes the selected item (layer or null) from the animation.",
    .action = "Remove Item",
    .focusWindow = IMGUI_TIMELINE.label,
    .isUndoable = true,
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_TIMELINE_FOOTER_ITEM_CHILD_ITEM_COUNT
});

const inline ImguiItem IMGUI_TIMELINE_FOOTER_OPTIONS_CHILD = ImguiItem
({
    .label = "## Timeline Footer Options Child",
    .flags = true
});

const inline ImguiItem IMGUI_TIMELINE_PLAY = ImguiItem
({
    .label = "|>  Play",
    .tooltip = "Play the current animation, if paused.",
    .chord = ImGuiKey_Space
});

const inline ImguiItem IMGUI_TIMELINE_PAUSE = ImguiItem
({
    .label = "|| Pause",
    .tooltip = "Pause the current animation, if playing.",
    .chord = ImGuiKey_Space
});

const inline ImguiItem IMGUI_TIMELINE_ADD_FRAME = ImguiItem
({
    .label = "+ Insert Frame",
    .tooltip = "Inserts a frame in the selected animation item, based on the preview time.",
    .action = "Insert Frame",
    .isUndoable = true
});

const inline ImguiItem IMGUI_TIMELINE_REMOVE_FRAME = ImguiItem
({
    .label = "- Delete Frame",
    .tooltip = "Removes the selected frame from the selected animation item.",
    .action = "Delete Frame",
    .focusWindow = IMGUI_TIMELINE.label,
    .chord = ImGuiKey_Delete,
    .isUndoable = true,
});

const inline ImguiItem IMGUI_TIMELINE_BAKE = ImguiItem
({
    .label = "Bake",
    .tooltip = "Opens the bake popup menu, if a frame is selected.\nBaking a frame takes the currently interpolated values at the time between it and the next frame and separates them based on the interval.",
    .popup = "Bake Frames",
    .popupType = IMGUI_POPUP_CENTER_SCREEN
});

const inline ImguiItem IMGUI_TIMELINE_BAKE_CHILD = ImguiItem
({
    .label = "## Bake Child",
    .size = {200, 110},
    .flags = true
});

const inline ImguiItem IMGUI_TIMELINE_BAKE_INTERVAL = ImguiItem
({
    .label = "Interval",
    .tooltip = "Sets the delay of the baked frames the selected frame will be separated out into.",
    .min = ANM2_FRAME_DELAY_MIN
});

const inline ImguiItem IMGUI_TIMELINE_BAKE_ROUND_SCALE = ImguiItem
({
    .label = "Round Scale",
    .tooltip = "The scale of the baked frames will be rounded to the nearest integer."
});

const inline ImguiItem IMGUI_TIMELINE_BAKE_ROUND_ROTATION = ImguiItem
({
    .label = "Round Rotation",
    .tooltip = "The rotation of the baked frames will be rounded to the nearest integer."
});

const inline ImguiItem IMGUI_TIMELINE_BAKE_CONFIRM = ImguiItem
({
    .label = "Bake",
    .tooltip = "Bake the selected frame with the options selected.",
    .action = "Baking Frames",
    .isUndoable = true,
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_TIMELINE_BAKE_OPTIONS_CHILD_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_TIMELINE_BAKE_CANCEL = ImguiItem
({
    .label = "Cancel",
    .tooltip = "Cancel baking the selected frame.",
    .isSizeToChild = true,
    .childRowItemCount = IMGUI_TIMELINE_BAKE_OPTIONS_CHILD_ROW_ITEM_COUNT
});

const inline ImguiItem IMGUI_TIMELINE_FIT_ANIMATION_LENGTH = ImguiItem
({
    .label = "Fit Animation Length",
    .tooltip = "Sets the animation's length to the latest frame.",
    .action = "Fit Animation Length",
    .isUndoable = true
});

const inline ImguiItem IMGUI_TIMELINE_ANIMATION_LENGTH = ImguiItem
({
    .label = "Animation Length",
    .tooltip = "Sets the animation length.\n(Will not change frames.)",
    .action = "Set Animation Length",
    .isUndoable = true,
    .size = {100, 0},
    .min = ANM2_FRAME_NUM_MIN,
    .max = ANM2_FRAME_NUM_MAX
});

const inline ImguiItem IMGUI_TIMELINE_FPS = ImguiItem
({
    .label = "FPS",
    .tooltip = "Sets the animation's frames per second (its speed).",
    .action = "Set FPS",
    .isUndoable = true,
    .size = {100, 0},
    .min = ANM2_FPS_MIN,
    .max = ANM2_FPS_MAX
});

const inline ImguiItem IMGUI_TIMELINE_LOOP = ImguiItem
({
    .label = "Loop",
    .tooltip = "Toggles the animation looping.",
    .action = "Set Loop",
    .isUndoable = true
});

const inline ImguiItem IMGUI_TIMELINE_CREATED_BY = ImguiItem
({
    .label = "Author",
    .tooltip = "Sets the author of the animation.",
    .size = {150, 0},
    .max = ANM2_STRING_MAX
});

const inline ImguiItem IMGUI_TIMELINE_CREATED_ON = ImguiItem({"Created on: "});
const inline ImguiItem IMGUI_TIMELINE_VERSION = ImguiItem({"Version: "});

const inline ImguiItem IMGUI_RECORDING = ImguiItem
({
    .label = "Recording...",
    .texture = TEXTURE_RECORD
});

const inline ImguiItem IMGUI_CONTEXT_MENU = ImguiItem
({
    .label = "## Context Menu",
    .popup = "## Context Menu Popup"
});

const inline ImguiItem IMGUI_CUT = ImguiItem
({
    .label = "Cut",
    .tooltip = "Cuts the currently selected contextual element; removing it and putting it to the clipboard.",
    .action = "Cut",
    .function = imgui_cut,
    .chord = ImGuiMod_Ctrl | ImGuiKey_X,
    .isUndoable = true,
});

const inline ImguiItem IMGUI_COPY = ImguiItem
({
    .label = "Copy",
    .tooltip = "Copies the currently selected contextual element to the clipboard.",
    .action = "Copy",
    .function = imgui_copy,
    .chord = ImGuiMod_Ctrl | ImGuiKey_C,
    .isUndoable = true,
});

const inline ImguiItem IMGUI_PASTE = ImguiItem
({
    .label          = "Paste",
    .tooltip        = "Pastes the currently selection contextual element from the clipboard.",
    .action = "Paste",
    .function = imgui_paste,
    .chord = ImGuiMod_Ctrl | ImGuiKey_V,
    .isUndoable = true,
});

const inline ImguiItem IMGUI_RENAMABLE = ImguiItem
({
    .label = "## Renaming",
    .tooltip = "Rename the selected item.",
    .action = "Rename",
    .isUndoable = true,
    .max = 255,
    .flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue
});

const inline ImguiItem IMGUI_CHANGEABLE = ImguiItem
({
    .label = "## Changing",
    .tooltip = "Change the selected item's value.",
    .action = "Change Value",
    .isUndoable = true,
    .step = 0
});

const inline ImguiItem IMGUI_EXIT_CONFIRMATION = ImguiItem
({
    .label = "Unsaved changes will be lost!\nAre you sure you want to exit?",
    .popup = "Exit Confirmation"
});

const inline ImguiItem IMGUI_POPUP_YES_BUTTON = ImguiItem
({
    .label = "Yes",
    .size = {120, 0}
});

const inline ImguiItem IMGUI_POPUP_NO_BUTTON = ImguiItem
({
    .label = "No",
    .size = {120, 0}
});

void imgui_init
(
    Imgui* self,
    Dialog* dialog,
    Resources* resources,
    Anm2* anm2,
    Anm2Reference* reference,
    Editor* editor,
    Preview* preview,
    Settings* settings,
    Snapshots* snapshots,
    Clipboard* clipboard,
    SDL_Window* window,
    SDL_GLContext* glContext
);

void imgui_update(Imgui* self);
void imgui_draw();
void imgui_free();