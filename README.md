# Anm2Ed

![Preview](https://shweetz.net/files/projects/anm2ed/preview2.png)

Anm2Ed is a modern editor for *The Binding of Isaac: Rebirth*'s XML-based `.anm2` animation format. It is built as an extended, more comfortable replacement for the original proprietary animation editor, with a dockable Dear ImGui interface and tools aimed at both quick edits and larger animation workflows.

### Clarification
This application was developed with the assistance of a large language model.

## Features
- Full `.anm2` editing: spritesheets, regions, layers, nulls, events, sounds, shaders, animations, frames and triggers.
- Dockable [Dear ImGui](https://github.com/ocornut/imgui) interface with persistent layout, drag and drop, context menus and rebindable shortcuts.
- Timeline: multi-select, cut/copy/paste (whole frames or single properties), duplicate, split, bake, drag to move/resize, and start/end markers to loop a range.
- Groups for animations, layers and nulls, with group transforms baked for the game on save.
- Frame properties for crop, size, position, pivot, scale, shear, rotation, tint, color offset, region, shader, visibility, interpolation (including easing) and flips; batch changes across selections.
- Spritesheet editor with pan, move, crop, draw, erase and color picker tools; regions; packing, merging, trimming; automatic reload when files change on disk.
- Animation preview with grid, axes, pivots, borders, onionskin (with pivot paths), and another document's animation as an overlay.
- Wizards: generate animations from a grid, generate regions, change all frame properties, merge animations and files.
- Render to video, GIF, spritesheet or PNG sequence (FFmpeg), with trigger sounds.
- Saves the game's format with the editor's own copy embedded (`SourceDocument`), so nothing is lost; warns before saving anything the game doesn't support.
- Undo/redo, autosave (spritesheet edits included) with crash restore, and overwrite warnings.

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
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Tests
```sh
ctest --test-dir build
```
