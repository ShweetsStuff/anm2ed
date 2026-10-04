#include "xml.hpp"

#include <algorithm>
#include <charconv>
#include <format>
#include <functional>
#include <map>
#include <optional>
#include <tuple>

#include <tinyxml2/tinyxml2.h>

#include "file.hpp"
#include "frames.hpp"
#include "math.hpp"
#include "path.hpp"
#include "time.hpp"

using namespace tinyxml2;
using namespace anm2ed::util;

namespace anm2ed::model
{
  constexpr std::string_view ELEMENT_TAGS[] = {
#define X(symbol, tag) tag,
      ANM2_ELEMENT_TYPES
#undef X
  };
  constexpr std::string_view INTERPOLATION_VALUES[] = {"", "", "EaseIn", "EaseOut", "EaseInOut"};
  constexpr std::string_view ORIGIN_VALUES[] = {"", "TopLeft", "Center"};
  constexpr const char* TINT_ATTRIBUTES[] = {"RedTint", "GreenTint", "BlueTint", "AlphaTint"};
  constexpr const char* OFFSET_ATTRIBUTES[] = {"RedOffset", "GreenOffset", "BlueOffset"};
  // Attributes the editor reads (or older builds wrote); any other attribute is kept verbatim as Extras.
  // clang-format off
  constexpr std::string_view KNOWN_ATTRIBUTES[] = {
      "Name", "CreatedBy", "CreatedOn", "Binding", "Value", "DefaultAnimation", "Path", "Vertex", "Fragment", "Id",
      "LayerId", "NullId", "SpritesheetId", "Fps", "Version", "FrameNum", "Delay", "AtFrame", "EventId", "RegionId",
      "ShaderId", "SoundId", "GroupId", "Index", "Loop", "StartMarker", "EndMarker", "Visible", "ShowRect", "IsExpanded", "Enabled", "Rotation",
      "XPivot", "YPivot", "XCrop", "YCrop", "XPosition", "YPosition", "Width", "Height", "XScale", "YScale", "ShearX",
      "ShearY", "RedTint", "GreenTint", "BlueTint", "AlphaTint", "RedOffset", "GreenOffset", "BlueOffset",
      "Interpolated", "Origin", "BakeInterpolation", "BakeDelay", "BakeCount", "OriginalDelay"};
  // clang-format on
  constexpr std::string_view SOURCE_DOCUMENT_TAG = "SourceDocument";
  constexpr std::string_view FRAME_RESTORE_TAG = "FrameRestore";
  constexpr std::string_view LEGACY_OVERLAY_TAG = "Overlay";
  constexpr std::string_view LEGACY_GROUPS_TAG = "Groups";
  constexpr const char* CREATED_ON_FORMAT = "%m/%d/%Y %I:%M:%S %p";
  constexpr const char* SHADER_NAME_DEFAULT = "New Shader";
  constexpr std::size_t VALUE_BUFFER_SIZE = 64;
  constexpr int FLOAT_DIGITS = 8;
  constexpr int DOUBLE_DIGITS = 17;
  constexpr double FLOAT_WHOLE_MAX = 1e8;
  constexpr std::size_t FRAME_ATTRIBUTES_MAX = 27;
  constexpr std::string_view INDENT = "    ";
  constexpr int TEXT_ENTITY_COUNT = 3;
  constexpr std::pair<char, std::string_view> ENTITIES[] = {
      {'&', "&amp;"}, {'<', "&lt;"}, {'>', "&gt;"}, {'"', "&quot;"}, {'\'', "&apos;"}};
  constexpr std::uint64_t HASH_OFFSET = 14695981039346656037ull;
  constexpr std::uint64_t HASH_PRIME = 1099511628211ull;
  constexpr Flags CLIPBOARD_FLAGS = SERIALIZE_EDITOR_DEFAULT;

  const char* tag_get(ElementType type) { return ELEMENT_TAGS[(int)type].data(); }

  ElementType type_get(const XMLElement* element)
  {
    auto it = std::ranges::find(ELEMENT_TAGS, std::string_view(element->Name()));
    return it == std::end(ELEMENT_TAGS) ? ElementType::UNKNOWN : (ElementType)(it - std::begin(ELEMENT_TAGS));
  }

  ElementType track_element_type_get(ItemType type)
  {
    switch (type)
    {
      case ItemType::ROOT:
        return ElementType::ROOT_ANIMATION;
      case ItemType::NULL_:
        return ElementType::NULL_ANIMATION;
      case ItemType::TRIGGER:
        return ElementType::TRIGGERS;
      default:
        return ElementType::LAYER_ANIMATION;
    }
  }

  // Reading: tinyxml2 straight into the model.

  int int_get(const XMLElement* element, const char* name, int value = -1)
  {
    element->QueryIntAttribute(name, &value);
    return value;
  }

  float float_get(const XMLElement* element, const char* name, float value = 0.0f)
  {
    element->QueryFloatAttribute(name, &value);
    return value;
  }

  bool bool_get(const XMLElement* element, const char* name, bool value)
  {
    element->QueryBoolAttribute(name, &value);
    return value;
  }

  std::string string_get(const XMLElement* element, const char* name, std::string value = {})
  {
    if (auto attribute = element->Attribute(name)) value = attribute;
    return value;
  }

  std::filesystem::path path_get(const XMLElement* element, const char* name)
  {
    auto attribute = element->Attribute(name);
    return attribute ? path::from_utf8(attribute) : std::filesystem::path{};
  }

  float color_get(const XMLElement* element, const char* name, float value)
  {
    int color{};
    if (element->QueryIntAttribute(name, &color) == XML_SUCCESS) value = math::uint8_to_float(color);
    return value;
  }

  Interpolation interpolation_read(const XMLElement* element, const char* name)
  {
    auto value = element->Attribute(name);
    if (!value) return Interpolation::NONE;
    for (std::size_t i = 0; i < std::size(INTERPOLATION_VALUES); ++i)
      if (!INTERPOLATION_VALUES[i].empty() && INTERPOLATION_VALUES[i] == value) return (Interpolation)i;
    return bool_get(element, name, false) ? Interpolation::LINEAR : Interpolation::NONE;
  }

  // Shader elements are only read where they belong; one anywhere else is dropped.
  bool is_child_kept(const XMLNode* parent, const XMLElement* child)
  {
    std::string_view tag = child->Name();
    if (tag == LEGACY_OVERLAY_TAG || tag == SOURCE_DOCUMENT_TAG) return false;
    auto parentElement = parent->ToElement();
    auto parentType = parentElement ? type_get(parentElement) : ElementType::UNKNOWN;
    switch (type_get(child))
    {
      case ElementType::SHADERS:
        return parentType == ElementType::CONTENT;
      case ElementType::SHADER:
        return parentType == ElementType::SHADERS;
      case ElementType::UNIFORM:
        return parentType == ElementType::SHADER;
      case ElementType::COMPONENT:
        return parentType == ElementType::UNIFORM;
      default:
        return true;
    }
  }

  // Child elements with their position, skipping legacy Overlay, nested SourceDocument copies and misplaced shaders.
  template <class Callback> void children_each(const XMLNode* parent, Callback&& callback)
  {
    int position{};
    for (auto child = parent->FirstChildElement(); child; child = child->NextSiblingElement())
      if (is_child_kept(parent, child)) callback(child, type_get(child), position++);
  }

  // A repeated attribute keeps its first position and last value, as tinyxml2 writes it.
  void attribute_read(std::vector<std::pair<std::string, std::string>>& attributes, const XMLAttribute* attribute)
  {
    auto existing = std::ranges::find(attributes, std::string_view(attribute->Name()),
                                      [](const auto& pair) { return std::string_view(pair.first); });
    if (existing != attributes.end())
      existing->second = attribute->Value();
    else
      attributes.emplace_back(attribute->Name(), attribute->Value());
  }

  XmlNode node_read(const XMLElement* element)
  {
    XmlNode node{.tag = element->Name()};
    for (auto attribute = element->FirstAttribute(); attribute; attribute = attribute->Next())
      attribute_read(node.attributes, attribute);
    if (auto text = element->GetText()) node.text = text;
    children_each(element,
                  [&](const XMLElement* child, ElementType, int) { node.children.push_back(node_read(child)); });
    return node;
  }

  Extras extras_read(const XMLElement* element)
  {
    Extras extras{};
    for (auto attribute = element->FirstAttribute(); attribute; attribute = attribute->Next())
      if (!std::ranges::contains(KNOWN_ATTRIBUTES, std::string_view(attribute->Name())))
        attribute_read(extras.attributes, attribute);
    if (auto text = element->GetText()) extras.text = text;
    return extras;
  }

  void extras_child_add(Extras& extras, const XMLElement* child, int position)
  {
    extras.children.emplace_back(position, node_read(child));
  }

