#include "state.h"

static void _tick(State* self)
{
	SDL_GetWindowSize(self->window, &self->settings.windowW, &self->settings.windowH);

	editor_tick(&self->editor);
	preview_tick(&self->preview);
	dialog_tick(&self->dialog);
	imgui_tick(&self->imgui);

	if (self->imgui.isQuit) self->isRunning = false;
}

static void _draw(State* self)
{
	editor_draw(&self->editor);
	preview_draw(&self->preview);
	imgui_draw();

	SDL_GL_SwapWindow(self->window);
}

void init(State* self)
{
	settings_init(&self->settings);
	
	log_info(STATE_INIT_INFO);

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		log_error(std::format(STATE_SDL_INIT_ERROR, SDL_GetError()));
		quit(self);
	}

	log_info(STATE_SDL_INIT_INFO);
	
	SDL_CreateWindowAndRenderer
	(
		WINDOW_TITLE, 
		self->settings.windowW, 
		self->settings.windowH, 
		WINDOW_FLAGS, 
		&self->window, 
		&self->renderer
	);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	glewInit();
	
	self->glContext = SDL_GL_CreateContext(self->window);
	
	if (!self->glContext)
	{
		log_error(std::format(STATE_GL_CONTEXT_INIT_ERROR, SDL_GetError()));
		quit(self);
	}

	log_info(std::format(STATE_GL_CONTEXT_INIT_INFO, (const char*)glGetString(GL_VERSION)));
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
	glDisable(GL_DEPTH_TEST);
	glLineWidth(STATE_GL_LINE_WIDTH);

	resources_init(&self->resources);
	clipboard_init(&self->clipboard, &self->anm2);
	dialog_init(&self->dialog, &self->anm2, &self->reference, &self->resources, self->window);
	snapshots_init(&self->snapshots, &self->anm2, &self->reference, &self->preview);
	preview_init(&self->preview, &self->anm2, &self->reference, &self->resources, &self->settings);
	editor_init(&self->editor, &self->anm2, &self->reference, &self->resources, &self->settings);
	
	imgui_init
	(
		&self->imgui,
		&self->dialog,
		&self->resources,
		&self->anm2,
		&self->reference,
		&self->editor,
		&self->preview,
		&self->settings,
		&self->snapshots,
		&self->clipboard,
		self->window,
		&self->glContext
	);

	if (self->is_argument())
	{
		anm2_deserialize(&self->anm2, &self->resources, self->argument);
		window_title_from_path_set(self->window, self->argument);
	}
	else
		anm2_new(&self->anm2);
}

void loop(State* self)
{
	self->tick = SDL_GetTicks();
	
	while (self->tick > self->lastTick + TICK_DELAY)
	{
		self->tick = SDL_GetTicks();
		
		if (self->tick - self->lastTick < TICK_DELAY)
        	SDL_Delay(TICK_DELAY - (self->tick - self->lastTick));

		_tick(self);

		self->lastTick = self->tick;

	}

	_draw(self);
}

void quit(State* self)
{
	imgui_free();
	settings_save(&self->settings);
	preview_free(&self->preview);
	editor_free(&self->editor);
	resources_free(&self->resources);

	SDL_GL_DestroyContext(self->glContext);
	SDL_Quit();

	log_info(STATE_QUIT_INFO);
}