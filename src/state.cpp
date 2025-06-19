#include "state.h"

static void _tick(State* state);
static void _draw(State* state);

static void
_tick(State* state)
{
	SDL_Event event;

	while(SDL_PollEvent(&event))
	{
    		ImGui_ImplSDL3_ProcessEvent(&event);
		
		if (event.type == SDL_EVENT_QUIT)
			state->isRunning = false;
	}

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
	Shader shader;

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

	preview_init(&state->preview);

	printf(STRING_INFO_GLEW_INIT);

	packed_init(&state->packed);

	if (state->isArgument)
		anm2_deserialize(&state->anm2, state->argument);
	else
		anm2_new(&state->anm2);

	window_title_from_anm2_set(state->window, &state->anm2);
	
	state->imgui = 
	{
		&state->dialog,
		&state->packed,
		&state->anm2, 
		&state->preview,
		state->window
	};

	imgui_init(state->window, state->glContext);

	state->dialog.anm2 = &state->anm2;
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
	packed_free(&state->packed);

	SDL_GL_DestroyContext(state->glContext);
	SDL_Quit();

	printf(STRING_INFO_SDL_QUIT);

	printf(STRING_INFO_QUIT);
}

