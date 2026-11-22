#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <format>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/imgui_internal.h>

#include "cli.h"
#include "gl.h"
#include "instance.h"
#include "localization.h"
#include "log.h"
#include "math_util.h"
#include "path.h"
#include "sdl.h"
#include "snapshots.h"
#include "texture.h"
#include "ui/dockspace.h"
#include "ui/shortcut.h"
#include "ui/theme.h"
#include "ui/toast.h"

#ifdef _WIN32
#include <windows.h>
#endif

constexpr auto UPDATE_INTERVAL_MS = 1000 / 120;
constexpr auto TICK_DELTA_MAX_MS = 250;
constexpr auto WINDOW_4K_SIZE = glm::ivec2(3840, 2160);
constexpr auto UI_SCALE_4K_DEFAULT = 150.0f;
constexpr auto WINDOW_DEFAULT_SIZE_MULTIPLIER = 0.90f;
constexpr auto GL_VERSION_MAJOR = 3;
constexpr auto GL_VERSION_MINOR = 3;
constexpr auto GLSL_VERSION = "#version 330";
constexpr auto LINE_WIDTH = 2.0f;
constexpr ImGuiKey DOCUMENT_KEYS[] = {ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5,
                                      ImGuiKey_6, ImGuiKey_7, ImGuiKey_8, ImGuiKey_9, ImGuiKey_0};

// The window, its OpenGL context, the settings it opened with, and the main loop's clocks.
struct App
{
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;
    Settings settings;
    std::vector<std::filesystem::path> spritesheetDropPaths;
    std::vector<std::filesystem::path> soundDropPaths;
    Uint64 previousTick = 0;
    Uint64 previousUpdate = 0;
    double tickAccumulatorMs = 0;
    bool isQuit = false;
    bool isQuitting = false;
    bool isRecording = false;
};

// Everything the window shows. Made once ImGui is up, which its fonts and icons need.
struct Editor
{
    Manager manager;
    Resources resources;
    Dialog dialog;
    Clipboard clipboard;
    Taskbar taskbar;
    Documents documents;
    Dockspace dockspace;
};

static std::filesystem::path settings_path_get()
{
    return preferences_directory_get() / "settings.ini";
}

// A first run's window covers most of the primary display, with a larger interface on 4K ones.
static void window_size_default(Settings& settings)
{
    auto mode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    if (!mode)
    {
        logger.warning(std::format("Failed to query primary display mode: {}", SDL_GetError()));
        return;
    }
    if (mode->w >= WINDOW_4K_SIZE.x || mode->h >= WINDOW_4K_SIZE.y)
        settings.uiScale = UI_SCALE_4K_DEFAULT;
    settings.windowSize.x = mode->w * WINDOW_DEFAULT_SIZE_MULTIPLIER;
    settings.windowSize.y = mode->h * WINDOW_DEFAULT_SIZE_MULTIPLIER;
}

static bool window_open(App& app)
{
    auto& settings = app.settings;
    settings = Settings(settings_path_get());
    SnapshotStack::max_size_set(settings.fileSnapshotStackSize);
    localize.language = (Language)settings.language;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        logger.fatal(std::format("Could not initialize SDL! {}", SDL_GetError()));
        return false;
    }
    if (settings.isDefault)
        window_size_default(settings);

    auto properties = SDL_CreateProperties();
    SDL_SetStringProperty(properties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Anm2Ed");
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, settings.windowSize.x);
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, settings.windowSize.y);
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_X_NUMBER,
                          settings.isDefault ? SDL_WINDOWPOS_CENTERED : settings.windowPosition.x);
    SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_Y_NUMBER,
                          settings.isDefault ? SDL_WINDOWPOS_CENTERED : settings.windowPosition.y);
    SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
    app.window = SDL_CreateWindowWithProperties(properties);
    SDL_DestroyProperties(properties);
    if (!app.window)
    {
        logger.fatal(std::format("Could not initialize window! {}", SDL_GetError()));
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, GL_VERSION_MAJOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, GL_VERSION_MINOR);
    app.context = SDL_GL_CreateContext(app.window);
    if (!app.context || !gl_load())
    {
        logger.fatal(std::format("Could not initialize OpenGL! {}", SDL_GetError()));
        return false;
    }
    logger.info(std::format("Initialized OpenGL {}", (const char*)gl.GetString(GL_VERSION)));
    gl.Enable(GL_BLEND);
    gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    gl.LineWidth(LINE_WIDTH);
    gl.Disable(GL_DEPTH_TEST);
    gl.Disable(GL_LINE_SMOOTH);
    SDL_GL_SetSwapInterval(settings.isVsync);
    random_seed_set();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    theme_set((Theme)settings.theme);
    ImGui_ImplSDL3_InitForOpenGL(app.window, app.context);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);

    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
    if (settings.isDetachableWindows)
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.KeyRepeatDelay = settings.keyboardRepeatDelay;
    io.KeyRepeatRate = settings.keyboardRepeatRate;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    ImGui::GetStyle().FontScaleMain = percent_to_unit(settings.uiScale);
    if (auto imguiData = Settings::imgui_data_load(settings_path_get()); !imguiData.empty())
        ImGui::LoadIniSettingsFromMemory(imguiData.c_str(), imguiData.size());
    return true;
}

