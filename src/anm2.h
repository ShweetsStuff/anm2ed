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

    void previous_frame(int max = FRAME_NUM_MAX - 1);
    void next_frame(int max = FRAME_NUM_MAX - 1);
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
    Info(tinyxml2::XMLElement* element);
    void serialize(tinyxml2::XMLDocument& document, tinyxml2::XMLElement* parent);
  };

  class Spritesheet
  {
  public:
    std::filesystem::path path{};
    texture::Texture texture;

    Spritesheet();
    Spritesheet(tinyxml2::XMLElement* element, int& id);
    Spritesheet(const std::string& directory, const std::string& path = {});
    bool save(const std::string& directory, const std::string& path = {});
    void serialize(tinyxml2::XMLDocument& document, tinyxml2::XMLElement* parent, int id);
    void reload(const std::string& directory);
    bool is_valid();
  };

  class Layer
  {
  public:
    std::string name{"New Layer"};
    int spritesheetID{};

    Layer();
    Layer(tinyxml2::XMLElement* element, int& id);
    void serialize(tinyxml2::XMLDocument& document, tinyxml2::XMLElement* parent, int id);
  };

  class Null
  {
  public:
    std::string name{"New Null"};
    bool isShowRect{};

    Null();
    Null(tinyxml2::XMLElement* element, int& id);
    void serialize(tinyxml2::XMLDocument& document, tinyxml2::XMLElement* parent, int id);
  };

  class Event
  {
  public:
    std::string name{"New Event"};

    Event();
    Event(tinyxml2::XMLElement* element, int& id);
    void serialize(tinyxml2::XMLDocument& document, tinyxml2::XMLElement* parent, int id);
  };

  struct Content
  {
    std::map<int, Spritesheet> spritesheets{};
    std::map<int, Layer> layers{};
    std::map<int, Null> nulls{};
    std::map<int, Event> events{};

    Content();

    void serialize(tinyxml2::XMLDocument& document, tinyxml2::XMLElement* parent);
    Content(tinyxml2::XMLElement* element);
    bool spritesheet_add(const std::string& directory, const std::string& path, int& id);
    void spritesheet_remove(int& id);
    std::set<int> spritesheets_unused();
    void layer_add(int& id);
    void null_add(int& id);
    void event_add(int& id);
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
  X(offset, glm::vec3, types::color::TRANSPARENT)                                                                      \
  X(tint, glm::vec4, types::color::WHITE)

  class Frame
  {
  public:
#define X(name, type, ...) type name = __VA_ARGS__;
    MEMBERS
#undef X

    Frame();
    Frame(tinyxml2::XMLElement* element, Type type);
    void serialize(tinyxml2::XMLDocument& document, tinyxml2::XMLElement* parent, Type type);
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

    Item(tinyxml2::XMLElement* element, Type type, int* id = nullptr);
    void serialize(tinyxml2::XMLDocument& document, tinyxml2::XMLElement* parent, Type type, int id = -1);
    int length(Type type);
    Frame frame_generate(float time, Type type);
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
    Animation(tinyxml2::XMLElement* element);
    Item* item_get(Type type, int id = -1);
    void serialize(tinyxml2::XMLDocument& document, tinyxml2::XMLElement* parent);
    int length();
  };

  struct Animations
  {
    std::string defaultAnimation{};
    std::vector<Animation> items{};

    Animations();

    Animations(tinyxml2::XMLElement* element);
    void serialize(tinyxml2::XMLDocument& document, tinyxml2::XMLElement* parent);
    int length();
    int merge(int target, std::set<int>& sources, types::merge::Type type, bool isDeleteAfter = true);
  };

  class Anm2
  {
    bool isValid{false};

  public:
    Info info{};
    Content content{};
    Animations animations{};

    Anm2();
    bool serialize(const std::string& path, std::string* errorString = nullptr);
    std::string to_string();
    Anm2(const std::string& path, std::string* errorString = nullptr);
    uint64_t hash();
    Animation* animation_get(Reference& reference);
    Item* item_get(Reference& reference);
    Frame* frame_get(Reference& reference);
    bool spritesheet_add(const std::string& directory, const std::string& path, int& id);
    Spritesheet* spritesheet_get(int id);
    void spritesheet_remove(int id);
    std::set<int> spritesheets_unused();
    int layer_add();
    Reference layer_add(Reference reference = REFERENCE_DEFAULT, std::string name = {}, int spritesheetID = 0,
                        types::locale::Type locale = types::locale::GLOBAL);
    Reference null_add(Reference reference = REFERENCE_DEFAULT, std::string name = {},
                       types::locale::Type locale = types::locale::GLOBAL);
    void event_add(int& id);
    std::set<int> events_unused(Reference reference = REFERENCE_DEFAULT);
    std::set<int> layers_unused(Reference reference = REFERENCE_DEFAULT);
    std::set<int> nulls_unused(Reference reference = REFERENCE_DEFAULT);
    std::vector<std::string> spritesheet_names_get();
  };
}