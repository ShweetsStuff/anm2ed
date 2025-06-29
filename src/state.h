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
	Anm2Reference reference;
	Resources resources;
	Settings settings;
	Tool tool;
	bool isArgument = false;
	bool isRunning = true;
	char argument[PATH_MAX] = STRING_EMPTY;
	char startPath[PATH_MAX] = STRING_EMPTY;
	s32 animationID = -1;
	s32 spritesheetID = -1;
	u64 lastTick = 0;
	u64 tick = 0;
}; 

void init(State* state);
void loop(State* state);
void quit(State* state);