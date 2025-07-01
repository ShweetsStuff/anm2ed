#pragma once

#include "COMMON.h"
#include "resources.h"

#define ANM2_SCALE_CONVERT(x) ((f32)x / 100.0f)
#define ANM2_TINT_CONVERT(x) ((f32)x / 255.0f)

#define ANM2_FPS_MIN 0
#define ANM2_FPS_MAX 120
#define ANM2_FRAME_NUM_MIN 1
#define ANM2_FRAME_NUM_MAX 1000000
#define ANM2_FRAME_DELAY_MIN 1
#define ANM2_STRING_MAX 0xFF

/* Elements */
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

typedef enum {
    #define X(name, str) ANM2_ELEMENT_##name,
    ANM2_ELEMENT_LIST
    #undef X
    ANM2_ELEMENT_COUNT
} Anm2Element;

static const char* ANM2_ELEMENT_STRINGS[] = {
    #define X(name, str) str,
    ANM2_ELEMENT_LIST
    #undef X
};

DEFINE_STRING_TO_ENUM_FUNCTION(ANM2_ELEMENT_STRING_TO_ENUM, Anm2Element, ANM2_ELEMENT_STRINGS, ANM2_ELEMENT_COUNT)
DEFINE_ENUM_TO_STRING_FUNCTION(ANM2_ELEMENT_ENUM_TO_STRING, ANM2_ELEMENT_STRINGS, ANM2_ELEMENT_COUNT)

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

typedef enum {
    #define X(name, str) ANM2_ATTRIBUTE_##name,
    ANM2_ATTRIBUTE_LIST
    #undef X
    ANM2_ATTRIBUTE_COUNT
} Anm2Attribute;

static const char* ANM2_ATTRIBUTE_STRINGS[] = {
    #define X(name, str) str,
    ANM2_ATTRIBUTE_LIST
    #undef X
};

DEFINE_STRING_TO_ENUM_FUNCTION(ANM2_ATTRIBUTE_STRING_TO_ENUM, Anm2Attribute, ANM2_ATTRIBUTE_STRINGS, ANM2_ATTRIBUTE_COUNT)
DEFINE_ENUM_TO_STRING_FUNCTION(ANM2_ATTRIBUTE_ENUM_TO_STRING, ANM2_ATTRIBUTE_STRINGS, ANM2_ATTRIBUTE_COUNT)

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
    std::string path;
};

struct Anm2Layer
{
    std::string name = STRING_ANM2_NEW_LAYER;
	s32 spritesheetID = -1;
};

struct Anm2Null
{
    std::string name = STRING_ANM2_NEW_NULL;
    bool isShowRect = false;
};

struct Anm2Event
{
    std::string name = STRING_ANM2_NEW_EVENT;
};

struct Anm2Frame
{
	bool isInterpolated = false;
	bool isVisible = true;
	f32 rotation = 1.0f;
	s32 delay = ANM2_FRAME_DELAY_MIN;
    s32 atFrame = -1;
    s32 eventID = -1;
	vec2 crop = {0.0f, 0.0f};
	vec2 pivot = {0.0f, 0.0f};
	vec2 position = {0.0f, 0.0f};
	vec2 size = {0.0f, 0.0f};
	vec2 scale = {100.0f, 100.0f};
	vec3 offsetRGB = {0.0f, 0.0f, 0.0f};
	vec4 tintRGBA = {1.0f, 1.0f, 1.0f, 1.0f};
};

struct Anm2Item
{
    bool isVisible = true;
	std::vector<Anm2Frame> frames;
};

struct Anm2Animation
{
	s32 frameNum = ANM2_FRAME_NUM_MIN;
    std::string name = STRING_ANM2_NEW_ANIMATION;
	bool isLoop = true;
    Anm2Item rootAnimation;
    std::map<s32, Anm2Item> layerAnimations;
    std::map<s32, Anm2Item> nullAnimations;
    Anm2Item triggers;
};

struct Anm2 
{
    std::string path;
    std::string defaultAnimation;
    std::string createdBy = STRING_ANM2_CREATED_BY_DEFAULT;
    std::string createdOn;
	std::map<s32, Anm2Spritesheet> spritesheets; 
	std::map<s32, Anm2Layer> layers; 
	std::map<s32, Anm2Null> nulls; 
	std::map<s32, Anm2Event> events; 
	std::map<s32, Anm2Animation> animations; 
    s32 fps = 30;
	s32 version = 0;
};

struct Anm2Reference
{
    s32 animationID = -1;
    Anm2Type itemType = ANM2_NONE;
    s32 itemID = -1;
    s32 frameIndex = -1;

    auto operator<=>(const Anm2Reference&) const = default; 
};

void anm2_layer_add(Anm2* self);
void anm2_layer_remove(Anm2* self, s32 id);
void anm2_null_add(Anm2* self);
void anm2_null_remove(Anm2* self, s32 id);
bool anm2_serialize(Anm2* self, const std::string& path);
bool anm2_deserialize(Anm2* self, Resources* resources, const std::string& path);
void anm2_new(Anm2* self);
void anm2_created_on_set(Anm2* self);
s32 anm2_animation_add(Anm2* self);
void anm2_animation_remove(Anm2* self, s32 id);
void anm2_spritesheet_texture_load(Anm2* self, Resources* resources, const std::string& path, s32 id);
Anm2Animation* anm2_animation_from_reference(Anm2* self, Anm2Reference* reference);
Anm2Item* anm2_item_from_reference(Anm2* self, Anm2Reference* reference);
Anm2Frame* anm2_frame_from_reference(Anm2* self, Anm2Reference* reference);
Anm2Frame* anm2_frame_add(Anm2* self, Anm2Reference* reference, s32 time);
void anm2_frame_from_time(Anm2* self, Anm2Frame* frame, Anm2Reference reference, f32 time);
void anm2_reference_clear(Anm2Reference* self);
void anm2_reference_item_clear(Anm2Reference* self);
void anm2_reference_frame_clear(Anm2Reference* self);
s32 anm2_animation_length_get(Anm2* self, s32 animationID);