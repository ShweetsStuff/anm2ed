#pragma once

#include "imgui.h"

#define STATE_INIT_INFO "Initializing..."
#define STATE_SDL_INIT_ERROR "Failed to initialize SDL! {}"
#define STATE_SDL_INIT_INFO "Initialized SDL"
#define STATE_GL_CONTEXT_INIT_ERROR "Failed to initialize OpenGL context! {}"
#define STATE_GL_CONTEXT_INIT_INFO "Initialized OpenGL context (OpenGL {})"
#define STATE_QUIT_INFO "Exiting..."
#define STATE_GL_LINE_WIDTH 2.0f

struct State
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_GLContext glContext;
    Imgui imgui;
	Dialog dialog;
	Editor editor;
	Preview preview;
    Anm2 anm2;
	Anm2Reference reference;
	Resources resources;
	Settings settings;
	Snapshots snapshots;
	Clipboard clipboard;
	bool isRunning = true;
	std::string argument{};
	u64 lastTick = 0;
	u64 tick = 0;

	bool is_argument() const { return !argument.empty(); }
}; 

void init(State* state);
void loop(State* state);
void quit(State* state);