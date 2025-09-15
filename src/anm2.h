#pragma once

#include "resources.h"

#define ANM2_SCALE_CONVERT(x) ((f32)x / 100.0f)
#define ANM2_TINT_CONVERT(x) ((f32)x / 255.0f)

#define ANM2_FPS_MIN 0
#define ANM2_FPS_DEFAULT 30
#define ANM2_FPS_MAX 120
#define ANM2_FRAME_NUM_MIN 1
#define ANM2_FRAME_NUM_MAX 1000000
#define ANM2_FRAME_DELAY_MIN 1
#define ANM2_STRING_MAX 0xFF

#define ANM2_EMPTY_ERROR "No path given for anm2"
#define ANM2_READ_ERROR "Failed to read anm2 from file: {}"
#define ANM2_PARSE_ERROR "Failed to parse anm2: {} ({})"
#define ANM2_FRAME_PARSE_ERROR "Failed to parse frame: {} ({})"
#define ANM2_ANIMATION_PARSE_ERROR "Failed to parse frame: {} ({})"
#define ANM2_READ_INFO "Read anm2 from file: {}"
#define ANM2_WRITE_ERROR "Failed to write anm2 to file: {}"
#define ANM2_WRITE_INFO "Wrote anm2 to file: {}"
#define ANM2_CREATED_ON_FORMAT "%d-%B-%Y %I:%M:%S %p"

#define ANM2_EXTENSION "anm2"
#define ANM2_SPRITESHEET_EXTENSION "png"

#define ANM2_ELEMENT_LIST \
    X(ANIMATED_ACTOR,     "AnimatedActor")     \
    X(INFO,               "Info")              \
    X(CONTENT,            "Content")           \
    X(SPRITESHEETS,       "Spritesheets")      \
    X(SPRITESHEET,        "Spritesheet")       \
    X(LAYERS,             "Layers")            \
    X(LAYER,              "Layer")             \
    X(NULLS,              "Nulls")             \
    X(NULL,               "Null")              \
    X(EVENTS,             "Events")            \
    X(EVENT,              "Event")             \
    X(ANIMATIONS,         "Animations")        \
    X(ANIMATION,          "Animation")         \
    X(ROOT_ANIMATION,     "RootAnimation")     \
    X(FRAME,              "Frame")             \
    X(LAYER_ANIMATIONS,   "LayerAnimations")   \
    X(LAYER_ANIMATION,    "LayerAnimation")    \
    X(NULL_ANIMATIONS,    "NullAnimations")    \
    X(NULL_ANIMATION,     "NullAnimation")     \
    X(TRIGGERS,           "Triggers")          \
    X(TRIGGER,            "Trigger")

typedef enum 
{
    #define X(name, str) ANM2_ELEMENT_##name,
    ANM2_ELEMENT_LIST
    #undef X
    ANM2_ELEMENT_COUNT
} Anm2Element;

const inline char* ANM2_ELEMENT_STRINGS[] = 
{
    #define X(name, str) str,
    ANM2_ELEMENT_LIST
    #undef X
};

DEFINE_STRING_TO_ENUM_FUNCTION(ANM2_ELEMENT_STRING_TO_ENUM, Anm2Element, ANM2_ELEMENT_STRINGS, ANM2_ELEMENT_COUNT)

#define ANM2_ATTRIBUTE_LIST \
    X(CREATED_BY,        "CreatedBy")        \
    X(CREATED_ON,        "CreatedOn")        \
    X(VERSION,           "Version")          \
    X(FPS,               "Fps")              \
    X(ID,                "Id")               \
    X(PATH,              "Path")             \
    X(NAME,              "Name")             \
    X(SPRITESHEET_ID,    "SpritesheetId")    \
    X(SHOW_RECT,         "ShowRect")         \
    X(DEFAULT_ANIMATION, "DefaultAnimation") \
    X(FRAME_NUM,         "FrameNum")         \
    X(LOOP,              "Loop")             \
    X(X_POSITION,        "XPosition")        \
    X(Y_POSITION,        "YPosition")        \
    X(X_PIVOT,           "XPivot")           \
    X(Y_PIVOT,           "YPivot")           \
    X(X_CROP,            "XCrop")            \
    X(Y_CROP,            "YCrop")            \
    X(WIDTH,             "Width")            \
    X(HEIGHT,            "Height")           \
    X(X_SCALE,           "XScale")           \
    X(Y_SCALE,           "YScale")           \
    X(DELAY,             "Delay")            \
    X(VISIBLE,           "Visible")          \
    X(RED_TINT,          "RedTint")          \
    X(GREEN_TINT,        "GreenTint")        \
    X(BLUE_TINT,         "BlueTint")         \
    X(ALPHA_TINT,        "AlphaTint")        \
    X(RED_OFFSET,        "RedOffset")        \
    X(GREEN_OFFSET,      "GreenOffset")      \
    X(BLUE_OFFSET,       "BlueOffset")       \
    X(ROTATION,          "Rotation")         \
    X(INTERPOLATED,      "Interpolated")     \
    X(LAYER_ID,          "LayerId")          \
    X(NULL_ID,           "NullId")           \
    X(EVENT_ID,          "EventId")          \
    X(AT_FRAME,          "AtFrame")