static void window_close(App& app)
{
    if (ImGui::GetCurrentContext())
    {
        app.settings.save(settings_path_get(), ImGui::SaveIniSettingsToMemory(nullptr));
        texture_garbage_collect();
        ImGui_ImplSDL3_Shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }
    SDL_GL_DestroyContext(app.context);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
}

static void file_drop(App& app, Editor& editor, const std::filesystem::path& path)
{
    auto& manager = editor.manager;
    if (is_path_extension(path, "anm2"))
    {
        if (manager.documents.empty())
            manager.open(path);
        else
        {
            if (std::ranges::find(manager.anm2DragDropPaths, path) == manager.anm2DragDropPaths.end())
                manager.anm2DragDropPaths.push_back(path);
            manager.isAnm2DragDrop = true;
        }
        SDL_FlashWindow(app.window, SDL_FLASH_UNTIL_FOCUSED);
    }
    else if (is_path_extension(path, PNG_EXTENSION) || is_path_extension(path, SVG_EXTENSION))
        app.spritesheetDropPaths.push_back(path);
    else if (is_path_extension(path, "wav") || is_path_extension(path, "ogg"))
        app.soundDropPaths.push_back(path);
}

// Dropped spritesheets and sounds go into the current document, together.
static void file_drop_complete(App& app, Editor& editor)
{
    auto& manager = editor.manager;
    if (manager.get())
    {
        if (auto paths = app.spritesheetDropPaths; !paths.empty())
            manager.command_push(
                {manager.selected, [paths](Manager&, Document& document) { document.spritesheets_add(paths); }});
        if (auto paths = app.soundDropPaths; !paths.empty())
            manager.command_push(
                {manager.selected, [paths](Manager&, Document& document) { document.sounds_add(paths); }});
    }
    else
    {
        if (!app.spritesheetDropPaths.empty())
            toast_log(Level::WARNING, TOAST_ADD_SPRITESHEET_FAILED);
        if (!app.soundDropPaths.empty())
            toast_log(Level::WARNING, TOAST_ADD_SOUND_FAILED);
    }
    app.spritesheetDropPaths.clear();
    app.soundDropPaths.clear();
}

static void events_poll(App& app, Editor& editor)
{
    SDL_Event event = {};
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type)
        {
            case SDL_EVENT_DROP_BEGIN:
                app.spritesheetDropPaths.clear();
                app.soundDropPaths.clear();
                break;
            case SDL_EVENT_DROP_FILE:
                if (event.drop.data && *event.drop.data)
                    file_drop(app, editor, path_from_utf8(event.drop.data));
                break;
            case SDL_EVENT_DROP_COMPLETE:
                file_drop_complete(app, editor);
                break;
            // A path handed over by another instance (see instance.h).
            case SDL_EVENT_USER:
            {
                auto path = path_from_utf8((const char*)event.user.data1);
                SDL_free(event.user.data1);
                if (is_path_extension(path, "anm2"))
                {
                    editor.manager.open(path);
                    SDL_FlashWindow(app.window, SDL_FLASH_UNTIL_FOCUSED);
                }
                break;
            }
            case SDL_EVENT_QUIT:
                app.isQuitting = true;
                break;
            // With detached windows open, closing the main window doesn't end the event loop by itself.
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (event.window.windowID == SDL_GetWindowID(app.window))
                    app.isQuitting = true;
                break;
            default:
                break;
        }
    }
}

// Ctrl+1 to Ctrl+0 switch to the first ten documents.
static void document_shortcuts_update(Manager& manager)
{
    if (ImGui::GetTopMostPopupModal())
        return;
    for (int i = 0; i < (int)std::size(DOCUMENT_KEYS) && i < (int)manager.documents.size(); ++i)
        if (ImGui::Shortcut(ImGuiMod_Ctrl | DOCUMENT_KEYS[i], ImGuiInputFlags_RouteGlobal))
        {
            manager.set(i);
            manager.pendingSelected = i;
            return;
        }
}