  Frame frame_read(const XMLElement* element)
  {
    Frame frame{};
    frame.crop = {float_get(element, "XCrop"), float_get(element, "YCrop")};
    frame.size = {float_get(element, "Width"), float_get(element, "Height")};
    frame.pivot = {float_get(element, "XPivot"), float_get(element, "YPivot")};
    frame.position = {float_get(element, "XPosition"), float_get(element, "YPosition")};
    frame.scale = {float_get(element, "XScale", frame.scale.x), float_get(element, "YScale", frame.scale.y)};
    frame.shear = {float_get(element, "ShearX"), float_get(element, "ShearY")};
    frame.rotation = float_get(element, "Rotation");
    for (int i = 0; i < (int)std::size(TINT_ATTRIBUTES); ++i)
      frame.tint[i] = color_get(element, TINT_ATTRIBUTES[i], frame.tint[i]);
    for (int i = 0; i < (int)std::size(OFFSET_ATTRIBUTES); ++i)
      frame.colorOffset[i] = color_get(element, OFFSET_ATTRIBUTES[i], frame.colorOffset[i]);
    frame.duration = int_get(element, "Delay", frame.duration);
    frame.isVisible = bool_get(element, "Visible", frame.isVisible);
    frame.interpolation = interpolation_read(element, "Interpolated");
    frame.regionId = int_get(element, "RegionId");
    frame.shaderId = int_get(element, "ShaderId");
    frame.atFrame = int_get(element, "AtFrame");
    frame.eventId = int_get(element, "EventId");
    frame.extras = extras_read(element);
    if (type_get(element) != ElementType::TRIGGER) return frame;

    // A trigger's sounds are <Sound Id> children; older files name one with a SoundId attribute instead.
    auto soundId = int_get(element, "SoundId");
    std::vector<int> soundIds{};
    children_each(element,
                  [&](const XMLElement* child, ElementType type, int)
                  {
                    if (type == ElementType::SOUND_ELEMENT) soundIds.push_back(int_get(child, "Id"));
                  });
    if (soundId != -1 && !std::ranges::contains(soundIds, soundId)) soundIds.insert(soundIds.begin(), soundId);
    std::ranges::copy_if(soundIds, std::back_inserter(frame.soundIds), [](int id) { return id != -1; });
    return frame;
  }

  Track track_read(const XMLElement* element, ItemType type)
  {
    Track track{.type = type, .isVisible = bool_get(element, "Visible", true), .extras = extras_read(element)};
    if (type == ItemType::LAYER) track.id = int_get(element, "LayerId");
    if (type == ItemType::NULL_) track.id = int_get(element, "NullId");
    auto frameType = type == ItemType::TRIGGER ? ElementType::TRIGGER : ElementType::FRAME;
    for (auto child = element->FirstChildElement(); child;)
    {
      auto childType = type_get(child);
      // A frame written baked (BakeInterpolation/BakeDelay) is read back as the one interpolated frame it came from.
      if (auto delay = int_get(child, "BakeDelay", 0);
          childType == ElementType::FRAME && child->Attribute("BakeInterpolation") && delay >= FRAME_DURATION_MIN)
      {
        auto frame = frame_read(child);
        frame.interpolation = interpolation_read(child, "BakeInterpolation");
        frame.duration = delay;
        if (frameType == ElementType::FRAME) track.frames.push_back(std::move(frame));
        for (int i = 0; child && i < delay; ++i)
          child = child->NextSiblingElement();
        continue;
      }
      if (childType == frameType) track.frames.push_back(frame_read(child));
      child = child->NextSiblingElement();
    }
    return track;
  }

  // A container's children before groups are resolved: a child (with the group it names), a group (with any children
  // nested in it), or unknown XML.
  template <class Child, class GroupType> struct ContainerItem
  {
    using Group = GroupType;

    std::optional<Child> child{};
    std::optional<Group> group{};
    std::optional<XmlNode> node{};
    int groupId{-1};
    int index{-1};
    std::vector<Child> children{};
    std::optional<Track> root{};
    std::optional<Track> backupRoot{};
    std::vector<Track> backupTracks{};
    bool isBackup{};
  };

  using TrackItem = ContainerItem<Track, TrackGroup>;
  using AnimationItem = ContainerItem<std::shared_ptr<const Animation>, AnimationGroup>;

  auto& group_children_get(TrackGroup& group) { return group.tracks; }
  auto& group_children_get(AnimationGroup& group) { return group.animations; }

  // Groups become entries followed by their children: nested children move out after their group, and a child joins a
  // group seen before it. Unknown XML becomes extras at its position.
  template <class Entry, class Item> std::vector<Entry> entries_build(std::vector<Item> items, Extras& extras)
  {
    int nextGroupId{};
    for (const auto& item : items)
      if (item.group) nextGroupId = std::max(nextGroupId, item.group->id + 1);

    std::vector<Item> flat{};
    for (auto& item : items)
    {
      if (!item.group)
      {
        flat.push_back(std::move(item));
        continue;
      }
      if (item.group->id < 0) item.group->id = nextGroupId++;
      auto groupId = item.group->id;
      auto children = std::move(item.children);
      flat.push_back(std::move(item));
      for (auto& child : children)
        flat.push_back({.child = std::move(child), .groupId = groupId});
    }

    std::vector<Entry> entries{};
    std::map<int, std::size_t> groupEntries{};
    for (int i = 0; i < (int)flat.size(); ++i)
    {
      auto& item = flat[i];
      if (item.node)
        extras.children.emplace_back(i, std::move(*item.node));
      else if (item.group)
      {
        groupEntries[item.group->id] = entries.size();
        entries.emplace_back(std::move(*item.group));
      }
      else if (auto group = groupEntries.find(item.groupId); group != groupEntries.end())
        group_children_get(std::get<typename Item::Group>(entries[group->second])).push_back(std::move(*item.child));
      else
        entries.emplace_back(std::move(*item.child));
    }
    return entries;
  }

  TrackItem track_group_item_read(const XMLElement* element, ItemType type)
  {
    TrackItem item{.group = TrackGroup{.id = int_get(element, "Id"),
                                       .name = string_get(element, "Name"),
                                       .isExpanded = bool_get(element, "IsExpanded", true),
                                       .isVisible = bool_get(element, "Visible", true),
                                       .extras = extras_read(element)},
                   .index = int_get(element, "Index")};
    auto trackType = track_element_type_get(type);
    children_each(element,
                  [&](const XMLElement* child, ElementType childType, int)
                  {
                    if (childType == ElementType::ROOT_ANIMATION && !item.root)
                      item.root = track_read(child, ItemType::ROOT);
                    else if (childType == trackType)
                      item.children.push_back(track_read(child, type));
                    else if (child->Name() == FRAME_RESTORE_TAG && !item.isBackup)
                    {
                      item.isBackup = true;
                      children_each(child,
                                    [&](const XMLElement* backup, ElementType backupType, int)
                                    {
                                      if (backupType == ElementType::ROOT_ANIMATION && !item.backupRoot)
                                        item.backupRoot = track_read(backup, ItemType::ROOT);
                                      else if (backupType == trackType)
                                        item.backupTracks.push_back(track_read(backup, type));
                                    });
                    }
                  });
    return item;
  }

  std::vector<TrackItem> track_items_read(const XMLElement* container, ItemType type)
  {
    std::vector<TrackItem> items{};
    children_each(container,
                  [&](const XMLElement* child, ElementType childType, int)
                  {
                    if (childType == ElementType::GROUP)
                      items.push_back(track_group_item_read(child, type));
                    else if (childType == track_element_type_get(type))
                      items.push_back({.child = track_read(child, type), .groupId = int_get(child, "GroupId")});
                    else
                      items.push_back({.node = node_read(child)});
                  });
    return items;
  }

  // Track groups as any version wrote them: legacy *AnimationGroups metadata is placed back at its Index, and a group
  // whose tracks were written baked (FrameRestore) gets its original root and tracks back.
  std::vector<TrackEntry> track_entries_build(std::vector<TrackItem> items, std::vector<TrackItem> legacyGroups,
                                              Extras& extras)
  {
    if (!legacyGroups.empty())
    {
      std::set<int> legacyIds{};
      for (const auto& item : legacyGroups)
        if (item.group->id >= 0) legacyIds.insert(item.group->id);
      std::erase_if(items, [&](const TrackItem& item) { return item.group && legacyIds.contains(item.group->id); });
      std::ranges::stable_sort(legacyGroups,
                               [](const TrackItem& left, const TrackItem& right)
                               {
                                 if (left.index == -1 || right.index == -1) return left.index != -1;
                                 return left.index < right.index;
                               });
      for (auto& legacy : legacyGroups)
      {
        auto first = std::ranges::find_if(items, [&](const TrackItem& item)
                                          { return item.child && item.groupId == legacy.group->id; });
        auto position = legacy.index >= 0 ? std::min(legacy.index, (int)items.size()) : (int)(first - items.begin());
        items.insert(items.begin() + position, std::move(legacy));
      }
    }

    std::set<int> restoredIds{};
    for (auto& item : items)
      if (item.group && item.group->id >= 0 && item.isBackup && (item.backupRoot || !item.backupTracks.empty()))
      {
        if (item.backupRoot) item.root = item.backupRoot;
        item.children = std::move(item.backupTracks);
        restoredIds.insert(item.group->id);
      }
    std::erase_if(items, [&](const TrackItem& item) { return item.child && restoredIds.contains(item.groupId); });

    for (auto& item : items)
      if (item.group) item.group->root = item.root.value_or(TrackGroup{}.root);
    return entries_build<TrackEntry>(std::move(items), extras);
  }

