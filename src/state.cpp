#include "state.h"

static void _tick(State* state);
static void _draw(State* state);

static void
_tick(State* state)
{
	SDL_Event event;
	SDL_MouseWheelEvent* mouseWheelEvent;

	state->input.mouse.wheelDeltaY = 0;

	while(SDL_PollEvent(&event))
	{
    	ImGui_ImplSDL3_ProcessEvent(&event);
		
		switch (event.type)
		{
			case SDL_EVENT_QUIT:
				state->isRunning = false;
				break;
			case SDL_EVENT_MOUSE_WHEEL:
				mouseWheelEvent = &event.wheel;
            	state->input.mouse.wheelDeltaY = mouseWheelEvent->y;
				break;
			default:
				break;
		}
	}

	SDL_GetWindowSize(state->window, &state->settings.windowW, &state->settings.windowH);

	input_tick(&state->input);
	editor_tick(&state->editor);
	preview_tick(&state->preview);
	tool_tick(&state->tool);
	dialog_tick(&state->dialog);
	imgui_tick(&state->imgui);
}

static void 
_draw(State* state)
{
	editor_draw(&state->editor);
	preview_draw(&state->preview);
	imgui_draw(&state->imgui);

	SDL_GL_SwapWindow(state->window);
}

void
init(State* state)
{
	/* set start working directory */
    std::filesystem::path startPath = std::filesystem::current_path();

	memset(state->startPath, '\0', PATH_MAX - 1);

	strncpy(state->startPath, startPath.c_str(), PATH_MAX - 1);
	
	settings_load(&state->settings);
	
	printf(STRING_INFO_INIT);

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		printf(STRING_ERROR_SDL_INIT, SDL_GetError());
		quit(state);
	}

	printf(STRING_INFO_SDL_INIT);

	SDL_CreateWindowAndRenderer
	(
		STRING_WINDOW_TITLE, 
		state->settings.windowW, 
		state->settings.windowH, 
		WINDOW_FLAGS, 
		&state->window, 
		&state->renderer
	);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	state->glContext = SDL_GL_CreateContext(state->window);
	
	printf(STRING_INFO_OPENGL, glGetString(GL_VERSION));

	if (!state->glContext)
	{
		printf(STRING_ERROR_GL_CONTEXT_INIT, SDL_GetError());
		quit(state);
	}

	glewInit();
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
	glDisable(GL_DEPTH_TEST);
	glLineWidth(LINE_WIDTH);

	printf(STRING_INFO_GLEW_INIT);

	resources_init(&state->resources);
	dialog_init(&state->dialog, &state->anm2, &state->reference, &state->resources, state->window);
	tool_init(&state->tool, &state->input);

	preview_init
	(
		&state->preview, 
		&state->anm2, 
		&state->reference, 
		&state->animationID,
		&state->resources, 
		&state->settings
	);

	editor_init
	(
		&state->editor, 
		&state->anm2, 
		&state->reference, 
		&state->animationID, 
		&state->spritesheetID, 
		&state->resources, 
		&state->settings
	);
	
	imgui_init
	(
		&state->imgui,
		&state->dialog,
		&state->resources,
		&state->input,
		&state->anm2,
		&state->reference,
		&state->animationID,
		&state->spritesheetID,
		&state->editor,
		&state->preview,
		&state->settings,
		&state->tool,
		state->window,
		&state->glContext
	);

	if (state->isArgument)
		anm2_deserialize(&state->anm2, &state->resources, state->argument);
	else
		anm2_new(&state->anm2);

	window_title_from_anm2_set(state->window, &state->anm2);
}

void
loop(State* state)
{
	state->tick = SDL_GetTicks();
	
	while (state->tick > state->lastTick + TICK_DELAY)
	{
		state->tick = SDL_GetTicks();
		
		if (state->tick - state->lastTick < TICK_DELAY)
        	SDL_Delay(TICK_DELAY - (state->tick - state->lastTick));

		_tick(state);

		state->lastTick = state->tick;
	}

	_draw(state);
}

void
quit(State* state)
{
	/* return to base path */
    std::filesystem::current_path(state->startPath);

	imgui_free(&state->imgui);
	settings_save(&state->settings);
	preview_free(&state->preview);
	editor_free(&state->editor);
	resources_free(&state->resources);

	SDL_GL_DestroyContext(state->glContext);
	SDL_Quit();

	printf(STRING_INFO_QUIT);
}

