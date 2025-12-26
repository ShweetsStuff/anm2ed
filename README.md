# Anm2Ed

![Preview](https://shweetz.net/files/projects/anm2ed/preview2.png)

A reimplementation of *The Binding of Isaac: Rebirth*'s proprietary animation editor. Manipulates the XML-based ".anm2" format, used for in-game tweened animations.

## Features
- Extended version of the original proprietary Nicalis animation editor
- Smooth [Dear ImGui](https://github.com/ocornut/imgui) interface; docking, dragging and dropping, etc. You might be familiar with it from [Repentogon](https://repentogon.com/).
- Greatly polished in many areas, to assist with all your animating needs
- New features
    - Additional hotkeys/shortcuts (rebindable!)
    - Additional wizard options
    - Broadened sound support
    - Can output .webm, .mp4 or *.png sequence (wih FFmpeg)
    - Cutting, copying and pasting
    - Onionskinning
    - Robust snapshot (undo/redo) system
    - Selecting multiple frames at a time
    - Settings that will preserve on exit (stored in %APPDATA% on Windows or ~/.local/share on Linux)

### Supported Languages
- English
- Español (Latinoamérica) (Spanish (Latin America))
- Pусский (Russian)
- 中文 (Chinese)
- 한국어 (Korean)

**If you want to help localize for your language, feel free to get in touch to contribute!**

### Note: Rendering Animations
You will need FFmpeg installed to output GIF/video! Get it from [here](https://ffmpeg.org/download.html), and point to the downloaded ffmpeg executable within the program!

## Build

After cloning and enter the repository's directory, make sure to initialize the submodules:

```git submodule update --init --recursive```

### Windows
Visual Studio is recommended for build.

### Linux

```
mkdir build
cd build
cmake ..
make 
```

## Happy animating!
![Isaac](https://private-user-images.githubusercontent.com/129694724/482938896-b7f4c7c4-ce38-4062-81e9-bea119c66d1a.gif?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3NTc4MDY3NTIsIm5iZiI6MTc1NzgwNjQ1MiwicGF0aCI6Ii8xMjk2OTQ3MjQvNDgyOTM4ODk2LWI3ZjRjN2M0LWNlMzgtNDA2Mi04MWU5LWJlYTExOWM2NmQxYS5naWY_WC1BbXotQWxnb3JpdGhtPUFXUzQtSE1BQy1TSEEyNTYmWC1BbXotQ3JlZGVudGlhbD1BS0lBVkNPRFlMU0E1M1BRSzRaQSUyRjIwMjUwOTEzJTJGdXMtZWFzdC0xJTJGczMlMkZhd3M0X3JlcXVlc3QmWC1BbXotRGF0ZT0yMDI1MDkxM1QyMzM0MTJaJlgtQW16LUV4cGlyZXM9MzAwJlgtQW16LVNpZ25hdHVyZT0xZmU3YmExYWJhZjg1NGZiNTNjODM0NGYyZGI5MjM2MzIxNGM1YTEyOWM2MjAxNDQwZWJhODRhMzUxYjcyZjQ5JlgtQW16LVNpZ25lZEhlYWRlcnM9aG9zdCJ9.ZROJVPS4oIyhl3kt-FQc3DjI5mci32AHwStjG0Sk8TM)
