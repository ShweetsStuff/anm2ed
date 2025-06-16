#include "main.h"

static void _init(State* state);
static void _draw(State* state);
static void _loop(State* state);
static void _quit(State* state);
static void _tick(State* state);

static void
_init(State* state)
{
	Shader shader;

	printf(STRING_INFO_INIT);

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		printf(STRING_ERROR_SDL_INIT, SDL_GetError());
		_quit(state);
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
		_quit(state);
	}

	glewInit();

	printf(STRING_INFO_GLEW_INIT);

	shader_init(&shader, RESOURCE_SHADER_VERTEX_TEXTURE_QUAD, RESOURCE_SHADER_FRAGMENT_TEXTURE_QUAD);

	imgui_init(state->window, state->glContext);

	Anm2 anm2;

	anm2_init(&anm2, "res/anm2/005.031_key.anm2");
}

void
_loop(State* state)
{
	state->tick = SDL_GetTicks();
	
	while (state->tick > state->lastTick + TICK_DELAY)
	{
		state->tick = SDL_GetTicks();
		
		if (state->tick - state->lastTick < TICK_DELAY)
            		SDL_Delay(TICK_DELAY - (state->tick - state->lastTick));

		_tick(state);

		imgui_tick(state);

		_draw(state);

		state->lastTick = state->tick;
	}
}

void
_tick(State* state)
{
	SDL_Event event;

	while(SDL_PollEvent(&event))
	{
    		ImGui_ImplSDL3_ProcessEvent(&event);
		
		if (event.type == SDL_EVENT_QUIT)
			state->isRunning = false;
	}
}

void 
_draw(State* state)
{
	glClearColor(state->clearColor.x, state->clearColor.y, state->clearColor.z, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	imgui_draw();

	SDL_GL_SwapWindow(state->window);
}

void
_quit(State* state)
{
	imgui_free();

	SDL_GL_DestroyContext(state->glContext);
	SDL_Quit();

	printf(STRING_INFO_SDL_QUIT);

	printf(STRING_INFO_QUIT);
}

s32
main(s32 argc, char* argv[])
{
	State state;

	_init(&state);

	while (state.isRunning)
		_loop(&state);

	_quit(&state);

	return EXIT_SUCCESS;
}
