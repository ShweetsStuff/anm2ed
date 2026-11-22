#pragma once

#include "vec.h"

enum ShortcutScope
{
    SCOPE_FOCUSED,
    SCOPE_GLOBAL,
    SCOPE_FOCUSED_SET,
    SCOPE_GLOBAL_SET
};

enum Destination
{
    DESTINATION_ALL,
    DESTINATION_THIS
};

enum Source
{
    SOURCE_NEW,
    SOURCE_EXISTING
};

enum OverlayDrawOrder
{
    OVERLAY_OVER,
    OVERLAY_UNDER
};

enum Merge
{
    MERGE_PREPEND,
    MERGE_APPEND,
    MERGE_REPLACE,
    MERGE_IGNORE
};

enum EditState
{
    EDIT_STATE_NONE,
    EDIT_STATE_START,
    EDIT_STATE_DURING,
    EDIT_STATE_END,
    EDIT_STATE_COMPLETE
};

constexpr auto COLOR_WHITE = Vec4(1.0);
constexpr auto COLOR_BLACK = Vec4(0.0, 0.0, 0.0, 1.0);
constexpr auto COLOR_RED = Vec4(1.0, 0.0, 0.0, 1.0);
constexpr auto COLOR_GREEN = Vec4(0.0, 1.0, 0.0, 1.0);
constexpr auto COLOR_BLUE = Vec4(0.0, 0.0, 1.0, 1.0);
constexpr auto COLOR_PINK = Vec4(1.0, 0.0, 1.0, 1.0);

enum class OnionskinMode
{
    TIME,
    INDEX
};

