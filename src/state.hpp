#pragma once

#include <SDL3/SDL.h>

#include <filesystem>
#include <vector>

#include "dockspace.hpp"

namespace anm2ed
{
  class State
  {
    void tick(Settings&, float);
    void update(SDL_Window*&, Settings&);
    void render(SDL_Window*&, Settings&);

  public:
    bool isQuit{};
    bool isQuitting{};
    Manager manager;
    Resources resources;
    Dialog dialog;
    Clipboard clipboard;

    imgui::Taskbar taskbar;
    imgui::Documents documents;
    imgui::Dockspace dockspace;

    uint64_t previousTick{};
    uint64_t previousUpdate{};
    double tickAccumulatorMs{};
    bool wasRecording{};
    std::vector<std::filesystem::path> spritesheetDropPaths{};
    std::vector<std::filesystem::path> soundDropPaths{};

    State(SDL_Window*&, Settings& settings, std::vector<std::string>&);

    void loop(SDL_Window*&, Settings&);
  };
};