typedef enum 
{
    #define X(name, str) ANM2_ATTRIBUTE_##name,
    ANM2_ATTRIBUTE_LIST
    #undef X
    ANM2_ATTRIBUTE_COUNT
} Anm2Attribute;

static const char* ANM2_ATTRIBUTE_STRINGS[] = 
{
    #define X(name, str) str,
    ANM2_ATTRIBUTE_LIST
    #undef X
};

DEFINE_STRING_TO_ENUM_FUNCTION(ANM2_ATTRIBUTE_STRING_TO_ENUM, Anm2Attribute, ANM2_ATTRIBUTE_STRINGS, ANM2_ATTRIBUTE_COUNT)

enum Anm2Type
{
    ANM2_NONE,
	ANM2_ROOT,
	ANM2_LAYER,
	ANM2_NULL,
    ANM2_TRIGGERS,
    ANM2_COUNT
};

struct Anm2Spritesheet
{
    std::string path{};
    Texture texture;
    std::vector<u8> pixels; 

    auto operator<=>(const Anm2Spritesheet&) const = default;
};

struct Anm2Layer
{
    std::string name = "New Layer";
	s32 spritesheetID{};

    auto operator<=>(const Anm2Layer&) const = default;
};

struct Anm2Null
{
    std::string name = "New Null";   
    bool isShowRect = false;

    auto operator<=>(const Anm2Null&) const = default;
};

struct Anm2Event
{
    std::string name = "New Event";

    auto operator<=>(const Anm2Event&) const = default;
};

struct Anm2Frame
{
	bool isVisible = true;
	bool isInterpolated = false;
	f32 rotation{};
	s32 delay = ANM2_FRAME_DELAY_MIN;
    s32 atFrame = INDEX_NONE;
    s32 eventID = ID_NONE;
	vec2 crop{};
	vec2 pivot{};
	vec2 position{};
	vec2 size{};
	vec2 scale = {100, 100};
	vec3 offsetRGB = COLOR_OFFSET_NONE;
	vec4 tintRGBA = COLOR_OPAQUE;

    auto operator<=>(const Anm2Frame&) const = default;
};

struct Anm2Item
{
    bool isVisible = true;
	std::vector<Anm2Frame> frames;

    auto operator<=>(const Anm2Item&) const = default;
};

struct Anm2Animation
{
	s32 frameNum = ANM2_FRAME_NUM_MIN;
    std::string name = "New Animation";
	bool isLoop = true;
    Anm2Item rootAnimation;
    std::unordered_map<s32, Anm2Item> layerAnimations;
    std::vector<s32> layerOrder;
    std::map<s32, Anm2Item> nullAnimations;
    Anm2Item triggers;
    
    auto operator<=>(const Anm2Animation&) const = default;
};

struct Anm2 
{
    std::string path{};
    std::string createdBy = "robot";
    std::string createdOn{};
	std::map<s32, Anm2Spritesheet> spritesheets; 
	std::map<s32, Anm2Layer> layers; 
	std::map<s32, Anm2Null> nulls; 
    std::map<s32, Anm2Event> events;
	std::map<s32, Anm2Animation> animations; 
    s32 defaultAnimationID = ID_NONE;
    s32 fps = ANM2_FPS_DEFAULT;
	s32 version{};

    auto operator<=>(const Anm2&) const = default;
};

struct Anm2Reference
{
    s32 animationID = ID_NONE;
    Anm2Type itemType = ANM2_NONE;
    s32 itemID = ID_NONE;
    s32 frameIndex = INDEX_NONE;
    f32 time = VALUE_NONE;
    auto operator<=>(const Anm2Reference&) const = default; 
};

