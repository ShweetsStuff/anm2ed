#pragma once

#include <set>
#include <vector>

#include "model/draw.h"

// How baking makes frames: whole scales and rotations, a root's easing kept, and turning around the root's position.
struct BakeOptions
{
    bool isRoundScale = false;
    bool isRoundRotation = false;
    bool isMatchRootInterpolation = false;
    bool isUseRootPivot = false;
};

void frame_mix(Frame&, const Frame&, float);
std::vector<Frame> frame_bake_split(const Frame&, const Frame&, int, BakeOptions = {});
Mat4 frame_parent_model_get(const Frame&);
void frame_root_transform_apply(Frame&, const Frame&, BakeOptions = {});
Frame frame_generate(const Track&, float);
void track_group_root_bake(Track&, const Track&, int);
int frame_index_from_at_frame_get(const Track&, int);
int frame_index_from_time_get(const Track&, float);
float frame_time_from_index_get(const Track&, int);
void frame_bake(Track&, int, int, BakeOptions = {});
void frames_generate_from_grid(Track&, IVec2, IVec2, Vec2, int, int, int);
void frames_sort_by_at_frame(Track&);
void frames_change(Track&, FrameChange, ItemType, ChangeType, const std::set<int>&, bool isAnimationLayer = false);
Vec4 animation_rect(const Model&, const Animation&, bool, const LibraryGet& = {});
