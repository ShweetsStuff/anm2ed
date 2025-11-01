#include "loader.h"

#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl3.h>

#include <SDL3_mixer/SDL_mixer.h>

#include "filesystem_.h"
#include "log.h"

using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed
{
  std::string Loader::settings_path()
  {
    return filesystem::path_preferences_get() + "settings.ini";
  }

  Loader::Loader(int argc, const char** argv)
  {
    for (int i = 1; i < argc; i++)
      arguments.emplace_back(argv[i]);

    settings = Settings(settings_path());

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
      logger.fatal(std::format("Could not initialize SDL! {}", SDL_GetError()));
      isError = true;
      return;
    }

    logger.info("Initialized SDL");

    if (!MIX_Init())
      logger.warning(std::format("Could not initialize SDL_mixer! {}", SDL_GetError()));
    else
      logger.info("Initialized SDL_mixer");

    window = SDL_CreateWindow("Anm2Ed", settings.windowSize.x, settings.windowSize.y,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    glContext = SDL_GL_CreateContext(window);

    if (!glContext)
    {
      logger.fatal(std::format("Could not initialize OpenGL context! {}", SDL_GetError()));
      isError = true;
      return;
    }

    if (!gladLoadGLLoader((GLADloadproc)(SDL_GL_GetProcAddress)))
    {
      logger.fatal(std::format("Could not initialize OpenGL!"));
      isError = true;
      return;
    }

    logger.info(std::format("Initialized OpenGL {}", (const char*)glGetString(GL_VERSION)));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LINE_SMOOTH);
    glClearColor(color::BLACK.r, color::BLACK.g, color::BLACK.b, color::BLACK.a);

    IMGUI_CHECKVERSION();
    if (!ImGui::CreateContext())
    {
      logger.fatal("Could not initialize Dear ImGui!");
      isError = true;
      return;
    }

    logger.info("Initialized Dear ImGui");

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImGui::LoadIniSettingsFromDisk(settings_path().c_str());
  }

  Loader::~Loader()
  {
    if (ImGui::GetCurrentContext())
    {
      settings.save(settings_path(), ImGui::SaveIniSettingsToMemory(nullptr));

      ImGui_ImplSDL3_Shutdown();
      ImGui_ImplOpenGL3_Shutdown();
      ImGui::DestroyContext();
    }

    MIX_Quit();

    if (SDL_WasInit(0))
    {
      SDL_GL_DestroyContext(glContext);
      SDL_DestroyWindow(window);
      SDL_Quit();
    }
  }
}