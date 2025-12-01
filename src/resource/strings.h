#pragma once

namespace anm2ed
{
#define SELECT_ENGLISH(symbol, english, spanish, russian, turkish, chinese, korean) english,
#define SELECT_SPANISH(symbol, english, spanish, russian, turkish, chinese, korean) spanish,
#define SELECT_RUSSIAN(symbol, english, spanish, russian, turkish, chinese, korean) russian,
#define SELECT_TURKISH(symbol, english, spanish, russian, turkish, chinese, korean) turkish,
#define SELECT_CHINESE(symbol, english, spanish, russian, turkish, chinese, korean) chinese,
#define SELECT_KOREAN(symbol, english, spanish, russian, turkish, chinese, korean) korean,

#define LANGUAGES(X)                                                                                                   \
  X(0, ENGLISH, "English", SELECT_ENGLISH)                                                                             \
  X(1, SPANISH, "Spanish", SELECT_SPANISH)                                                                             \
  X(2, RUSSIAN, "Russian", SELECT_RUSSIAN)                                                                             \
  X(3, TURKISH, "Turkish", SELECT_TURKISH)                                                                             \
  X(4, CHINESE, "Chinese", SELECT_CHINESE)                                                                             \
  X(5, KOREAN, "Korean", SELECT_KOREAN)

  enum Language
  {
#define X(idx, symbol, label, selector) symbol = idx,
    LANGUAGES(X)
#undef X
        LANGUAGE_COUNT
  };

