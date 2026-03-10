#pragma once

#include <SDL3/SDL.h>

#include "dockspace.h"

namespace anm2ed
{
  class State
  {
    void tick(Settings&);
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

    State(SDL_Window*&, Settings& settings, std::vector<std::string>&);

    void loop(SDL_Window*&, Settings&);
  };
};