  Animation animation_read(const XMLElement* element)
  {
    Animation animation{.name = string_get(element, "Name"),
                        .frameNum = int_get(element, "FrameNum", 1),
                        .isLoop = bool_get(element, "Loop", true),
                        .startMarker = int_get(element, "StartMarker"),
                        .endMarker = int_get(element, "EndMarker"),
                        .root = {.type = ItemType::ROOT},
                        .layout = {},
                        .extras = extras_read(element)};
    std::map<ElementType, std::vector<TrackItem>> items{};
    std::map<ElementType, std::vector<TrackItem>> legacyGroups{};
    std::set<ElementType> legacyRead{};
    int skipped{};
    children_each(element,
                  [&](const XMLElement* child, ElementType type, int position)
                  {
                    if (type == ElementType::LAYER_ANIMATION_GROUPS || type == ElementType::NULL_ANIMATION_GROUPS)
                    {
                      auto isLayers = type == ElementType::LAYER_ANIMATION_GROUPS;
                      auto container = isLayers ? ElementType::LAYER_ANIMATIONS : ElementType::NULL_ANIMATIONS;
                      if (legacyRead.insert(type).second)
                        children_each(child,
                                      [&](const XMLElement* group, ElementType groupType, int)
                                      {
                                        if (groupType == ElementType::GROUP)
                                          legacyGroups[container].push_back(track_group_item_read(
                                              group, isLayers ? ItemType::LAYER : ItemType::NULL_));
                                      });
                      ++skipped;
                      return;
                    }

                    auto isKnown = type == ElementType::ROOT_ANIMATION || type == ElementType::LAYER_ANIMATIONS ||
                                   type == ElementType::NULL_ANIMATIONS || type == ElementType::TRIGGERS;
                    if (!isKnown || std::ranges::contains(animation.layout, type))
                      return extras_child_add(animation.extras, child, position - skipped);

                    animation.layout.push_back(type);
                    if (type == ElementType::ROOT_ANIMATION) animation.root = track_read(child, ItemType::ROOT);
                    if (type == ElementType::TRIGGERS) animation.triggers = track_read(child, ItemType::TRIGGER);
                    if (type == ElementType::LAYER_ANIMATIONS)
                    {
                      items[type] = track_items_read(child, ItemType::LAYER);
                      animation.layersExtras = extras_read(child);
                    }
                    if (type == ElementType::NULL_ANIMATIONS)
                    {
                      items[type] = track_items_read(child, ItemType::NULL_);
                      animation.nullsExtras = extras_read(child);
                    }
                  });

    for (auto [type, entries, extras] :
         {std::tuple{ElementType::LAYER_ANIMATIONS, &animation.layers, &animation.layersExtras},
          std::tuple{ElementType::NULL_ANIMATIONS, &animation.nulls, &animation.nullsExtras}})
    {
      if (!legacyGroups[type].empty() && !std::ranges::contains(animation.layout, type))
        animation.layout.push_back(type);
      if (std::ranges::contains(animation.layout, type))
        *entries = track_entries_build(std::move(items[type]), std::move(legacyGroups[type]), *extras);
    }
    return animation;
  }

  std::vector<AnimationEntry> animation_entries_read(const XMLNode* parent, Extras& extras)
  {
    std::vector<AnimationItem> items{};
    children_each(parent,
                  [&](const XMLElement* child, ElementType type, int)
                  {
                    if (type == ElementType::GROUP)
                    {
                      AnimationItem item{.group = AnimationGroup{.id = int_get(child, "Id"),
                                                                 .name = string_get(child, "Name"),
                                                                 .isExpanded = bool_get(child, "IsExpanded", true),
                                                                 .isVisible = bool_get(child, "Visible", true),
                                                                 .extras = extras_read(child)}};
                      children_each(child,
                                    [&](const XMLElement* animation, ElementType animationType, int)
                                    {
                                      if (animationType == ElementType::ANIMATION)
                                        item.children.push_back(
                                            std::make_shared<const Animation>(animation_read(animation)));
                                    });
                      items.push_back(std::move(item));
                    }
                    else if (type == ElementType::ANIMATION)
                      items.push_back({.child = std::make_shared<const Animation>(animation_read(child)),
                                       .groupId = int_get(child, "GroupId")});
                    else
                      items.push_back({.node = node_read(child)});
                  });
    return entries_build<AnimationEntry>(std::move(items), extras);
  }

  Region region_read(const XMLElement* element)
  {
    Region region{.id = int_get(element, "Id"),
                  .name = string_get(element, "Name"),
                  .crop = {float_get(element, "XCrop"), float_get(element, "YCrop")},
                  .size = {float_get(element, "Width"), float_get(element, "Height")},
                  .pivot = {float_get(element, "XPivot"), float_get(element, "YPivot")},
                  .extras = extras_read(element)};
    if (auto origin = element->Attribute("Origin"))
      for (std::size_t i = 0; i < std::size(ORIGIN_VALUES); ++i)
        if (!ORIGIN_VALUES[i].empty() && ORIGIN_VALUES[i] == origin) region.origin = (Origin)i;
    if (region.origin == Origin::TOP_LEFT) region.pivot = {};
    if (region.origin == Origin::CENTER) region.pivot = region.size * 0.5f;
    return region;
  }

  Spritesheet spritesheet_read(const XMLElement* element)
  {
    Spritesheet spritesheet{
        .id = int_get(element, "Id"), .path = path_get(element, "Path"), .extras = extras_read(element)};
    children_each(element,
                  [&](const XMLElement* child, ElementType type, int position)
                  {
                    if (type == ElementType::REGION)
                      spritesheet.regions.push_back(region_read(child));
                    else
                      extras_child_add(spritesheet.extras, child, position);
                  });
    return spritesheet;
  }

  Shader shader_read(const XMLElement* element)
  {
    Shader shader{.id = int_get(element, "Id"),
                  .name = string_get(element, "Name"),
                  .vertex = path_get(element, "Vertex"),
                  .fragment = path_get(element, "Fragment"),
                  .extras = extras_read(element)};
    children_each(element,
                  [&](const XMLElement* child, ElementType type, int position)
                  {
                    if (type != ElementType::UNIFORM) return extras_child_add(shader.extras, child, position);
                    Uniform uniform{.name = string_get(child, "Name"),
                                    .binding = string_get(child, "Binding"),
                                    .value = string_get(child, "Value"),
                                    .extras = extras_read(child)};
                    children_each(child,
                                  [&](const XMLElement* component, ElementType componentType, int componentPosition)
                                  {
                                    if (componentType != ElementType::COMPONENT)
                                      return extras_child_add(uniform.extras, component, componentPosition);
                                    uniform.components.push_back({.index = int_get(component, "Index"),
                                                                  .binding = string_get(component, "Binding"),
                                                                  .value = string_get(component, "Value"),
                                                                  .extras = extras_read(component)});
                                  });
                    shader.uniforms.push_back(std::move(uniform));
                  });
    return shader;
  }

  template <class Item> Item item_read(const XMLElement*);
  template <> Spritesheet item_read(const XMLElement* element) { return spritesheet_read(element); }
  template <> Region item_read(const XMLElement* element) { return region_read(element); }
  template <> Shader item_read(const XMLElement* element) { return shader_read(element); }
  template <> Layer item_read(const XMLElement* element)
  {
    return {.id = int_get(element, "Id"),
            .name = string_get(element, "Name"),
            .spritesheetId = int_get(element, "SpritesheetId", 0),
            .extras = extras_read(element)};
  }
  template <> Null item_read(const XMLElement* element)
  {
    return {.id = int_get(element, "Id"),
            .name = string_get(element, "Name"),
            .isShowRect = bool_get(element, "ShowRect", false),
            .extras = extras_read(element)};
  }
  template <> Event item_read(const XMLElement* element)
  {
    return {.id = int_get(element, "Id"), .name = string_get(element, "Name"), .extras = extras_read(element)};
  }
  template <> Sound item_read(const XMLElement* element)
  {
    return {.id = int_get(element, "Id"), .path = path_get(element, "Path"), .extras = extras_read(element)};
  }

  template <class Item> constexpr ElementType ITEM_TYPE{};
  template <> constexpr ElementType ITEM_TYPE<Spritesheet>{ElementType::SPRITESHEET};
  template <> constexpr ElementType ITEM_TYPE<Region>{ElementType::REGION};
  template <> constexpr ElementType ITEM_TYPE<Shader>{ElementType::SHADER};
  template <> constexpr ElementType ITEM_TYPE<Layer>{ElementType::LAYER_ELEMENT};
  template <> constexpr ElementType ITEM_TYPE<Null>{ElementType::NULL_ELEMENT};
  template <> constexpr ElementType ITEM_TYPE<Event>{ElementType::EVENT_ELEMENT};
  template <> constexpr ElementType ITEM_TYPE<Sound>{ElementType::SOUND_ELEMENT};

  template <class Item> std::vector<Item> items_read(const XMLElement* container, Content& content, ElementType type)
  {
    std::vector<Item> items{};
    auto extras = extras_read(container);
    children_each(container,
                  [&](const XMLElement* child, ElementType childType, int position)
                  {
                    if (childType == ITEM_TYPE<Item>)
                      items.push_back(item_read<Item>(child));
                    else
                      extras_child_add(extras, child, position);
                  });
    if (extras != Extras{}) content.containerExtras.emplace_back(type, std::move(extras));
    return items;
  }

