#pragma once

#include "ui/canvas_view.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

class SpritesheetEditor : public CanvasView
{
    Vec2 previousMousePos{};
    Vec2 cropAnchor{};
    int hoveredRegionId = -1;

public:
    void update(Manager&, Settings&, Resources&);
};
