#include "state.h"

#include <algorithm>

#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl3.h>

#include "filesystem_.h"
#include "log.h"
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
      manager.open(argument);

    manager.chords_set(settings);
  }

  void State::tick(Settings& settings) { dockspace.tick(manager, settings); }

  void State::update(SDL_Window*& window, Settings& settings)
  {
    SDL_Event event{};
    bool openDroppedDocumentsImmediately = manager.documents.empty();

    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSDL3_ProcessEvent(&event);
      switch (event.type)
      {
        case SDL_EVENT_DROP_FILE:
        {
          auto droppedFile = event.drop.data;
          if (filesystem::path_is_extension(droppedFile, "anm2"))
          {
            std::filesystem::path droppedPath{droppedFile};
            if (openDroppedDocumentsImmediately)
            {
              manager.open(droppedPath);
            }
            else
            {
              if (std::find(manager.anm2DragDropPaths.begin(), manager.anm2DragDropPaths.end(), droppedPath) ==
                  manager.anm2DragDropPaths.end())
                manager.anm2DragDropPaths.push_back(droppedPath);
              manager.isAnm2DragDrop = true;
            }
            SDL_FlashWindow(window, SDL_FLASH_UNTIL_FOCUSED);
          }
          else if (filesystem::path_is_extension(droppedFile, "png"))
          {
            if (auto document = manager.get())
              document->spritesheet_add(droppedFile);
            else
            {
              toasts.push(localize.get(TOAST_ADD_SPRITESHEET_FAILED));
              logger.warning(localize.get(TOAST_ADD_SPRITESHEET_FAILED, anm2ed::ENGLISH));
            }
          }
          else if (filesystem::path_is_extension(droppedFile, "wav") ||
                   filesystem::path_is_extension(droppedFile, "ogg"))
          {
            if (auto document = manager.get())
              document->sound_add(droppedFile);
            else
            {
              toasts.push(localize.get(TOAST_ADD_SOUND_FAILED));
              logger.warning(localize.get(TOAST_ADD_SOUND_FAILED, anm2ed::ENGLISH));
            }
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
