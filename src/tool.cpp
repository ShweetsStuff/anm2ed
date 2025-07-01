#include "tool.h"

// Initializes tools
void
tool_init(Tool* self, Input* input)
{
    self->input = input;
}

// Ticks tools
void
tool_tick(Tool* self)
{
    if (!self->isEnabled) return;
        
	// Input handling for tools
	if (input_release(self->input, INPUT_PAN))
		self->type = TOOL_PAN;

	if (input_release(self->input, INPUT_MOVE))
		self->type = TOOL_MOVE;

	if (input_release(self->input, INPUT_SCALE))
		self->type = TOOL_SCALE;

	if (input_release(self->input, INPUT_ROTATE))
		self->type = TOOL_ROTATE;

	if (input_release(self->input, INPUT_CROP))
		self->type = TOOL_CROP;
}