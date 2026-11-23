#pragma once

#include <tuple>
#include <vector>

#include "ui/canvas_view.h"
#include "manager.h"
#include "model/draw.h"
#include "render.h"
#include "resources.h"
#include "settings.h"

class AnimationPreview : public CanvasView
{
    bool isPlaybackPlayingBefore = false;
    int soundFrame = -1;
    bool isMoveDragging = false;
    Vec2 moveOffset{};
    Vec2 nullRectScaleAnchor{};
    Recorder recorder = {};
    Vec2 recordSize{};
    Vec2 recordPan{};
    float recordZoom = 0;

    // Where each item's pivot is at every frame of the animation, kept until the document or view changes.
    struct PivotPath
    {
        Draw item = {};
        std::vector<Vec2> points{};
    };
    std::vector<PivotPath> pivotPaths;
    std::tuple<std::uint64_t, std::uint64_t, int, bool> pivotPathsKey;

    void recording_start(Manager&, Settings&, Document&, const Animation*);
    void recording_stop(Manager&, Document&);

public:
    void tick(Manager&, Settings&, float);
    void update(Manager&, Settings&, Resources&);
};
