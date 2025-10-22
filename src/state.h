#pragma once

#include <SDL3/SDL.h>

#include "dockspace.h"

namespace anm2ed::state
{
  class State
  {
    void tick(settings::Settings& settings);
    void update(SDL_Window*& window, settings::Settings& settings);
    void render(SDL_Window*& window, settings::Settings& settings);

  public:
    bool isQuit{};
    dialog::Dialog dialog;
    resources::Resources resources;
    playback::Playback playback;
    document_manager::DocumentManager manager;

    taskbar::Taskbar taskbar;
    documents::Documents documents;
    dockspace::Dockspace dockspace;

    uint64_t previousTick{};
    uint64_t previousUpdate{};

    State(SDL_Window*& window, std::vector<std::string>& arguments);

    void loop(SDL_Window*& window, settings::Settings& settings);
  };
};
