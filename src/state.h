#pragma once

#include <SDL3/SDL.h>

#include "dockspace.h"

namespace anm2ed::state
{
  class State
  {
    void tick(settings::Settings&);
    void update(SDL_Window*&, settings::Settings&);
    void render(SDL_Window*&, settings::Settings&);

  public:
    bool isQuit{};
    bool isQuitting{};
    manager::Manager manager;
    resources::Resources resources;
    dialog::Dialog dialog;
    clipboard::Clipboard clipboard;

    taskbar::Taskbar taskbar;
    documents::Documents documents;
    dockspace::Dockspace dockspace;

    uint64_t previousTick{};
    uint64_t previousUpdate{};

    State(SDL_Window*&, std::vector<std::string>&);

    void loop(SDL_Window*&, settings::Settings&);
  };
};
