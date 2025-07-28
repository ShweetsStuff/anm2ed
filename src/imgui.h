#pragma once

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#define IMGUI_ENABLE_DOCKING
#define IM_VEC2_CLASS_EXTRA                                                                   \
    inline bool operator==(const ImVec2& rhs) const { return x == rhs.x && y == rhs.y; }      \
    inline bool operator!=(const ImVec2& rhs) const { return !(*this == rhs); }               \
    inline ImVec2 operator+(const ImVec2& rhs) const { return ImVec2(x + rhs.x, y + rhs.y); } \
    inline ImVec2 operator-(const ImVec2& rhs) const { return ImVec2(x - rhs.x, y - rhs.y); } \
    inline ImVec2 operator*(const ImVec2& rhs) const { return ImVec2(x * rhs.x, y * rhs.y); } \

#define IM_VEC3_CLASS_EXTRA                                                                              \
    inline bool operator==(const ImVec3& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }   \
    inline bool operator!=(const ImVec3& rhs) const { return !(*this == rhs); }                          \
    inline ImVec3 operator+(const ImVec3& rhs) const { return ImVec3(x + rhs.x, y + rhs.y, z + rhs.z); } \
    inline ImVec3 operator-(const ImVec3& rhs) const { return ImVec3(x - rhs.x, y - rhs.y, z - rhs.z); } \
    inline ImVec3 operator*(const ImVec3& rhs) const { return ImVec3(x * rhs.x, y * rhs.y, z * rhs.z); }