static void ui_scale_update(App& app, Editor& editor)
{
    if (editor.dockspace.is_canvas_focused_get())
        return;
    auto& settings = app.settings;
    auto delta = 0.0f;
    if (shortcut(editor.manager.chords[SHORTCUT_ZOOM_IN], SCOPE_GLOBAL))
        delta += UI_SCALE_STEP;
    if (shortcut(editor.manager.chords[SHORTCUT_ZOOM_OUT], SCOPE_GLOBAL))
        delta -= UI_SCALE_STEP;
    if (delta == 0.0f)
        return;
    settings.uiScale = std::clamp(settings.uiScale + delta, UI_SCALE_MIN, UI_SCALE_MAX);
    ImGui::GetStyle().FontScaleMain = percent_to_unit(settings.uiScale);
}

static void frame_update(App& app, Editor& editor)
{
    auto& settings = app.settings;
    events_poll(app, editor);

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    editor.documents.pending_document_order_apply(editor.manager);
    document_shortcuts_update(editor.manager);
    editor.taskbar.update(editor.manager, settings, editor.resources, editor.dialog, app.isQuitting);
    editor.documents.update(editor.taskbar, editor.manager, settings, editor.resources, app.isQuitting);
    editor.dockspace.update(editor.taskbar, editor.documents, editor.manager, settings, editor.resources,
                            editor.dialog, editor.clipboard);
    ui_scale_update(app, editor);
    toasts.update();
    editor.manager.commands_run();

    SDL_GetWindowSize(app.window, &settings.windowSize.x, &settings.windowSize.y);
    SDL_GetWindowPosition(app.window, &settings.windowPosition.x, &settings.windowPosition.y);
    if (app.isQuitting && editor.manager.documents.empty())
        app.isQuit = true;
}

static void frame_render(App& app)
{
    gl.Viewport(0, 0, app.settings.windowSize.x, app.settings.windowSize.y);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Windows dragged out of the main window are drawn into their own OS windows (and GL contexts).
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        auto context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(app.window, context);
    }

    SDL_GL_SwapWindow(app.window);
    texture_garbage_collect();
    SDL_GL_SetSwapInterval(app.settings.isVsync);
}

// Draws at most every UPDATE_INTERVAL_MS, and ticks playback at the playback tick rate in between.
static void loop_step(App& app, Editor& editor)
{
    auto now = SDL_GetTicks();
    auto tickIntervalMs = 1000.0 / std::max(app.settings.playbackTickRate, 1);

    // Recording starting or stopping drops the backlog.
    if (editor.manager.isRecording != app.isRecording)
    {
        app.tickAccumulatorMs = 0;
        app.previousTick = now;
        app.isRecording = editor.manager.isRecording;
    }
    if (app.previousTick == 0)
        app.previousTick = now;
    app.tickAccumulatorMs += (double)std::min<Uint64>(now - app.previousTick, TICK_DELTA_MAX_MS);
    app.previousTick = now;

    if (now - app.previousUpdate >= UPDATE_INTERVAL_MS)
    {
        frame_update(app, editor);
        frame_render(app);
        app.previousUpdate = now;
    }

    if (app.tickAccumulatorMs >= tickIntervalMs)
    {
        editor.dockspace.tick(editor.manager, app.settings, (float)(tickIntervalMs / 1000.0));
        app.tickAccumulatorMs -= tickIntervalMs;
    }

    SDL_Delay(1);
}

static int editor_run(int argc, char* argv[])
{
    std::vector<std::string> arguments(argv + 1, argv + argc);
    switch (instance_start(arguments))
    {
        case INSTANCE_FORWARDED:
            logger.info("Handed the arguments to the running instance.");
            return EXIT_SUCCESS;
        case INSTANCE_ALONE:
            logger.warning("Another program holds the single-instance port. Running without it.");
            break;
        default:
            break;
    }

    App app;
    auto isOpen = window_open(app);
    if (isOpen)
    {
        Editor editor;
        editor.dialog = Dialog(app.window);
        for (auto& argument : arguments)
            editor.manager.open(path_from_utf8(argument));
        editor.manager.settings_apply(app.settings);
        while (!app.isQuit)
            loop_step(app, editor);
    }
    instance_stop();
    window_close(app);
    return isOpen ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char* argv[])
{
    if (is_cli(argc, (const char**)argv))
    {
#ifdef _WIN32
        // A GUI-subsystem build has no console of its own. Print to the one it was started from.
        if (AttachConsole(ATTACH_PARENT_PROCESS))
        {
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
        }
#endif
        return cli_run(argc, (const char**)argv);
    }

    logger.session_open(preferences_directory_get());
    return log_exceptions([&] { return editor_run(argc, argv); });
}
