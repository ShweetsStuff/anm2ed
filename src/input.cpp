#include "input.h"

static void _mouse_tick(Mouse* self);

// Ticks mouse
static void
_mouse_tick(Mouse* self)
{
    s32 state;

    std::memcpy(&self->previous, &self->current, sizeof(self->current));
    std::memset(&self->current, '\0', sizeof(self->current));

    state = SDL_GetMouseState(NULL, NULL);

    if ((state & SDL_BUTTON_LMASK) != 0)
    {
        self->current[MOUSE_LEFT] = true;
    }

    if ((state & SDL_BUTTON_RMASK) != 0)
    {
        self->current[MOUSE_RIGHT] = true;
    }

    SDL_GetMouseState(&self->position.x, &self->position.y);

    self->delta = self->position - self->oldPosition;
    self->oldPosition = self->position;
}

// Ticks keyboard
static void
_keyboard_tick(Keyboard* self)
{
	const bool* state;

	std::memcpy(&self->previous, &self->current, sizeof(self->previous));
	std::memset(&self->current, '\0', sizeof(self->current));

	state = SDL_GetKeyboardState(NULL);

	std::memcpy(&self->current, state, KEY_COUNT);
}

// Checks to see if the given mouse button has been pressed
bool
mouse_press(Mouse* self, MouseType type)
{
    return (self->current[type] && !self->previous[type]);
}

// Checks to see if the given mouse button is held
bool
mouse_held(Mouse* self, MouseType type)
{
    return (self->current[type] && self->previous[type]);
}

// Checks to see if the given mouse button is released
bool
mouse_release(Mouse* self, MouseType type)
{
    return (!self->current[type] && self->previous[type]);
}

// Checks to see if the given key is pressed
bool
key_press(Keyboard* self, KeyType type)
{
	return (self->current[type] && !self->previous[type]);
}

// Checks to see if the given key is held
bool
key_held(Keyboard* self, KeyType type)
{
	return (self->current[type] && self->previous[type]);
}

// Checks to see if the given key is released
bool
key_release(Keyboard* self, KeyType type)
{
	return (!self->current[type] && self->previous[type]);
}

// Checks to see if the given input is pressed
bool
input_press(Input* self, InputType type)
{
    for (KeyType key : INPUT_KEYS[type])
        if (!key_press(&self->keyboard, (key))) 
            return false; 
    return true;
}

// Checks to see if the given input is held
bool
input_held(Input* self, InputType type)
{
    for (KeyType key : INPUT_KEYS[type])
        if (!key_held(&self->keyboard, (key))) 
            return false; 
    return true;
}

// Checks to see if the given input is held
bool
input_release(Input* self, InputType type)
{
    for (KeyType key : INPUT_KEYS[type])
        if (!key_release(&self->keyboard, (key))) 
            return false; 
    return true;
}

// Ticks input
void
input_tick(Input* self)
{
    _mouse_tick(&self->mouse);
    _keyboard_tick(&self->keyboard);
}
