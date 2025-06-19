#pragma once

#define STRING_UNDEFINED "undefined"
#define STRING_EMPTY ""

#define STRING_WINDOW_TITLE "Anm2Ed"
#define STRING_WINDOW_TITLE_EDITING "Anm2Ed (%s)"

#define STRING_ERROR_SDL_INIT "[ERROR] Could not initialize SDL (%s)\n"
#define STRING_ERROR_GL_CONTEXT_INIT "[ERROR] Could not initialize OpenGL context (%s)\n"
#define STRING_ERROR_FILE_READ "[ERROR] Could not read from file: %s\n"
#define STRING_ERROR_SHADER_INIT "[ERROR] Could not initialize shader: %i\n%s"
#define STRING_ERROR_TEXTURE_INIT "[ERROR] Could not initialize texture: %s\n"
#define STRING_ERROR_ANM2_READ "[ERROR] Could not read anm2 from file %s: %s\n"
#define STRING_ERROR_ANM2_WRITE "[ERROR] Could not write anm2 to file %s: %s\n"

#define STRING_INFO_INIT "[INFO] Initializing\n"
#define STRING_INFO_QUIT "[INFO] Exited\n"
#define STRING_INFO_SDL_INIT "[INFO] Initialized SDL\n"
#define STRING_INFO_SDL_QUIT "[INFO] Quit SDL\n"
#define STRING_INFO_GLEW_INIT "[INFO] Initialized GLEW\n"
#define STRING_INFO_GL_CONTEXT_INIT "[INFO] Initialized OpenGL context\n"
#define STRING_INFO_FILE_READ "[INFO] Read from file: %s\n"
#define STRING_INFO_SHADER_INIT "[INFO] Initialized shader: %i\n"
#define STRING_INFO_TEXTURE_INIT "[INFO] Initialized texture: %s\n"
#define STRING_INFO_ANM2_WRITE "[INFO] Wrote anm2 to file: %s\n"
#define STRING_INFO_ANM2_READ "[INFO] Read anm2 from file: %s\n"

#define STRING_INFO_IMGUI_INIT "[INFO] Initialized Dear Imgui\n"
#define STRING_INFO_IMGUI_FREE "[INFO] Freed Dear Imgui\n"

#define STRING_DIALOG_ANM2_READ "Select .anm2 file to read..."
#define STRING_DIALOG_ANM2_WRITE "Select .anm2 file to write to..."
#define STRING_DIALOG_ANM2_FILTER "Anm2 Files (*.anm2)"
#define STRING_DIALOG_ANM2_FILTER_WILDCARD "*.anm2"
#define STRING_DIALOG_ALL_FILTER "All Files (*.*)"
#define STRING_DIALOG_ALL_FILTER_WILDCARD "*"

#define STRING_ANM2_CREATED_BY_DEFAULT "Unknown"
#define STRING_ANM2_NEW_ANIMATION "New Animation"
#define STRING_ANM2_NEW_LAYER "New Layer"
#define STRING_ANM2_NEW_NULL "New Null"
#define STRING_ANM2_NEW_EVENT "New Event"
#define STRING_ANM2_ROOT "Root"

#define STRING_IMGUI_WINDOW "Window"
#define STRING_IMGUI_DOCKSPACE "Dockspace"
#define STRING_IMGUI_TASKBAR "Taskbar"
#define STRING_IMGUI_TASKBAR_FILE "File"

#define STRING_IMGUI_FILE_MENU "File Menu"
#define STRING_IMGUI_FILE_NEW "New"
#define STRING_IMGUI_FILE_OPEN "Open"
#define STRING_IMGUI_FILE_SAVE "Save"
#define STRING_IMGUI_FILE_SAVE_AS "Save As"

#define STRING_IMGUI_PROPERTIES "Properties"
#define STRING_IMGUI_PROPERTIES_FPS "FPS"
#define STRING_IMGUI_PROPERTIES_FPS_LABEL "##FPS"
#define STRING_IMGUI_PROPERTIES_CREATED_BY "Created by:"
#define STRING_IMGUI_PROPERTIES_CREATED_BY_LABEL "##Created By"
#define STRING_IMGUI_PROPERTIES_CREATED_ON "Created on: %s"
#define STRING_IMGUI_PROPERTIES_VERSION "Version: %i"

