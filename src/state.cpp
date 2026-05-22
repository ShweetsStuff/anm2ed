#include "state.hpp"

#include <algorithm>

#include <imgui/imgui_internal.h>

#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl3.h>

#include "log.hpp"
#include "path.hpp"
#include "strings.hpp"
#include "toast.hpp"
#include "util/imgui/shortcut.hpp"

using namespace anm2ed::imgui;
using namespace anm2ed::util;
using namespace anm2ed::types;

namespace anm2ed
{
  constexpr auto UPDATE_RATE = 120;
  constexpr auto UPDATE_INTERVAL = (1000 / UPDATE_RATE);

  State::State(SDL_Window*& window, Settings& settings, std::vector<std::string>& arguments)
  {
    dialog = Dialog(window);

    for (const auto& argument : arguments)
      manager.open(path::from_utf8(argument));

    manager.chords_set(settings);
  }

  void State::tick(Settings& settings, float deltaSeconds) { dockspace.tick(manager, settings, deltaSeconds); }

  void State::update(SDL_Window*& window, Settings& settings)
  {
    /*
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    SDL_GetWindowSize(window, &settings.windowSize.x, &settings.windowSize.y);
    SDL_GetWindowPosition(window, &settings.windowPosition.x, &settings.windowPosition.y);

    SDL_Event event{};

    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_EVENT_QUIT:
          isQuit = true;
        default:
          break;
      }
    }

    dockspace.update(taskbar, documents, manager, settings, resources, dialog, clipboard);
    */

    SDL_Event event{};

    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSDL3_ProcessEvent(&event);
      switch (event.type)
      {
        case SDL_EVENT_DROP_BEGIN:
          spritesheetDropPaths.clear();
          soundDropPaths.clear();
          break;
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
            spritesheetDropPaths.push_back(droppedPath);
          }
          else if (path::is_extension(droppedPath, "wav") || path::is_extension(droppedPath, "ogg"))
          {
            soundDropPaths.push_back(droppedPath);
          }
          break;
        }
        case SDL_EVENT_DROP_COMPLETE:
        {
          if (auto document = manager.get(); document)
          {
            if (!spritesheetDropPaths.empty())
            {
              auto paths = spritesheetDropPaths;
              manager.command_push({manager.selected, [paths](Manager&, Document& document)
                                    { document.spritesheets_add(paths); }});
            }
            if (!soundDropPaths.empty())
            {
              auto paths = soundDropPaths;
              manager.command_push({manager.selected, [paths](Manager&, Document& document)
                                    { document.sounds_add(paths); }});
            }
          }
          else
          {
            if (!spritesheetDropPaths.empty())
            {
              toasts.push(localize.get(TOAST_ADD_SPRITESHEET_FAILED));
              logger.warning(localize.get(TOAST_ADD_SPRITESHEET_FAILED, anm2ed::ENGLISH));
            }
            if (!soundDropPaths.empty())
            {
              toasts.push(localize.get(TOAST_ADD_SOUND_FAILED));
              logger.warning(localize.get(TOAST_ADD_SOUND_FAILED, anm2ed::ENGLISH));
            }
          }
          spritesheetDropPaths.clear();
          soundDropPaths.clear();
          break;
        }
        case SDL_EVENT_USER: // Opening files
        {
          std::string droppedFile = event.user.data1 ? static_cast<const char*>(event.user.data1) : "";
          if (event.user.data1) SDL_free(event.user.data1);
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

    if (ImGui::GetTopMostPopupModal() == nullptr)
    {
      constexpr ImGuiKey DOCUMENT_SHORTCUT_KEYS[] = {ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5,
                                                     ImGuiKey_6, ImGuiKey_7, ImGuiKey_8, ImGuiKey_9, ImGuiKey_0};
      for (int i = 0; i < (int)(sizeof(DOCUMENT_SHORTCUT_KEYS) / sizeof(DOCUMENT_SHORTCUT_KEYS[0])); ++i)
      {
        if (i >= (int)manager.documents.size()) break;
        if (ImGui::Shortcut(ImGuiMod_Ctrl | DOCUMENT_SHORTCUT_KEYS[i], ImGuiInputFlags_RouteGlobal))
        {
          manager.set(i);
          manager.pendingSelected = i;
          break;
        }
      }
    }

    taskbar.update(manager, settings, resources, dialog, isQuitting);
    documents.update(taskbar, manager, settings, resources, isQuitting);
    dockspace.update(taskbar, documents, manager, settings, resources, dialog, clipboard);

    if (!dockspace.is_canvas_focused_get())
    {
      float uiScaleDelta{};
      if (imgui::shortcut(manager.chords[SHORTCUT_ZOOM_IN], shortcut::GLOBAL)) uiScaleDelta += UI_SCALE_STEP;
      if (imgui::shortcut(manager.chords[SHORTCUT_ZOOM_OUT], shortcut::GLOBAL)) uiScaleDelta -= UI_SCALE_STEP;
      if (uiScaleDelta != 0.0f)
      {
        settings.uiScale = std::clamp(settings.uiScale + uiScaleDelta, UI_SCALE_MIN, UI_SCALE_MAX);
        ImGui::GetStyle().FontScaleMain = settings.uiScale;
      }
    }

    toasts.update();
    manager.commands_run();

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
    auto isRecording = manager.isRecording;
    auto tickRate = std::max(settings.playbackTickRate, 1);
    auto tickIntervalMs = 1000.0 / (double)tickRate;

    if (isRecording != wasRecording)
    {
      // Drop any accumulated backlog when entering/leaving recording mode.
      tickAccumulatorMs = 0.0;
      previousTick = currentTick;
      wasRecording = isRecording;
    }

    if (previousTick == 0) previousTick = currentTick;
    auto tickDeltaMs = currentTick - previousTick;
    tickDeltaMs = std::min<Uint64>(tickDeltaMs, 250);
    tickAccumulatorMs += (double)tickDeltaMs;
    previousTick = currentTick;

    if (currentUpdate - previousUpdate >= UPDATE_INTERVAL)
    {
      update(window, settings);
      render(window, settings);
      previousUpdate = currentUpdate;
    }

    if (tickAccumulatorMs >= tickIntervalMs)
    {
      tick(settings, (float)(tickIntervalMs / 1000.0));
      tickAccumulatorMs -= tickIntervalMs;
    }

    SDL_Delay(1);
  }
}
