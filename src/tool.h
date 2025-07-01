#pragma once

#include "input.h"

enum ToolType
{
    TOOL_PAN,
    TOOL_MOVE,
    TOOL_ROTATE,
    TOOL_SCALE,
    TOOL_CROP,
    TOOL_COUNT
};

struct Tool
{
    Input* input = NULL;
    ToolType type = TOOL_PAN;
    bool isEnabled = false;
};

void tool_init(Tool* self, Input* input);
void tool_tick(Tool* self);