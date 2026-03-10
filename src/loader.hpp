#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include <SDL3/SDL.h>

#include "settings.h"
#include "socket.h"

namespace anm2ed
{
  class Loader
  {
    std::filesystem::path settings_path();

  public:
    Socket socket{};
    std::thread socketThread{};
    std::atomic_bool isSocketRunning{};
    SDL_Window* window{};
    SDL_GLContext glContext{};
    Settings settings;
    std::vector<std::string> arguments;
    bool isError{};
    bool isSocketThread{};

    Loader(int, const char**);
    ~Loader();
  };
}
