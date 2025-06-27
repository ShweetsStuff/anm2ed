#pragma once

#include "imgui.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define WINDOW_FLAGS SDL_WINDOW_RESIZABLE
#define LINE_WIDTH 2.0f

struct State
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_GLContext glContext;
    Imgui imgui;
	Input input;
	Dialog dialog;
	Editor editor;
	Preview preview;
    Anm2 anm2;
	Resources resources;
	Settings settings;
	char argument[PATH_MAX] = STRING_EMPTY;
	char startPath[PATH_MAX] = STRING_EMPTY;
	bool isArgument = false;
	u64 tick = 0;
	u64 lastTick = 0;
	bool isRunning = true;
}; 

void init(State* state);
void loop(State* state);
void quit(State* state);