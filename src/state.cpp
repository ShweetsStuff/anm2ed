#include "state.h"

#include <algorithm>

#include <imgui/imgui_internal.h>

#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl3.h>

#include "log.h"
#include "path_.h"
#include "strings.h"
#include "toast.h"

using namespace anm2ed::imgui;
using namespace anm2ed::util;
using namespace anm2ed::types;

namespace anm2ed
{
  constexpr auto TICK_RATE = 30;
  constexpr auto TICK_INTERVAL = (1000 / TICK_RATE);
  constexpr auto UPDATE_RATE = 120;
  constexpr auto UPDATE_INTERVAL = (1000 / UPDATE_RATE);

  State::State(SDL_Window*& window, Settings& settings, std::vector<std::string>& arguments)
  {
    dialog = Dialog(window);

    for (const auto& argument : arguments)
      manager.open(path::from_utf8(argument));

    manager.chords_set(settings);
  }

  void State::tick(Settings& settings) { dockspace.tick(manager, settings); }

  void State::update(SDL_Window*& window, Settings& settings)
  {
    SDL_Event event{};

    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSDL3_ProcessEvent(&event);
      switch (event.type)
      {
        case SDL_EVENT_DROP_FILE:
        {
          std::string droppedFile = event.drop.data ? event.drop.data : "";
          if (droppedFile.empty()) break;
          auto droppedPath = path::from_utf8(droppedFile);
          if (path::is_extension(droppedPath, "anm2"))
          {
            if (manager.documents.empty())
              manager.open(droppedPath);
            else
            {
              if (std::find(manager.anm2DragDropPaths.begin(), manager.anm2DragDropPaths.end(), droppedPath) ==
                  manager.anm2DragDropPaths.end())
                manager.anm2DragDropPaths.push_back(droppedPath);
              manager.isAnm2DragDrop = true;
            }
            SDL_FlashWindow(window, SDL_FLASH_UNTIL_FOCUSED);
          }
          else if (path::is_extension(droppedPath, "png"))
          {
            if (auto document = manager.get())
              document->spritesheet_add(droppedPath);
            else
            {
              toasts.push(localize.get(TOAST_ADD_SPRITESHEET_FAILED));
              logger.warning(localize.get(TOAST_ADD_SPRITESHEET_FAILED, anm2ed::ENGLISH));
            }
          }
          else if (path::is_extension(droppedPath, "wav") || path::is_extension(droppedPath, "ogg"))
          {
            if (auto document = manager.get())
              document->sound_add(droppedPath);
            else
            {
              toasts.push(localize.get(TOAST_ADD_SOUND_FAILED));
              logger.warning(localize.get(TOAST_ADD_SOUND_FAILED, anm2ed::ENGLISH));
            }
          }
          break;
        }
        case SDL_EVENT_USER: // Opening files
        {
          std::string droppedFile = event.drop.data ? event.drop.data : "";
          if (droppedFile.empty()) break;
          auto droppedPath = path::from_utf8(droppedFile);
          if (path::is_extension(droppedPath, "anm2"))
          {
            manager.open(droppedPath);
            SDL_FlashWindow(window, SDL_FLASH_UNTIL_FOCUSED);
          }
          break;
        }
        case SDL_EVENT_QUIT:
          isQuitting = true;
          break;
        default:
          break;
      }
    }

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    taskbar.update(manager, settings, resources, dialog, isQuitting);
    documents.update(taskbar, manager, settings, resources, isQuitting);
    dockspace.update(taskbar, documents, manager, settings, resources, dialog, clipboard);
    toasts.update();

    SDL_GetWindowSize(window, &settings.windowSize.x, &settings.windowSize.y);
    SDL_GetWindowPosition(window, &settings.windowPosition.x, &settings.windowPosition.y);

    if (isQuitting && manager.documents.empty()) isQuit = true;
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

    if (currentUpdate - previousUpdate >= UPDATE_INTERVAL)
    {
      update(window, settings);
      render(window, settings);
      previousUpdate = currentUpdate;
    }

    if (currentTick - previousTick >= TICK_INTERVAL)
    {
      tick(settings);
      previousTick = currentTick;
    }

    SDL_Delay(1);
  }
}
