# Anm2Ed

![Preview](https://shweetz.net/files/projects/anm2ed/screenshot.png)

A reimplementation of *The Binding of Isaac: Rebirth*'s proprietary animation editor. Manipulates the XML-based ".anm2" format, used for in-game tweened animations.

## Features
- Extended version of the original proprietary Nicalis animation editor
- Smooth [Dear ImGui](https://github.com/ocornut/imgui) interface; docking, dragging and dropping, etc.
- New features
    - Can output .webm or *.png sequence
    - Cutting, copying and pasting
    - Additional wizard options
    - Robust snapshot (undo/redo) system
    - Additional hotkeys/shortcuts
    - Settings that will preserve on exit (stored in %APPDATA% on Windows or ~/.local/share on Linux)

## Dependencies
Download these from your package manager:
- SDL3
- GLEW
  
Note, to render animations, you'll need to download [FFmpeg](https://ffmpeg.org/download.html) and specify its install path in the program.

## Build (Linux)

After cloning and enter the repository's directory, make sure to initialize the submodules:

```git submodules update --init```

Then:

```
mkdir build
cd build
cmake ..
make 
```