  static constexpr const char* LANGUAGE_STRINGS[LANGUAGE_COUNT] = {
#define X(idx, symbol, label, selector) label,
      LANGUAGES(X)
#undef X
  };

#define STRINGS(X)                                                                                                     \
  X(BASIC_YES, "Yes", "", "", "", "", "")                                                                              \
  X(BASIC_NO, "No", "", "", "", "", "")                                                                                \
  X(BASIC_CONFIRM, "Confirm", "", "", "", "", "")                                                                      \
  X(BASIC_CANCEL, "Cancel", "", "", "", "", "")                                                                        \
  X(BASIC_FRAMES, "Frames", "", "", "", "", "")                                                                        \
  X(BASIC_COLOR, "Color", "", "", "", "", "")                                                                          \
  X(BASIC_ENABLED, "Enabled", "", "", "", "", "")                                                                      \
  X(BASIC_GRID, "Grid", "", "", "", "", "")                                                                            \
  X(BASIC_SIZE, "Size", "", "", "", "", "")                                                                            \
  X(BASIC_OFFSET, "Offset", "", "", "", "", "")                                                                        \
  X(BASIC_ZOOM, "Zoom", "", "", "", "", "")                                                                            \
  X(BASIC_ALPHA, "Alpha", "", "", "", "", "")                                                                          \
  X(BASIC_POSITION, "Position", "", "", "", "", "")                                                                    \
  X(BASIC_PIVOT, "Pivot", "", "", "", "", "")                                                                          \
  X(BASIC_SCALE, "Scale", "", "", "", "", "")                                                                          \
  X(BASIC_ROTATION, "Rotation", "", "", "", "", "")                                                                    \
  X(BASIC_BEFORE, "Before", "", "", "", "", "")                                                                        \
  X(BASIC_AFTER, "After", "", "", "", "", "")                                                                          \
  X(BASIC_INDEX, "Index", "", "", "", "", "")                                                                          \
  X(BASIC_TIME, "Time", "", "", "", "", "")                                                                            \
  X(BASIC_ID, "ID", "", "", "", "", "")                                                                                \
  X(BASIC_CUT, "Cut", "", "", "", "", "")                                                                              \
  X(BASIC_COPY, "Copy", "", "", "", "", "")                                                                            \
  X(BASIC_PASTE, "Paste", "", "", "", "", "")                                                                          \
  X(BASIC_NAME, "Name", "", "", "", "", "")                                                                            \
  X(BASIC_CROP, "Crop", "", "", "", "", "")                                                                            \
  X(BASIC_DURATION, "Duration", "", "", "", "", "")                                                                    \
  X(BASIC_TINT, "Tint", "", "", "", "", "")                                                                            \
  X(BASIC_COLOR_OFFSET, "Color Offset", "", "", "", "", "")                                                            \
  X(BASIC_ADD, "Add", "", "", "", "", "")                                                                              \
  X(BASIC_DUPLICATE, "Duplicate", "", "", "", "", "")                                                                  \
  X(BASIC_REMOVE, "Remove", "", "", "", "", "")                                                                        \
  X(BASIC_DEFAULT, "Default", "", "", "", "", "")                                                                      \
  X(LABEL_ANIMATION_PREVIEW_WINDOW, "Animation Preview###Animation Preview", "", "", "", "", "")                       \
  X(LABEL_ANIMATIONS_CHILD, "Animations", "", "", "", "", "")                                                          \
  X(LABEL_ALT_ICONS, "Alt Icons", "", "", "", "", "")                                                                  \
  X(LABEL_BACKGROUND_COLOR, "Background", "", "", "", "", "")                                                          \
  X(LABEL_AXES, "Axes", "", "", "", "", "")                                                                            \
  X(LABEL_BORDER, "Border", "", "", "", "", "")                                                                        \
  X(LABEL_CENTER_VIEW, "Center View", "", "", "", "", "")                                                              \
  X(LABEL_FIT, "Fit", "", "", "", "", "")                                                                              \
  X(LABEL_OVERLAY, "Overlay", "", "", "", "", "")                                                                      \
  X(LABEL_PIVOTS, "Pivots", "", "", "", "", "")                                                                        \
  X(LABEL_ROOT_TRANSFORM, "Root Transform", "", "", "", "", "")                                                        \
  X(LABEL_ONIONSKIN, "Onionskin###Onionskin", "", "", "", "", "")                                                      \
  X(LABEL_SOUNDS, "Sounds###Sounds", "", "", "", "", "")                                                               \
  X(LABEL_RECORDING_PROGRESS, "Once recording is complete, rendering may take some time.\\nPlease be patient...", "",  \
    "", "", "", "")                                                                                                    \
  X(TOAST_SOUNDS_PASTE, "Paste Sound(s)", "", "", "", "", "")                                                          \
  X(TOAST_SOUNDS_PASTE_ERROR, "Failed to paste sound(s)", "", "", "", "", "")                                          \
  X(TOAST_SPRITESHEET_NO_FRAMES, "No frames captured for spritesheet export.", "", "", "", "", "")                     \
  X(TOAST_SPRITESHEET_EMPTY, "Spritesheet export failed: captured frames are empty.", "", "", "", "", "")              \
  X(TOOLTIP_GRID_VISIBILITY, "Toggle the visibility of the grid.", "", "", "", "", "")                                 \
  X(TOOLTIP_GRID_COLOR, "Change the grid's color.", "", "", "", "", "")                                                \
  X(TOOLTIP_GRID_SIZE, "Change the size of all cells in the grid.", "", "", "", "", "")                                \
  X(TOOLTIP_GRID_OFFSET, "Change the offset of the grid.", "", "", "", "", "")                                         \
  X(TOOLTIP_PREVIEW_ZOOM, "Change the zoom of the preview.", "", "", "", "", "")                                       \
  X(TOOLTIP_CENTER_VIEW, "Centers the view.", "", "", "", "", "")                                                      \
  X(TOOLTIP_FIT, "Set the view to match the extent of the animation.", "", "", "", "", "")                             \
  X(TOOLTIP_BACKGROUND_COLOR, "Change the background color.", "", "", "", "", "")                                      \
  X(TOOLTIP_AXES, "Toggle the axes' visibility.", "", "", "", "", "")                                                  \
  X(TOOLTIP_AXES_COLOR, "Set the color of the axes.", "", "", "", "", "")                                              \
  X(TOOLTIP_OVERLAY, "Set an animation to be drawn over the current animation.", "", "", "", "", "")                   \
  X(TOOLTIP_OVERLAY_ALPHA, "Set the alpha of the overlayed animation.", "", "", "", "", "")                            \
  X(TOOLTIP_ROOT_TRANSFORM, "Root frames will transform the rest of the animation.", "", "", "", "", "")               \
  X(TOOLTIP_PIVOTS, "Toggle the visibility of the animation's pivots.", "", "", "", "", "")                            \
  X(TOOLTIP_ALT_ICONS, "Toggle a different appearance of the target icons.", "", "", "", "", "")                       \
  X(TOOLTIP_BORDER, "Toggle the visibility of borders around layers.", "", "", "", "", "")                             \
  X(TOOLTIP_ONIONSKIN_ENABLED, "Toggle onionskinning.", "", "", "", "", "")                                            \
  X(TOOLTIP_ONIONSKIN_FRAMES, "Change the amount of frames this onionskin will use.", "", "", "", "", "")              \
  X(TOOLTIP_ONIONSKIN_COLOR, "Change the color this onionskin will use.", "", "", "", "", "")                          \
  X(TOOLTIP_ONIONSKIN_TIME, "The onionskinned frames will be based on frame time.", "", "", "", "", "")                \
  X(TOOLTIP_ONIONSKIN_INDEX, "The onionskinned frames will be based on frame index.", "", "", "", "", "")              \
  X(TOOLTIP_SOUNDS_PLAY, "Click to play.", "", "", "", "", "")                                                         \
  X(TOAST_EXPORT_RENDERED_FRAMES, "Exported rendered frames to:", "", "", "", "", "")                                  \
  X(TOAST_EXPORT_RENDERED_FRAMES_FAILED, "Could not export frames to:", "", "", "", "", "")                            \
  X(TOAST_EXPORT_SPRITESHEET, "Exported spritesheet to:", "", "", "", "", "")                                          \
  X(TOAST_EXPORT_SPRITESHEET_FAILED, "Could not export spritesheet to:", "", "", "", "", "")                           \
  X(TOAST_EXPORT_RENDERED_ANIMATION, "Exported rendered animation to:", "", "", "", "", "")                            \
  X(TOAST_EXPORT_RENDERED_ANIMATION_FAILED, "Could not output rendered animation:", "", "", "", "", "")                \
  X(LABEL_ANIMATIONS_WINDOW, "Animations###Animations", "", "", "", "", "")                                            \
  X(LABEL_MERGE, "Merge", "", "", "", "", "")                                                                          \
  X(LABEL_CLOSE, "Close", "", "", "", "", "")                                                                          \
  X(LABEL_LENGTH, "Length", "", "", "", "", "")                                                                        \
  X(LABEL_LOOP, "Loop", "", "", "", "", "")                                                                            \
  X(LABEL_APPEND_FRAMES, "Append Frames", "", "", "", "", "")                                                          \
  X(LABEL_PREPEND_FRAMES, "Prepend Frames", "", "", "", "", "")                                                        \
  X(LABEL_REPLACE_FRAMES, "Replace Frames", "", "", "", "", "")                                                        \
  X(LABEL_IGNORE_FRAMES, "Ignore Frames", "", "", "", "", "")                                                          \
  X(LABEL_DELETE_ANIMATIONS_AFTER, "Delete Animations After", "", "", "", "", "")                                      \
  X(TOOLTIP_ADD_ANIMATION, "Add a new animation.", "", "", "", "", "")                                                 \
  X(TOOLTIP_DUPLICATE_ANIMATION, "Duplicate the selected animation(s).", "", "", "", "", "")                           \
  X(TOOLTIP_OPEN_MERGE_POPUP,                                                                                          \
    "Open the merge popup.\nUsing the shortcut will merge the animations with\nthe last configured merge settings.",   \
    "", "", "", "", "")                                                                                                \
  X(TOOLTIP_REMOVE_ANIMATION, "Remove the selected animation(s).", "", "", "", "", "")                                 \
  X(TOOLTIP_SET_DEFAULT_ANIMATION, "Set the selected animation as the default.", "", "", "", "", "")                   \
  X(TOOLTIP_DELETE_ANIMATIONS_AFTER, "Delete animations after merging them.", "", "", "", "", "")                      \
  X(TOAST_DESERIALIZE_ANIMATIONS_FAILED, "Failed to deserialize animation(s):", "", "", "", "", "")                    \
  X(LABEL_EVENT, "Event", "", "", "", "", "")                                                                          \
  X(LABEL_SOUND, "Sound", "", "", "", "", "")                                                                          \
  X(LABEL_APPEND, "Append", "", "", "", "", "")                                                                        \
  X(LABEL_REPLACE, "Replace", "", "", "", "", "")                                                                      \
  X(EDIT_RENAME_EVENT, "Rename Event", "", "", "", "", "")                                                             \
  X(EDIT_ADD_EVENT, "Add Event", "", "", "", "", "")                                                                   \
  X(LABEL_EVENTS_WINDOW, "Events###Events", "", "", "", "", "")                                                        \
  X(LABEL_REMOVE_UNUSED_EVENTS, "Remove Unused", "", "", "", "", "")                                                   \
  X(TOOLTIP_TRIGGER_EVENT, "Change the event this trigger uses.", "", "", "", "", "")                                  \
  X(TOOLTIP_TRIGGER_SOUND, "Change the sound this trigger uses.", "", "", "", "", "")                                  \
  X(TOOLTIP_TRIGGER_AT_FRAME, "Change the frame the trigger will be activated at.", "", "", "", "", "")                \
  X(TOOLTIP_TRIGGER_VISIBILITY, "Toggle the trigger's visibility.", "", "", "", "", "")                                \
  X(TOOLTIP_ADD_EVENT, "Add an event.", "", "", "", "", "")                                                            \
  X(TOOLTIP_REMOVE_UNUSED_EVENTS, "Remove unused events (i.e., ones not used by any trigger in any animation.)", "",   \
    "", "", "", "")                                                                                                    \
  X(TOAST_DESERIALIZE_EVENTS_FAILED, "Failed to deserialize event(s):", "", "", "", "", "")                            \
  X(EDIT_TRIGGER_EVENT, "Trigger Event", "", "", "", "", "")                                                           \
  X(EDIT_TRIGGER_SOUND, "Trigger Sound", "", "", "", "", "")                                                           \
  X(EDIT_TRIGGER_AT_FRAME, "Trigger At Frame", "", "", "", "", "")                                                     \
  X(EDIT_TRIGGER_VISIBILITY, "Trigger Visibility", "", "", "", "", "")                                                 \
  X(EDIT_PASTE_EVENTS, "Paste Event(s)", "", "", "", "", "")                                                           \
  X(EDIT_PASTE_LAYERS, "Paste Layer(s)", "", "", "", "", "")                                                           \
  X(EDIT_REMOVE_UNUSED_EVENTS, "Remove Unused Events", "", "", "", "", "")                                             \
  X(TOOLTIP_ITEM_NAME, "Set the item's name.", "", "", "", "", "")                                                     \
  X(TOOLTIP_LAYER_SPRITESHEET, "Set the layer item's spritesheet.", "", "", "", "", "")                                \
  X(LABEL_LAYERS_WINDOW, "Layers###Layers", "", "", "", "", "")                                                        \
  X(LABEL_LAYERS_CHILD, "Layers List", "", "", "", "", "")                                                             \
  X(LABEL_REMOVE_UNUSED_LAYERS, "Remove Unused", "", "", "", "", "")                                                   \
  X(TOOLTIP_ADD_LAYER, "Add a layer.", "", "", "", "", "")                                                             \
  X(TOOLTIP_REMOVE_UNUSED_LAYERS, "Remove unused layers (i.e., ones not used in any animation.)", "", "", "", "", "")  \
  X(TOAST_DESERIALIZE_LAYERS_FAILED, "Failed to deserialize layer(s):", "", "", "", "", "")                            \
  X(EDIT_ADD_LAYER, "Add Layer", "", "", "", "", "")                                                                   \
  X(EDIT_REMOVE_UNUSED_LAYERS, "Remove Unused Layers", "", "", "", "", "")                                             \
  X(EDIT_SET_LAYER_PROPERTIES, "Set Layer Properties", "", "", "", "", "")                                             \
  X(LABEL_FRAME_PROPERTIES_WINDOW, "Frame Properties###Frame Properties", "", "", "", "", "")                          \
  X(BASIC_FRAME_PROPERTIES_EVENT, "Event", "", "", "", "", "")                                                         \
  X(BASIC_FRAME_PROPERTIES_SOUND, "Sound", "", "", "", "", "")                                                         \
  X(BASIC_FRAME_PROPERTIES_AT_FRAME, "At Frame", "", "", "", "", "")                                                   \
  X(BASIC_VISIBLE, "Visible", "", "", "", "", "")                                                                      \
  X(BASIC_INTERPOLATED, "Interpolated", "", "", "", "", "")                                                            \
  X(LABEL_CROP, "Crop", "", "", "", "", "")                                                                            \
  X(LABEL_TINT, "Tint", "", "", "", "", "")                                                                            \
  X(LABEL_COLOR_OFFSET, "Color Offset", "", "", "", "", "")                                                            \
  X(LABEL_FLIP_X, "Flip X", "", "", "", "", "")                                                                        \
  X(LABEL_FLIP_Y, "Flip Y", "", "", "", "", "")                                                                        \
  X(TOOLTIP_CROP, "Change the crop position the frame uses.", "", "", "", "", "")                                      \
  X(TOOLTIP_SIZE, "Change the size of the crop the frame uses.", "", "", "", "", "")                                   \
  X(TOOLTIP_POSITION, "Change the position of the frame.", "", "", "", "", "")                                         \
  X(TOOLTIP_PIVOT, "Change the pivot of the frame; i.e., where it is centered.", "", "", "", "", "")                   \
  X(TOOLTIP_SCALE, "Change the scale of the frame, in percent.", "", "", "", "", "")                                   \
  X(TOOLTIP_ROTATION, "Change the rotation of the frame.", "", "", "", "", "")                                         \
  X(TOOLTIP_DURATION, "Change how long the frame lasts.", "", "", "", "", "")                                          \
  X(TOOLTIP_TINT, "Change the tint of the frame.", "", "", "", "", "")                                                 \
  X(TOOLTIP_COLOR_OFFSET, "Change the color added onto the frame.", "", "", "", "", "")                                \
  X(TOOLTIP_FRAME_VISIBILITY, "Toggle the frame's visibility.", "", "", "", "", "")                                    \
  X(TOOLTIP_FRAME_INTERPOLATION,                                                                                       \
    "Toggle the frame interpolating; i.e., blending its values into the next frame based on the time.", "", "", "",    \
    "", "")                                                                                                            \
  X(TOOLTIP_FLIP_X,                                                                                                    \
    "Flip the horizontal scale of the frame, to cheat mirroring the frame horizontally.\n(Note: the format does not "  \
    "support mirroring.)",                                                                                             \
    "", "", "", "", "")                                                                                                \
  X(TOOLTIP_FLIP_Y,                                                                                                    \
    "Flip the vertical scale of the frame, to cheat mirroring the frame vertically.\n(Note: the format does not "      \
    "support mirroring.)",                                                                                             \
    "", "", "", "", "")                                                                                                \
  X(EDIT_FRAME_CROP, "Frame Crop", "", "", "", "", "")                                                                 \
  X(EDIT_FRAME_SIZE, "Frame Size", "", "", "", "", "")                                                                 \
  X(EDIT_FRAME_POSITION, "Frame Position", "", "", "", "", "")                                                         \
  X(EDIT_FRAME_PIVOT, "Frame Pivot", "", "", "", "", "")                                                               \
  X(EDIT_FRAME_SCALE, "Frame Scale", "", "", "", "", "")                                                               \
  X(EDIT_FRAME_ROTATION, "Frame Rotation", "", "", "", "", "")                                                         \
  X(EDIT_FRAME_DURATION, "Frame Duration", "", "", "", "", "")                                                         \
  X(EDIT_FRAME_TINT, "Frame Tint", "", "", "", "", "")                                                                 \
  X(EDIT_FRAME_COLOR_OFFSET, "Frame Color Offset", "", "", "", "", "")                                                 \
  X(EDIT_FRAME_VISIBILITY, "Frame Visibility", "", "", "", "", "")                                                     \
  X(EDIT_FRAME_INTERPOLATION, "Frame Interpolation", "", "", "", "", "")                                               \
  X(EDIT_FRAME_FLIP_X, "Frame Flip X", "", "", "", "", "")                                                             \
  X(EDIT_FRAME_FLIP_Y, "Frame Flip Y", "", "", "", "", "")                                                             \
  X(LABEL_ADJUST, "Adjust", "", "", "", "", "")                                                                        \
  X(LABEL_SUBTRACT, "Subtract", "", "", "", "", "")                                                                    \
  X(LABEL_MULTIPLY, "Multiply", "", "", "", "", "")                                                                    \
  X(LABEL_DIVIDE, "Divide", "", "", "", "", "")                                                                        \
  X(TOOLTIP_ADJUST, "Set the value of each specified value onto the frame's equivalent.", "", "", "", "", "")          \
  X(TOOLTIP_ADD_VALUES, "Add the specified values onto each frame.\\n(Boolean values will simply be set.)", "", "",    \
    "", "", "")                                                                                                        \
  X(TOOLTIP_SUBTRACT_VALUES, "Subtract the specified values from each frame.\\n(Boolean values will simply be set.)",  \
    "", "", "", "", "")                                                                                                \
  X(TOOLTIP_MULTIPLY_VALUES, "Multiply the specified values for each frame.\\n(Boolean values will simply be set.)",   \
    "", "", "", "", "")                                                                                                \
  X(TOOLTIP_DIVIDE_VALUES, "Divide the specified values for each frame.\\n(Boolean values will simply be set.)", "",   \
    "", "", "", "")                                                                                                    \
  X(EDIT_CHANGE_FRAME_PROPERTIES, "Change Frame Properties", "", "", "", "", "")                                       \
  X(EDIT_MOVE_ANIMATIONS, "Move Animation(s)", "", "", "", "", "")                                                     \
  X(EDIT_CUT_ANIMATIONS, "Cut Animation(s)", "", "", "", "", "")                                                       \
  X(EDIT_PASTE_ANIMATIONS, "Paste Animation(s)", "", "", "", "", "")                                                   \
  X(EDIT_ADD_ANIMATION, "Add Animation", "", "", "", "", "")                                                           \
  X(EDIT_DUPLICATE_ANIMATIONS, "Duplicate Animation(s)", "", "", "", "", "")                                           \
  X(EDIT_MERGE_ANIMATIONS, "Merge Animations", "", "", "", "", "")                                                     \
  X(EDIT_REMOVE_ANIMATIONS, "Remove Animation(s)", "", "", "", "", "")                                                 \
  X(EDIT_DEFAULT_ANIMATION, "Default Animation", "", "", "", "", "")                                                   \
  X(SNAPSHOT_RENAME_ANIMATION, "Rename Animation", "", "", "", "", "")                                                 \
  X(LABEL_SPRITESHEET, "Spritesheet", "", "", "", "", "")                                                              \
  X(LABEL_NULLS_WINDOW, "Nulls###Nulls", "", "", "", "", "")                                                        \
  X(LABEL_NULLS_CHILD, "Nulls List", "", "", "", "", "")                                                               \
  X(LABEL_REMOVE_UNUSED_NULLS, "Remove Unused", "", "", "", "", "")                                                   \
  X(TOOLTIP_ADD_NULL, "Add a null.", "", "", "", "", "")                                                               \
  X(TOOLTIP_REMOVE_UNUSED_NULLS, "Remove unused nulls (i.e., ones not used in any animation.)", "", "", "", "", "") \
  X(TOAST_DESERIALIZE_NULLS_FAILED, "Failed to deserialize null(s):", "", "", "", "", "")                             \
  X(EDIT_PASTE_NULLS, "Paste Null(s)", "", "", "", "", "")                                                             \
  X(EDIT_ADD_NULL, "Add Null", "", "", "", "", "")                                                                      \
  X(EDIT_SET_NULL_PROPERTIES, "Set Null Properties", "", "", "", "", "")                                               \
  X(EDIT_REMOVE_UNUSED_NULLS, "Remove Unused Nulls", "", "", "", "", "")                                               \
  X(TOOLTIP_NULL_NAME, "Set the null's name.", "", "", "", "", "")                                                     \
  X(TOOLTIP_NULL_RECT, "The null will have a rectangle show around it.", "", "", "", "", "")                           \
  X(LABEL_RECT, "Rect", "", "", "", "", "")                                                                            \
  X(TOOLTIP_POSITION_LABEL, "Position:", "", "", "", "", "")                                                           \
  X(TOOLTIP_SCALE_LABEL, "Scale:", "", "", "", "", "")                                                                 \
  X(TOOLTIP_ROTATION_LABEL, "Rotation:", "", "", "", "", "")

