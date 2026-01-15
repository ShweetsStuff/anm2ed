#include "loader.h"

#include <cerrno>
#include <cstdint>
#include <filesystem>

#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl3.h>

#include <SDL3_mixer/SDL_mixer.h>

#include "log.h"
#include "sdl.h"

#include "imgui_.h"

#include "snapshots.h"
#include "socket.h"

#include "util/math_.h"

#ifdef _WIN32
  #include "util/path_.h"
  #include <DbgHelp.h>
  #include <format>
  #include <windows.h>
#endif

using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed
{
  constexpr auto WINDOW_4K_SIZE = glm::ivec2(3840, 2160);
  constexpr auto UI_SCALE_4K_DEFAULT = 1.5f;
  constexpr auto WINDOW_DEFAULT_SIZE_MULTIPLIER = 0.90f;

  constexpr auto SOCKET_ADDRESS = "127.0.0.1";
  constexpr auto SOCKET_PORT = 11414;

#ifdef _WIN32
  constexpr int SOCKET_ERROR_ADDRESS_IN_USE = WSAEADDRINUSE;
#else
  constexpr int SOCKET_ERROR_ADDRESS_IN_USE = EADDRINUSE;
#endif

#ifdef _WIN32
  std::filesystem::path g_minidump_dir{};
  PVOID g_vectored_handler{};
  LONG g_dump_in_progress{};

  void windows_minidump_write(EXCEPTION_POINTERS* exceptionPointers)
  {
    if (g_minidump_dir.empty()) return;
    if (InterlockedExchange(&g_dump_in_progress, 1) != 0) return;

    SYSTEMTIME st{};
    GetLocalTime(&st);

    auto pid = GetCurrentProcessId();
    auto code = exceptionPointers && exceptionPointers->ExceptionRecord
                    ? exceptionPointers->ExceptionRecord->ExceptionCode
                    : 0u;

    auto filename = std::format("anm2ed_{:04}{:02}{:02}_{:02}{:02}{:02}_pid{:08x}_code{:08x}.dmp", st.wYear, st.wMonth,
                                st.wDay, st.wHour, st.wMinute, st.wSecond, pid, code);
    auto dumpPath = g_minidump_dir / path::from_utf8(filename);

    auto dumpPathW = dumpPath.wstring();
    HANDLE file = CreateFileW(dumpPathW.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;

    MINIDUMP_EXCEPTION_INFORMATION mei{};
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = exceptionPointers;
    mei.ClientPointers = FALSE;

    const MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory |
                                                              MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);
    MiniDumpWriteDump(GetCurrentProcess(), pid, file, dumpType, exceptionPointers ? &mei : nullptr, nullptr, nullptr);

    FlushFileBuffers(file);
    CloseHandle(file);
  }

  LONG WINAPI windows_unhandled_exception_filter(EXCEPTION_POINTERS* exceptionPointers)
  {
    windows_minidump_write(exceptionPointers);
    return EXCEPTION_EXECUTE_HANDLER;
  }

  LONG CALLBACK windows_vectored_exception_handler(EXCEPTION_POINTERS* exceptionPointers)
  {
    windows_minidump_write(exceptionPointers);
    return EXCEPTION_CONTINUE_SEARCH;
  }

  void windows_minidumps_configure()
  {
    auto prefDir = sdl::preferences_directory_get();
    if (prefDir.empty()) return;

    std::error_code ec{};
    auto dumpDir = prefDir / "crash";
    std::filesystem::create_directories(dumpDir, ec);
    if (ec) return;

    g_minidump_dir = dumpDir;

    if (!g_vectored_handler) g_vectored_handler = AddVectoredExceptionHandler(1, windows_vectored_exception_handler);
    SetUnhandledExceptionFilter(windows_unhandled_exception_filter);

    logger.info(std::format("MiniDumpWriteDump enabled: {}", path::to_utf8(dumpDir)));
  }
#endif

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

  std::filesystem::path Loader::settings_path() { return sdl::preferences_directory_get() / "settings.ini"; }

  Loader::Loader(int argc, const char** argv)
  {
    for (int i = 1; i < argc; i++)
      arguments.emplace_back(argv[i]);

    Socket testSocket;
    if (!testSocket.open(SERVER))
      logger.warning(std::format("Failed to open socket; single instancing will not work."));

    if (testSocket.bind({SOCKET_ADDRESS, SOCKET_PORT}))
    {
      socket = std::move(testSocket);

      if (!socket.listen())
        logger.warning("Could not listen on socket; single instancing disabled.");
      else
      {
        isSocketThread = true;
        logger.info(std::format("Opened socket at {}:{}", SOCKET_ADDRESS, SOCKET_PORT));
      }
    }
    else
    {
      if (testSocket.last_error() != SOCKET_ERROR_ADDRESS_IN_USE)
      {
        logger.fatal(std::format("Failed to bind single-instance socket (error {}).", testSocket.last_error()));
        isError = true;
        return;
      }

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

      isError = true;
      return;
    }

    settings = Settings(settings_path());
    SnapshotStack::max_size_set(settings.fileSnapshotStackSize);
    localize.language = (Language)settings.language;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
      logger.fatal(std::format("Could not initialize SDL! {}", SDL_GetError()));
      isError = true;
      return;
    }

    logger.info("Initialized SDL");

#ifdef _WIN32
    windows_minidumps_configure();
#endif

    auto windowProperties = SDL_CreateProperties();

    if (windowProperties == 0)
    {
      logger.fatal(std::format("Could not initialize window properties! {}", SDL_GetError()));
      isError = true;
      return;
    }

    if (settings.isDefault)
    {
      if (auto display = SDL_GetPrimaryDisplay(); display != 0)
      {
        if (auto mode = SDL_GetCurrentDisplayMode(display))
        {
          if (mode->w >= WINDOW_4K_SIZE.x || mode->h >= WINDOW_4K_SIZE.y) settings.uiScale = UI_SCALE_4K_DEFAULT;
          settings.windowSize.x = mode->w * WINDOW_DEFAULT_SIZE_MULTIPLIER;
          settings.windowSize.y = mode->h * WINDOW_DEFAULT_SIZE_MULTIPLIER;
        }
        else
          logger.warning(std::format("Failed to query primary display mode: {}", SDL_GetError()));
      }
      else
        logger.warning("Failed to detect primary display for UI scaling.");
    }

    if (!MIX_Init())
      logger.warning(std::format("Could not initialize SDL_mixer! {}", SDL_GetError()));
    else
      logger.info("Initialized SDL_mixer");

    SDL_SetStringProperty(windowProperties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Anm2Ed");
    SDL_SetNumberProperty(windowProperties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, settings.windowSize.x);
    SDL_SetNumberProperty(windowProperties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, settings.windowSize.y);

    SDL_SetNumberProperty(windowProperties, SDL_PROP_WINDOW_CREATE_X_NUMBER,
                          settings.isDefault ? SDL_WINDOWPOS_CENTERED : settings.windowPosition.x);
    SDL_SetNumberProperty(windowProperties, SDL_PROP_WINDOW_CREATE_Y_NUMBER,
                          settings.isDefault ? SDL_WINDOWPOS_CENTERED : settings.windowPosition.y);

    SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetBooleanProperty(windowProperties, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);

    window = SDL_CreateWindowWithProperties(windowProperties);

    if (!window)
    {
      logger.fatal(std::format("Could not initialize window! {}", SDL_GetError()));
      isError = true;
      return;
    }

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

    math::random_seed_set();

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

    if (auto imguiData = Settings::imgui_data_load(settings_path()); !imguiData.empty())
      ImGui::LoadIniSettingsFromMemory(imguiData.c_str(), imguiData.size());

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
                event.type = SDL_EVENT_USER;
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