#pragma once

#include "anm2.h"
#include <variant>

#define CLIPBOARD_TEXT_SET_WARNING "Unable to set clipboard text! ({})"

enum ClipboardType { CLIPBOARD_NONE, CLIPBOARD_FRAME, CLIPBOARD_ANIMATION };

using ClipboardValue = std::variant<std::monostate, Anm2Reference, std::set<int>, int>;

struct Clipboard {
  Anm2* anm2 = nullptr;
  ClipboardType type;
  ClipboardValue source;
  ClipboardValue destination;
};

bool clipboard_is_value(void);
void clipboard_copy(Clipboard* self);
void clipboard_cut(Clipboard* self);
bool clipboard_paste(Clipboard* self);
void clipboard_init(Clipboard* self, Anm2* anm2);