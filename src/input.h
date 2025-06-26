#pragma once

#include "COMMON.h"

#define MOUSE_COUNT (MOUSE_RIGHT + 1)
enum MouseType 
{
    MOUSE_LEFT,
    MOUSE_RIGHT
};

#define KEY_COUNT (KEY_DELETE + 1) 
enum KeyType
{
    KEY_DELETE
};

#define INPUT_COUNT (INPUT_MOUSE_CLICK + 1)
enum InputType
{
    INPUT_MOUSE_CLICK
};

struct Keyboard
{
    bool current[MOUSE_COUNT];
    bool previous[MOUSE_COUNT];
};

struct Mouse
{
    bool current[MOUSE_COUNT];
    bool previous[MOUSE_COUNT];
    vec2 position = {-1, -1};
    vec2 oldPosition = {-1, -1};
    vec2 delta = {-1, -1};
    s32 wheelDeltaY = 0;
};

struct Input
{
    Keyboard keyboard;
    Mouse mouse;
};

bool mouse_press(Mouse* self, MouseType type);
bool mouse_held(Mouse* self, MouseType type);
bool mouse_release(Mouse* self, MouseType type);
void input_tick(Input* self);