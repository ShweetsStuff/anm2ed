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

	input_tick(&state->input);
	preview_tick(&state->preview);
	dialog_tick(&state->dialog);
	imgui_tick(&state->imgui);
}

static void 
_draw(State* state)
{
	preview_draw(&state->preview);
	imgui_draw(&state->imgui);

	SDL_GL_SwapWindow(state->window);
}

void
init(State* state)
{
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
		WINDOW_WIDTH, 
		WINDOW_HEIGHT, 
		WINDOW_FLAGS, 
		&state->window, 
		&state->renderer
	);

	state->glContext = SDL_GL_CreateContext(state->window);
	
	if (!state->glContext)
	{
		printf(STRING_ERROR_GL_CONTEXT_INIT, SDL_GetError());
		quit(state);
	}

	glewInit();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

	printf(STRING_INFO_GLEW_INIT);
	
	resources_init(&state->resources);
	dialog_init(&state->dialog, &state->anm2, &state->resources);
	
	preview_init(&state->preview, &state->resources, &state->input);

	if (state->isArgument)
		anm2_deserialize(&state->anm2, &state->resources, state->argument);
	else
		anm2_new(&state->anm2);

	window_title_from_anm2_set(state->window, &state->anm2);
	
	imgui_init
	(
		&state->imgui,
		&state->dialog,
		&state->resources,
		&state->input,
		&state->anm2,
		&state->preview,
		state->window, 
		&state->glContext
	);
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
		_draw(state);

		state->lastTick = state->tick;
	}
}

void
quit(State* state)
{
	imgui_free(&state->imgui);
	preview_free(&state->preview);
	resources_free(&state->resources);

	SDL_GL_DestroyContext(state->glContext);
	SDL_Quit();

	printf(STRING_INFO_QUIT);
}

