#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "image.h"
#include "model/draw.h"
#include "model/model.h"

// Document edits as plain functions of the model. Each returns the uids of the elements it created or changed that
// the caller may want to select (the first one is the focus). Selection itself stays with the UI.
using Uids = std::vector<std::uint64_t>;
using ImageGet = std::function<const Image*(int)>;

struct RowTarget
{
    bool isGroup = false;
    int type = NONE;
    int id = -1;
    int groupId = -1;
};

struct RootBakeOptions
{
    bool isRoundScale = false;
    bool isRoundRotation = false;
    bool isMatchRootInterpolation = false;
    bool isUseRootPivot = false;
};

struct GridOptions
{
    IVec2 start{};
    IVec2 size{};
    Vec2 pivot{};
    int columns = 0;
    int count = 0;
    int delay = 0;
    bool isMakeRegions = false;
    std::string regionNameFormat;
};

struct ChangeTargets
{
    std::set<Reference> references;
    std::set<int> animations;
    bool isRoot = false;
    bool isLayers = false;
    bool isNulls = false;
};

struct AnimationTarget
{
    int animationIndex = -1;
    int groupId = -1;
    bool isAfter = false;
    bool isInto = false;
};

// Frame properties that can be copied on their own.
enum class FrameProperty
{
    POSITION,
    SCALE,
    ROTATION,
    SKEW,
    PIVOT,
    CROP,
    SIZE,
    REGION,
    TINT,
    COLOR_OFFSET,
    VISIBLE,
    INTERPOLATION,
    DURATION,
    SHADER,
    DEPTH,
    BLUR,
    CHILD,
    EVENT,
    SOUNDS,
    COUNT
};

bool is_frame_property_valid(FrameProperty, ItemType);
void frame_property_copy(Frame&, const Frame&, FrameProperty);
Uids frames_property_set(Model&, const std::set<Reference>&, FrameProperty, const Frame&);

std::map<Reference, std::set<int>> frames_by_track_get(const std::set<Reference>&);

Uids frame_insert(Model&, Reference, int);
Uids frames_delete(Model&, const std::set<Reference>&);
Uids frames_duplicate(Model&, const std::set<Reference>&, Reference);
Uids frames_reverse(Model&, const std::set<Reference>&);
Uids frames_bake(Model&, const std::set<Reference>&, int, bool, bool);
Uids frame_split(Model&, Reference, int);
Uids frames_split(Model&, const std::set<Reference>&, int);
Uids frames_move(Model&, const std::set<Reference>&, Reference, int);
Uids frame_durations_set(Model&, const std::map<Reference, int>&);
Uids trigger_at_frame_set(Model&, Reference, int);
Uids frames_paste(Model&, Reference, const std::set<Reference>&, const std::string&, int, std::string*);
Uids root_bake_into(Model&, const std::set<Reference>&, const std::set<Reference>&, RootBakeOptions);

Uids items_remove(Model&, int, const std::map<int, std::set<int>>&, const std::map<int, std::set<int>>&);
Uids items_group(Model&, int, int, const std::set<int>&, const std::string&);
Uids items_move(Model&, int, int, const std::set<int>&, const std::set<int>&, RowTarget, bool, bool);
Uids animation_length_fit(Model&, int);
int item_add(Model&, ItemType, int, int, Layer, Null, int, Destination);

std::set<int> animation_group_ids_get(const Model&);
std::set<int> animation_group_indices_get(const Model&, int);
void animation_groups_remove(Model&, const std::set<int>&);
Uids animation_add(Model&, int, int, int, const std::string&);
Uids animations_remove(Model&, const std::set<int>&, const std::set<int>&);
Uids animations_move(Model&, const std::set<int>&, const std::set<int>&, AnimationTarget);
Uids animations_group(Model&, const std::set<int>&, const std::string&);
Uids animations_paste(Model&, const std::string&, int, int, std::set<int>&, std::string*);
Uids animations_merge(Model&, int, std::set<int>, Merge, bool, const std::set<int>&);

std::string region_name_get(const std::string&, int);
Uids regions_generate(Model&, const std::set<int>&, const std::set<Reference>&, const std::string&, RegionFrameMapping);
Uids regions_scan(Model&);
Uids region_frames_sync(Model&);
Uids regions_paste(Model&, int, const std::string&, int, std::string*);
Uids animation_grid_generate(Model&, const std::vector<Reference>&, const GridOptions&);
Uids frames_change_apply(Model&, const ChangeTargets&, FrameChange, ChangeType);
bool regions_trim(Model&, int, const std::set<int>&, const Image&);
std::optional<Image> spritesheet_pack(Model&, int, const Image&, int);
std::optional<Image> spritesheets_merge(Model&, const std::set<int>&, const ImageGet&, bool, bool, bool, Origin);

// Where a spritesheet path of a document (this one, or a library) points, from this document.
using SpritesheetPathGet = std::function<std::filesystem::path(const Model&, const std::filesystem::path&)>;
Model animation_layers_flatten(Model, const LibraryGet&, const SpritesheetPathGet&, const std::set<int>& = {});
Uids content_remove(Model&, ElementType, const std::set<int>&, int, const LibraryGet&, const SpritesheetPathGet&);
Model isaac_bridge(Model);
bool file_merge(Model&, const std::filesystem::path&, const std::filesystem::path&, FileMergePreset);