struct Anm2FrameChange
{
    std::optional<bool> isVisible;
    std::optional<bool> isInterpolated;
    std::optional<f32> rotation;
    std::optional<s32> delay;
    std::optional<vec2> crop;
    std::optional<vec2> pivot;
    std::optional<vec2> position;
    std::optional<vec2> size;
    std::optional<vec2> scale;
    std::optional<vec3> offsetRGB;
    std::optional<vec4> tintRGBA;
};

enum Anm2MergeType
{
    ANM2_MERGE_APPEND_FRAMES,
    ANM2_MERGE_REPLACE_FRAMES,
    ANM2_MERGE_PREPEND_FRAMES,
    ANM2_MERGE_IGNORE
};

enum Anm2ChangeType
{
    ANM2_CHANGE_ADD,
    ANM2_CHANGE_SUBTRACT,
    ANM2_CHANGE_SET
};

enum OnionskinDrawOrder 
{
    ONIONSKIN_BELOW,
    ONIONSKIN_ABOVE
};

Anm2Animation* anm2_animation_from_reference(Anm2* self, Anm2Reference* reference);
Anm2Frame* anm2_frame_add(Anm2* self, Anm2Frame* frame, Anm2Reference* reference);
Anm2Frame* anm2_frame_from_reference(Anm2* self, Anm2Reference* reference);
Anm2Item* anm2_item_from_reference(Anm2* self, Anm2Reference* reference);
bool anm2_animation_deserialize_from_xml(Anm2Animation* animation, const std::string& xml);
bool anm2_deserialize(Anm2* self, const std::string& path, bool isTextures = true);
bool anm2_frame_deserialize_from_xml(Anm2Frame* frame, const std::string& xml);
bool anm2_serialize(Anm2* self, const std::string& path);
s32 anm2_animation_add(Anm2* self, Anm2Animation* animation = nullptr, s32 id = ID_NONE);
s32 anm2_animation_length_get(Anm2Animation* self);
s32 anm2_frame_index_from_time(Anm2* self, Anm2Reference reference, f32 time);
s32 anm2_layer_add(Anm2* self);
s32 anm2_null_add(Anm2* self);
vec4 anm2_animation_rect_get(Anm2* anm2, Anm2Reference* reference, bool isRootTransform);
void anm2_animation_layer_animation_add(Anm2Animation* animation, s32 id);
void anm2_animation_layer_animation_remove(Anm2Animation* animation, s32 id);
void anm2_animation_length_set(Anm2Animation* self);
void anm2_animation_merge(Anm2* self, s32 animationID, const std::vector<s32>& mergeIDs, Anm2MergeType type);
void anm2_animation_null_animation_add(Anm2Animation* animation, s32 id);
void anm2_animation_null_animation_remove(Anm2Animation* animation, s32 id);
void anm2_animation_remove(Anm2* self, s32 id);
void anm2_animation_serialize_to_string(Anm2Animation* animation, std::string* string);
void anm2_frame_bake(Anm2* self, Anm2Reference* reference, s32 interval, bool isRoundScale, bool isRoundRotation);
void anm2_frame_from_time(Anm2* self, Anm2Frame* frame, Anm2Reference reference, f32 time);
void anm2_frame_remove(Anm2* self, Anm2Reference* reference);
void anm2_frame_serialize_to_string(Anm2Frame* frame, Anm2Type type, std::string* string);
void anm2_free(Anm2* self);
void anm2_generate_from_grid(Anm2* self, Anm2Reference* reference, vec2 startPosition, vec2 size, vec2 pivot, s32 columns, s32 count, s32 delay);
void anm2_item_frame_set(Anm2* self, Anm2Reference* reference, const Anm2FrameChange& change, Anm2ChangeType type, s32 start, s32 count);
void anm2_layer_remove(Anm2* self, s32 id);
void anm2_new(Anm2* self);
void anm2_null_remove(Anm2* self, s32 id);
void anm2_reference_clear(Anm2Reference* self);
void anm2_reference_frame_clear(Anm2Reference* self);
void anm2_reference_item_clear(Anm2Reference* self);
void anm2_scale(Anm2* self, f32 scale);
void anm2_spritesheet_texture_pixels_download(Anm2* self);
void anm2_spritesheet_texture_pixels_upload(Anm2* self);