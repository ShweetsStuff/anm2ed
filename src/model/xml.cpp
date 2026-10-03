#include "xml.hpp"

#include <algorithm>
#include <format>
#include <map>
#include <optional>
#include <tuple>

#include <tinyxml2/tinyxml2.h>

#include "file.hpp"
#include "frames.hpp"
#include "math.hpp"
#include "path.hpp"
#include "time.hpp"
#include "util/xml.hpp"

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
      "ShaderId", "SoundId", "GroupId", "Index", "Loop", "Visible", "ShowRect", "IsExpanded", "Enabled", "Rotation",
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

  XmlNode node_read(const XMLElement* element)
  {
    XmlNode node{.tag = element->Name()};
    for (auto attribute = element->FirstAttribute(); attribute; attribute = attribute->Next())
      node.attributes.emplace_back(attribute->Name(), attribute->Value());
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
        extras.attributes.emplace_back(attribute->Name(), attribute->Value());
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

  // Writing: the model to an XmlNode tree in the shape a set of flags asks for, then to tinyxml2 or a hash.

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

  template <class T> void attribute_add(XmlNode& node, const char* name, const T& value)
  {
    if constexpr (std::is_convertible_v<T, std::string_view>)
      node.attributes.emplace_back(name, std::string(std::string_view(value)));
    else
    {
      char buffer[VALUE_BUFFER_SIZE]{};
      XMLUtil::ToStr(value, buffer, sizeof(buffer));
      node.attributes.emplace_back(name, buffer);
    }
  }

  void path_add(XmlNode& node, const char* name, const std::filesystem::path& value)
  {
    if (!value.empty()) attribute_add(node, name, path::to_utf8(value));
  }

  void interpolation_add(XmlNode& node, const char* name, Interpolation interpolation)
  {
    if (interpolation == Interpolation::NONE || interpolation == Interpolation::LINEAR)
      attribute_add(node, name, interpolation == Interpolation::LINEAR);
    else
      attribute_add(node, name, INTERPOLATION_VALUES[(int)interpolation]);
  }

  // Unknown XML goes back where it was read: attributes after the known ones, children at their recorded position.
  void extras_write(XmlNode& node, const Extras& extras)
  {
    node.attributes.insert(node.attributes.end(), extras.attributes.begin(), extras.attributes.end());
    if (!extras.text.empty()) node.text = extras.text;
    for (const auto& [index, child] : extras.children)
      node.children.insert(node.children.begin() + std::min(index, (int)node.children.size()), child);
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

  XmlNode frame_write(const Writer& writer, const Frame& frame, ItemType type)
  {
    if (type == ItemType::TRIGGER)
    {
      XmlNode node{.tag = tag_get(ElementType::TRIGGER)};
      if (frame.eventId != -1) attribute_add(node, "EventId", frame.eventId);
      attribute_add(node, "AtFrame", frame.atFrame);
      if (writer.is(SERIALIZE_SOUNDS))
        for (auto soundId : frame.soundIds)
          if (soundId != -1)
            attribute_add(node.children.emplace_back(XmlNode{.tag = tag_get(ElementType::SOUND_ELEMENT)}), "Id",
                          soundId);
      extras_write(node, frame.extras);
      return node;
    }

    XmlNode node{.tag = tag_get(ElementType::FRAME)};
    if (type == ItemType::LAYER)
    {
      auto isRegion = writer.is(SERIALIZE_REGIONS) && frame.regionId != -1;
      if (isRegion) attribute_add(node, "RegionId", frame.regionId);
      if (writer.is(SERIALIZE_REDUNDANT_FRAME_REGION_VALUES) || !isRegion)
      {
        attribute_add(node, "XPivot", frame.pivot.x);
        attribute_add(node, "YPivot", frame.pivot.y);
        attribute_add(node, "XCrop", frame.crop.x);
        attribute_add(node, "YCrop", frame.crop.y);
        attribute_add(node, "Width", frame.size.x);
        attribute_add(node, "Height", frame.size.y);
      }
    }
    attribute_add(node, "XPosition", frame.position.x);
    attribute_add(node, "YPosition", frame.position.y);
    attribute_add(node, "Delay", frame.duration);
    attribute_add(node, "Visible", frame.isVisible);
    attribute_add(node, "XScale", frame.scale.x);
    attribute_add(node, "YScale", frame.scale.y);
    if (writer.is(SERIALIZE_EXTENSIONS))
    {
      attribute_add(node, "ShearX", frame.shear.x);
      attribute_add(node, "ShearY", frame.shear.y);
    }
    for (int i = 0; i < (int)std::size(TINT_ATTRIBUTES); ++i)
      attribute_add(node, TINT_ATTRIBUTES[i], math::float_to_uint8(frame.tint[i]));
    for (int i = 0; i < (int)std::size(OFFSET_ATTRIBUTES); ++i)
      attribute_add(node, OFFSET_ATTRIBUTES[i], math::float_to_uint8(frame.colorOffset[i]));
    attribute_add(node, "Rotation", frame.rotation);
    interpolation_add(node, "Interpolated", frame.interpolation);
    if (writer.is(SERIALIZE_EXTENSIONS) && frame.shaderId != -1) attribute_add(node, "ShaderId", frame.shaderId);
    extras_write(node, frame.extras);
    return node;
  }

  // A backup (FrameRestore) track keeps its frames' region ids as they are.
  XmlNode track_write(const Writer& writer, const Track& track, int groupId = -1, bool isBackup = false)
  {
    XmlNode node{.tag = tag_get(track_element_type_get(track.type))};
    if (track.type == ItemType::LAYER || track.type == ItemType::NULL_)
    {
      auto id = track.id;
      if (track.type == ItemType::LAYER && writer.model)
        if (auto it = writer.layerIds.find(id); it != writer.layerIds.end()) id = it->second;
      attribute_add(node, track.type == ItemType::LAYER ? "LayerId" : "NullId", id);
      attribute_add(node, "Visible", track.isVisible);
      if (groupId != -1 && is_group_id_written(writer)) attribute_add(node, "GroupId", groupId);
    }

    auto layerTrack = track.type == ItemType::LAYER && !isBackup ? &track : nullptr;
    for (int i = 0; i < (int)track.frames.size(); ++i)
    {
      auto frame = frame_normalize(writer, track.frames[i], layerTrack);
      if (!writer.is(SERIALIZE_BAKE_SPECIAL_INTERPOLATED_FRAMES) || track.type == ItemType::TRIGGER ||
          !is_special_interpolation(frame.interpolation))
      {
        node.children.push_back(frame_write(writer, frame, track.type));
        continue;
      }
      // Written for the game as single-step frames; the first records how to read them back as one.
      auto baked = frame_bake_split(frame, i + 1 < (int)track.frames.size() ? track.frames[i + 1] : frame,
                                    FRAME_DURATION_MIN, false, false);
      for (int bakeIndex = 0; bakeIndex < (int)baked.size(); ++bakeIndex)
      {
        auto& child = node.children.emplace_back(frame_write(writer, baked[bakeIndex], track.type));
        if (bakeIndex > 0) continue;
        attribute_add(child, "BakeInterpolation", INTERPOLATION_VALUES[(int)frame.interpolation]);
        attribute_add(child, "BakeDelay", (int)baked.size());
      }
    }
    extras_write(node, track.extras);
    return node;
  }

  template <class Group> XmlNode group_write(const Writer& writer, const Group& group)
  {
    XmlNode node{.tag = tag_get(ElementType::GROUP)};
    if (!writer.is(SERIALIZE_NESTED_GROUPS)) attribute_add(node, "Id", group.id);
    attribute_add(node, "Name", group.name);
    attribute_add(node, "IsExpanded", group.isExpanded);
    attribute_add(node, "Visible", group.isVisible);
    if constexpr (requires { group.root; }) node.children.push_back(track_write(writer, group.root));
    extras_write(node, group.extras);
    return node;
  }

  struct ContainerChild
  {
    XmlNode node{};
    int groupId{-1};
    std::optional<int> ownGroupId{};
    std::size_t knownCount{};
  };

  template <class Group> ContainerChild group_child_make(const Writer& writer, const Group& group)
  {
    auto node = group_write(writer, group);
    auto knownCount = node.attributes.size() - group.extras.attributes.size();
    return {.node = std::move(node), .ownGroupId = group.id, .knownCount = knownCount};
  }

  // Children are laid out flat (a group, then its children) and reshaped for the flags: nested into their groups, or,
  // in a track container, moved to a *AnimationGroups sibling that records each group's position as Index.
  XmlNode container_write(const Writer& writer, XmlNode container, std::vector<ContainerChild> flat,
                          const Extras& extras, XmlNode* groups = nullptr)
  {
    container.attributes.insert(container.attributes.end(), extras.attributes.begin(), extras.attributes.end());
    if (!extras.text.empty()) container.text = extras.text;
    for (const auto& [index, node] : extras.children)
      flat.insert(flat.begin() + std::min(index, (int)flat.size()), ContainerChild{.node = node});

    auto isGroups = writer.is(SERIALIZE_GROUPS);
    if (isGroups && writer.is(SERIALIZE_NESTED_GROUPS))
    {
      std::set<int> groupIds{};
      for (const auto& child : flat)
        if (child.ownGroupId && *child.ownGroupId >= 0) groupIds.insert(*child.ownGroupId);
      std::map<int, std::vector<XmlNode>> grouped{};
      for (const auto& child : flat)
        if (!child.ownGroupId && groupIds.contains(child.groupId)) grouped[child.groupId].push_back(child.node);
      std::erase_if(flat,
                    [&](const ContainerChild& child) { return !child.ownGroupId && groupIds.contains(child.groupId); });
      for (auto& child : flat)
        if (child.ownGroupId) std::ranges::copy(grouped[*child.ownGroupId], std::back_inserter(child.node.children));
    }

    auto isExtracted = groups && isGroups && !writer.is(SERIALIZE_NESTED_GROUPS);
    for (int i = 0; i < (int)flat.size(); ++i)
    {
      auto& child = flat[i];
      if (child.ownGroupId && !isGroups) continue;
      if (!child.ownGroupId || !isExtracted)
      {
        container.children.push_back(std::move(child.node));
        continue;
      }
      XmlNode index{};
      attribute_add(index, "Index", i);
      child.node.attributes.insert(child.node.attributes.begin() + child.knownCount, index.attributes.front());
      groups->children.push_back(std::move(child.node));
    }
    return container;
  }

  XmlNode track_entries_write(const Writer& writer, const std::vector<TrackEntry>& entries, ElementType type,
                              const Extras& extras, XmlNode& groups)
  {
    std::vector<ContainerChild> flat{};
    for (const auto& entry : entries)
      if (auto track = std::get_if<Track>(&entry))
        flat.push_back({.node = track_write(writer, *track)});
      else
      {
        auto& group = std::get<TrackGroup>(entry);
        flat.push_back(group_child_make(writer, group));
        for (const auto& groupTrack : group.tracks)
          flat.push_back({.node = track_write(writer, groupTrack, group.id), .groupId = group.id});
      }
    return container_write(writer, {.tag = tag_get(type)}, std::move(flat), extras, &groups);
  }

  bool is_frame_transform_default(const Frame& frame)
  {
    Frame base{};
    return frame.position == base.position && frame.scale == base.scale && frame.rotation == base.rotation &&
           frame.shear == base.shear && frame.tint == base.tint && frame.colorOffset == base.colorOffset;
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
                      XmlNode backup{.tag = std::string(FRAME_RESTORE_TAG)};
                      backup.children.push_back(track_write(writer, group.root));
                      for (auto& track : group.tracks)
                      {
                        backup.children.push_back(track_write(writer, track, group.id, true));
                        track_group_root_bake(track, group.root, animation.frameNum);
                      }
                      group.root = TrackGroup{}.root;
                      group.extras.children.emplace_back(1, std::move(backup));
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

  XmlNode animation_write(const Writer& writer, const Animation& source, int groupId = -1)
  {
    auto isBaked = writer.model &&
                   (writer.is(SERIALIZE_BAKE_GROUP_FRAMES) || writer.is(SERIALIZE_FLATTEN_SPECIAL_INTERPOLATED_FRAMES));
    auto baked = isBaked ? std::optional(animation_bake(writer, source)) : std::nullopt;
    const auto& animation = baked ? *baked : source;

    XmlNode node{.tag = tag_get(ElementType::ANIMATION)};
    attribute_add(node, "Name", animation.name);
    attribute_add(node, "FrameNum", animation.frameNum);
    attribute_add(node, "Loop", animation.isLoop);
    if (groupId != -1 && is_group_id_written(writer)) attribute_add(node, "GroupId", groupId);

    std::vector<std::pair<ElementType, XmlNode>> groups{};
    for (auto type : layout_get(animation.layout, {{ElementType::ROOT_ANIMATION, !animation.root.frames.empty()},
                                                   {ElementType::LAYER_ANIMATIONS, !animation.layers.empty()},
                                                   {ElementType::NULL_ANIMATIONS, !animation.nulls.empty()},
                                                   {ElementType::TRIGGERS, !animation.triggers.frames.empty()}}))
    {
      if (type == ElementType::ROOT_ANIMATION) node.children.push_back(track_write(writer, animation.root));
      if (type == ElementType::TRIGGERS) node.children.push_back(track_write(writer, animation.triggers));
      if (type != ElementType::LAYER_ANIMATIONS && type != ElementType::NULL_ANIMATIONS) continue;
      auto isLayers = type == ElementType::LAYER_ANIMATIONS;
      auto& [groupsType, groupsNode] = groups.emplace_back(
          type,
          XmlNode{.tag = tag_get(isLayers ? ElementType::LAYER_ANIMATION_GROUPS : ElementType::NULL_ANIMATION_GROUPS)});
      node.children.push_back(track_entries_write(writer, isLayers ? animation.layers : animation.nulls, type,
                                                  isLayers ? animation.layersExtras : animation.nullsExtras,
                                                  groupsNode));
    }
    extras_write(node, animation.extras);

    for (auto& [type, groupsNode] : groups)
    {
      if (groupsNode.children.empty()) continue;
      auto container = std::ranges::find(node.children, std::string(tag_get(type)), &XmlNode::tag);
      node.children.insert(container + 1, std::move(groupsNode));
    }
    return node;
  }

  XmlNode animations_write(const Writer& writer, const Animations& animations)
  {
    std::vector<ContainerChild> flat{};
    for (const auto& entry : animations.entries)
      if (auto animation = std::get_if<std::shared_ptr<const Animation>>(&entry))
        flat.push_back({.node = animation_write(writer, **animation)});
      else
      {
        auto& group = std::get<AnimationGroup>(entry);
        flat.push_back(group_child_make(writer, group));
        for (const auto& groupAnimation : group.animations)
          flat.push_back({.node = animation_write(writer, *groupAnimation, group.id), .groupId = group.id});
      }
    XmlNode node{.tag = tag_get(ElementType::ANIMATIONS)};
    attribute_add(node, "DefaultAnimation", animations.defaultAnimation);
    return container_write(writer, std::move(node), std::move(flat), animations.extras);
  }

  XmlNode item_write(const Writer&, const Region& region, int id)
  {
    XmlNode node{.tag = tag_get(ElementType::REGION)};
    attribute_add(node, "Id", id);
    attribute_add(node, "Name", region.name);
    attribute_add(node, "XCrop", region.crop.x);
    attribute_add(node, "YCrop", region.crop.y);
    attribute_add(node, "Width", region.size.x);
    attribute_add(node, "Height", region.size.y);
    if (region.origin != Origin::CUSTOM)
      attribute_add(node, "Origin", ORIGIN_VALUES[(int)region.origin]);
    else
    {
      attribute_add(node, "XPivot", region.pivot.x);
      attribute_add(node, "YPivot", region.pivot.y);
    }
    extras_write(node, region.extras);
    return node;
  }

  XmlNode item_write(const Writer& writer, const Spritesheet& spritesheet, int id)
  {
    XmlNode node{.tag = tag_get(ElementType::SPRITESHEET)};
    attribute_add(node, "Id", id);
    path_add(node, "Path", spritesheet.path);
    if (writer.is(SERIALIZE_REGIONS))
      for (int i = 0; i < (int)spritesheet.regions.size(); ++i)
        node.children.push_back(
            item_write(writer, spritesheet.regions[i], writer.model ? i : spritesheet.regions[i].id));
    extras_write(node, spritesheet.extras);
    return node;
  }

  void binding_value_add(XmlNode& node, const std::string& binding, const std::string& value)
  {
    if (!binding.empty()) attribute_add(node, "Binding", binding);
    if (!value.empty()) attribute_add(node, "Value", value);
  }

  XmlNode item_write(const Writer&, const Shader& shader, int id)
  {
    XmlNode node{.tag = tag_get(ElementType::SHADER)};
    attribute_add(node, "Id", id);
    attribute_add(node, "Name", shader.name);
    path_add(node, "Vertex", shader.vertex);
    path_add(node, "Fragment", shader.fragment);
    for (const auto& uniform : shader.uniforms)
    {
      auto& uniformNode = node.children.emplace_back(XmlNode{.tag = tag_get(ElementType::UNIFORM)});
      attribute_add(uniformNode, "Name", uniform.name);
      binding_value_add(uniformNode, uniform.binding, uniform.value);
      for (const auto& component : uniform.components)
      {
        auto& componentNode = uniformNode.children.emplace_back(XmlNode{.tag = tag_get(ElementType::COMPONENT)});
        attribute_add(componentNode, "Index", component.index);
        binding_value_add(componentNode, component.binding, component.value);
        extras_write(componentNode, component.extras);
      }
      extras_write(uniformNode, uniform.extras);
    }
    extras_write(node, shader.extras);
    return node;
  }

  XmlNode item_write(const Writer&, const Layer& layer, int id)
  {
    XmlNode node{.tag = tag_get(ElementType::LAYER_ELEMENT)};
    attribute_add(node, "Id", id);
    attribute_add(node, "Name", layer.name);
    attribute_add(node, "SpritesheetId", layer.spritesheetId);
    extras_write(node, layer.extras);
    return node;
  }

  XmlNode item_write(const Writer&, const Null& null, int id)
  {
    XmlNode node{.tag = tag_get(ElementType::NULL_ELEMENT)};
    attribute_add(node, "Id", id);
    attribute_add(node, "Name", null.name);
    if (null.isShowRect) attribute_add(node, "ShowRect", null.isShowRect);
    extras_write(node, null.extras);
    return node;
  }

  XmlNode item_write(const Writer&, const Event& event, int id)
  {
    XmlNode node{.tag = tag_get(ElementType::EVENT_ELEMENT)};
    attribute_add(node, "Id", id);
    attribute_add(node, "Name", event.name);
    extras_write(node, event.extras);
    return node;
  }

  XmlNode item_write(const Writer&, const Sound& sound, int id)
  {
    XmlNode node{.tag = tag_get(ElementType::SOUND_ELEMENT)};
    attribute_add(node, "Id", id);
    path_add(node, "Path", sound.path);
    extras_write(node, sound.extras);
    return node;
  }

  template <class Item>
  XmlNode items_write(const Writer& writer, const std::vector<Item>& items, ElementType type, const Content& content)
  {
    XmlNode node{.tag = tag_get(type)};
    auto isRenumbered = writer.model && (type == ElementType::LAYERS || type == ElementType::SHADERS);
    for (int i = 0; i < (int)items.size(); ++i)
      node.children.push_back(item_write(writer, items[i], isRenumbered ? i : items[i].id));
    for (const auto& [extrasType, extras] : content.containerExtras)
      if (extrasType == type) extras_write(node, extras);
    return node;
  }

  XmlNode content_write(const Writer& writer, const Content& content)
  {
    XmlNode node{.tag = tag_get(ElementType::CONTENT)};
    for (auto type : layout_get(content.layout, {{ElementType::SPRITESHEETS, !content.spritesheets.empty()},
                                                 {ElementType::SHADERS, !content.shaders.empty()},
                                                 {ElementType::LAYERS, !content.layers.empty()},
                                                 {ElementType::NULLS, !content.nulls.empty()},
                                                 {ElementType::EVENTS, !content.events.empty()},
                                                 {ElementType::SOUNDS, !content.sounds.empty()}}))
    {
      auto& container = node.children.emplace_back();
      if (type == ElementType::SPRITESHEETS) container = items_write(writer, content.spritesheets, type, content);
      if (type == ElementType::SHADERS) container = items_write(writer, content.shaders, type, content);
      if (type == ElementType::LAYERS) container = items_write(writer, content.layers, type, content);
      if (type == ElementType::NULLS) container = items_write(writer, content.nulls, type, content);
      if (type == ElementType::EVENTS) container = items_write(writer, content.events, type, content);
      if (type == ElementType::SOUNDS) container = items_write(writer, content.sounds, type, content);
      // Optional containers are left out when the flags drop them or they are empty; an emptied tag marks them so
      // extras still land at their recorded positions.
      auto isOptional = type == ElementType::SHADERS || type == ElementType::SOUNDS;
      auto isEnabled = writer.is(type == ElementType::SHADERS ? SERIALIZE_EXTENSIONS : SERIALIZE_SOUNDS);
      if (isOptional && (!isEnabled || container.children.empty())) container.tag.clear();
    }
    extras_write(node, content.extras);
    std::erase_if(node.children, [](const XmlNode& child) { return child.tag.empty(); });
    return node;
  }

  XmlNode model_write(const Model& model, Flags flags)
  {
    Writer writer(flags, &model);
    XmlNode node{.tag = tag_get(ElementType::ANIMATED_ACTOR)};
    auto isInfo = model.info.fps != Info{}.fps || model.info.createdBy != Info{}.createdBy;
    for (auto type : layout_get(model.layout, {{ElementType::INFO, isInfo},
                                               {ElementType::CONTENT, true},
                                               {ElementType::ANIMATIONS, !model.animations.entries.empty()}}))
    {
      if (type == ElementType::CONTENT) node.children.push_back(content_write(writer, model.content));
      if (type == ElementType::ANIMATIONS) node.children.push_back(animations_write(writer, model.animations));
      if (type != ElementType::INFO) continue;
      auto& info = node.children.emplace_back(XmlNode{.tag = tag_get(ElementType::INFO)});
      attribute_add(info, "CreatedBy", model.info.createdBy);
      attribute_add(info, "CreatedOn", model.info.createdOn);
      attribute_add(info, "Fps", model.info.fps);
      attribute_add(info, "Version", model.info.version);
      extras_write(info, model.info.extras);
    }
    extras_write(node, model.extras);
    return node;
  }

  // A save is the game's document with the editor's own copy embedded as SourceDocument (or just the editor's copy).
  XmlNode document_write(const Model& model, Options options)
  {
    auto editor = model_write(model, SERIALIZE_ANM2ED_DEFAULT);
    if (options.isExtendedFormat) return editor;
    auto game = model_write(model, SERIALIZE_ISAAC_DEFAULT);
    editor.tag = SOURCE_DOCUMENT_TAG;
    game.children.push_back(std::move(editor));
    return game;
  }

  XMLElement* element_write(XMLDocument& document, const XmlNode& node)
  {
    auto element = document.NewElement(node.tag.c_str());
    for (const auto& [name, value] : node.attributes)
      element->SetAttribute(name.c_str(), value.c_str());
    if (!node.text.empty()) element->SetText(node.text.c_str());
    for (const auto& child : node.children)
      element->InsertEndChild(element_write(document, child));
    return element;
  }

  std::string node_to_string(const XmlNode& node)
  {
    XMLDocument document{};
    document.InsertEndChild(element_write(document, node));
    return xml::document_to_string(document);
  }

  void node_hash(std::uint64_t& hash, const XmlNode& node)
  {
    auto add = [&](char marker, std::string_view value)
    {
      hash = (hash ^ (unsigned char)marker) * HASH_PRIME;
      for (unsigned char character : value)
        hash = (hash ^ character) * HASH_PRIME;
      hash *= HASH_PRIME;
    };
    add('E', node.tag);
    for (const auto& [name, value] : node.attributes)
    {
      add('A', name);
      add('V', value);
    }
    add('T', node.text);
    for (const auto& child : node.children)
      node_hash(hash, child);
    add(']', {});
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
    XMLDocument document{};
    document.InsertFirstChild(element_write(document, document_write(model, options)));
    File file(path, "wb");
    if (!file)
    {
      if (errorString) *errorString = "File permissions.";
      return false;
    }
    if (document.SaveFile(file.get()) == XML_SUCCESS) return true;
    if (errorString) *errorString = document.ErrorStr();
    return false;
  }

  std::string model_to_string(const Model& model, Options options)
  {
    return node_to_string(document_write(model, options));
  }

  std::string model_serialize(const Model& model, Flags flags) { return node_to_string(model_write(model, flags)); }

  // The hash of what the editor would save, so it changes exactly when a save would.
  std::uint64_t model_hash(const Model& model)
  {
    auto hash = HASH_OFFSET;
    node_hash(hash, model_write(model, SERIALIZE_ANM2ED_DEFAULT));
    return hash;
  }

  bool is_special_interpolated_frames(const Model& model)
  {
    for (auto [index, animation] : model.animations_get())
    {
      bool isSpecial{};
      animation_tracks_each(*animation,
                            [&](const Track& track)
                            {
                              for (const auto& frame : track.frames)
                                isSpecial |=
                                    track.type != ItemType::TRIGGER && is_special_interpolation(frame.interpolation);
                            });
      if (isSpecial) return true;
    }
    return false;
  }

  std::string frame_to_string(const Frame& frame, ItemType type)
  {
    return node_to_string(frame_write(Writer(CLIPBOARD_FLAGS), frame, type));
  }

  std::string animation_to_string(const Animation& animation, int groupId)
  {
    return node_to_string(animation_write(Writer(CLIPBOARD_FLAGS), animation, groupId));
  }

  std::string animation_group_to_string(const AnimationGroup& group)
  {
    return node_to_string(group_write(Writer(CLIPBOARD_FLAGS), group));
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
    return node_to_string(item_write(Writer(CLIPBOARD_FLAGS), item, item.id));
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