  Content content_read(const XMLElement* element)
  {
    Content content{.layout = {}, .extras = extras_read(element)};
    children_each(element,
                  [&](const XMLElement* child, ElementType type, int position)
                  {
                    if (child->Name() == LEGACY_GROUPS_TAG) return;
                    auto isKnown = type == ElementType::SPRITESHEETS || type == ElementType::SHADERS ||
                                   type == ElementType::LAYERS || type == ElementType::NULLS ||
                                   type == ElementType::EVENTS || type == ElementType::SOUNDS;
                    if (!isKnown || std::ranges::contains(content.layout, type))
                      return extras_child_add(content.extras, child, position);
                    content.layout.push_back(type);
                    if (type == ElementType::SPRITESHEETS)
                      content.spritesheets = items_read<Spritesheet>(child, content, type);
                    if (type == ElementType::SHADERS) content.shaders = items_read<Shader>(child, content, type);
                    if (type == ElementType::LAYERS) content.layers = items_read<Layer>(child, content, type);
                    if (type == ElementType::NULLS) content.nulls = items_read<Null>(child, content, type);
                    if (type == ElementType::EVENTS) content.events = items_read<Event>(child, content, type);
                    if (type == ElementType::SOUNDS) content.sounds = items_read<Sound>(child, content, type);
                  });
    return content;
  }

  // Gives every item a unique, non-negative id; the first holder of a duplicated id keeps it.
  template <class Items> void ids_repair(Items& items)
  {
    std::set<int> usedIds{};
    for (const auto& item : items)
      if (item.id >= 0) usedIds.insert(item.id);
    std::set<int> keptIds{};
    int nextId{};
    for (auto& item : items)
    {
      if (item.id >= 0 && keptIds.insert(item.id).second) continue;
      while (usedIds.contains(nextId))
        ++nextId;
      item.id = nextId;
      usedIds.insert(nextId);
      keptIds.insert(nextId);
    }
  }

  bool is_region_matched(const Region& region, const Frame& frame)
  {
    return glm::ivec2(region.crop) == glm::ivec2(frame.crop) && glm::ivec2(region.size) == glm::ivec2(frame.size) &&
           glm::ivec2(region.pivot) == glm::ivec2(frame.pivot);
  }

  // Loaded documents get unique content ids, frames lose dangling shader ids, and layer frames point at the region
  // matching their rectangle and take that region's rectangle.
  void model_repair(Model& model)
  {
    auto& content = model.content;
    ids_repair(content.spritesheets);
    ids_repair(content.shaders);
    ids_repair(content.layers);
    ids_repair(content.nulls);
    ids_repair(content.events);
    ids_repair(content.sounds);
    for (auto& spritesheet : content.spritesheets)
      ids_repair(spritesheet.regions);
    for (auto& shader : content.shaders)
      if (shader.name.empty()) shader.name = SHADER_NAME_DEFAULT;

    for (int i = 0; i < model.animations_count_get(); ++i)
    {
      auto& animation = *model.animation_edit(i);
      animation_tracks_each(animation,
                            [&](Track& track)
                            {
                              if (track.type == ItemType::TRIGGER) return;
                              for (auto& frame : track.frames)
                                if (frame.shaderId != -1 && !item_get(content.shaders, frame.shaderId))
                                  frame.shaderId = -1;
                            });
      tracks_each(animation.layers,
                  [&](Track& track, auto*)
                  {
                    auto layer = item_get(content.layers, track.id);
                    auto spritesheet = layer ? item_get(content.spritesheets, layer->spritesheetId) : nullptr;
                    if (!spritesheet) return;
                    for (auto& frame : track.frames)
                    {
                      auto region = frame.regionId == -1 ? nullptr : item_get(spritesheet->regions, frame.regionId);
                      auto match = std::ranges::find_if(spritesheet->regions, [&](const Region& candidate)
                                                        { return is_region_matched(candidate, frame); });
                      if ((!region || !is_region_matched(*region, frame)) && match != spritesheet->regions.end())
                        frame.regionId = match->id;
                      if (frame.regionId == -1) continue;
                      region = item_get(spritesheet->regions, frame.regionId);
                      if (!region)
                      {
                        frame.regionId = -1;
                        continue;
                      }
                      frame.crop = region->crop;
                      frame.size = region->size;
                      frame.pivot = region->pivot;
                    }
                  });
    }
  }

  Model model_read(const XMLElement* element)
  {
    Model model{.layout = {}, .extras = extras_read(element)};
    children_each(element,
                  [&](const XMLElement* child, ElementType type, int position)
                  {
                    auto isKnown =
                        type == ElementType::INFO || type == ElementType::CONTENT || type == ElementType::ANIMATIONS;
                    if (!isKnown || std::ranges::contains(model.layout, type))
                      return extras_child_add(model.extras, child, position);
                    model.layout.push_back(type);
                    if (type == ElementType::INFO)
                    {
                      model.info = {.createdBy = string_get(child, "CreatedBy", Info{}.createdBy),
                                    .createdOn = string_get(child, "CreatedOn"),
                                    .fps = int_get(child, "Fps", Info{}.fps),
                                    .version = int_get(child, "Version", 0),
                                    .extras = extras_read(child)};
                      children_each(child, [&](const XMLElement* infoChild, ElementType, int infoPosition)
                                    { extras_child_add(model.info.extras, infoChild, infoPosition); });
                    }
                    if (type == ElementType::CONTENT) model.content = content_read(child);
                    if (type == ElementType::ANIMATIONS)
                    {
                      model.animations.defaultAnimation = string_get(child, "DefaultAnimation");
                      model.animations.extras = extras_read(child);
                      model.animations.entries = animation_entries_read(child, model.animations.extras);
                    }
                  });
    model_repair(model);
    return model;
  }

  // Writing: the model goes straight to a Sink in the shape a set of flags asks for, as text, a hash or an XmlNode tree.

  struct Writer
  {
    Flags flags{};
    const Model* model{};
    std::map<int, int> shaderIds{};
    std::map<int, int> layerIds{};
    std::map<int, std::map<int, int>> regionIds{};

    // With a model, a whole document is written: shader, layer and region ids are renumbered by position.
    Writer(Flags flags, const Model* model = nullptr) : flags(flags), model(model)
    {
      if (!model) return;
      auto& content = model->content;
      for (int i = 0; i < (int)content.shaders.size(); ++i)
        shaderIds[content.shaders[i].id] = i;
      for (int i = 0; i < (int)content.layers.size(); ++i)
        layerIds[content.layers[i].id] = i;
      std::set<int> spritesheetIds{};
      for (const auto& spritesheet : content.spritesheets)
        if (spritesheetIds.insert(spritesheet.id).second && !spritesheet.regions.empty())
          for (int i = 0; i < (int)spritesheet.regions.size(); ++i)
            regionIds[spritesheet.id][spritesheet.regions[i].id] = i;
    }

    bool is(Flag flag) const { return has_flag(flags, flag); }
  };

  int id_map_get(const std::map<int, int>& ids, int id)
  {
    auto it = ids.find(id);
    return it == ids.end() ? -1 : it->second;
  }

  bool is_float_whole(double value)
  {
    return std::abs(value) < FLOAT_WHOLE_MAX && value == std::trunc(value) && !(value == 0 && std::signbit(value));
  }

  // Where written XML goes. TEXT prints exactly as tinyxml2's XMLPrinter would (4-space indents, entities escaped,
  // text keeping its element's children inline); HASH folds the same elements into an FNV hash; TREE builds an XmlNode.
  struct Sink
  {
    enum Mode
    {
      TEXT,
      HASH,
      TREE
    };

    struct Open
    {
      std::string_view tag{};
      bool isEmpty{true};
      bool isTextDone{};
    };

    Mode mode{TEXT};
    std::string text{};
    std::uint64_t hash{HASH_OFFSET};
    XmlNode root{};
    std::vector<Open> stack{};
    std::vector<XmlNode*> nodes{};
    int textDepth{-1};

    void hash_add(char marker, std::string_view value)
    {
      hash = (hash ^ (unsigned char)marker) * HASH_PRIME;
      for (unsigned char character : value)
        hash = (hash ^ character) * HASH_PRIME;
      hash *= HASH_PRIME;
    }

    // Attributes escape all five entities; text only the first three.
    void escape(std::string_view value, bool isAttribute)
    {
      for (auto character : value)
      {
        auto entity = std::ranges::find(ENTITIES, character, &std::pair<char, std::string_view>::first);
        if (entity != std::end(ENTITIES) && (isAttribute || entity - ENTITIES < TEXT_ENTITY_COUNT))
          text += entity->second;
        else
          text += character;
      }
    }

    void indent(std::size_t depth)
    {
      for (std::size_t i = 0; i < depth; ++i)
        text += INDENT;
    }

    // The open element gets content: its start tag is finished, and its (empty) text is hashed.
    void content_begin()
    {
      if (stack.empty()) return;
      auto& parent = stack.back();
      if (mode == TEXT && parent.isEmpty) text += '>';
      if (mode == HASH && !parent.isTextDone) hash_add('T', {});
      parent.isEmpty = false;
      parent.isTextDone = true;
    }

