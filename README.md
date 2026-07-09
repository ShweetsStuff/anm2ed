# Anm2Ed

![Preview](https://shweetz.net/files/projects/anm2ed/preview2.png)

Anm2Ed is a modern editor for *The Binding of Isaac: Rebirth*'s XML-based `.anm2` animation format. It is built as an extended, more comfortable replacement for the original proprietary animation editor, with a dockable Dear ImGui interface and tools aimed at both quick edits and larger animation workflows.

### Clarification
This application was developed with the assistance of a large language model.

## Features
- Full `.anm2` document editing for spritesheets, layers, nulls, events, sounds, animations, frames, and triggers.
- Dockable [Dear ImGui](https://github.com/ocornut/imgui) interface with draggable windows, persistent layout, drag and drop, context menus, and keyboard-focused workflows.
- Timeline editing with multi-selection, cut/copy/paste, duplicate, merge, insert/delete, frame baking, frame dragging, and editable animation length/FPS/loop settings.
- Animation, layer, and null groups for organizing work, including collapsible trees, group roots, visibility toggles, and save-time baking for game compatibility.
- Spritesheet tools for adding, replacing, reloading, packing, saving, editing, and generating reusable regions.
- Overlay spritesheets that can be placed on top of base spritesheets for editing and previewing alternate texture work.
- Custom spritesheet shaders with vertex/fragment file paths, reload/compile feedback, enable/disable behavior, and configurable uniform bindings.
- Animation preview with grid, axes, pivots, root transform display, transparency, borders, zoom controls, onionskinning, and isolated render modes.
- Sound and trigger support, including playback through animation timelines and rendered output.
- Wizards and batch tools for generating animations from grids, generating regions, scanning frames, changing frame properties across selections, baking frames, merging animations, merging spritesheets, and rendering animations.
- Render output to video, GIF-compatible workflows, spritesheets, or PNG sequences through FFmpeg.
- Merge external `.anm2` files into the current document with merge, append, and replace behaviors.
- Robust undo/redo snapshots, autosave support, overwrite warnings, and persistent settings.
- Rebindable shortcuts for common editing, timeline, tool, and navigation actions.
- Localized UI with support for English, Spanish (Latin America), Russian, Chinese, and Korean.

### Supported Languages
- English
- Español (Latinoamérica) (Spanish (Latin America))
- Pусский (Russian)
- 中文 (Chinese)
- 한국어 (Korean)

**If you want to help localize for your language, feel free to get in touch to contribute.**

### Rendering Animations
FFmpeg is required for video/GIF-style export. Download it from [ffmpeg.org](https://ffmpeg.org/download.html), then point Anm2Ed to the `ffmpeg` executable in the render window.

## Build

After cloning and entering the repository directory, initialize the submodules:

```sh
git submodule update --init --recursive
```

### Windows
Visual Studio is recommended.

### Linux

```sh
mkdir build
cd build
cmake ..
make
```