#define IM_VEC4_CLASS_EXTRA                                                                                          \
    inline bool operator==(const ImVec4& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; } \
    inline bool operator!=(const ImVec4& rhs) const { return !(*this == rhs); }                                      \
    inline ImVec4 operator+(const ImVec4& rhs) const { return ImVec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }  \
    inline ImVec4 operator-(const ImVec4& rhs) const { return ImVec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }  \
    inline ImVec4 operator*(const ImVec4& rhs) const { return ImVec4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "clipboard.h"
#include "dialog.h"
#include "editor.h"
#include "preview.h"
#include "resources.h"
#include "settings.h"
#include "snapshots.h"
#include "window.h"

#define IMGUI_ANIMATIONS_LABEL "Animations"
#define IMGUI_ANIMATION_DEFAULT_FORMAT "(*) {}"
#define IMGUI_CHORD_NONE (ImGuiMod_None)
#define IMGUI_CONTEXT_MENU "## Context Menu"
#define IMGUI_EVENTS_LABEL "Events"
#define IMGUI_EVENT_FORMAT "#{} {}"
#define IMGUI_EVENT_NONE "None"
#define IMGUI_FILE_POPUP "## File Popup"
#define IMGUI_FRAME_BORDER 2.0f
#define IMGUI_FRAME_PROPERTIES_NO_FRAME "Select a frame to show properties..."
#define IMGUI_ITEM_SELECTABLE_EDITABLE_LABEL "## Editing"
#define IMGUI_OPENGL_VERSION "#version 330"
#define IMGUI_PICKER_LINE_COLOR IM_COL32(255, 255, 255, 255)
#define IMGUI_PLAYBACK_POPUP "## Playback Popup"
#define IMGUI_EXIT_CONFIRMATION_POPUP "Exit Confirmation"
#define IMGUI_POSITION_FORMAT "Position: {{{:5}, {:5}}}"
#define IMGUI_SPRITESHEET_FORMAT "#{} {}"
#define IMGUI_TIMELINE_ADD_ELEMENT_POPUP "## Add Element Popup"
#define IMGUI_TIMELINE_CHILD_ID_LABEL "#{} {}"
#define IMGUI_TIMELINE_FOOTER_HEIGHT 20
#define IMGUI_TIMELINE_FRAME_BORDER 2
#define IMGUI_TIMELINE_FRAME_LABEL_FORMAT "## {}"
#define IMGUI_TIMELINE_FRAME_MULTIPLE 5
#define IMGUI_TIMELINE_LABEL "Timeline"
#define IMGUI_MERGE_POPUP "Merge Animations"
#define IMGUI_TIMELINE_NO_ANIMATION "Select an animation to show timeline..."
#define IMGUI_TIMELINE_PICKER_LINE_WIDTH 2.0f
#define IMGUI_TIMELINE_SPRITESHEET_ID_FORMAT "#{}"
#define IMGUI_TOOL_STEP 1
#define IMGUI_TOOL_STEP_MOD 10
#define IMGUI_WIZARD_POPUP "## Wizard Popup"

#define VEC2_TO_IMVEC2(value) ImVec2(value.x, value.y)
#define IMVEC2_TO_VEC2(value) glm::vec2(value.x, value.y)
#define IMVEC2_ATLAS_UV_GET(type) VEC2_TO_IMVEC2(ATLAS_UVS[type][0]), VEC2_TO_IMVEC2(ATLAS_UVS[type][1])

const ImVec2 IMGUI_TIMELINE_FRAME_SIZE = {16, 40};
const ImVec2 IMGUI_TIMELINE_FRAME_CONTENT_OFFSET = {TEXTURE_SIZE_SMALL.x * 0.25f, (IMGUI_TIMELINE_FRAME_SIZE.y * 0.5f) - (TEXTURE_SIZE_SMALL.y * 0.5f)};
const ImVec2 IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE = {150, 0};
const ImVec2 IMGUI_TIMELINE_ITEM_SIZE = {300, 40};

const ImVec4 IMGUI_TIMELINE_FRAME_COLOR = {0.0f, 0.0f, 0.0f, 0.125};
const ImVec4 IMGUI_TIMELINE_FRAME_MULTIPLE_COLOR = {0.113, 0.184, 0.286, 0.125};
const ImVec4 IMGUI_TIMELINE_HEADER_FRAME_COLOR = {0.113, 0.184, 0.286, 0.5};
const ImVec4 IMGUI_TIMELINE_HEADER_FRAME_MULTIPLE_COLOR = {0.113, 0.184, 0.286, 1.0};
const ImVec4 IMGUI_INACTIVE_COLOR = {0.5, 0.5, 0.5, 1.0};
const ImVec4 IMGUI_ACTIVE_COLOR = {1.0, 1.0, 1.0, 1.0};

const ImVec2 IMGUI_SPRITESHEET_PREVIEW_SIZE = {125.0, 125.0};
const ImVec2 IMGUI_TOOLTIP_OFFSET = {16, 8};
const vec2 IMGUI_SPRITESHEET_EDITOR_CROP_FORGIVENESS = {1, 1};
const ImVec2 IMGUI_CANVAS_CHILD_SIZE = {200, 85};

const ImGuiKey IMGUI_INPUT_DELETE = ImGuiKey_Delete;
const ImGuiKey IMGUI_INPUT_LEFT = ImGuiKey_LeftArrow;
const ImGuiKey IMGUI_INPUT_RIGHT = ImGuiKey_RightArrow;
const ImGuiKey IMGUI_INPUT_UP = ImGuiKey_UpArrow;
const ImGuiKey IMGUI_INPUT_DOWN = ImGuiKey_DownArrow;
const ImGuiKey IMGUI_INPUT_MOD = ImGuiMod_Shift;
const ImGuiKey IMGUI_INPUT_ZOOM_IN = ImGuiKey_1;
const ImGuiKey IMGUI_INPUT_ZOOM_OUT = ImGuiKey_2;
const ImGuiKey IMGUI_INPUT_PLAY = ImGuiKey_Space;

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
enum ToolType
{
    TOOL_PAN,
    TOOL_MOVE,
    TOOL_ROTATE,
    TOOL_SCALE,
    TOOL_CROP,
    TOOL_DRAW,
    TOOL_ERASE,
    TOOL_COLOR_PICKER,
    TOOL_UNDO,
    TOOL_REDO,
    TOOL_COLOR,
    TOOL_COUNT,
};

#define SDL_MOUSE_CURSOR_NULL (-1)
const SDL_SystemCursor IMGUI_TOOL_MOUSE_CURSORS[TOOL_COUNT] = 
{
    SDL_SYSTEM_CURSOR_POINTER,
    SDL_SYSTEM_CURSOR_MOVE,
    SDL_SYSTEM_CURSOR_CROSSHAIR,
    SDL_SYSTEM_CURSOR_NE_RESIZE,
    SDL_SYSTEM_CURSOR_CROSSHAIR,
    SDL_SYSTEM_CURSOR_CROSSHAIR,
    SDL_SYSTEM_CURSOR_CROSSHAIR,
    SDL_SYSTEM_CURSOR_CROSSHAIR,
    SDL_SYSTEM_CURSOR_DEFAULT,
    SDL_SYSTEM_CURSOR_DEFAULT
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
    ToolType tool = TOOL_PAN;
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
    POPUP_BY_ITEM,
    POPUP_CENTER_SCREEN
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
    std::string popup{};
    std::string dragDrop{};
    std::string format{};
    std::string focusWindow{};
    ImguiFunction function = nullptr;
    ImGuiKeyChord chord = IMGUI_CHORD_NONE;
    TextureType texture = TEXTURE_NONE;
    PopupType popupType = POPUP_BY_ITEM;
    bool isUndoable = false;
    bool isSizeToText = true;
    ImguiColorSet color{};
    ImVec2 size{};
    ImVec2 contentOffset{};
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
    std::string popup{};
    std::string dragDrop{};
    std::string format{};
    std::string focusWindow{};
    ImguiFunction function = nullptr;
    ImGuiKeyChord chord = IMGUI_CHORD_NONE;
    TextureType texture = TEXTURE_NONE;
    PopupType popupType = POPUP_BY_ITEM;
    bool isUndoable = false;
    bool isInactive = false;
    bool isSizeToText = true;
    bool isSelected = false;
    f64 speed{};
    f64 min{};
    f64 max{};
    s32 border{};
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
 
    bool is_popup() const { return !popup.empty(); }
    bool is_tooltip() const { return !tooltip.empty(); }
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
        dragDrop      = builder.dragDrop;
        format        = builder.format;
        focusWindow   = builder.focusWindow;
        function      = builder.function;
        chord         = builder.chord;
        popupType     = builder.popupType;
        texture       = builder.texture;
        isUndoable    = builder.isUndoable;
        isSizeToText  = builder.isSizeToText;
        color         = builder.color;
        size          = builder.size;
        contentOffset = builder.contentOffset;
        speed         = builder.speed;
        min           = builder.min;
        max           = builder.max;
        border        = builder.border;
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
		anm2_serialize(self->anm2, self->anm2->path);
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

static inline void imgui_undo_stack_push(Imgui* self)
{
    Snapshot snapshot = {*self->anm2, *self->reference, self->preview->time};
    snapshots_undo_stack_push(self->snapshots, &snapshot);
}

static inline void imgui_tool_pan_set(Imgui* self)
{
    self->tool = TOOL_PAN;
}

static inline void imgui_tool_move_set(Imgui* self)
{
    self->tool = TOOL_MOVE;
}

static inline void imgui_tool_rotate_set(Imgui* self)
{
    self->tool = TOOL_ROTATE;
}

static inline void imgui_tool_scale_set(Imgui* self)
{
    self->tool = TOOL_SCALE;
}

static inline void imgui_tool_crop_set(Imgui* self)
{
    self->tool = TOOL_CROP;
}

static inline void imgui_tool_draw_set(Imgui* self)
{
    self->tool = TOOL_DRAW;
}

static inline void imgui_tool_erase_set(Imgui* self)
{
    self->tool = TOOL_ERASE;
}

static inline void imgui_tool_color_picker_set(Imgui* self)
{
    self->tool = TOOL_COLOR_PICKER;
}

static inline void imgui_undo(Imgui* self)
{
    snapshots_undo(self->snapshots);
}

static inline void imgui_redo(Imgui* self)
{
    snapshots_redo(self->snapshots);
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
    .popup = IMGUI_FILE_POPUP,
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

const inline ImguiItem IMGUI_TASKBAR_PLAYBACK = ImguiItem
({
    .label = "&Playback",
    .tooltip = "Opens the playback menu, for configuring playback settings.",
    .popup = IMGUI_PLAYBACK_POPUP,
    .isSizeToText = true
});

const inline ImguiItem IMGUI_PLAYBACK_ALWAYS_LOOP = ImguiItem
({
    .label = "&Always Loop",
    .tooltip = "Sets the animation playback to always loop, regardless of the animation's loop setting."
});

const inline ImguiItem IMGUI_TASKBAR_WIZARD = ImguiItem
({
    .label = "&Wizard",
    .tooltip = "Opens the wizard menu, for neat functions related to the .anm2.",
    .popup = IMGUI_WIZARD_POPUP,
    .isSizeToText  = true
});

const inline ImguiItem IMGUI_WIZARD_RECORD_GIF_ANIMATION = ImguiItem
({
    .label = "G&enerate GIF Animation",
    .tooltip = "Generates a GIF animation from the current animation.",
    .function = imgui_generate_gif_animation
});

const inline ImguiItem IMGUI_ANIMATIONS = ImguiItem({IMGUI_ANIMATIONS_LABEL});

const inline ImguiItem IMGUI_ANIMATION = ImguiItem
({
    .label = "## Animation Item",
    .dragDrop = "## Animation Drag Drop",
    .texture = TEXTURE_ANIMATION,
    .isUndoable = true,
});

const inline ImguiItem IMGUI_ANIMATION_ADD = ImguiItem
({
    .label = "Add",
    .tooltip = "Adds a new animation.",
    .isUndoable = true
});

const inline ImguiItem IMGUI_ANIMATION_DUPLICATE = ImguiItem
({
    .label = "Duplicate",
    .tooltip = "Duplicates the selected animation, placing it after.",
    .isUndoable = true
});

const inline ImguiItem IMGUI_ANIMATION_MERGE = ImguiItem
({
    .label = "Merge",
    .tooltip = "Open the animation merge popup, to merge animations together.",
    .popup = IMGUI_MERGE_POPUP,
    .popupType = POPUP_CENTER_SCREEN,
    .isUndoable = true
});

const inline ImguiItem IMGUI_MERGE_ANIMATIONS_CHILD = ImguiItem
({
    .label = "## Merge Animations",
    .size = {300, 250},
    .flags = true
});

const inline ImguiItem IMGUI_MERGE_ON_CONFLICT_CHILD = ImguiItem
({
    .label = "## Merge On Conflict",
    .size = {300, 75},
    .flags = true
});

const inline ImguiItem IMGUI_MERGE_ON_CONFLICT = ImguiItem({"On Conflict"});

const inline ImguiItem IMGUI_MERGE_APPEND_FRAMES = ImguiItem
({
    .label = "Append Frames ",
    .tooltip = "On frame conflict, the merged animation will have the selected animations' frames appended, in top-to-bottom order.",
    .value = 0
});

const inline ImguiItem IMGUI_MERGE_REPLACE_FRAMES = ImguiItem
({
    .label = "Replace Frames",
    .tooltip = "On frame conflict, the merged animation will have the latest selected animations' frames.",
    .value = 1
});

const inline ImguiItem IMGUI_MERGE_PREPEND_FRAMES = ImguiItem
({
    .label = "Prepend Frames",
    .tooltip = "On frame conflict, the merged animation will have the selected animations' frames prepend, in top-to-bottom order.",
    .value = 2
});

const inline ImguiItem IMGUI_MERGE_IGNORE = ImguiItem
({
    .label = "Ignore        ",
    .tooltip = "On frame conflict, the merged animation will ignore the other selected animations' frames.",
    .value = 3
});

const inline ImguiItem IMGUI_MERGE_OPTIONS_CHILD = ImguiItem
({
    .label = "## Merge Options",
    .size = {300, 35},
    .flags = true
});

const inline ImguiItem IMGUI_MERGE_DELETE_ANIMATIONS_AFTER = ImguiItem
({
    .label = "Delete Animations After Merging",
    .tooltip = "After merging, the selected animations (besides the original) will be deleted."
});

const inline ImguiItem IMGUI_MERGE_CONFIRM = ImguiItem
({
    .label = "Merge",
    .tooltip = "Merge the selected animations with the options set.",
    .size = {150, 25},
});

const inline ImguiItem IMGUI_MERGE_CANCEL = ImguiItem
({
    .label = "Cancel",
    .tooltip = "Cancel merging.",
    .size = {150, 25}
});

const inline ImguiItem IMGUI_ANIMATION_DEFAULT = ImguiItem
({
    .label = "Default",
    .tooltip = "Sets the selected animation to be the default.",
    .isUndoable = true
});

const inline ImguiItem IMGUI_ANIMATION_REMOVE = ImguiItem
({
    .label = "Remove",
    .tooltip = "Removes the selected animation.",
    .focusWindow = IMGUI_ANIMATIONS_LABEL,
    .chord = ImGuiKey_Delete,
    .isUndoable = true
});

const inline ImguiItem IMGUI_EVENTS = ImguiItem({IMGUI_EVENTS_LABEL});

const inline ImguiItem IMGUI_EVENT = ImguiItem
({
    .label = "## Event Item",
    .dragDrop = "## Event Drag Drop",
    .texture = TEXTURE_EVENT,
    .isUndoable = true
});

const inline ImguiItem IMGUI_EVENT_ADD = ImguiItem
({
    .label = "Add",
    .tooltip = "Adds a new event.",
    .isUndoable = true
});

const inline ImguiItem IMGUI_EVENT_REMOVE = ImguiItem
({
    .label = "Remove",
    .tooltip = "Remove the selected event.",
    .focusWindow = IMGUI_EVENTS_LABEL,
    .chord = ImGuiKey_Delete,
    .isUndoable = true
});

const inline ImguiItem IMGUI_SPRITESHEETS = ImguiItem({"Spritesheets"});

const inline ImguiItem IMGUI_SPRITESHEET_CHILD = ImguiItem
({
    .label = "## Spritesheet",
    .size = {0, 175},
    .flags = true
});

const inline ImguiItem IMGUI_SPRITESHEET_SELECTABLE = ImguiItem
({
    .label = "## Spritesheet Selectable",
    .dragDrop = "## Spritesheet Drag Drop",
    .texture = TEXTURE_SPRITESHEET,
    .flags = true
});

const inline ImguiItem IMGUI_SPRITESHEET_ADD = ImguiItem
({
    .label = "Add",
    .tooltip = "Select an image to add as a spritesheet.",
    .function = imgui_png_open
});

const inline ImguiItem IMGUI_SPRITESHEET_REMOVE = ImguiItem
({
    .label = "Remove",
    .tooltip = "Remove the selected spritesheet."
});

const inline ImguiItem IMGUI_SPRITESHEET_RELOAD = ImguiItem
({
    .label = "Reload",
    .tooltip = "Reload the selected spritesheet."
});

const inline ImguiItem IMGUI_SPRITESHEET_REPLACE = ImguiItem
({
    .label = "Replace",
    .tooltip = "Replace the selected spritesheet with another."
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW = ImguiItem
({
    .label = "Animation Preview",
    .flags =  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
});

const inline ImguiItem IMGUI_ANIMATION_PREVIEW_GRID_SETTINGS = ImguiItem
({
        .label      = "## Grid Settings",
        .size       = IMGUI_CANVAS_CHILD_SIZE,
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
    .min = EDITOR_GRID_MIN,
    .max = EDITOR_GRID_MAX
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
    .format = "%.0f",
    .isUndoable = true,
    .speed = 0.25f,
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_CROP = ImguiItem
({
    .label = "Crop",
    .tooltip = "Change the crop position of the selected frame.",
    .format = "%.0f",
    .isUndoable = true,
    .speed = 0.25f,
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_SIZE = ImguiItem
({
    .label = "Size",
    .tooltip = "Change the size of the crop of the selected frame.",
    .format = "%.0f",
    .isUndoable = true,
    .speed = 0.25f
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_PIVOT = ImguiItem
({
    .label = "Pivot",
    .tooltip = "Change the pivot of the selected frame.",
    .format = "%.0f",
    .isUndoable = true,
    .speed = 0.25f
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_SCALE = ImguiItem
({
    .label = "Scale",
    .tooltip = "Change the scale of the selected frame.",
    .format = "%.1f",
    .isUndoable = true,
    .speed = 0.25f
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_ROTATION = ImguiItem
({
    .label = "Rotation",
    .tooltip = "Change the rotation of the selected frame.",
    .format = "%.1f",
    .isUndoable = true,
    .speed = 0.25f
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_DURATION = ImguiItem
({
    .label = "Duration",
    .tooltip = "Change the duration of the selected frame.",
    .isUndoable = true,
    .min = 1,
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_TINT = ImguiItem
({
    .label = "Tint",
    .tooltip = "Change the tint of the selected frame.",
    .isUndoable = true,
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_COLOR_OFFSET = ImguiItem
({
    .label = "Color Offset",
    .tooltip = "Change the color offset of the selected frame.",
    .isUndoable = true,
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_VISIBLE = ImguiItem
({
    .label = "Visible",
    .tooltip = "Toggles the visibility of the selected frame.",
    .isUndoable = true,
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_INTERPOLATED = ImguiItem
({
    .label = "Interpolation",
    .tooltip = "Toggles the interpolation of the selected frame.",
    .isUndoable = true,
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_FLIP_X = ImguiItem
({
    .label = "Flip X",
    .tooltip = "Change the sign of the X scale, to cheat flipping the layer horizontally.\n(Anm2 doesn't support flipping directly)",
    .isUndoable = true
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_FLIP_Y = ImguiItem
({
    .label = "Flip Y",
    .tooltip = "Change the sign of the Y scale, to cheat flipping the layer vertically.\n(Anm2 doesn't support flipping directly)",
    .isUndoable = true
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_EVENT = ImguiItem
({
    .label = "Event",
    .tooltip = "Change the event the trigger uses.\nNOTE: This sets the event ID, not the event. If the events change IDs, then this will need to be changed.",
    .isUndoable = true
});

const inline ImguiItem IMGUI_FRAME_PROPERTIES_AT_FRAME = ImguiItem
({
    .label = "At Frame",
    .tooltip = "Change the frame where the trigger occurs.",
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
    .label = IMGUI_TIMELINE_LABEL,
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
    .texture = TEXTURE_ROOT,
    .size = IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_LAYER_SELECTABLE = ImguiItem
({
    .label = "## Layer Selectable",
    .tooltip = "A layer item.\nA graphical item within the animation.",
    .dragDrop = "## Layer Drag Drop",
    .texture = TEXTURE_LAYER,
    .size = IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_NULL_SELECTABLE = ImguiItem
({
    .label = "## Null Selectable",
    .tooltip = "A null item.\nAn invisible item within the animation that is accessible via a game engine.",
    .dragDrop = "## Null Drag Drop",
    .texture = TEXTURE_NULL,
    .size = IMGUI_TIMELINE_ITEM_SELECTABLE_SIZE
});

const inline ImguiItem IMGUI_TIMELINE_ITEM_TRIGGERS_SELECTABLE = ImguiItem
({
    .label = "Triggers",
    .tooltip = "The animation's triggers.\nWill fire based on an event.",
    .texture = TEXTURE_TRIGGERS,
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
    .label = "## Invisible",
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

const inline ImguiItem IMGUI_TIMELINE_FRAME = ImguiItem
({
    .label = "## Frame",
    .size = IMGUI_TIMELINE_FRAME_SIZE,
    .contentOffset = IMGUI_TIMELINE_FRAME_CONTENT_OFFSET,
    .border = IMGUI_TIMELINE_FRAME_BORDER,
});

const inline ImguiItem IMGUI_TIMELINE_ROOT_FRAME = ImguiItem
({
    .label = "## Root Frame",
    .color = {{0.020, 0.294, 0.569, 0.5}, {0.471, 0.882, 1.000, 0.75}, {0.314, 0.588, 0.843, 0.75}},
    .size = IMGUI_TIMELINE_FRAME_SIZE,
    .contentOffset = IMGUI_TIMELINE_FRAME_CONTENT_OFFSET,
    .border = IMGUI_TIMELINE_FRAME_BORDER,
});

const inline ImguiItem IMGUI_TIMELINE_LAYER_FRAME = ImguiItem
({
    .label = "## Layer Frame",
    .dragDrop = "## Layer Frame Drag Drop",
    .color = {{0.529, 0.157, 0.000, 0.5}, {1.000, 0.618, 0.324, 0.75}, {0.882, 0.412, 0.216, 0.75}},
    .size = IMGUI_TIMELINE_FRAME_SIZE,
    .contentOffset = IMGUI_TIMELINE_FRAME_CONTENT_OFFSET,
    .border = IMGUI_TIMELINE_FRAME_BORDER,
});

const inline ImguiItem IMGUI_TIMELINE_NULL_FRAME = ImguiItem
({
    .label = "## Null Frame",
    .dragDrop = "## Null Frame Drag Drop",
    .color = {{0.137, 0.353, 0.000, 0.5}, {0.646, 0.971, 0.441, 0.75}, {0.431, 0.647, 0.294, 0.75}},
    .size = IMGUI_TIMELINE_FRAME_SIZE,
    .contentOffset = IMGUI_TIMELINE_FRAME_CONTENT_OFFSET,
    .border = IMGUI_TIMELINE_FRAME_BORDER,
});

const inline ImguiItem IMGUI_TIMELINE_TRIGGERS_FRAME = ImguiItem
({
    .label = "## Triggers Frame",
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

const inline ImguiItem IMGUI_TIMELINE_ADD_ELEMENT = ImguiItem
({
    .label = "Add Element",
    .tooltip = "Adds an element (layer or null) to the animation.",
    .popup = IMGUI_TIMELINE_ADD_ELEMENT_POPUP
});

const inline ImguiItem IMGUI_TIMELINE_ADD_ELEMENT_LAYER = ImguiItem
({
    .label = "Layer",
    .tooltip = "Adds a layer element.\nA layer element is a primary graphical element, using a spritesheet.",
    .isUndoable = true
});

const inline ImguiItem IMGUI_TIMELINE_ADD_ELEMENT_NULL = ImguiItem
({
    .label = "Null",
    .tooltip = "Adds a null element.\nA null element is an invisible element, often accessed by the game engine.",
    .isUndoable = true
});

const inline ImguiItem IMGUI_TIMELINE_REMOVE_ELEMENT = ImguiItem
({
    .label = "Remove Element",
    .tooltip = "Removes an element (layer or null) from the animation."
});

const inline ImguiItem IMGUI_TIMELINE_PLAY = ImguiItem
({
    .label = "|>  Play",
    .tooltip = "Play the current animation, if paused."
});

const inline ImguiItem IMGUI_TIMELINE_PAUSE = ImguiItem
({
    .label = "|| Pause",
    .tooltip = "Pause the current animation, if playing."
});

const inline ImguiItem IMGUI_TIMELINE_ADD_FRAME = ImguiItem
({
    .label = "Add Frame",
    .tooltip = "Adds a frame to the selected animation item.",
    .isUndoable = true
});

const inline ImguiItem IMGUI_TIMELINE_REMOVE_FRAME = ImguiItem
({
    .label = "Remove Frame",
    .tooltip = "Removes the selected frame from the selected animation item.",
    .focusWindow = IMGUI_TIMELINE_LABEL,
    .chord = ImGuiKey_Delete,
    .isUndoable = true,
});

const inline ImguiItem IMGUI_TIMELINE_FIT_ANIMATION_LENGTH = ImguiItem
({
    .label = "Fit Animation Length",
    .tooltip = "Sets the animation's length to the latest frame.",
    .isUndoable = true
});

const inline ImguiItem IMGUI_TIMELINE_ANIMATION_LENGTH = ImguiItem
({
    .label = "Animation Length",
    .tooltip = "Sets the animation length.\n(Will not change frames.)",
    .isUndoable = true,
    .size = {100, 0},
    .min = ANM2_FRAME_NUM_MIN,
    .max = ANM2_FRAME_NUM_MAX
});

const inline ImguiItem IMGUI_TIMELINE_FPS = ImguiItem
({
    .label = "FPS",
    .tooltip = "Sets the animation's frames per second (its speed).",
    .isUndoable = true,
    .size = {100, 0},
    .min = ANM2_FPS_MIN,
    .max = ANM2_FPS_MAX
});

const inline ImguiItem IMGUI_TIMELINE_LOOP = ImguiItem
({
    .label = "Loop",
    .tooltip = "Toggles the animation looping.",
    .isUndoable = true
});

const inline ImguiItem IMGUI_TIMELINE_CREATED_BY = ImguiItem
({
    .label = "Author",
    .tooltip = "Sets the author of the animation.",
    .isUndoable = true,
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

const inline ImguiItem IMGUI_CUT = ImguiItem
({
    .label          = "Cut",
    .tooltip        = "Cuts the currently selected contextual element; removing it and putting it to the clipboard.",
    .function = imgui_cut,
    .chord = ImGuiMod_Ctrl | ImGuiKey_X
});

const inline ImguiItem IMGUI_COPY = ImguiItem
({
    .label          = "Copy",
    .tooltip        = "Copies the currently selected contextual element to the clipboard.",
    .function = imgui_copy,
    .chord = ImGuiMod_Ctrl | ImGuiKey_C
});

const inline ImguiItem IMGUI_PASTE = ImguiItem
({
    .label          = "Paste",
    .tooltip        = "Pastes the currently selection contextual element from the clipboard.",
    .function = imgui_paste,
    .chord = ImGuiMod_Ctrl | ImGuiKey_V
});

const inline ImguiItem IMGUI_RENAMABLE = ImguiItem
({
    .label = "## Renaming",
    .tooltip = "Rename the selected item.",
    .max = 255,
    .flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue
});

const inline ImguiItem IMGUI_CHANGEABLE = ImguiItem
({
    .label = "## Changing",
    .tooltip = "Change the selected item's value.",
    .step = 0
});

const inline ImguiItem IMGUI_EXIT_CONFIRMATION = ImguiItem
({
    .label = "Unsaved changes will be lost!\nAre you sure you want to exit?",
    .popup = IMGUI_EXIT_CONFIRMATION_POPUP
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

void imgui_tick(Imgui* self);
void imgui_draw();
void imgui_free();