    void open(std::string_view tag)
    {
      content_begin();
      if (mode == TEXT)
      {
        if (!text.empty() && textDepth < 0) text += '\n';
        if (text.empty() || textDepth < 0) indent(stack.size());
        text += '<';
        text += tag;
      }
      if (mode == HASH) hash_add('E', tag);
      if (mode == TREE)
      {
        auto& node = nodes.empty() ? root : nodes.back()->children.emplace_back();
        node.tag = tag;
        nodes.push_back(&node);
      }
      stack.push_back({tag});
    }

    // Numbers never need escaping.
    void attribute_text(std::string_view name, std::string_view value, bool isEscaped = true)
    {
      if (mode == TEXT)
      {
        text += ' ';
        text += name;
        text += "=\"";
        if (isEscaped)
          escape(value, true);
        else
          text += value;
        text += '"';
      }
      if (mode == HASH)
      {
        hash_add('A', name);
        hash_add('V', value);
      }
      if (mode == TREE) nodes.back()->attributes.emplace_back(name, value);
    }

    // Numbers print as tinyxml2's XMLUtil::ToStr does ("%d", "%.8g", "true"/"false"), without printf; whole numbers
    // (most values) print as integers, which "%.8g" also does below 1e8 (but keeps "-0").
    template <class T> void attribute(std::string_view name, const T& value)
    {
      if constexpr (std::is_convertible_v<T, std::string_view>)
        attribute_text(name, value);
      else
      {
        char buffer[VALUE_BUFFER_SIZE]{};
        auto end = buffer;
        if constexpr (std::is_same_v<T, bool>)
          end = std::ranges::copy(std::string_view(value ? "true" : "false"), buffer).out;
        else if constexpr (std::is_floating_point_v<T>)
          end = is_float_whole(value)
                    ? std::to_chars(buffer, buffer + sizeof(buffer), (long long)value).ptr
                    : std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general,
                                    std::is_same_v<T, float> ? FLOAT_DIGITS : DOUBLE_DIGITS)
                          .ptr;
        else
          end = std::to_chars(buffer, buffer + sizeof(buffer), (long long)value).ptr;
        attribute_text(name, std::string_view(buffer, end), false);
      }
    }

    // An element's text, after its attributes and before its children.
    void text_add(std::string_view value)
    {
      auto& open = stack.back();
      if (mode == HASH) hash_add('T', value);
      if (mode == TREE) nodes.back()->text = value;
      if (mode == TEXT && !value.empty())
      {
        textDepth = (int)stack.size() - 1;
        text += '>';
        open.isEmpty = false;
        escape(value, false);
      }
      open.isTextDone = true;
    }

    void close()
    {
      auto open = stack.back();
      stack.pop_back();
      if (mode == HASH)
      {
        if (!open.isTextDone) hash_add('T', {});
        hash_add(']', {});
      }
      if (mode == TREE) nodes.pop_back();
      if (mode != TEXT) return;
      if (open.isEmpty)
        text += "/>";
      else
      {
        if (textDepth < 0)
        {
          text += '\n';
          indent(stack.size());
        }
        text += "</";
        text += open.tag;
        text += '>';
      }
      if (textDepth == (int)stack.size()) textDepth = -1;
      if (stack.empty()) text += '\n';
    }

