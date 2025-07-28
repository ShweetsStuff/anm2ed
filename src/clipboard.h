#pragma once

#include "anm2.h"

enum ClipboardItemType
{
    CLIPBOARD_NONE,
    CLIPBOARD_FRAME,
    CLIPBOARD_ANIMATION,
    CLIPBOARD_EVENT
};

struct ClipboardItem
{
    std::variant<std::monostate, Anm2FrameWithReference, Anm2AnimationWithID, Anm2EventWithID> data = std::monostate();
    ClipboardItemType type = CLIPBOARD_NONE;

    ClipboardItem() = default; 
    
    ClipboardItem(const Anm2FrameWithReference& frame)
        : data(frame), type(CLIPBOARD_FRAME) {}

    ClipboardItem(const Anm2AnimationWithID& anim)
        : data(anim), type(CLIPBOARD_ANIMATION) {}

    ClipboardItem(const Anm2EventWithID& event)
        : data(event), type(CLIPBOARD_EVENT) {}
};
    
using ClipboardLocation = std::variant<std::monostate, Anm2Reference, s32>;

struct Clipboard
{
    Anm2* anm2 = nullptr;
    ClipboardItem item;
    ClipboardItem hoveredItem;
    ClipboardLocation location;
};

void clipboard_copy(Clipboard* self);
void clipboard_cut(Clipboard* self);
void clipboard_paste(Clipboard* self);
void clipboard_init(Clipboard* self, Anm2* anm2);

