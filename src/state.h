#pragma once

#include "imgui.h"

#define STATE_INIT_INFO "Initializing anm2ed (Version 1.1)"
#define STATE_SDL_INIT_ERROR "Failed to initialize SDL! {}"
#define STATE_SDL_INIT_INFO "Initialized SDL"
#define STATE_MIX_INIT_WARNING "Unable to initialize SDL_mixer! {}"
#define STATE_MIX_AUDIO_DEVICE_INIT_WARNING "Unable to initialize audio device! {}"
#define STATE_MIX_INIT_INFO "Initialized SDL_mixer"
#define STATE_GL_CONTEXT_INIT_ERROR "Failed to initialize OpenGL context! {}"
#define STATE_GLAD_INIT_ERROR "Failed to initialize GLAD!"
#define STATE_GL_CONTEXT_INIT_INFO "Initialized OpenGL context (OpenGL {})"
#define STATE_QUIT_INFO "Exiting..."
#define STATE_GL_LINE_WIDTH 2.0f

#define STATE_DELAY_MIN 1

#define STATE_MIX_FLAGS (MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_WAV)
#define STATE_MIX_SAMPLE_RATE 44100
#define STATE_MIX_FORMAT MIX_DEFAULT_FORMAT
#define STATE_MIX_CHANNELS 2
#define STATE_MIX_CHUNK_SIZE 1024
#define STATE_MIX_DEVICE NULL
#define STATE_MIX_ALLOWED_CHANGES SDL_AUDIO_ALLOW_FORMAT_CHANGE

#define STATE_GL_VERSION_MAJOR 3
#define STATE_GL_VERSION_MINOR 3

struct State {
  SDL_Window* window;
  SDL_GLContext glContext;
  Imgui imgui;
  Dialog dialog;
  Editor editor;
  Preview preview;
  GeneratePreview generatePreview;
  Anm2 anm2;
  Anm2Reference reference;
  Resources resources;
  Settings settings;
  Snapshots snapshots;
  Clipboard clipboard;
  std::string argument{};
  std::string lastAction{};
  uint64_t lastTick{};
  uint64_t tick{};
  uint64_t update{};
  uint64_t lastUpdate{};
  bool isRunning = true;
};

bool sdl_init(State* self, bool isTestMode);
void init(State* state);
void loop(State* state);
void quit(State* state);