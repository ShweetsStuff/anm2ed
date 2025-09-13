#include "state.h"

static void _tick(State* self)
{
	preview_tick(&self->preview);
}

static void _update(State* self)
{
	SDL_GetWindowSize(self->window, &self->settings.windowSize.x, &self->settings.windowSize.y);
	
	imgui_update(&self->imgui);

	if (self->imgui.isQuit) 
		self->isRunning = false;
}

static void _draw(State* self)
{
	imgui_draw();

	SDL_GL_SwapWindow(self->window);
}

bool sdl_init(State* self, bool isTestMode = false)
{
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		log_error(std::format(STATE_SDL_INIT_ERROR, SDL_GetError()));
		quit(self);
		return false;
	}

	if (!isTestMode) log_info(STATE_SDL_INIT_INFO);
	
	// Todo, when sdl3 mixer is released officially
	/*
    if ((Mix_Init(STATE_MIX_FLAGS) & mixFlags) != mixFlags) 
		log_warning(std::format(STATE_MIX_INIT_WARNING, Mix_GetError()));
		
    if 
	(
		Mix_OpenAudioDevice
		(
			STATE_MIX_SAMPLE_RATE, 
			STATE_MIX_FORMAT, 
			STATE_MIX_CHANNELS, 
			STATE_CHUNK_SIZE, 
			STATE_MIX_DEVICE, 
			STATE_MIX_ALLOWED_CHANGES
		) 
		< 0 
	)
	{
		log_warning(std::format(STATE_MIX_INIT_WARNING, Mix_GetError()));
        Mix_Quit();
    }
	else
		log_info(STATE_MIX_INIT_INFO);
	*/


	if (isTestMode)
	{
		self->window = SDL_CreateWindow
		(
			WINDOW_TITLE, 
			WINDOW_TEST_MODE_SIZE.x, WINDOW_TEST_MODE_SIZE.y,
			WINDOW_TEST_MODE_FLAGS
		);
	}
	else
	{
		ivec2 windowSize = self->settings.windowSize;

		// Fix for auto-fullscreen on Windows
		if (SDL_DisplayID* displayIDs = SDL_GetDisplays(nullptr))
			if (displayIDs[0])
				if (const SDL_DisplayMode* displayMode = SDL_GetDesktopDisplayMode(displayIDs[0]))
					if (windowSize.x == displayMode->w && windowSize.y == displayMode->h)
						windowSize -= ivec2(1, 1);
				
		self->window = SDL_CreateWindow
		(
			WINDOW_TITLE, 
			windowSize.x,
			windowSize.y, 
			WINDOW_FLAGS
		);
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, STATE_GL_VERSION_MAJOR);
   	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, STATE_GL_VERSION_MINOR);

	self->glContext = SDL_GL_CreateContext(self->window);
	
	if (!self->glContext)
	{
		log_error(std::format(STATE_GL_CONTEXT_INIT_ERROR, SDL_GetError()));
		quit(self);
		return false;
	}

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) 
	{
		log_error(std::format(STATE_GLAD_INIT_ERROR));
		quit(self);
		return false;
	}

	if (!isTestMode) log_info(std::format(STATE_GL_CONTEXT_INIT_INFO, (const char*)glGetString(GL_VERSION)));
	
	window_vsync_set(self->settings.isVsync);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
	glLineWidth(STATE_GL_LINE_WIDTH);
	glDisable(GL_MULTISAMPLE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LINE_SMOOTH);

	return true;
}

void init(State* self)
{
	log_info(STATE_INIT_INFO);

	settings_init(&self->settings);

	if (!sdl_init(self)) return;

	if (!self->argument.empty())
	{
		anm2_deserialize(&self->anm2, self->argument);
		window_title_from_path_set(self->window, self->argument);
	}
	else
		anm2_new(&self->anm2);

	resources_init(&self->resources);
	dialog_init(&self->dialog, self->window);
	clipboard_init(&self->clipboard, &self->anm2);
	preview_init(&self->preview, &self->anm2, &self->reference, &self->resources, &self->settings);
	generate_preview_init(&self->generatePreview, &self->anm2, &self->reference, &self->resources, &self->settings);
	editor_init(&self->editor, &self->anm2, &self->reference, &self->resources, &self->settings);
	snapshots_init(&self->snapshots, &self->anm2, &self->reference, &self->preview);
	
	imgui_init
	(
		&self->imgui,
		&self->dialog,
		&self->resources,
		&self->anm2,
		&self->reference,
		&self->editor,
		&self->preview,
		&self->generatePreview,
		&self->settings,
		&self->snapshots,
		&self->clipboard,
		self->window,
		&self->glContext
	);
}

void loop(State* self)
{
	self->tick = SDL_GetTicks();
	self->update = self->tick;

	while (self->tick > self->lastTick + TICK_DELAY)
	{
		self->tick = SDL_GetTicks();
		
		if (self->tick - self->lastTick < TICK_DELAY)
        	SDL_Delay(TICK_DELAY - (self->tick - self->lastTick));

		_tick(self);

		self->lastTick = self->tick;
	}

	if (self->settings.isVsync)
	{
		_update(self);
		_draw(self);
	}
	else
	{
		while (self->update > self->lastUpdate + UPDATE_DELAY)
		{
			self->update = SDL_GetTicks();
			
			if (self->update - self->lastUpdate < UPDATE_DELAY)
				SDL_Delay(UPDATE_DELAY - (self->update - self->lastUpdate));

			_update(self);
			_draw(self);

			self->lastUpdate = self->update;
		}

		SDL_Delay(STATE_DELAY_MIN);
	}
}

void quit(State* self)
{
	imgui_free();
	generate_preview_free(&self->generatePreview);
	preview_free(&self->preview);
	editor_free(&self->editor);
	resources_free(&self->resources);

	/*
    Mix_CloseAudio();
    Mix_Quit();
	*/

	SDL_GL_DestroyContext(self->glContext);
	SDL_Quit();

	settings_save(&self->settings);
	log_info(STATE_QUIT_INFO);
	log_free();
}