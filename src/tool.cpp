#include "tool.h"

void
tool_init(Tool* self, Input* input)
{
    self->input = input;
}

void
tool_tick(Tool* self)
{
    if (!self->isEnabled) return;
        
	/* Input handling */
	if (key_press(&self->input->keyboard, INPUT_KEYS[INPUT_PAN]))
		self->type = TOOL_PAN;

	if (key_press(&self->input->keyboard, INPUT_KEYS[INPUT_MOVE]))
		self->type = TOOL_MOVE;

	if (key_press(&self->input->keyboard, INPUT_KEYS[INPUT_SCALE]))
		self->type = TOOL_SCALE;

	if (key_press(&self->input->keyboard, INPUT_KEYS[INPUT_CROP]))
		self->type = TOOL_CROP;

	if 
	(
		key_press(&self->input->keyboard, INPUT_KEYS[INPUT_ROTATE_LEFT]) ||
		key_press(&self->input->keyboard, INPUT_KEYS[INPUT_ROTATE_RIGHT])
	)
		self->type = TOOL_ROTATE;
}