#define STRING_IMGUI_ANIMATIONS "Animations"
#define STRING_IMGUI_ANIMATIONS_ANIMATION_LABEL "##Animation"
#define STRING_IMGUI_ANIMATIONS_ADD "Add"
#define STRING_IMGUI_ANIMATIONS_REMOVE "Remove"
#define STRING_IMGUI_ANIMATIONS_DUPLICATE "Duplicate"
#define STRING_IMGUI_ANIMATIONS_SET_AS_DEFAULT "Set as Default"
#define STRING_IMGUI_ANIMATIONS_DEFAULT_ANIMATION_FORMAT "%s (*)"

#define STRING_IMGUI_EVENTS "Events"
#define STRING_IMGUI_EVENTS_EVENT_LABEL "##Event"
#define STRING_IMGUI_EVENTS_ADD "Add"
#define STRING_IMGUI_EVENTS_REMOVE "Remove"
#define STRING_IMGUI_EVENT_FORMAT "#%i %s"

#define STRING_IMGUI_SPRITESHEETS "Spritesheets"
#define STRING_IMGUI_SPRITESHEETS_ADD "Add"
#define STRING_IMGUI_SPRITESHEETS_REMOVE "Remove"
#define STRING_IMGUI_SPRITESHEETS_RELOAD "Reload"
#define STRING_IMGUI_SPRITESHEETS_REPLACE "Replace"
#define STRING_IMGUI_SPRITESHEET_FORMAT "#%i %s"

#define STRING_IMGUI_FRAME_PROPERTIES "Frame Properties"
#define STRING_IMGUI_FRAME_PROPERTIES_CROP_POSITION "Crop Position"
#define STRING_IMGUI_FRAME_PROPERTIES_CROP_SIZE "Crop Size"
#define STRING_IMGUI_FRAME_PROPERTIES_POSITION "Position"
#define STRING_IMGUI_FRAME_PROPERTIES_PIVOT "Pivot"
#define STRING_IMGUI_FRAME_PROPERTIES_SCALE "Scale"
#define STRING_IMGUI_FRAME_PROPERTIES_ROTATION "Rotation"
#define STRING_IMGUI_FRAME_PROPERTIES_VISIBLE "Visible"
#define STRING_IMGUI_FRAME_PROPERTIES_INTERPOLATED "Interpolated"
#define STRING_IMGUI_FRAME_PROPERTIES_DURATION "Duration"
#define STRING_IMGUI_FRAME_PROPERTIES_TINT "Tint"
#define STRING_IMGUI_FRAME_PROPERTIES_COLOR_OFFSET "Color Offset"

#define STRING_IMGUI_ANIMATION_PREVIEW "Animation Preview"
#define STRING_IMGUI_ANIMATION_PREVIEW_SETTINGS "##Animation Preview Settings"
#define STRING_IMGUI_ANIMATION_PREVIEW_GRID_SETTINGS "##Grid Settings"
#define STRING_IMGUI_ANIMATION_PREVIEW_GRID "Grid"
#define STRING_IMGUI_ANIMATION_PREVIEW_GRID_SIZE "##Grid Size"
#define STRING_IMGUI_ANIMATION_PREVIEW_GRID_COLOR "Color"
#define STRING_IMGUI_ANIMATION_PREVIEW_ZOOM_SETTINGS "##Zoom Settings"
#define STRING_IMGUI_ANIMATION_PREVIEW_ZOOM "Zoom"
#define STRING_IMGUI_ANIMATION_PREVIEW_BACKGROUND_SETTINGS "##Background Settings"
#define STRING_IMGUI_ANIMATION_PREVIEW_BACKGROUND_COLOR "Background Color"

#define STRING_IMGUI_SPRITESHEET_EDITOR "Spritesheet Editor"

