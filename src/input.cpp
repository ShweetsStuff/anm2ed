#include "input.h"

static void _mouse_tick(Mouse* self);

static void
_mouse_tick(Mouse* self)
{
    s32 state;
    SDL_Event event;

    memcpy(&self->previous, &self->current, sizeof(bool) * MOUSE_COUNT);
    memset(&self->current, '\0', sizeof(bool) * MOUSE_COUNT);

    state = SDL_GetMouseState(NULL, NULL);

    if (state & SDL_BUTTON_LMASK != 0)
    {
        self->current[MOUSE_LEFT] = true;
    }

    if (state & SDL_BUTTON_RMASK != 0)
    {
        self->current[MOUSE_RIGHT] = true;
    }

	SDL_GetMouseState(&self->position.x, &self->position.y);

    self->delta = self->position - self->oldPosition;
    self->oldPosition = self->position;
}

bool
mouse_press(Mouse* self, MouseType type)
{
    return (self->current[type] && !self->previous[type]);
}

bool
mouse_held(Mouse* self, MouseType type)
{
    return (self->current[type] && self->previous[type]);
}

bool
mouse_release(Mouse* self, MouseType type)
{
    return (!self->current[type] && self->previous[type]);
}

void
input_tick(Input* self)
{
    _mouse_tick(&self->mouse);
}
