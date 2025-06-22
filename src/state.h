#pragma once

#include "shader.h"
#include "imgui.h"

#define TICK_DELAY 16
#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900
#define WINDOW_FLAGS SDL_WINDOW_RESIZABLE

struct State
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_GLContext glContext;
    Imgui imgui;
	Input input;
	Dialog dialog;
	Preview preview;
    Anm2 anm2;
	Resources resources;
	char argument[PATH_MAX] = STRING_EMPTY;
	bool isArgument = false;
	u64 tick = 0;
	u64 lastTick = 0;
	bool isRunning = true;
}; 

void init(State* state);
void loop(State* state);
void quit(State* state);