  enum StringType
  {
#define X(symbol, ...) symbol,
    STRINGS(X)
#undef X
        STRING_TYPE_COUNT
  };

#define DEFINE_LANGUAGE_ARRAY(idx, symbol, label, selector)                                                            \
  static constexpr const char* STRINGS_##symbol[] = {STRINGS(selector)};
  LANGUAGES(DEFINE_LANGUAGE_ARRAY)
#undef DEFINE_LANGUAGE_ARRAY

  static constexpr const char* const* LANGUAGE_STRING_COLLECTIONS[LANGUAGE_COUNT] = {
#define X(idx, symbol, label, selector) STRINGS_##symbol,
      LANGUAGES(X)
#undef X
  };

#undef SELECT_ENGLISH
#undef SELECT_SPANISH
#undef SELECT_RUSSIAN
#undef SELECT_TURKISH
#undef SELECT_CHINESE
#undef SELECT_KOREAN
#undef STRINGS
#undef LANGUAGES

  class Localizer
  {
  public:
    Language language{ENGLISH};

    inline const char* get(StringType type) const
    {
      if (language < 0 || language >= LANGUAGE_COUNT || type < 0 || type >= STRING_TYPE_COUNT) return "undefined";
      return LANGUAGE_STRING_COLLECTIONS[language][type];
    }
  };
}

extern anm2ed::Localizer localize;