    void node(const XmlNode& node)
    {
      open(node.tag);
      for (const auto& [name, value] : node.attributes)
        attribute_text(name, value);
      text_add(node.text);
      for (const auto& child : node.children)
        this->node(child);
      close();
    }
  };

  // Unknown XML goes back where it was read: attributes after the known ones, then text, and (through Children)
  // children at their recorded positions among the known ones.
  void extras_attributes_write(Sink& sink, const Extras& extras)
  {
    for (const auto& [name, value] : extras.attributes)
      sink.attribute_text(name, value);
  }

  void extras_write(Sink& sink, const Extras& extras)
  {
    extras_attributes_write(sink, extras);
    sink.text_add(extras.text);
  }

  // Interleaves an element's unknown children with its known ones: add() comes before each known child.
  struct Children
  {
    Sink& sink;
    const Extras& extras;
    std::size_t next{};
    int position{};

    void add()
    {
      while (next < extras.children.size() && extras.children[next].first <= position)
      {
        sink.node(extras.children[next++].second);
        ++position;
      }
      ++position;
    }

    void flush()
    {
      while (next < extras.children.size())
        sink.node(extras.children[next++].second);
    }

    void close()
    {
      flush();
      sink.close();
    }
  };

  void element_close(Sink& sink, const Extras& extras) { Children{sink, extras}.close(); }

  void path_add(Sink& sink, const char* name, const std::filesystem::path& value)
  {
    if (!value.empty()) sink.attribute(name, path::to_utf8(value));
  }

  void interpolation_add(Sink& sink, const char* name, Interpolation interpolation)
  {
    if (interpolation == Interpolation::NONE || interpolation == Interpolation::LINEAR)
      sink.attribute(name, interpolation == Interpolation::LINEAR);
    else
      sink.attribute(name, INTERPOLATION_VALUES[(int)interpolation]);
  }

  bool is_group_id_written(const Writer& writer)
  {
    return writer.is(SERIALIZE_GROUPS) && !writer.is(SERIALIZE_NESTED_GROUPS);
  }

  bool is_special_interpolation(Interpolation interpolation)
  {
    return interpolation != Interpolation::NONE && interpolation != Interpolation::LINEAR;
  }

  // A whole document's frames use renumbered shader and region ids; a layer frame on a region takes its rectangle.
  Frame frame_normalize(const Writer& writer, Frame frame, const Track* layerTrack)
  {
    if (!writer.model) return frame;
    if (frame.shaderId != -1) frame.shaderId = id_map_get(writer.shaderIds, frame.shaderId);
    if (!layerTrack || frame.regionId == -1) return frame;

    auto& content = writer.model->content;
    auto layer = item_get(content.layers, layerTrack->id);
    auto spritesheet = layer ? item_get(content.spritesheets, layer->spritesheetId) : nullptr;
    if (!spritesheet) return frame;
    auto regionIds = writer.regionIds.find(spritesheet->id);
    frame.regionId = regionIds == writer.regionIds.end() ? -1 : id_map_get(regionIds->second, frame.regionId);
    if (frame.regionId == -1) return frame;
    auto& region = spritesheet->regions[frame.regionId];
    frame.crop = region.crop;
    frame.size = region.size;
    frame.pivot = region.pivot;
    return frame;
  }

  // A frame baked into single steps for the game records, on its first step, how to read them back as one.
  struct FrameBake
  {
    Interpolation interpolation{};
    int delay{};
  };

  void frame_write(Sink& sink, const Writer& writer, const Frame& frame, ItemType type, FrameBake bake = {})
  {
    if (type == ItemType::TRIGGER)
    {
      sink.open(tag_get(ElementType::TRIGGER));
      if (frame.eventId != -1) sink.attribute("EventId", frame.eventId);
      sink.attribute("AtFrame", frame.atFrame);
      extras_write(sink, frame.extras);
      Children children{sink, frame.extras};
      if (writer.is(SERIALIZE_SOUNDS))
        for (auto soundId : frame.soundIds)
          if (soundId != -1)
          {
            children.add();
            sink.open(tag_get(ElementType::SOUND_ELEMENT));
            sink.attribute("Id", soundId);
            sink.close();
          }
      return children.close();
    }

    sink.open(tag_get(ElementType::FRAME));
    if (type == ItemType::LAYER)
    {
      auto isRegion = writer.is(SERIALIZE_REGIONS) && frame.regionId != -1;
      if (isRegion) sink.attribute("RegionId", frame.regionId);
      if (writer.is(SERIALIZE_REDUNDANT_FRAME_REGION_VALUES) || !isRegion)
      {
        sink.attribute("XPivot", frame.pivot.x);
        sink.attribute("YPivot", frame.pivot.y);
        sink.attribute("XCrop", frame.crop.x);
        sink.attribute("YCrop", frame.crop.y);
        sink.attribute("Width", frame.size.x);
        sink.attribute("Height", frame.size.y);
      }
    }
    sink.attribute("XPosition", frame.position.x);
    sink.attribute("YPosition", frame.position.y);
    sink.attribute("Delay", frame.duration);
    sink.attribute("Visible", frame.isVisible);
    sink.attribute("XScale", frame.scale.x);
    sink.attribute("YScale", frame.scale.y);
    if (writer.is(SERIALIZE_EXTENSIONS))
    {
      sink.attribute("ShearX", frame.shear.x);
      sink.attribute("ShearY", frame.shear.y);
    }
    for (int i = 0; i < (int)std::size(TINT_ATTRIBUTES); ++i)
      sink.attribute(TINT_ATTRIBUTES[i], math::float_to_uint8(frame.tint[i]));
    for (int i = 0; i < (int)std::size(OFFSET_ATTRIBUTES); ++i)
      sink.attribute(OFFSET_ATTRIBUTES[i], math::float_to_uint8(frame.colorOffset[i]));
    sink.attribute("Rotation", frame.rotation);
    interpolation_add(sink, "Interpolated", frame.interpolation);
    if (writer.is(SERIALIZE_EXTENSIONS) && frame.shaderId != -1) sink.attribute("ShaderId", frame.shaderId);
    extras_attributes_write(sink, frame.extras);
    if (bake.delay > 0)
    {
      sink.attribute("BakeInterpolation", INTERPOLATION_VALUES[(int)bake.interpolation]);
      sink.attribute("BakeDelay", bake.delay);
    }
    sink.text_add(frame.extras.text);
    element_close(sink, frame.extras);
  }

  // A backup (FrameRestore) track keeps its frames' region ids as they are.
  void track_write(Sink& sink, const Writer& writer, const Track& track, int groupId = -1, bool isBackup = false)
  {
    sink.open(tag_get(track_element_type_get(track.type)));
    if (track.type == ItemType::LAYER || track.type == ItemType::NULL_)
    {
      auto id = track.id;
      if (track.type == ItemType::LAYER && writer.model)
        if (auto it = writer.layerIds.find(id); it != writer.layerIds.end()) id = it->second;
      sink.attribute(track.type == ItemType::LAYER ? "LayerId" : "NullId", id);
      sink.attribute("Visible", track.isVisible);
      if (groupId != -1 && is_group_id_written(writer)) sink.attribute("GroupId", groupId);
    }
    extras_write(sink, track.extras);

    Children children{sink, track.extras};
    auto layerTrack = track.type == ItemType::LAYER && !isBackup ? &track : nullptr;
    for (int i = 0; i < (int)track.frames.size(); ++i)
    {
      auto frame = frame_normalize(writer, track.frames[i], layerTrack);
      if (!writer.is(SERIALIZE_BAKE_SPECIAL_INTERPOLATED_FRAMES) || track.type == ItemType::TRIGGER ||
          !is_special_interpolation(frame.interpolation))
      {
        children.add();
        frame_write(sink, writer, frame, track.type);
        continue;
      }
      // Written for the game as single-step frames; the first records how to read them back as one.
      auto baked = frame_bake_split(frame, i + 1 < (int)track.frames.size() ? track.frames[i + 1] : frame,
                                    FRAME_DURATION_MIN, false, false);
      for (int bakeIndex = 0; bakeIndex < (int)baked.size(); ++bakeIndex)
      {
        children.add();
        frame_write(sink, writer, baked[bakeIndex], track.type,
                    bakeIndex == 0 ? FrameBake{frame.interpolation, (int)baked.size()} : FrameBake{});
      }
    }
    children.close();
  }

  // A group; `index` records its position when groups are written apart from their container, and `members` writes
  // the children nested into it.
  template <class Group>
  void group_write(Sink& sink, const Writer& writer, const Group& group, std::optional<int> index = {},
                   const std::function<void()>& members = {})
  {
    sink.open(tag_get(ElementType::GROUP));
    if (!writer.is(SERIALIZE_NESTED_GROUPS)) sink.attribute("Id", group.id);
    sink.attribute("Name", group.name);
    sink.attribute("IsExpanded", group.isExpanded);
    sink.attribute("Visible", group.isVisible);
    if (index) sink.attribute("Index", *index);
    extras_write(sink, group.extras);
    Children children{sink, group.extras};
    if constexpr (requires { group.root; })
    {
      children.add();
      track_write(sink, writer, group.root);
    }
    children.flush();
    if (members) members();
    sink.close();
  }

  struct ContainerChild
  {
    std::function<void(std::optional<int>, const std::function<void()>&)> write{};
    int groupId{-1};
    std::optional<int> ownGroupId{};
  };

  template <class Group> ContainerChild group_child_make(Sink& sink, const Writer& writer, const Group& group)
  {
    return {.write = [&](std::optional<int> index, const std::function<void()>& members)
            { group_write(sink, writer, group, index, members); },
            .ownGroupId = group.id};
  }

  // Children are laid out flat (a group, then its children) and reshaped for the flags: nested into their groups, or,
  // in a track container, moved to a *AnimationGroups sibling (`groupsType`) that records each group's position.
  void container_write(Sink& sink, const Writer& writer, std::vector<ContainerChild> flat, const Extras& extras,
                       ElementType groupsType = ElementType::UNKNOWN)
  {
    extras_write(sink, extras);
    for (const auto& [index, node] : extras.children)
      flat.insert(flat.begin() + std::min(index, (int)flat.size()),
                  ContainerChild{.write = [&sink, &node](std::optional<int>, const std::function<void()>&)
                                 { sink.node(node); }});

    auto isGroups = writer.is(SERIALIZE_GROUPS);
    std::map<int, std::vector<ContainerChild>> grouped{};
    if (isGroups && writer.is(SERIALIZE_NESTED_GROUPS))
    {
      std::set<int> groupIds{};
      for (const auto& child : flat)
        if (child.ownGroupId && *child.ownGroupId >= 0) groupIds.insert(*child.ownGroupId);
      for (const auto& child : flat)
        if (!child.ownGroupId && groupIds.contains(child.groupId)) grouped[child.groupId].push_back(child);
      std::erase_if(flat,
                    [&](const ContainerChild& child) { return !child.ownGroupId && groupIds.contains(child.groupId); });
    }

    auto isExtracted = groupsType != ElementType::UNKNOWN && isGroups && !writer.is(SERIALIZE_NESTED_GROUPS);
    std::vector<int> extracted{};
    for (int i = 0; i < (int)flat.size(); ++i)
    {
      auto& child = flat[i];
      if (child.ownGroupId && !isGroups) continue;
      if (child.ownGroupId && isExtracted)
      {
        extracted.push_back(i);
        continue;
      }
      auto members = child.ownGroupId ? grouped.find(*child.ownGroupId) : grouped.end();
      child.write({},
                  [&]()
                  {
                    if (members != grouped.end())
                      for (auto& member : members->second)
                        member.write({}, {});
                  });
    }
    sink.close();

    if (extracted.empty()) return;
    sink.open(tag_get(groupsType));
    for (auto i : extracted)
      flat[i].write(i, {});
    sink.close();
  }

  void track_entries_write(Sink& sink, const Writer& writer, const std::vector<TrackEntry>& entries, ElementType type,
                           const Extras& extras)
  {
    std::vector<ContainerChild> flat{};
    auto track_child_make = [&](const Track& track, int groupId)
    {
      return ContainerChild{.write = [&, groupId](std::optional<int>, const std::function<void()>&)
                            { track_write(sink, writer, track, groupId); },
                            .groupId = groupId};
    };
    for (const auto& entry : entries)
      if (auto track = std::get_if<Track>(&entry))
        flat.push_back(track_child_make(*track, -1));
      else
      {
        auto& group = std::get<TrackGroup>(entry);
        flat.push_back(group_child_make(sink, writer, group));
        for (const auto& groupTrack : group.tracks)
          flat.push_back(track_child_make(groupTrack, group.id));
      }
    sink.open(tag_get(type));
    container_write(sink, writer, std::move(flat), extras,
                    type == ElementType::LAYER_ANIMATIONS ? ElementType::LAYER_ANIMATION_GROUPS
                                                          : ElementType::NULL_ANIMATION_GROUPS);
  }

  bool is_frame_transform_default(const Frame& frame)
  {
    Frame base{};
    return frame.position == base.position && frame.scale == base.scale && frame.rotation == base.rotation &&
           frame.shear == base.shear && frame.tint == base.tint && frame.colorOffset == base.colorOffset;
  }

  IsaacIssues isaac_issues_get(const Model& model)
  {
    IsaacIssues issues{};
    for (auto [index, animation] : model.animations_get())
    {
      for (const auto* entries : {&animation->layers, &animation->nulls})
        groups_each(*entries, [&](const TrackGroup& group)
                    { issues.groupTransforms += !std::ranges::all_of(group.root.frames, is_frame_transform_default); });
      animation_tracks_each(*animation,
                            [&](const Track& track)
                            {
                              for (const auto& frame : track.frames)
                              {
                                issues.easedFrames +=
                                    track.type != ItemType::TRIGGER && is_special_interpolation(frame.interpolation);
                                issues.shearFrames += frame.shear != glm::vec2();
                                issues.shaderFrames += frame.shaderId != -1;
                                issues.triggerSounds +=
                                    track.type == ItemType::TRIGGER &&
                                    std::ranges::any_of(frame.soundIds, [](int id) { return id != -1; });
                              }
                            });
    }
    return issues;
  }

  // For the game, a group root's transform is baked into its tracks one frame at a time.
  void track_group_root_bake(Track& track, const Track& root, int frameNum)
  {
    auto trackLength = track_length_get(track);
    if (trackLength <= 0) return;
    auto length = std::max({trackLength, track_length_get(root), frameNum, FRAME_DURATION_MIN});
    std::vector<Frame> frames{};
    for (int time = 0; time < length; ++time)
    {
      auto& frame = frames.emplace_back(frame_generate(track, (float)time));
      frame_root_transform_apply(frame, frame_generate(root, (float)time), false, false, false);
      frame.duration = FRAME_DURATION_MIN;
      frame.interpolation = Interpolation::NONE;
    }
    track.frames = std::move(frames);
  }

  // The game's copy: group roots baked into their tracks (originals kept in a FrameRestore node the editor reads
  // back), and interpolation flattened into single-step frames.
  Animation animation_bake(const Writer& writer, Animation animation)
  {
    if (writer.is(SERIALIZE_BAKE_GROUP_FRAMES))
      for (auto* entries : {&animation.layers, &animation.nulls})
        groups_each(*entries,
                    [&](TrackGroup& group)
                    {
                      if (std::ranges::all_of(group.root.frames, is_frame_transform_default)) return;
                      Sink backup{.mode = Sink::TREE};
                      backup.open(FRAME_RESTORE_TAG);
                      track_write(backup, writer, group.root);
                      for (auto& track : group.tracks)
                      {
                        track_write(backup, writer, track, group.id, true);
                        track_group_root_bake(track, group.root, animation.frameNum);
                      }
                      backup.close();
                      group.root = TrackGroup{}.root;
                      // Kept in order of position, where it reads back the same as appended at position 1.
                      auto& children = group.extras.children;
                      children.insert(std::ranges::lower_bound(children, 1, {}, &std::pair<int, XmlNode>::first),
                                      {1, std::move(backup.root)});
                    });

    if (writer.is(SERIALIZE_FLATTEN_SPECIAL_INTERPOLATED_FRAMES))
      animation_tracks_each(animation,
                            [](Track& track)
                            {
                              if (track.type == ItemType::TRIGGER) return;
                              for (int index = (int)track.frames.size() - 1; index >= 0; --index)
                                if (track.frames[index].interpolation != Interpolation::NONE)
                                  frame_bake(track, index, FRAME_DURATION_MIN, false, false);
                            });
    return animation;
  }

  // Known children in their recorded order; a part with content that was never in the file is appended.
  std::vector<ElementType> layout_get(std::vector<ElementType> layout,
                                      std::initializer_list<std::pair<ElementType, bool>> parts)
  {
    for (auto [type, isContent] : parts)
      if (isContent && !std::ranges::contains(layout, type)) layout.push_back(type);
    return layout;
  }

  void animation_write(Sink& sink, const Writer& writer, const Animation& source, int groupId = -1)
  {
    auto isBaked = writer.model &&
                   (writer.is(SERIALIZE_BAKE_GROUP_FRAMES) || writer.is(SERIALIZE_FLATTEN_SPECIAL_INTERPOLATED_FRAMES));
    auto baked = isBaked ? std::optional(animation_bake(writer, source)) : std::nullopt;
    const auto& animation = baked ? *baked : source;

    sink.open(tag_get(ElementType::ANIMATION));
    sink.attribute("Name", animation.name);
    sink.attribute("FrameNum", animation.frameNum);
    sink.attribute("Loop", animation.isLoop);
    if (writer.is(SERIALIZE_EXTENSIONS))
      for (auto [name, marker] : {std::pair{"StartMarker", animation.startMarker}, {"EndMarker", animation.endMarker}})
        if (marker != -1) sink.attribute(name, marker);
    if (groupId != -1 && is_group_id_written(writer)) sink.attribute("GroupId", groupId);
    extras_write(sink, animation.extras);

    Children children{sink, animation.extras};
    for (auto type : layout_get(animation.layout, {{ElementType::ROOT_ANIMATION, !animation.root.frames.empty()},
                                                   {ElementType::LAYER_ANIMATIONS, !animation.layers.empty()},
                                                   {ElementType::NULL_ANIMATIONS, !animation.nulls.empty()},
                                                   {ElementType::TRIGGERS, !animation.triggers.frames.empty()}}))
    {
      children.add();
      if (type == ElementType::ROOT_ANIMATION) track_write(sink, writer, animation.root);
      if (type == ElementType::TRIGGERS) track_write(sink, writer, animation.triggers);
      if (type == ElementType::LAYER_ANIMATIONS)
        track_entries_write(sink, writer, animation.layers, type, animation.layersExtras);
      if (type == ElementType::NULL_ANIMATIONS)
        track_entries_write(sink, writer, animation.nulls, type, animation.nullsExtras);
    }
    children.close();
  }

  void animations_write(Sink& sink, const Writer& writer, const Animations& animations)
  {
    std::vector<ContainerChild> flat{};
    auto animation_child_make = [&](const Animation& animation, int groupId)
    {
      return ContainerChild{.write = [&, groupId](std::optional<int>, const std::function<void()>&)
                            { animation_write(sink, writer, animation, groupId); },
                            .groupId = groupId};
    };
    for (const auto& entry : animations.entries)
      if (auto animation = std::get_if<std::shared_ptr<const Animation>>(&entry))
        flat.push_back(animation_child_make(**animation, -1));
      else
      {
        auto& group = std::get<AnimationGroup>(entry);
        flat.push_back(group_child_make(sink, writer, group));
        for (const auto& groupAnimation : group.animations)
          flat.push_back(animation_child_make(*groupAnimation, group.id));
      }
    sink.open(tag_get(ElementType::ANIMATIONS));
    sink.attribute("DefaultAnimation", animations.defaultAnimation);
    container_write(sink, writer, std::move(flat), animations.extras);
  }

  void item_write(Sink& sink, const Writer&, const Region& region, int id)
  {
    sink.open(tag_get(ElementType::REGION));
    sink.attribute("Id", id);
    sink.attribute("Name", region.name);
    sink.attribute("XCrop", region.crop.x);
    sink.attribute("YCrop", region.crop.y);
    sink.attribute("Width", region.size.x);
    sink.attribute("Height", region.size.y);
    if (region.origin != Origin::CUSTOM)
      sink.attribute("Origin", ORIGIN_VALUES[(int)region.origin]);
    else
    {
      sink.attribute("XPivot", region.pivot.x);
      sink.attribute("YPivot", region.pivot.y);
    }
    extras_write(sink, region.extras);
    element_close(sink, region.extras);
  }

  void item_write(Sink& sink, const Writer& writer, const Spritesheet& spritesheet, int id)
  {
    sink.open(tag_get(ElementType::SPRITESHEET));
    sink.attribute("Id", id);
    path_add(sink, "Path", spritesheet.path);
    extras_write(sink, spritesheet.extras);
    Children children{sink, spritesheet.extras};
    if (writer.is(SERIALIZE_REGIONS))
      for (int i = 0; i < (int)spritesheet.regions.size(); ++i)
      {
        children.add();
        item_write(sink, writer, spritesheet.regions[i], writer.model ? i : spritesheet.regions[i].id);
      }
    children.close();
  }

  void binding_value_add(Sink& sink, const std::string& binding, const std::string& value)
  {
    if (!binding.empty()) sink.attribute("Binding", binding);
    if (!value.empty()) sink.attribute("Value", value);
  }

  void item_write(Sink& sink, const Writer&, const Shader& shader, int id)
  {
    sink.open(tag_get(ElementType::SHADER));
    sink.attribute("Id", id);
    sink.attribute("Name", shader.name);
    path_add(sink, "Vertex", shader.vertex);
    path_add(sink, "Fragment", shader.fragment);
    extras_write(sink, shader.extras);
    Children children{sink, shader.extras};
    for (const auto& uniform : shader.uniforms)
    {
      children.add();
      sink.open(tag_get(ElementType::UNIFORM));
      sink.attribute("Name", uniform.name);
      binding_value_add(sink, uniform.binding, uniform.value);
      extras_write(sink, uniform.extras);
      Children components{sink, uniform.extras};
      for (const auto& component : uniform.components)
      {
        components.add();
        sink.open(tag_get(ElementType::COMPONENT));
        sink.attribute("Index", component.index);
        binding_value_add(sink, component.binding, component.value);
        extras_write(sink, component.extras);
        element_close(sink, component.extras);
      }
      components.close();
    }
    children.close();
  }

  void item_write(Sink& sink, const Writer&, const Layer& layer, int id)
  {
    sink.open(tag_get(ElementType::LAYER_ELEMENT));
    sink.attribute("Id", id);
    sink.attribute("Name", layer.name);
    sink.attribute("SpritesheetId", layer.spritesheetId);
    extras_write(sink, layer.extras);
    element_close(sink, layer.extras);
  }

  void item_write(Sink& sink, const Writer&, const Null& null, int id)
  {
    sink.open(tag_get(ElementType::NULL_ELEMENT));
    sink.attribute("Id", id);
    sink.attribute("Name", null.name);
    if (null.isShowRect) sink.attribute("ShowRect", null.isShowRect);
    extras_write(sink, null.extras);
    element_close(sink, null.extras);
  }

  void item_write(Sink& sink, const Writer&, const Event& event, int id)
  {
    sink.open(tag_get(ElementType::EVENT_ELEMENT));
    sink.attribute("Id", id);
    sink.attribute("Name", event.name);
    extras_write(sink, event.extras);
    element_close(sink, event.extras);
  }

  void item_write(Sink& sink, const Writer&, const Sound& sound, int id)
  {
    sink.open(tag_get(ElementType::SOUND_ELEMENT));
    sink.attribute("Id", id);
    path_add(sink, "Path", sound.path);
    extras_write(sink, sound.extras);
    element_close(sink, sound.extras);
  }

  // A content container's unknown XML (a file may repeat a container; its parts are merged in order).
  Extras container_extras_get(const Content& content, ElementType type)
  {
    Extras merged{};
    for (const auto& [extrasType, extras] : content.containerExtras)
    {
      if (extrasType != type) continue;
      merged.attributes.insert(merged.attributes.end(), extras.attributes.begin(), extras.attributes.end());
      if (!extras.text.empty()) merged.text = extras.text;
      merged.children.insert(merged.children.end(), extras.children.begin(), extras.children.end());
    }
    return merged;
  }

  template <class Item>
  void items_write(Sink& sink, const Writer& writer, const std::vector<Item>& items, ElementType type,
                   const Extras& extras)
  {
    sink.open(tag_get(type));
    extras_write(sink, extras);
    Children children{sink, extras};
    auto isRenumbered = writer.model && (type == ElementType::LAYERS || type == ElementType::SHADERS);
    for (int i = 0; i < (int)items.size(); ++i)
    {
      children.add();
      item_write(sink, writer, items[i], isRenumbered ? i : items[i].id);
    }
    children.close();
  }

  void content_write(Sink& sink, const Writer& writer, const Content& content)
  {
    sink.open(tag_get(ElementType::CONTENT));
    extras_write(sink, content.extras);
    Children children{sink, content.extras};
    for (auto type : layout_get(content.layout, {{ElementType::SPRITESHEETS, !content.spritesheets.empty()},
                                                 {ElementType::SHADERS, !content.shaders.empty()},
                                                 {ElementType::LAYERS, !content.layers.empty()},
                                                 {ElementType::NULLS, !content.nulls.empty()},
                                                 {ElementType::EVENTS, !content.events.empty()},
                                                 {ElementType::SOUNDS, !content.sounds.empty()}}))
    {
      children.add();
      auto extras = container_extras_get(content, type);
      // Optional containers are left out when the flags drop them or they are empty (their place still counts for
      // where unknown children go).
      if (type == ElementType::SHADERS && writer.is(SERIALIZE_EXTENSIONS) &&
          (!content.shaders.empty() || !extras.children.empty()))
        items_write(sink, writer, content.shaders, type, extras);
      if (type == ElementType::SOUNDS && writer.is(SERIALIZE_SOUNDS) &&
          (!content.sounds.empty() || !extras.children.empty()))
        items_write(sink, writer, content.sounds, type, extras);
      if (type == ElementType::SPRITESHEETS) items_write(sink, writer, content.spritesheets, type, extras);
      if (type == ElementType::LAYERS) items_write(sink, writer, content.layers, type, extras);
      if (type == ElementType::NULLS) items_write(sink, writer, content.nulls, type, extras);
      if (type == ElementType::EVENTS) items_write(sink, writer, content.events, type, extras);
    }
    children.close();
  }

  // A whole document; `inner` writes anything after its children (the SourceDocument copy) and `tag` renames it.
  void model_write(Sink& sink, const Model& model, Flags flags, const std::function<void()>& inner = {},
                   std::string_view tag = {})
  {
    Writer writer(flags, &model);
    sink.open(tag.empty() ? tag_get(ElementType::ANIMATED_ACTOR) : tag);
    extras_write(sink, model.extras);
    Children children{sink, model.extras};
    auto isInfo = model.info.fps != Info{}.fps || model.info.createdBy != Info{}.createdBy;
    for (auto type : layout_get(model.layout, {{ElementType::INFO, isInfo},
                                               {ElementType::CONTENT, true},
                                               {ElementType::ANIMATIONS, !model.animations.entries.empty()}}))
    {
      children.add();
      if (type == ElementType::CONTENT) content_write(sink, writer, model.content);
      if (type == ElementType::ANIMATIONS) animations_write(sink, writer, model.animations);
      if (type != ElementType::INFO) continue;
      sink.open(tag_get(ElementType::INFO));
      sink.attribute("CreatedBy", model.info.createdBy);
      sink.attribute("CreatedOn", model.info.createdOn);
      sink.attribute("Fps", model.info.fps);
      sink.attribute("Version", model.info.version);
      extras_write(sink, model.info.extras);
      element_close(sink, model.info.extras);
    }
    children.flush();
    if (inner) inner();
    sink.close();
  }

  // A save is the game's document with the editor's own copy embedded as SourceDocument (or just the editor's copy).
  void document_write(Sink& sink, const Model& model, Options options)
  {
    if (options.isExtendedFormat) return model_write(sink, model, SERIALIZE_ANM2ED_DEFAULT);
    model_write(sink, model, SERIALIZE_ISAAC_DEFAULT,
                [&]() { model_write(sink, model, SERIALIZE_ANM2ED_DEFAULT, {}, SOURCE_DOCUMENT_TAG); });
  }

  // The text of whatever `write` sends to a sink.
  template <class Write> std::string text_get(Write&& write)
  {
    Sink sink{};
    write(sink);
    return std::move(sink.text);
  }

  bool document_load(Model& model, XMLDocument& document, XMLError result, std::string* errorString)
  {
    auto root = result == XML_SUCCESS ? document.RootElement() : nullptr;
    if (!root)
    {
      if (errorString) *errorString = result == XML_SUCCESS ? "No root element." : document.ErrorStr();
      model = model_make();
      model.isValid = false;
      return false;
    }
    auto source = root->Name() == SOURCE_DOCUMENT_TAG ? root : root->FirstChildElement(SOURCE_DOCUMENT_TAG.data());
    model = model_read(source ? source : root);
    return true;
  }

  Model model_make()
  {
    Model model{};
    model.info.createdOn = time::get(CREATED_ON_FORMAT);
    return model;
  }

  bool model_load(Model& model, const std::filesystem::path& path, std::string* errorString)
  {
    XMLDocument document{};
    File file(path, "rb");
    if (file) return document_load(model, document, document.LoadFile(file.get()), errorString);
    if (errorString) *errorString = "File not found.";
    model = model_make();
    model.isValid = false;
    return false;
  }

  bool model_load_string(Model& model, std::string_view text, std::string* errorString)
  {
    XMLDocument document{};
    return document_load(model, document, document.Parse(text.data(), text.size()), errorString);
  }

  bool model_save(const Model& model, const std::filesystem::path& path, std::string* errorString, Options options)
  {
    auto text = model_to_string(model, options);
    File file(path, "wb");
    if (!file)
    {
      if (errorString) *errorString = "File permissions.";
      return false;
    }
    if (std::fwrite(text.data(), 1, text.size(), file.get()) == text.size()) return true;
    if (errorString) *errorString = "Could not write the file.";
    return false;
  }

  std::string model_to_string(const Model& model, Options options)
  {
    return text_get([&](Sink& sink) { document_write(sink, model, options); });
  }

  std::string model_serialize(const Model& model, Flags flags)
  {
    return text_get([&](Sink& sink) { model_write(sink, model, flags); });
  }

  // The hash of what the editor would save, so it changes exactly when a save would.
  std::uint64_t model_hash(const Model& model)
  {
    Sink sink{.mode = Sink::HASH};
    model_write(sink, model, SERIALIZE_ANM2ED_DEFAULT);
    return sink.hash;
  }

  std::string frame_to_string(const Frame& frame, ItemType type)
  {
    return text_get([&](Sink& sink) { frame_write(sink, Writer(CLIPBOARD_FLAGS), frame, type); });
  }

  std::string animation_to_string(const Animation& animation, int groupId)
  {
    return text_get([&](Sink& sink) { animation_write(sink, Writer(CLIPBOARD_FLAGS), animation, groupId); });
  }

  std::string animation_group_to_string(const AnimationGroup& group)
  {
    return text_get([&](Sink& sink) { group_write(sink, Writer(CLIPBOARD_FLAGS), group); });
  }

  bool document_parse(XMLDocument& document, const std::string& text, std::string* errorString)
  {
    if (document.Parse(text.c_str()) == XML_SUCCESS) return true;
    if (errorString) *errorString = document.ErrorStr();
    return false;
  }

  std::vector<Frame> frames_from_string(const std::string& text, ItemType type, std::string* errorString)
  {
    XMLDocument document{};
    if (!document_parse(document, text, errorString)) return {};
    auto tag = tag_get(type == ItemType::TRIGGER ? ElementType::TRIGGER : ElementType::FRAME);
    std::vector<Frame> frames{};
    for (auto element = document.FirstChildElement(tag); element; element = element->NextSiblingElement(tag))
      frames.push_back(frame_read(element));
    if (frames.empty() && errorString) *errorString = std::format("No valid {}(s).", tag);
    return frames;
  }

  std::vector<AnimationEntry> animations_from_string(const std::string& text, std::string* errorString)
  {
    XMLDocument document{};
    if (!document_parse(document, text, errorString)) return {};
    Extras extras{};
    auto entries = animation_entries_read(&document, extras);
    if (entries.empty() && errorString) *errorString = "No valid animation(s).";
    return entries;
  }

  template <class Item> std::string item_to_string(const Item& item)
  {
    return text_get([&](Sink& sink) { item_write(sink, Writer(CLIPBOARD_FLAGS), item, item.id); });
  }

  template <class Item> std::vector<Item> items_from_string(const std::string& text, std::string* errorString)
  {
    XMLDocument document{};
    if (!document_parse(document, text, errorString)) return {};
    auto tag = tag_get(ITEM_TYPE<Item>);
    std::vector<Item> items{};
    for (auto element = document.FirstChildElement(tag); element; element = element->NextSiblingElement(tag))
    {
      auto item = item_read<Item>(element);
      if constexpr (requires { item.path; }) item.path = path::backslash_handle(item.path);
      items.push_back(std::move(item));
    }
    if (items.empty() && errorString) *errorString = std::format("No valid {}(s).", tag);
    return items;
  }

#define X(Item)                                                                                                        \
  template std::string item_to_string(const Item&);                                                                    \
  template std::vector<Item> items_from_string(const std::string&, std::string*);
  X(Spritesheet)
  X(Region)
  X(Shader)
  X(Layer)
  X(Null)
  X(Event)
  X(Sound)
#undef X
}
