#include "loader.h"

#include <cstdint>

#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl3.h>

#include <SDL3_mixer/SDL_mixer.h>

#include "filesystem_.h"
#include "log.h"

#include "imgui_.h"

#include "snapshots.h"
#include "socket.h"

using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed
{
  constexpr auto SOCKET_ADDRESS = "127.0.0.1";
  constexpr auto SOCKET_PORT = 11414;

  namespace
  {
    bool socket_paths_send(Socket& socket, const std::vector<std::string>& paths)
    {
      uint32_t count = htonl(static_cast<uint32_t>(paths.size()));
      if (!socket.send(&count, sizeof(count))) return false;

      for (const auto& path : paths)
      {
        uint32_t length = htonl(static_cast<uint32_t>(path.size()));
        if (!socket.send(&length, sizeof(length))) return false;
        if (!path.empty() && !socket.send(path.data(), path.size())) return false;
      }

      return true;
    }

    std::vector<std::string> socket_paths_receive(Socket& socket)
    {
      uint32_t count{};
      if (!socket.receive(&count, sizeof(count))) return {};
      count = ntohl(count);

      std::vector<std::string> paths;
      paths.reserve(count);

      for (uint32_t i = 0; i < count; ++i)
      {
        uint32_t length{};
        if (!socket.receive(&length, sizeof(length))) return {};
        length = ntohl(length);

        std::string path(length, '\0');
        if (length > 0 && !socket.receive(path.data(), length)) return {};
        paths.emplace_back(std::move(path));
      }

      return paths;
    }
  }

  std::string Loader::settings_path() { return filesystem::path_preferences_get() + "settings.ini"; }

  Loader::Loader(int argc, const char** argv)
  {
    for (int i = 1; i < argc; i++)
      arguments.emplace_back(argv[i]);

    Socket testSocket;
    if (!testSocket.open(SERVER))
      logger.warning(std::format("Failed to open socket; single instancing will not work."));

    bool isPrimaryInstance = false;

    if (testSocket.bind({SOCKET_ADDRESS, SOCKET_PORT}))
    {
      socket = std::move(testSocket);

      if (!socket.listen())
        logger.warning("Could not listen on socket; single instancing disabled.");
      else
      {
        isPrimaryInstance = true;
        isSocketThread = true;
        logger.info(std::format("Opened socket at {}:{}", SOCKET_ADDRESS, SOCKET_PORT));
      }
    }
    else
    {
      logger.info(std::format("Existing instance of program exists; passing arguments..."));
      Socket clientSocket;
      if (!clientSocket.open(CLIENT))
        logger.warning("Could not open client socket to forward arguments.");
      else if (!clientSocket.connect({SOCKET_ADDRESS, SOCKET_PORT}))
        logger.warning("Could not connect to existing instance.");
      else if (!socket_paths_send(clientSocket, arguments))
        logger.warning("Failed to transfer arguments to existing instance.");
      else
        logger.info("Sent arguments to existing instance. Exiting.");

      if (!isPrimaryInstance)
      {
        isError = true;
        return;
      }
    }

    settings = Settings(settings_path());
    SnapshotStack::max_size_set(settings.fileSnapshotStackSize);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
      logger.fatal(std::format("Could not initialize SDL! {}", SDL_GetError()));
      isError = true;
      return;
    }

    logger.info("Initialized SDL");

    if (settings.isDefault)
    {
      if (auto display = SDL_GetPrimaryDisplay(); display != 0)
      {
        if (auto mode = SDL_GetCurrentDisplayMode(display))
        {
          if (mode->w >= 3840 || mode->h >= 2160)
            settings.uiScale = 1.5f;
          else
            logger.warning(std::format("Failed to query primary display mode: {}", SDL_GetError()));
        }
      }
      else
        logger.warning("Failed to detect primary display for UI scaling.");
    }

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
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LINE_SMOOTH);

    IMGUI_CHECKVERSION();
    if (!ImGui::CreateContext())
    {
      logger.fatal("Could not initialize Dear ImGui!");
      isError = true;
      return;
    }

    logger.info("Initialized Dear ImGui");

    imgui::theme_set((theme::Type)settings.theme);

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.KeyRepeatDelay = settings.keyboardRepeatDelay;
    io.KeyRepeatRate = settings.keyboardRepeatRate;
    ImGui::GetStyle().FontScaleMain = settings.uiScale;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImGui::LoadIniSettingsFromDisk(settings_path().c_str());

    if (isSocketThread)
    {
      isSocketRunning = true;
      socketThread = std::thread(
          [this]()
          {
            while (isSocketRunning)
            {
              auto client = socket.accept();
              if (!client.is_valid())
              {
                if (!isSocketRunning) break;
                continue;
              }

              SDL_FlashWindow(window, SDL_FLASH_UNTIL_FOCUSED);

              auto paths = socket_paths_receive(client);
              for (auto& path : paths)
              {
                if (path.empty()) continue;
                SDL_Event event{};
                event.type = SDL_EVENT_DROP_FILE;
                event.drop.data = SDL_strdup(path.c_str());
                event.drop.windowID = window ? SDL_GetWindowID(window) : 0;
                SDL_PushEvent(&event);
              }
            }
          });
    }
  }

  Loader::~Loader()
  {
    if (isSocketThread)
    {
      isSocketRunning = false;

      if (socket.is_valid())
      {
        Socket wakeSocket;
        if (wakeSocket.open(CLIENT) && wakeSocket.connect({SOCKET_ADDRESS, SOCKET_PORT}))
          socket_paths_send(wakeSocket, {});
      }

      socket.close();
      if (socketThread.joinable()) socketThread.join();
    }

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
