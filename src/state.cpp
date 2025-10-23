#include "state.h"

#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl3.h>

#include "filesystem.h"
#include "toast.h"

using namespace anm2ed::settings;
using namespace anm2ed::dialog;
using namespace anm2ed::toast;
using namespace anm2ed::types;

namespace anm2ed::state
{
  constexpr auto TICK_RATE = 30;
  constexpr auto TICK_INTERVAL = (1000 / TICK_RATE);
  constexpr auto UPDATE_RATE = 120;
  constexpr auto UPDATE_INTERVAL = (1000 / UPDATE_RATE);

  State::State(SDL_Window*& window, std::vector<std::string>& arguments)
  {
    dialog = Dialog(window);

    for (auto argument : arguments)
      manager.open(argument);
  }

  void State::tick(Settings& settings)
  {
    if (auto document = manager.get())
      if (auto animation = document->animation_get())
        if (playback.isPlaying)
          playback.tick(document->anm2.info.fps, animation->frameNum, animation->isLoop || settings.playbackIsLoop);
  }

  void State::update(SDL_Window*& window, Settings& settings)
  {
    SDL_Event event{};

    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSDL3_ProcessEvent(&event);
      switch (event.type)
      {
        case SDL_EVENT_DROP_FILE:
          if (auto droppedFile = event.drop.data; filesystem::path_is_extension(droppedFile, "anm2"))
            manager.open(std::string(droppedFile));
          break;
        case SDL_EVENT_QUIT:
          isQuit = true;
          break;
        default:
          break;
      }
    }

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    taskbar.update(settings, dialog, manager, isQuit);
    dockspace.update(taskbar, documents, manager, settings, resources, dialog, playback);
    toasts.update();

    documents.update(taskbar, manager, resources);

    ImGui::GetStyle().FontScaleMain = settings.displayScale;
    SDL_GetWindowSize(window, &settings.windowSize.x, &settings.windowSize.y);
  }

  void State::render(SDL_Window*& window, Settings& settings)
  {
    glViewport(0, 0, settings.windowSize.x, settings.windowSize.y);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
    SDL_GL_SetSwapInterval(settings.isVsync);
  }

  void State::loop(SDL_Window*& window, Settings& settings)
  {
    auto currentTick = SDL_GetTicks();
    auto currentUpdate = SDL_GetTicks();

    if (currentTick - previousTick >= TICK_INTERVAL)
    {
      tick(settings);
      previousTick = currentTick;
    }

    if (currentUpdate - previousUpdate >= UPDATE_INTERVAL)
    {
      update(window, settings);
      render(window, settings);
      previousUpdate = currentUpdate;
    }

    SDL_Delay(1);
  }
}