#define STRING_IMGUI_TIMELINE "Timeline"
#define STRING_IMGUI_TIMELINE_ELEMENT_SHIFT_ABOVE "Shift Above"
#define STRING_IMGUI_TIMELINE_ELEMENT_SHIFT_BELOW "Shift Below"
#define STRING_IMGUI_TIMELINE_ROOT "Root"
#define STRING_IMGUI_TIMELINE_ELEMENT_FORMAT "#%i %s"
#define STRING_IMGUI_TIMELINE_SPRITESHEET_FORMAT "#%i"
#define STRING_IMGUI_TIMELINE_SPRITESHEET_UNKNOWN "#?"
#define STRING_IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_LABEL "##Timeline Element Spritesheet ID"
#define STRING_IMGUI_TIMELINE_ELEMENT_NAME_LABEL "##Timeline Element Name"
#define STRING_IMGUI_TIMELINE_TRIGGERS "Triggers"
#define STRING_IMGUI_TIMELINE_ANIMATION_LABEL "##Timeline Animation"
#define STRING_IMGUI_TIMELINE_ELEMENT_SHIFT_ARROWS_LABEL "##Timeline Shift Arrows"
#define STRING_IMGUI_TIMELINE_ANIMATIONS "Animations"
#define STRING_IMGUI_TIMELINE_PLAY  "|>  Play"
#define STRING_IMGUI_TIMELINE_PAUSE "|| Pause"
#define STRING_IMGUI_TIMELINE_ADD_FRAME    "+    Add Frame"
#define STRING_IMGUI_TIMELINE_REMOVE_FRAME "- Remove Frame"
#define STRING_IMGUI_TIMELINE_FIT_ANIMATION_LENGTH "= Fit Animation Length"
#define STRING_IMGUI_TIMELINE_ANIMATION LENGTH "Animation Length:"
#define STRING_IMGUI_TIMELINE_VISIBLE "Visible"
#define STRING_IMGUI_TIMELINE_LAYER "Layer"
#define STRING_IMGUI_TIMELINE_NULL "Null"
#define STRING_IMGUI_TIMELINE_RECT "Rect"
#define STRING_IMGUI_TIMELINE_ELEMENTS "Elements"
#define STRING_IMGUI_TIMELINE_ELEMENT_ADD "Add"
#define STRING_IMGUI_TIMELINE_ELEMENT_ADD_MENU "Element Add Menu"
#define STRING_IMGUI_TIMELINE_ELEMENT_ADD_MENU_LAYER "Add Layer..."
#define STRING_IMGUI_TIMELINE_ELEMENT_ADD_MENU_NULL "Add Null..."
#define STRING_IMGUI_TIMELINE_ELEMENT_REMOVE "Remove"

