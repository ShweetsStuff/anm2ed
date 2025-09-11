#pragma once

#include "anm2.h"

#define CLIPBOARD_TEXT_SET_WARNING "Unable to set clipboard text! ({})"

enum ClipboardType
{
    CLIPBOARD_NONE,
    CLIPBOARD_FRAME,
    CLIPBOARD_ANIMATION
};

using ClipboardLocation = std::variant<std::monostate, Anm2Reference, s32>;

struct Clipboard
{
    Anm2* anm2 = nullptr;
    ClipboardType type;
    ClipboardLocation location;
};

bool clipboard_is_value(void);
void clipboard_copy(Clipboard* self);
void clipboard_cut(Clipboard* self);
void clipboard_paste(Clipboard* self);
void clipboard_init(Clipboard* self, Anm2* anm2);