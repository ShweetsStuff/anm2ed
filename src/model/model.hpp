#pragma once

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "model/common.hpp"
#include "uid.hpp"

// The typed document model. XML exists only in model/xml.cpp; everything else edits and reads these structs.
namespace anm2ed::model
{
  // An XML element the editor does not understand, kept verbatim so it survives a save.
  struct XmlNode
  {
    std::string tag{};
    std::vector<std::pair<std::string, std::string>> attributes{};
    std::string text{};
    std::vector<XmlNode> children{};

    bool operator==(const XmlNode&) const = default;
  };

  // Unknown XML attached to a known node: attributes, text, and unknown children with the child index they had.
  struct Extras
  {
    std::vector<std::pair<std::string, std::string>> attributes{};
    std::string text{};
    std::vector<std::pair<int, XmlNode>> children{};

    bool operator==(const Extras&) const = default;
  };

  // A keyframe; on a triggers track the same struct is a trigger (atFrame, eventId, soundIds).
  struct Frame
  {
    std::uint64_t uid{util::uid_next()};
    glm::vec2 crop{};
    glm::vec2 size{};
    glm::vec2 pivot{};
    glm::vec2 position{};
    glm::vec2 scale{100.0f, 100.0f};
    glm::vec2 shear{};
    float rotation{};
    glm::vec4 tint{types::color::WHITE};
    glm::vec3 colorOffset{};
    int duration{1};
    bool isVisible{true};
    Interpolation interpolation{Interpolation::NONE};
    int regionId{-1};
    int shaderId{-1};
    int atFrame{-1};
    int eventId{-1};
    std::vector<int> soundIds{};
    Extras extras{};

    bool operator==(const Frame&) const = default;
  };

  struct Track
  {
    std::uint64_t uid{util::uid_next()};
    ItemType type{ItemType::LAYER};
    int id{-1};
    bool isVisible{true};
    std::vector<Frame> frames{};
    Extras extras{};

    bool operator==(const Track&) const = default;
  };

  struct TrackGroup
  {
    std::uint64_t uid{util::uid_next()};
    int id{-1};
    std::string name{};
    bool isExpanded{true};
    bool isVisible{true};
    Track root{.type = ItemType::ROOT, .frames = {Frame{}}};
    std::vector<Track> tracks{};
    Extras extras{};

    bool operator==(const TrackGroup&) const = default;
  };

  using TrackEntry = std::variant<Track, TrackGroup>;

  struct Animation
  {
    std::uint64_t uid{util::uid_next()};
    std::string name{};
    int frameNum{1};
    bool isLoop{true};
    // Playback stays between these frames (inclusive) when set; -1 for none. Saved only in the editor's copy.
    int startMarker{-1};
    int endMarker{-1};
    Track root{.type = ItemType::ROOT, .frames = {Frame{}}};
    std::vector<TrackEntry> layers{};
    std::vector<TrackEntry> nulls{};
    Track triggers{.type = ItemType::TRIGGER};
    std::vector<ElementType> layout{ElementType::ROOT_ANIMATION, ElementType::LAYER_ANIMATIONS,
                                    ElementType::NULL_ANIMATIONS, ElementType::TRIGGERS};
    Extras extras{};
    Extras layersExtras{};
    Extras nullsExtras{};

    bool operator==(const Animation&) const = default;
  };

  struct AnimationGroup
  {
    std::uint64_t uid{util::uid_next()};
    int id{-1};
    std::string name{};
    bool isExpanded{true};
    bool isVisible{true};
    std::vector<std::shared_ptr<const Animation>> animations{};
    Extras extras{};

    bool operator==(const AnimationGroup&) const = default;
  };

  using AnimationEntry = std::variant<std::shared_ptr<const Animation>, AnimationGroup>;

  struct Region
  {
    std::uint64_t uid{util::uid_next()};
    int id{-1};
    std::string name{};
    glm::vec2 crop{};
    glm::vec2 size{};
    glm::vec2 pivot{};
    Origin origin{Origin::CUSTOM};
    Extras extras{};

    bool operator==(const Region&) const = default;
  };

  struct Spritesheet
  {
    std::uint64_t uid{util::uid_next()};
    int id{-1};
    std::filesystem::path path{};
    std::vector<Region> regions{};
    Extras extras{};

    bool operator==(const Spritesheet&) const = default;
  };

  struct Component
  {
    int index{-1};
    std::string binding{};
    std::string value{};
    Extras extras{};

    bool operator==(const Component&) const = default;
  };

  struct Uniform
  {
    std::string name{};
    std::string binding{};
    std::string value{};
    std::vector<Component> components{};
    Extras extras{};

    bool operator==(const Uniform&) const = default;
  };

  struct Shader
  {
    std::uint64_t uid{util::uid_next()};
    int id{-1};
    std::string name{};
    std::filesystem::path vertex{};
    std::filesystem::path fragment{};
    std::vector<Uniform> uniforms{};
    Extras extras{};

    bool operator==(const Shader&) const = default;
  };

  struct Layer
  {
    std::uint64_t uid{util::uid_next()};
    int id{-1};
    std::string name{};
    int spritesheetId{};
    Extras extras{};

    bool operator==(const Layer&) const = default;
  };

  struct Null
  {
    std::uint64_t uid{util::uid_next()};
    int id{-1};
    std::string name{};
    bool isShowRect{};
    Extras extras{};

    bool operator==(const Null&) const = default;
  };

  struct Event
  {
    std::uint64_t uid{util::uid_next()};
    int id{-1};
    std::string name{};
    Extras extras{};

