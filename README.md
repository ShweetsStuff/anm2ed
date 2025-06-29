# Anm2ed

![Preview](https://shweetz.net/files/projects/anm2ed/preview.png)

A reimplementation of *The Binding of Isaac: Rebirth*'s proprietary animation editor. Manipulates the XML-based ".anm2" format, used for in-game tweened animations.

## Features

- Most things present in the original IsaacAnimationEditor.exe, except some stuff like drawing (why not use an art program!)
- Smooth [Dear ImGui](https://github.com/ocornut/imgui) interface; docking, dragging and dropping, etc.
- Keybinds/keyboard control for common actions(see [src/input.h](https://github.com/ShweetsStuff/anm2ed/blob/master/src/input.h))

### To do
- Windows release
- Undo queue
- GIF export
- Some other things I can't think of

### Known Issues
- Root Transform doesn't work for scale/rotation (matrix math is hard; if you can help me fix it I will give you $100.)
- Some .anm2 files used in Rebirth might not render correctly due to the ordering of layers; just drag and drop to fix the ordering and save, they will work fine afterwards. 
- On startup, you will have to configure the windows yourself.
- Probably several bugs that elude me

## Dependencies
Download these from your package manager:

- SDL3
- GLEW

## Build

After cloning and enter the repository's directory, make sure to initialize the submodules:

```git submodules update --init```

Then:

```
mkdir build

cd build

cmake ..

make -j
```
