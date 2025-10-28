#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <tinyxml2/tinyxml2.h>
#include <vector>

#include "texture.h"
#include "types.h"

namespace anm2ed::anm2
{
  constexpr auto FRAME_NUM_MIN = 1;
  constexpr auto FRAME_NUM_MAX = 100000000;
  constexpr auto FRAME_DELAY_MIN = 1;
  constexpr auto FRAME_DELAY_MAX = FRAME_NUM_MAX;
  constexpr auto FPS_MIN = 1;
  constexpr auto FPS_MAX = 120;

  constexpr auto MERGED_STRING = "(Merged)";

  constexpr auto NO_PATH = "(No Path)";
  constexpr auto LAYER_FORMAT = "#{} {} (Spritesheet: #{})";
  constexpr auto NULL_FORMAT = "#{} {}";
  constexpr auto SPRITESHEET_FORMAT = "#%d %s";

  enum Type
  {
    NONE,
    ROOT,
    LAYER,
    NULL_,
    TRIGGER
  };

  class Reference
  {
  public:
    int animationIndex{-1};
    Type itemType{NONE};
    int itemID{-1};
    int frameIndex{-1};
    int frameTime{-1};

    auto operator<=>(const Reference&) const = default;
  };

  constexpr anm2::Reference REFERENCE_DEFAULT = {-1, anm2::NONE, -1, -1, -1};

  class Info
  {
  public:
    std::string createdBy{"robot"};
    std::string createdOn{};
    int fps = 30;
    int version{};

    Info();
    Info(tinyxml2::XMLElement*);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*);
  };

  class Spritesheet
  {
  public:
    std::filesystem::path path{};
    texture::Texture texture;

    Spritesheet();
    Spritesheet(tinyxml2::XMLElement*, int&);
    Spritesheet(const std::string&, const std::string& = {});
    bool save(const std::string&, const std::string& = {});
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, int);
    void reload(const std::string&);
    bool is_valid();
    std::string to_string(int id);
  };

  class Layer
  {
  public:
    std::string name{"New Layer"};
    int spritesheetID{};

    Layer();
    Layer(tinyxml2::XMLElement*, int&);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, int);
    std::string to_string(int);
  };

  class Null
  {
  public:
    std::string name{"New Null"};
    bool isShowRect{};

    Null();
    Null(tinyxml2::XMLElement*, int&);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, int);
    std::string to_string(int);
  };

  class Event
  {
  public:
    std::string name{"New Event"};

    Event();
    Event(tinyxml2::XMLElement*, int&);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, int);
    std::string to_string(int);
  };

  struct Content
  {
    std::map<int, Spritesheet> spritesheets{};
    std::map<int, Layer> layers{};
    std::map<int, Null> nulls{};
    std::map<int, Event> events{};

    Content();

    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*);
    Content(tinyxml2::XMLElement*);
    bool spritesheet_add(const std::string&, const std::string&, int&);
    void spritesheet_remove(int&);
    std::set<int> spritesheets_unused();
    void layer_add(int&);
    void null_add(int&);
    void event_add(int&);
    bool spritesheets_deserialize(const std::string&, const std::string&, types::merge::Type, std::string* = nullptr);
    bool layers_deserialize(const std::string&, types::merge::Type, std::string* = nullptr);
    bool nulls_deserialize(const std::string&, types::merge::Type, std::string* = nullptr);
    bool events_deserialize(const std::string&, types::merge::Type, std::string* = nullptr);
  };

#define MEMBERS                                                                                                        \
  X(isVisible, bool, true)                                                                                             \
  X(isInterpolated, bool, false)                                                                                       \
  X(rotation, float, 0.0f)                                                                                             \
  X(delay, int, FRAME_DELAY_MIN)                                                                                       \
  X(atFrame, int, -1)                                                                                                  \
  X(eventID, int, -1)                                                                                                  \
  X(pivot, glm::vec2, {})                                                                                              \
  X(crop, glm::vec2, {})                                                                                               \
  X(position, glm::vec2, {})                                                                                           \
  X(size, glm::vec2, {})                                                                                               \
  X(scale, glm::vec2, glm::vec2(100.0f))                                                                               \
  X(colorOffset, glm::vec3, types::color::TRANSPARENT)                                                                 \
  X(tint, glm::vec4, types::color::WHITE)

  class Frame
  {
  public:
#define X(name, type, ...) type name = __VA_ARGS__;
    MEMBERS
#undef X

    Frame();
    Frame(tinyxml2::XMLElement*, Type);
    std::string to_string(Type type);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, Type);
    void shorten();
    void extend();
  };

  struct FrameChange
  {
#define X(name, type, ...) std::optional<type> name{};
    MEMBERS
#undef X
  };

#undef MEMBERS

  class Item
  {
  public:
    std::vector<Frame> frames{};
    bool isVisible{true};

    Item();

    Item(tinyxml2::XMLElement*, Type, int* = nullptr);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*, Type, int = -1);
    int length(Type);
    Frame frame_generate(float, Type);
    void frames_change(anm2::FrameChange&, types::frame_change::Type, int, int = 0);
    bool frames_deserialize(const std::string&, Type, int, std::set<int>&, std::string*);
  };

  class Animation
  {
  public:
    std::string name{"New Animation"};
    int frameNum{FRAME_NUM_MIN};
    bool isLoop{true};
    Item rootAnimation;
    std::unordered_map<int, Item> layerAnimations{};
    std::vector<int> layerOrder{};
    std::map<int, Item> nullAnimations{};
    Item triggers;

    Animation();
    Animation(tinyxml2::XMLElement*);
    Item* item_get(Type, int = -1);
    void item_remove(Type, int = -1);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*);
    int length();
    glm::vec4 rect(bool);
    std::string to_string();
  };

  struct Animations
  {
    std::string defaultAnimation{};
    std::vector<Animation> items{};

    Animations();

    Animations(tinyxml2::XMLElement*);
    void serialize(tinyxml2::XMLDocument&, tinyxml2::XMLElement*);
    int length();
    int merge(int, std::set<int>&, types::merge::Type = types::merge::APPEND, bool = true);
    bool animations_deserialize(const std::string&, int, std::set<int>&, std::string* = nullptr);
  };

  class Anm2
  {
  public:
    Info info{};
    Content content{};
    Animations animations{};

    Anm2();
    bool serialize(const std::string&, std::string* = nullptr);
    std::string to_string();
    Anm2(const std::string&, std::string* = nullptr);
    uint64_t hash();
    Animation* animation_get(Reference);
    Item* item_get(Reference);
    Frame* frame_get(Reference);
    bool spritesheet_add(const std::string&, const std::string&, int&);
    Spritesheet* spritesheet_get(int);
    void spritesheet_remove(int);
    std::set<int> spritesheets_unused();
    int layer_add();
    Reference layer_add(Reference = REFERENCE_DEFAULT, std::string = {}, int = 0,
                        types::locale::Type = types::locale::GLOBAL);
    Reference null_add(Reference = REFERENCE_DEFAULT, std::string = {}, types::locale::Type = types::locale::GLOBAL);
    void event_add(int&);
    std::set<int> events_unused(Reference = REFERENCE_DEFAULT);
    std::set<int> layers_unused(Reference = REFERENCE_DEFAULT);
    std::set<int> nulls_unused(Reference = REFERENCE_DEFAULT);
    std::vector<std::string> spritesheet_names_get();
    void bake(Reference, int = 1, bool = true, bool = true);
    void generate_from_grid(Reference, glm::ivec2, glm::ivec2, glm::ivec2, int, int, int);
  };
}