    bool operator==(const Event&) const = default;
  };

  struct Sound
  {
    std::uint64_t uid{util::uid_next()};
    int id{-1};
    std::filesystem::path path{};
    Extras extras{};

    bool operator==(const Sound&) const = default;
  };

  struct Info
  {
    std::string createdBy{"robot"};
    std::string createdOn{};
    int fps{30};
    int version{};
    Extras extras{};

    bool operator==(const Info&) const = default;
  };

  struct Content
  {
    std::vector<Spritesheet> spritesheets{};
    std::vector<Shader> shaders{};
    std::vector<Layer> layers{};
    std::vector<Null> nulls{};
    std::vector<Event> events{};
    std::vector<Sound> sounds{};
    std::vector<ElementType> layout{ElementType::SPRITESHEETS, ElementType::SHADERS, ElementType::LAYERS,
                                    ElementType::NULLS, ElementType::EVENTS};
    Extras extras{};
    std::vector<std::pair<ElementType, Extras>> containerExtras{};

    bool operator==(const Content&) const = default;
  };

  struct Animations
  {
    std::string defaultAnimation{};
    std::vector<AnimationEntry> entries{};
    Extras extras{};

    bool operator==(const Animations&) const = default;
  };

  // Content item lookup by file id (layers, nulls, events, sounds, spritesheets, shaders, regions).
  template <class Items> auto item_get(Items& items, int id) -> decltype(&items.front())
  {
    for (auto& item : items)
      if (item.id == id) return &item;
    return nullptr;
  }

  template <class Items> int item_next_id_get(const Items& items)
  {
    int id{-1};
    for (const auto& item : items)
      id = std::max(id, item.id);
    return id + 1;
  }

  // Tracks of one kind (layer or null) in order, including those inside groups; callback(track, group or nullptr).
  template <class Entries, class Callback> void tracks_each(Entries& entries, Callback&& callback)
  {
    for (auto& entry : entries)
      if (auto track = std::get_if<Track>(&entry))
        callback(*track, static_cast<decltype(&std::get<TrackGroup>(entry))>(nullptr));
      else
      {
        auto& group = std::get<TrackGroup>(entry);
        for (auto& groupTrack : group.tracks)
          callback(groupTrack, &group);
      }
  }

  template <class Entries, class Callback> void groups_each(Entries& entries, Callback&& callback)
  {
    for (auto& entry : entries)
      if (auto group = std::get_if<TrackGroup>(&entry)) callback(*group);
  }

  // Every track of an animation (root, group roots, layers, nulls, triggers).
  template <class A, class Callback> void animation_tracks_each(A& animation, Callback&& callback)
  {
    callback(animation.root);
    for (auto* entries : {&animation.layers, &animation.nulls})
      for (auto& entry : *entries)
        if (auto track = std::get_if<Track>(&entry))
          callback(*track);
        else
        {
          auto& group = std::get<TrackGroup>(entry);
          callback(group.root);
          for (auto& groupTrack : group.tracks)
            callback(groupTrack);
        }
    callback(animation.triggers);
  }

  template <class A> auto& animation_entries_get(A& animation, int itemType)
  {
    return itemType == NULL_ ? animation.nulls : animation.layers;
  }

  class Model
  {
  public:
    Info info{};
    Content content{};
    Animations animations{};
    std::vector<ElementType> layout{ElementType::INFO, ElementType::CONTENT, ElementType::ANIMATIONS};
    Extras extras{};
    bool isValid{true};

    int animations_count_get() const;
    const Animation* animation_get(int) const;
    Animation* animation_edit(int);
    int animation_group_id_get(int) const;
    const AnimationGroup* animation_group_get(int) const;
    AnimationGroup* animation_group_edit(int);
    std::vector<std::pair<int, const Animation*>> animations_get() const;

    const Track* track_get(Reference) const;
    Track* track_edit(Reference);
    const TrackGroup* track_group_get(int, int, int) const;
    TrackGroup* track_group_edit(int, int, int);
    const Frame* frame_get(Reference) const;
    Frame* frame_edit(Reference);

    const Spritesheet* layer_spritesheet_get(int) const;
    Frame frame_effective(int, const Frame&) const;
    std::set<int> unused_get(ElementType, const Animation* = nullptr) const;
    std::set<int> region_unused_get(int) const;
    std::uint64_t uid_get(Reference, bool = false) const;
    Handle handle_get(Reference, bool = false) const;
    void uids_repair();
  };

  // uid -> where that element is now (as a Reference) and what it is, built in one walk of the animations.
  class UidIndex
  {
  public:
    struct Location
    {
      Reference reference{};
      ElementType type{};
    };

  private:
    std::unordered_map<std::uint64_t, Location> locations{};

  public:
    UidIndex() = default;
    explicit UidIndex(const Model&);
    bool contains(std::uint64_t) const;
    std::optional<Reference> reference_get(std::uint64_t) const;
    ElementType type_get(std::uint64_t) const;
  };

  bool is_model_equal(const Model&, const Model&);
  int track_frames_count_get(const Track&);
  int track_length_get(const Track&);
  int animation_length_get(const Animation&);
  glm::ivec2 animation_play_range_get(const Animation&);
  const Track* animation_track_get(const Animation&, int, int, int = NONE, int = -1);
  Track* animation_track_get(Animation&, int, int, int = NONE, int = -1);
  TrackGroup* animation_track_group_get(Animation&, int, int);
  const TrackGroup* animation_track_group_get(const Animation&, int, int);
  Animation animation_clone(Animation);
  Frame frame_clone(Frame);
  Track track_clone(Track);
}