#define STRING_IMGUI_TOOLTIP_ANIMATIONS_SELECT "Select the animation to edit.\nYou can also click the name to edit it."
#define STRING_IMGUI_TOOLTIP_ANIMATIONS_ADD "Add a new animation."
#define STRING_IMGUI_TOOLTIP_ANIMATIONS_REMOVE "Removes the selected animation."
#define STRING_IMGUI_TOOLTIP_ANIMATIONS_DUPLICATE "Duplicates the selected animation."
#define STRING_IMGUI_TOOLTIP_ANIMATIONS_SET_AS_DEFAULT "Sets the selected animation as the default.\nDefault animations are marked with \"(*)\"."
#define STRING_IMGUI_TOOLTIP_EVENTS_SELECT "Set the event for the trigger, or rename it."
#define STRING_IMGUI_TOOLTIP_EVENTS_ADD "Add a new event."
#define STRING_IMGUI_TOOLTIP_EVENTS_REMOVE "Removes the selected event."
#define STRING_IMGUI_TOOLTIP_SPRITESHEETS_ADD "Opens the file dialog to load in a new sprite."
#define STRING_IMGUI_TOOLTIP_SPRITESHEETS_REMOVE "Removes the selected spritesheet."
#define STRING_IMGUI_TOOLTIP_SPRITESHEETS_RELOAD "Reloads the selected spritesheet."
#define STRING_IMGUI_TOOLTIP_SPRITESHEETS_REPLACE "Replaces the selected spritesheet; opens up the file dialog."
#define STRING_IMGUI_TOOLTIP_PROPERTIES_FPS "Change the FPS of the animation."
#define STRING_IMGUI_TOOLTIP_PROPERTIES_CREATED_BY "Change the author of the animation."
#define STRING_IMGUI_TOOLTIP_PROPERTIES_CREATED_ON_NOW "Set the date of creation to the current system time."
#define STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_POSITION "Change the position of the frame."
#define STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_CROP_POSITION "Change the crop position of the frame."
#define STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_CROP_SIZE "Change the crop size of the frame."
#define STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_PIVOT "Change the pivot of the frame."
#define STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_SCALE "Change the scale of the frame."
#define STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_ROTATION "Change the rotation of the frame."
#define STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_DURATION "Change the duration of the frame."
#define STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_TINT "Change the tint of the frame."
#define STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_COLOR_OFFSET "Change the color offset of the frame."
#define STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_VISIBLE "Toggles the visibility of the frame."
#define STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_INTERPOLATED "Toggles the interpolation of the frame.\nBetween keyframes, will transform the values in the in-betweens to be smooth."
#define STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_ADD "Add a layer or null timeline element."
#define STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_NAME "Click to rename the element."
#define STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_SPRITESHEET "Click to change the spritesheet the layer is using."
#define STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_REMOVE "Remove a timeline element."
#define STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_SHIFT_UP "Shift this timeline element above.\nElements with higher indices will display further behind."
#define STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_SHIFT_DOWN "Shift this timeline element below.\nElements with lower indices will display further in front."
#define STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_VISIBLE "Toggle visibility for this element."
#define STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_RECT "Toggle visibility for a rectangle around the null."
#define STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_ROOT "This is the Root element."
#define STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_LAYER "This is a Layer element.\nThese are the main graphical animation elements.\nClick to rename."
#define STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_NULL "This is a Null element.\nThese are invisible elements where a game engine may have access to them for additional effects.\nClick to rename."
#define STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_TRIGGERS "This is the animation's Triggers.\nTriggers are special activations; each is bound to an Event."
#define STRING_IMGUI_TOOLTIP_TIMELINE_PLAY "Plays the animation."
#define STRING_IMGUI_TOOLTIP_TIMELINE_PAUSE "Pauses the animation."
#define STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID "Toggles grid visibility on the animation preview."
#define STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID_COLOR "Changes the animation preview grid color."
#define STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID_SIZE "Changes the animation preview grid size (number of rows/columns)."
#define STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_ZOOM "Changes the animation preview zoom level."
#define STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_BACKGROUND_COLOR "Changes the background color of the animation preview."

#define STRING_OPENGL_VERSION "#version 330"

#define SHADER_VERTEX "#version 330 core\n" \
"in vec2 l_position;\n" \
"in vec2 l_uv;\n" \
"\n" \
"uniform mat3 u_view;\n" \
"uniform mat3 u_projection;\n" \
"uniform mat3 u_model;\n" \
"\n" \
"out vec2 o_uv;\n" \
"\n" \
"void main()\n" \
"{\n" \
"    gl_Position = vec4(u_projection * u_view * u_model * vec3(l_position, 1.0), 1.0);\n" \
"    o_uv = l_uv;\n" \
"}"

#define SHADER_FRAGMENT "#version 330 core\n" \
"uniform vec4 u_tint;\n" \
"uniform vec4 u_color_offset;\n" \
"uniform sampler2D u_texture;\n" \
"\n" \
"in vec2 i_uv;\n" \
"\n" \
"out vec4 FragColor;\n" \
"\n" \
"void main()\n" \
"{\n" \
"    FragColor = texture(u_texture, i_uv) * u_tint;\n" \
"    FragColor.rgb += u_color_offset.rgb;\n" \
"\n" \
"    if (FragColor.a == 0.0) discard;\n" \
"}\n"
