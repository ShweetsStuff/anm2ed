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
#include "model/frames.h"
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

// Tracks and whole groups (of one type, in one animation) moved next to, or into, a target row.
struct ItemsMove
{
    int animationIndex = 0;
    int type = NONE;
    std::set<int> ids;
    std::set<int> groupIds;
    RowTarget target = {};
    DropZone zone = DROP_BEFORE;
};

// A layer or null added (a new one, from `layer` or `null`, at id -1) with a track in one animation or all of them,
// placed before track `insertBeforeId` (at the end, at -1).
struct ItemAdd
{
    ItemType type = ItemType::LAYER;
    int animationIndex = 0;
    int id = -1;
    int insertBeforeId = -1;
    Destination destination = DESTINATION_ALL;
    Layer layer = {};
    Null null = {};
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
// Each frame property: the tracks that have it (all but triggers, layers only, or triggers only) and what copying it
// takes from a source frame.
#define FRAME_PROPERTIES(X)                                                                                            \
    X(POSITION, FRAMES, target.position = source.position)                                                             \
    X(SCALE, FRAMES, target.scale = source.scale)                                                                      \
    X(ROTATION, FRAMES, target.rotation = source.rotation)                                                             \
    X(SKEW, FRAMES, target.skew = source.skew)                                                                         \
    X(PIVOT, LAYERS, target.pivot = source.pivot)                                                                      \
    X(CROP, LAYERS, target.crop = source.crop)                                                                         \
    X(SIZE, LAYERS, target.size = source.size)                                                                         \
    X(REGION, LAYERS, target.regionId = source.regionId; target.crop = source.crop; target.size = source.size;         \
      target.pivot = source.pivot)                                                                                     \
    X(TINT, FRAMES, target.tint = source.tint)                                                                         \
    X(COLOR_OFFSET, FRAMES, target.colorOffset = source.colorOffset)                                                   \
    X(VISIBLE, FRAMES, target.isVisible = source.isVisible)                                                            \
    X(INTERPOLATION, FRAMES, target.interpolation = source.interpolation)                                              \
    X(DURATION, FRAMES, target.duration = source.duration)                                                             \
    X(SHADER, LAYERS, target.shaderId = source.shaderId)                                                               \
    X(DEPTH, LAYERS, target.depth = source.depth)                                                                      \
    X(BLUR, LAYERS, target.blur = source.blur)                                                                         \
    X(CHILD, LAYERS, target.childAnimation = source.childAnimation; target.childMode = source.childMode;               \
      target.childStart = source.childStart; target.isChildStartRandom = source.isChildStartRandom;                    \
      target.isChildRestart = source.isChildRestart)                                                                   \
    X(EVENT, TRIGGERS, target.eventIds = source.eventIds)                                                              \
    X(SOUNDS, TRIGGERS, target.soundIds = source.soundIds)

enum class FrameProperty
{
#define X(property, tracks, ...) property,
    FRAME_PROPERTIES(X)
#undef X
    COUNT
};

bool is_frame_property_valid(FrameProperty, ItemType);
void frame_property_copy(Frame&, const Frame&, FrameProperty);
Uids frames_property_set(Model&, const std::set<Reference>&, FrameProperty, const Frame&);

std::map<Reference, std::set<int>> frames_by_track_get(const std::set<Reference>&);

Uids frame_insert(Model&, Reference, int);
void frames_delete(Model&, const std::set<Reference>&);
Uids frames_duplicate(Model&, const std::set<Reference>&, Reference);
void frames_reverse(Model&, const std::set<Reference>&);
Uids frames_bake(Model&, const std::set<Reference>&, int, BakeOptions);
Uids frame_split(Model&, Reference, int);
Uids frames_split(Model&, const std::set<Reference>&, int);
Uids frames_move(Model&, const std::set<Reference>&, Reference, int);
void frame_durations_set(Model&, const std::map<Reference, int>&);
void trigger_at_frame_set(Model&, Reference, int);
Uids frames_paste(Model&, Reference, const std::set<Reference>&, const std::string&, int, std::string*);
Uids root_bake_into(Model&, const std::set<Reference>&, const std::set<Reference>&, BakeOptions);

void items_remove(Model&, int, const std::map<int, std::set<int>>&, const std::map<int, std::set<int>>&);
Uids items_group(Model&, int, int, const std::set<int>&, const std::string&);
void items_move(Model&, const ItemsMove&);
void animation_length_fit(Model&, int);
int item_add(Model&, ItemAdd);

std::set<int> animation_group_ids_get(const Model&);
std::set<int> animation_group_indices_get(const Model&, int);
void animation_groups_remove(Model&, const std::set<int>&);
Uids animation_add(Model&, int, int, int, const std::string&);
void animations_remove(Model&, const std::set<int>&, const std::set<int>&);
Uids animations_move(Model&, const std::set<int>&, const std::set<int>&, AnimationTarget);
void animations_group(Model&, const std::set<int>&, const std::string&);
Uids animations_paste(Model&, const std::string&, int, int, std::set<int>&, std::string*);
Uids animations_merge(Model&, int, std::set<int>, Merge, MergeSources, const std::set<int>&);

std::string region_name_get(const std::string&, int);
void regions_generate(Model&, const std::set<int>&, const std::set<Reference>&, const std::string&, RegionFrameMapping);
void regions_scan(Model&);
void region_frames_sync(Model&);
void regions_paste(Model&, int, const std::string&, int, std::string*);
Uids animation_grid_generate(Model&, const std::vector<Reference>&, const GridOptions&);
Uids frames_change_apply(Model&, const ChangeTargets&, FrameChange, ChangeType);
bool regions_trim(Model&, int, const std::set<int>&, const Image&);
std::optional<Image> spritesheet_pack(Model&, int, const Image&, int);
// How spritesheets merge: side by side or stacked, and the regions made for each sheet.
struct SpritesheetMergeOptions
{
    bool isAppendRight = false;
    bool isMakeRegions = false;
    bool isMakePrimaryRegion = false;
    Origin origin = Origin::CUSTOM;
};

std::optional<Image> spritesheets_merge(Model&, const std::set<int>&, const ImageGet&, SpritesheetMergeOptions);

// Where a spritesheet path of a document (this one, or a library) points, from this document.
using SpritesheetPathGet = std::function<std::filesystem::path(const Model&, const std::filesystem::path&)>;
Model animation_layers_flatten(Model, const LibraryGet&, const SpritesheetPathGet&, const std::set<int>& = {});
void content_remove(Model&, ElementType, const std::set<int>&, int, const LibraryGet&, const SpritesheetPathGet&);
Model isaac_bridge(Model);
bool file_merge(Model&, const std::filesystem::path&, const std::filesystem::path&, FileMergePreset);
