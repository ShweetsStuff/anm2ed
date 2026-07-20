#include "anm2.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <unordered_map>

#include "file.hpp"
#include "math.hpp"
#include "path.hpp"
#include "time.hpp"
#include "working_directory.hpp"
#include "xml.hpp"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed
{
#define ANM2_STRING_ATTRIBUTES                                                                                         \
  X("Name", name)                                                                                                      \
  X("CreatedBy", createdBy)                                                                                            \
  X("CreatedOn", createdOn)                                                                                            \
  X("Binding", binding)                                                                                                \
  X("Value", value)                                                                                                    \
  X("DefaultAnimation", defaultAnimation)

#define ANM2_PATH_ATTRIBUTES                                                                                           \
  X("Path", path)                                                                                                      \
  X("Vertex", vertex)                                                                                                  \
  X("Fragment", fragment)

#define ANM2_INT_ATTRIBUTES                                                                                            \
  X("Id", id)                                                                                                          \
  X("LayerId", layerId)                                                                                                \
  X("NullId", nullId)                                                                                                  \
  X("SpritesheetId", spritesheetId)                                                                                    \
  X("Fps", fps)                                                                                                        \
  X("Version", version)                                                                                                \
  X("FrameNum", frameNum)                                                                                              \
  X("Delay", duration)                                                                                                 \
  X("AtFrame", atFrame)                                                                                                \
  X("EventId", eventId)                                                                                                \
  X("RegionId", regionId)                                                                                              \
  X("SoundId", soundId)                                                                                                \
  X("GroupId", groupId)                                                                                                \
  X("Index", index)

#define ANM2_BOOL_ATTRIBUTES                                                                                           \
  X("Loop", isLoop)                                                                                                    \
  X("Visible", isVisible)                                                                                              \
  X("ShowRect", isShowRect)                                                                                            \
  X("IsExpanded", isExpanded)                                                                                          \
  X("Enabled", isEnabled)

#define ANM2_FLOAT_ATTRIBUTES                                                                                          \
  X("Rotation", rotation)                                                                                              \
  X("XPivot", pivot.x)                                                                                                 \
  X("YPivot", pivot.y)                                                                                                 \
  X("XCrop", crop.x)                                                                                                   \
  X("YCrop", crop.y)                                                                                                   \
  X("XPosition", position.x)                                                                                           \
  X("YPosition", position.y)                                                                                           \
  X("Width", size.x)                                                                                                   \
  X("Height", size.y)                                                                                                  \
  X("XScale", scale.x)                                                                                                 \
  X("YScale", scale.y)

#define ANM2_COLOR_ATTRIBUTES                                                                                          \
  X("RedTint", tint.r)                                                                                                 \
  X("GreenTint", tint.g)                                                                                               \
  X("BlueTint", tint.b)                                                                                                \
  X("AlphaTint", tint.a)                                                                                               \
  X("RedOffset", colorOffset.r)                                                                                        \
  X("GreenOffset", colorOffset.g)                                                                                      \
  X("BlueOffset", colorOffset.b)

  constexpr std::string_view ANIMATION_MERGED_SUFFIX = " (Merged)";

  constexpr std::array<std::string_view, (std::size_t)ElementType::COUNT> ELEMENT_TAGS = {
#define X(symbol, tag) std::string_view{tag},
      ANM2_ELEMENT_TYPES
#undef X
  };

  constexpr std::array<std::string_view, (std::size_t)Interpolation::COUNT> INTERPOLATION_VALUES = {
      "", "", "EaseIn", "EaseOut", "EaseInOut"};

  constexpr std::array<std::string_view, (std::size_t)Origin::COUNT> ORIGIN_VALUES = {"", "TopLeft", "Center"};
  constexpr std::array<std::string_view, (std::size_t)OverlayDrawOrder::COUNT> OVERLAY_DRAW_ORDER_VALUES = {"Below",
                                                                                                            "Above"};
  constexpr std::string_view ANM2ED_GROUP_FRAME_BACKUP_TAG = "FrameRestore";
  constexpr std::string_view SOURCE_DOCUMENT_TAG = "SourceDocument";

  void group_metadata_embed(Element&);
  void group_metadata_extract(Element&, Flags);
  void groups_flatten(Element&);
  void groups_nest(Element&);
  void group_frames_restore(Element&);
  void source_document_erase(Element&);
  void region_ids_remap(Element&);
  void region_frame_ids_repair(Element&);
  Element* child_first_get(Element&, ElementType);
  const Element* child_first_get(const Element&, ElementType);
  bool is_track(const Element&);
  bool is_track_child_valid(ElementType, ElementType);
  float interpolation_factor(Interpolation, float);

  int animations_count_get(const Element& animations)
  {
    int count{};
    for (const auto& child : animations.children)
      if (child.type == ElementType::ANIMATION) ++count;
    return count;
  }

  int animations_child_index_get(const Element& animations, int animationIndex)
  {
    int current{};
    for (int i = 0; i < (int)animations.children.size(); ++i)
    {
      if (animations.children[i].type != ElementType::ANIMATION) continue;
      if (current == animationIndex) return i;
      ++current;
    }
    return -1;
  }

  int animations_child_insert_index_get(const Element& animations, int animationIndex)
  {
    if (animationIndex <= 0)
    {
      for (int i = 0; i < (int)animations.children.size(); ++i)
        if (animations.children[i].type == ElementType::ANIMATION) return i;
      return (int)animations.children.size();
    }
    int current{};
    for (int i = 0; i < (int)animations.children.size(); ++i)
    {
      if (animations.children[i].type != ElementType::ANIMATION) continue;
      if (current == animationIndex) return i;
      ++current;
    }
    return (int)animations.children.size();
  }

  bool is_source_document_tag(std::string_view tag)
  {
    return tag == SOURCE_DOCUMENT_TAG;
  }

  const XMLElement* source_document_get(const XMLElement* rootElement)
  {
    if (!rootElement) return nullptr;
    if (is_source_document_tag(rootElement->Name() ? rootElement->Name() : "")) return rootElement;

    for (auto child = rootElement->FirstChildElement(); child; child = child->NextSiblingElement())
      if (is_source_document_tag(child->Name() ? child->Name() : "")) return child;

    return nullptr;
  }

  bool anm2_document_load(Anm2& anm2, XMLDocument& document, std::string* errorString)
  {
    auto rootElement = document.RootElement();
    if (!rootElement)
    {
      if (errorString) *errorString = "No root element.";
      anm2.isValid = false;
      return false;
    }

    auto sourceElement = source_document_get(rootElement);
    anm2.root = element_read(sourceElement ? sourceElement : rootElement);
    if (sourceElement)
    {
      anm2.root.type = ElementType::ANIMATED_ACTOR;
      anm2.root.tag = "AnimatedActor";
    }
    group_metadata_embed(anm2.root);
    group_frames_restore(anm2.root);
    groups_flatten(anm2.root);
    region_frame_ids_repair(anm2.root);
    anm2.region_frames_sync(true);
    anm2.isValid = true;
    return true;
  }

  ElementType element_type_get(std::string_view tag)
  {
    for (std::size_t i = 0; i < ELEMENT_TAGS.size(); ++i)
      if (ELEMENT_TAGS[i] == tag) return (ElementType)i;
    return ElementType::UNKNOWN;
  }

  std::string_view element_tag_get(ElementType type)
  {
    auto index = (std::size_t)type;
    if (index >= ELEMENT_TAGS.size()) return {};
    return ELEMENT_TAGS[index];
  }

  ElementType element_container_type_get(ElementType type)
  {
    switch (type)
    {
      case ElementType::SPRITESHEET:
        return ElementType::SPRITESHEETS;
      case ElementType::LAYER_ELEMENT:
        return ElementType::LAYERS;
      case ElementType::NULL_ELEMENT:
        return ElementType::NULLS;
      case ElementType::EVENT_ELEMENT:
        return ElementType::EVENTS;
      case ElementType::SOUND_ELEMENT:
        return ElementType::SOUNDS;
      case ElementType::ANIMATION:
        return ElementType::ANIMATIONS;
      default:
        return ElementType::UNKNOWN;
    }
  }

  Element element_make(ElementType type)
  {
    Element element{};
    element.type = type;
    element.tag = std::string(element_tag_get(type));
    if (type == ElementType::OVERLAY) element.index = OVERLAY_DRAW_ORDER_ABOVE;
    return element;
  }

  Element root_animation_make()
  {
    auto root = element_make(ElementType::ROOT_ANIMATION);
    root.children.push_back(element_make(ElementType::FRAME));
    return root;
  }

  bool is_frame_transform_default(const Element& frame)
  {
    auto base = element_make(ElementType::FRAME);
    return frame.position == base.position && frame.scale == base.scale && frame.rotation == base.rotation &&
           frame.tint == base.tint && frame.colorOffset == base.colorOffset;
  }

  bool is_root_animation_transform_default(const Element& root)
  {
    for (const auto& frame : root.children)
      if (frame.type == ElementType::FRAME && !is_frame_transform_default(frame)) return false;
    return true;
  }

  bool is_group_frame_backup(const Element& element)
  {
    return element.type == ElementType::UNKNOWN && element.tag == ANM2ED_GROUP_FRAME_BACKUP_TAG;
  }

  void group_frame_backups_erase(Element& group)
  {
    std::erase_if(group.children, [](const Element& child) { return is_group_frame_backup(child); });
  }

  Element group_frame_backup_make(const Element& container, const Element& group, ElementType trackType)
  {
    Element backup{};
    backup.type = ElementType::UNKNOWN;
    backup.tag = std::string(ANM2ED_GROUP_FRAME_BACKUP_TAG);

    if (auto root = child_first_get(group, ElementType::ROOT_ANIMATION)) backup.children.push_back(*root);

    for (const auto& item : container.children)
      if (item.type == trackType && item.groupId == group.id) backup.children.push_back(item);

    return backup;
  }

  void frame_group_root_apply(Element& frame, const Element& rootFrame)
  {
    auto rootScale = math::percent_to_unit(rootFrame.scale);
    auto offset = frame.position * rootScale;
    auto radians = glm::radians(rootFrame.rotation);
    auto cos = std::cos(radians);
    auto sin = std::sin(radians);

    frame.position = rootFrame.position + glm::vec2(offset.x * cos - offset.y * sin, offset.x * sin + offset.y * cos);
    frame.scale *= rootScale;
    frame.rotation += rootFrame.rotation;
    frame.tint *= rootFrame.tint;
    frame.colorOffset += rootFrame.colorOffset;
  }

  void track_group_root_bake(Element& track, const Element& root, int frameNum)
  {
    auto trackLength = track_length_get(track);
    if (trackLength <= 0) return;

    auto length = std::max({trackLength, track_length_get(root), frameNum, FRAME_DURATION_MIN});
    std::vector<Element> frames{};
    frames.reserve(length);

    for (int time = 0; time < length; ++time)
    {
      auto frame = frame_generate(track, (float)time);
      auto rootFrame = frame_generate(root, (float)time);
      frame_group_root_apply(frame, rootFrame);
      frame.duration = FRAME_DURATION_MIN;
      frame.interpolation = Interpolation::NONE;
      frames.push_back(std::move(frame));
    }

    track.children = std::move(frames);
  }

  void group_frames_bake(Element& container, ElementType trackType, int frameNum)
  {
    for (auto& group : container.children)
    {
      if (group.type != ElementType::GROUP) continue;
      group_frame_backups_erase(group);
      auto root = child_first_get(group, ElementType::ROOT_ANIMATION);
      if (!root || is_root_animation_transform_default(*root)) continue;

      auto backup = group_frame_backup_make(container, group, trackType);
      for (auto& item : container.children)
        if (item.type == trackType && item.groupId == group.id) track_group_root_bake(item, *root, frameNum);

      *root = root_animation_make();
      group.children.push_back(std::move(backup));
    }
  }

  void group_frames_bake(Element& element)
  {
    if (element.type == ElementType::ANIMATION)
    {
      if (auto layerAnimations = child_first_get(element, ElementType::LAYER_ANIMATIONS))
        group_frames_bake(*layerAnimations, ElementType::LAYER_ANIMATION, element.frameNum);
      if (auto nullAnimations = child_first_get(element, ElementType::NULL_ANIMATIONS))
        group_frames_bake(*nullAnimations, ElementType::NULL_ANIMATION, element.frameNum);
    }

    for (auto& child : element.children)
      group_frames_bake(child);
  }

  void group_frames_restore(Element& container, ElementType trackType)
  {
    std::set<int> restoredGroupIds{};

    for (auto& group : container.children)
    {
      if (group.type != ElementType::GROUP) continue;
      if (group.id < 0) continue;

      const Element* backup{};
      for (const auto& child : group.children)
        if (is_group_frame_backup(child))
        {
          backup = &child;
          break;
        }

      if (!backup) continue;

      auto root = root_animation_make();
      if (auto currentRoot = child_first_get(group, ElementType::ROOT_ANIMATION)) root = *currentRoot;

      std::vector<Element> restoredTracks{};
      bool isRootRestored{};
      for (const auto& child : backup->children)
      {
        if (child.type == ElementType::ROOT_ANIMATION && !isRootRestored)
        {
          root = child;
          isRootRestored = true;
        }
        else if (child.type == trackType)
        {
          auto track = child;
          track.groupId = group.id;
          restoredTracks.push_back(std::move(track));
        }
      }

      if (!isRootRestored && restoredTracks.empty()) continue;

      group.children = {std::move(root)};
      group.children.insert(group.children.end(), std::make_move_iterator(restoredTracks.begin()),
                            std::make_move_iterator(restoredTracks.end()));
      restoredGroupIds.insert(group.id);
    }

    if (restoredGroupIds.empty()) return;

    std::erase_if(container.children, [&](const Element& item)
                  { return item.type == trackType && restoredGroupIds.contains(item.groupId); });
  }

  void group_frames_restore(Element& element)
  {
    if (element.type == ElementType::ANIMATION)
    {
      if (auto layerAnimations = child_first_get(element, ElementType::LAYER_ANIMATIONS))
        group_frames_restore(*layerAnimations, ElementType::LAYER_ANIMATION);
      if (auto nullAnimations = child_first_get(element, ElementType::NULL_ANIMATIONS))
        group_frames_restore(*nullAnimations, ElementType::NULL_ANIMATION);
    }

    for (auto& child : element.children)
      group_frames_restore(child);
  }

  bool string_query(const XMLElement* element, const char* name, std::string& out)
  {
    if (!element) return false;
    if (auto value = element->Attribute(name); value)
    {
      out = value;
      return true;
    }
    return false;
  }

  bool path_query(const XMLElement* element, const char* name, std::filesystem::path& out)
  {
    std::string value{};
    if (!string_query(element, name, value)) return false;
    out = path::from_utf8(value);
    return true;
  }

  void path_set(XMLElement* element, const char* name, const std::filesystem::path& value)
  {
    if (!element || value.empty()) return;
    auto valueUtf8 = path::to_utf8(value);
    element->SetAttribute(name, valueUtf8.c_str());
  }

  float color_read(const XMLElement* element, const char* name, float fallback)
  {
    int value{};
    if (!element || element->QueryIntAttribute(name, &value) != XML_SUCCESS) return fallback;
    return math::uint8_to_float(value);
  }

  int color_write(float value) { return math::float_to_uint8(value); }

  int overlay_draw_order_get(const Element& element)
  {
    return element.index == OVERLAY_DRAW_ORDER_BELOW ? OVERLAY_DRAW_ORDER_BELOW : OVERLAY_DRAW_ORDER_ABOVE;
  }

  void overlay_draw_order_read(Element& out, const XMLElement* element)
  {
    if (out.type != ElementType::OVERLAY) return;
    out.index = OVERLAY_DRAW_ORDER_ABOVE;
    if (element)
    {
      element->QueryFloatAttribute("XOffset", &out.position.x);
      element->QueryFloatAttribute("YOffset", &out.position.y);
    }
    out.tint.a = color_read(element, "Alpha", out.tint.a);

    auto value = element ? element->Attribute("DrawOrder") : nullptr;
    if (!value) return;

    for (std::size_t i = 0; i < OVERLAY_DRAW_ORDER_VALUES.size(); ++i)
      if (OVERLAY_DRAW_ORDER_VALUES[i] == value) out.index = (int)i;
  }

  Interpolation interpolation_read(const XMLElement* element, const char* attribute)
  {
    bool isFallback{};
    if (element) element->QueryBoolAttribute(attribute, &isFallback);

    auto value = element ? element->Attribute(attribute) : nullptr;
    if (!value) return Interpolation::NONE;

    for (std::size_t i = 0; i < INTERPOLATION_VALUES.size(); ++i)
      if (!INTERPOLATION_VALUES[i].empty() && INTERPOLATION_VALUES[i] == value) return (Interpolation)i;

    return isFallback ? Interpolation::LINEAR : Interpolation::NONE;
  }

  Interpolation interpolation_read(const XMLElement* element) { return interpolation_read(element, "Interpolated"); }

  void interpolation_write(XMLElement* element, Interpolation interpolation)
  {
    if (interpolation == Interpolation::NONE || interpolation == Interpolation::LINEAR)
    {
      element->SetAttribute("Interpolated", interpolation == Interpolation::LINEAR);
      return;
    }

    auto value = INTERPOLATION_VALUES[(std::size_t)interpolation];
    if (!value.empty()) element->SetAttribute("Interpolated", value.data());
  }

  Origin origin_read(const XMLElement* element)
  {
    auto value = element ? element->Attribute("Origin") : nullptr;
    if (!value) return Origin::CUSTOM;

    for (std::size_t i = 0; i < ORIGIN_VALUES.size(); ++i)
      if (!ORIGIN_VALUES[i].empty() && ORIGIN_VALUES[i] == value) return (Origin)i;

    return Origin::CUSTOM;
  }

  void origin_write(XMLElement* out, const Element& element)
  {
    auto origin = ORIGIN_VALUES[(std::size_t)element.origin];
    if (!origin.empty())
    {
      out->SetAttribute("Origin", origin.data());
      return;
    }

    out->SetAttribute("XPivot", element.pivot.x);
    out->SetAttribute("YPivot", element.pivot.y);
  }

  void element_attributes_read(Element& out, const XMLElement* element)
  {
#define X(attribute, member) string_query(element, attribute, out.member);
    ANM2_STRING_ATTRIBUTES
#undef X
#define X(attribute, member) path_query(element, attribute, out.member);
    ANM2_PATH_ATTRIBUTES
#undef X
#define X(attribute, member) element->QueryIntAttribute(attribute, &out.member);
    ANM2_INT_ATTRIBUTES
#undef X
#define X(attribute, member) element->QueryBoolAttribute(attribute, &out.member);
    ANM2_BOOL_ATTRIBUTES
#undef X
#define X(attribute, member) element->QueryFloatAttribute(attribute, &out.member);
    ANM2_FLOAT_ATTRIBUTES
#undef X
#define X(attribute, member) out.member = color_read(element, attribute, out.member);
    ANM2_COLOR_ATTRIBUTES
#undef X

    out.interpolation = interpolation_read(element);
    out.origin = origin_read(element);
    overlay_draw_order_read(out, element);
    if (out.type == ElementType::REGION && out.origin == Origin::TOP_LEFT) out.pivot = {};
    if (out.type == ElementType::REGION && out.origin == Origin::CENTER) out.pivot = out.size * 0.5f;
  }

  bool bake_attributes_read(const XMLElement* element, Interpolation& interpolation, int& bakeDelay)
  {
    if (!element || !element->Attribute("BakeInterpolation")) return false;

    interpolation = interpolation_read(element, "BakeInterpolation");
    element->QueryIntAttribute("BakeDelay", &bakeDelay);
    return bakeDelay >= FRAME_DURATION_MIN;
  }

  const XMLElement* bake_frames_skip(const XMLElement* element, int bakeDelay)
  {
    for (int i = 0; element && i < bakeDelay; ++i)
      element = element->NextSiblingElement();
    return element;
  }

  void element_children_read(Element& out, const XMLElement* element)
  {
    auto child = element->FirstChildElement();
    while (child)
    {
      auto childType = element_type_get(child->Name() ? child->Name() : "");
      if (childType == ElementType::FRAME && is_track(out))
      {
        Interpolation bakeInterpolation{};
        int bakeDelay{};
        if (bake_attributes_read(child, bakeInterpolation, bakeDelay))
        {
          auto restored = element_read(child);
          restored.interpolation = bakeInterpolation;
          restored.duration = bakeDelay;
          if (is_track_child_valid(out.type, restored.type)) out.children.push_back(std::move(restored));
          child = bake_frames_skip(child, bakeDelay);
          continue;
        }
      }

      auto childElement = element_read(child);
      if (is_track_child_valid(out.type, childElement.type)) out.children.push_back(std::move(childElement));
      child = child->NextSiblingElement();
    }
  }

  Element element_read(const XMLElement* element)
  {
    Element out{};
    if (!element) return out;

    out.tag = element->Name() ? element->Name() : "";
    out.type = element_type_get(out.tag);
    element_attributes_read(out, element);

    element_children_read(out, element);

    if (out.type == ElementType::TRIGGER && out.soundId != -1)
    {
      bool isSoundFound{};
      for (const auto& child : out.children)
        if (child.type == ElementType::SOUND_ELEMENT && child.id == out.soundId) isSoundFound = true;

      if (!isSoundFound)
      {
        auto sound = element_make(ElementType::SOUND_ELEMENT);
        sound.id = out.soundId;
        out.children.insert(out.children.begin(), sound);
      }
    }

    if (out.type == ElementType::TRIGGER)
      for (const auto& child : out.children)
        if (child.type == ElementType::SOUND_ELEMENT && child.id != -1) out.soundIds.push_back(child.id);

    return out;
  }

  bool is_nested_group_parent(ElementType parentType, Flags flags)
  {
    return has_flag(flags, SERIALIZE_NESTED_GROUPS) &&
           (parentType == ElementType::ANIMATIONS || parentType == ElementType::LAYER_ANIMATIONS ||
            parentType == ElementType::NULL_ANIMATIONS);
  }

  bool is_group_id_serialized(Flags flags)
  {
    return has_flag(flags, SERIALIZE_GROUPS) && !has_flag(flags, SERIALIZE_NESTED_GROUPS);
  }

  bool element_write_skip(const Element& element, ElementType parentType, Flags flags)
  {
    if (!is_track_child_valid(parentType, element.type)) return true;
    if ((element.type == ElementType::OVERLAY || element.type == ElementType::SHADER ||
         element.type == ElementType::UNIFORM || element.type == ElementType::COMPONENT) &&
        !has_flag(flags, SERIALIZE_EXTENSIONS))
      return true;
    if (element.type == ElementType::SOUNDS && (!has_flag(flags, SERIALIZE_SOUNDS) || element.children.empty()))
      return true;
    if ((element.type == ElementType::LAYER_ANIMATION_GROUPS || element.type == ElementType::NULL_ANIMATION_GROUPS) &&
        (!has_flag(flags, SERIALIZE_GROUPS) || element.children.empty()))
      return true;
    if (element.type == ElementType::SOUND_ELEMENT && parentType == ElementType::TRIGGER) return true;
    if (element.type == ElementType::REGION && !has_flag(flags, SERIALIZE_REGIONS)) return true;
    if (element.type == ElementType::GROUP && !has_flag(flags, SERIALIZE_GROUPS)) return true;
    return false;
  }

  void frame_attributes_write(XMLElement* out, const Element& element, ElementType parentType, Flags flags)
  {
    if (parentType == ElementType::LAYER_ANIMATION)
    {
      bool isHasValidRegion = has_flag(flags, SERIALIZE_REGIONS) && element.regionId != -1;
      bool isWriteRegionValues = has_flag(flags, SERIALIZE_REDUNDANT_FRAME_REGION_VALUES) || !isHasValidRegion;

      if (isHasValidRegion) out->SetAttribute("RegionId", element.regionId);
      if (isWriteRegionValues)
      {
        out->SetAttribute("XPivot", element.pivot.x);
        out->SetAttribute("YPivot", element.pivot.y);
        out->SetAttribute("XCrop", element.crop.x);
        out->SetAttribute("YCrop", element.crop.y);
        out->SetAttribute("Width", element.size.x);
        out->SetAttribute("Height", element.size.y);
      }
    }

    out->SetAttribute("XPosition", element.position.x);
    out->SetAttribute("YPosition", element.position.y);
    out->SetAttribute("Delay", element.duration);
    out->SetAttribute("Visible", element.isVisible);
    out->SetAttribute("XScale", element.scale.x);
    out->SetAttribute("YScale", element.scale.y);
    out->SetAttribute("RedTint", color_write(element.tint.r));
    out->SetAttribute("GreenTint", color_write(element.tint.g));
    out->SetAttribute("BlueTint", color_write(element.tint.b));
    out->SetAttribute("AlphaTint", color_write(element.tint.a));
    out->SetAttribute("RedOffset", color_write(element.colorOffset.r));
    out->SetAttribute("GreenOffset", color_write(element.colorOffset.g));
    out->SetAttribute("BlueOffset", color_write(element.colorOffset.b));
    out->SetAttribute("Rotation", element.rotation);
    interpolation_write(out, element.interpolation);
  }

  void element_attributes_write(XMLElement* out, const Element& element, ElementType parentType, Flags flags)
  {
    if (element.type == ElementType::INFO)
    {
      out->SetAttribute("CreatedBy", element.createdBy.c_str());
      out->SetAttribute("CreatedOn", element.createdOn.c_str());
      out->SetAttribute("Fps", element.fps);
      out->SetAttribute("Version", element.version);
    }
    else if (element.type == ElementType::ANIMATIONS)
      out->SetAttribute("DefaultAnimation", element.defaultAnimation.c_str());
    else if (element.type == ElementType::SPRITESHEET || element.type == ElementType::OVERLAY)
    {
      out->SetAttribute("Id", element.id);
      path_set(out, "Path", element.path);
      if (element.type == ElementType::OVERLAY)
      {
        if (!element.isVisible) out->SetAttribute("Visible", element.isVisible);
        if (element.position != glm::vec2())
        {
          out->SetAttribute("XOffset", element.position.x);
          out->SetAttribute("YOffset", element.position.y);
        }
        if (color_write(element.tint.a) != 255) out->SetAttribute("Alpha", color_write(element.tint.a));
        if (overlay_draw_order_get(element) == OVERLAY_DRAW_ORDER_BELOW)
          out->SetAttribute("DrawOrder", OVERLAY_DRAW_ORDER_VALUES[OVERLAY_DRAW_ORDER_BELOW].data());
      }
    }
    else if (element.type == ElementType::SHADER)
    {
      if (!element.isEnabled) out->SetAttribute("Enabled", element.isEnabled);
      path_set(out, "Vertex", element.vertex);
      path_set(out, "Fragment", element.fragment);
    }
    else if (element.type == ElementType::UNIFORM)
    {
      out->SetAttribute("Name", element.name.c_str());
      if (!element.binding.empty()) out->SetAttribute("Binding", element.binding.c_str());
      if (!element.value.empty()) out->SetAttribute("Value", element.value.c_str());
    }
    else if (element.type == ElementType::COMPONENT)
    {
      out->SetAttribute("Index", element.index);
      if (!element.binding.empty()) out->SetAttribute("Binding", element.binding.c_str());
      if (!element.value.empty()) out->SetAttribute("Value", element.value.c_str());
    }
    else if (element.type == ElementType::REGION)
    {
      out->SetAttribute("Id", element.id);
      out->SetAttribute("Name", element.name.c_str());
      out->SetAttribute("XCrop", element.crop.x);
      out->SetAttribute("YCrop", element.crop.y);
      out->SetAttribute("Width", element.size.x);
      out->SetAttribute("Height", element.size.y);
      origin_write(out, element);
    }
    else if (element.type == ElementType::LAYER_ELEMENT)
    {
      out->SetAttribute("Id", element.id);
      out->SetAttribute("Name", element.name.c_str());
      out->SetAttribute("SpritesheetId", element.spritesheetId);
    }
    else if (element.type == ElementType::NULL_ELEMENT)
    {
      out->SetAttribute("Id", element.id);
      out->SetAttribute("Name", element.name.c_str());
      if (element.isShowRect) out->SetAttribute("ShowRect", element.isShowRect);
    }
    else if (element.type == ElementType::GROUP)
    {
      if (!is_nested_group_parent(parentType, flags)) out->SetAttribute("Id", element.id);
      out->SetAttribute("Name", element.name.c_str());
      out->SetAttribute("IsExpanded", element.isExpanded);
      out->SetAttribute("Visible", element.isVisible);
      if ((parentType == ElementType::LAYER_ANIMATION_GROUPS || parentType == ElementType::NULL_ANIMATION_GROUPS) &&
          element.index != -1)
        out->SetAttribute("Index", element.index);
    }
    else if (element.type == ElementType::EVENT_ELEMENT)
    {
      out->SetAttribute("Id", element.id);
      out->SetAttribute("Name", element.name.c_str());
    }
    else if (element.type == ElementType::SOUND_ELEMENT)
    {
      out->SetAttribute("Id", element.id);
      if (parentType != ElementType::TRIGGER) path_set(out, "Path", element.path);
    }
    else if (element.type == ElementType::ANIMATION)
    {
      out->SetAttribute("Name", element.name.c_str());
      out->SetAttribute("FrameNum", element.frameNum);
      out->SetAttribute("Loop", element.isLoop);
      if (element.groupId != -1 && is_group_id_serialized(flags)) out->SetAttribute("GroupId", element.groupId);
    }
    else if (element.type == ElementType::LAYER_ANIMATION)
    {
      out->SetAttribute("LayerId", element.layerId);
      out->SetAttribute("Visible", element.isVisible);
      if (element.groupId != -1 && is_group_id_serialized(flags)) out->SetAttribute("GroupId", element.groupId);
    }
    else if (element.type == ElementType::NULL_ANIMATION)
    {
      out->SetAttribute("NullId", element.nullId);
      out->SetAttribute("Visible", element.isVisible);
      if (element.groupId != -1 && is_group_id_serialized(flags)) out->SetAttribute("GroupId", element.groupId);
    }
    else if (element.type == ElementType::FRAME)
      frame_attributes_write(out, element, parentType, flags);
    else if (element.type == ElementType::TRIGGER)
    {
      if (element.eventId != -1) out->SetAttribute("EventId", element.eventId);
      out->SetAttribute("AtFrame", element.atFrame);
    }
  }

  void bake_attributes_write(XMLElement* out, Interpolation interpolation, int bakeDelay)
  {
    auto value = INTERPOLATION_VALUES[(std::size_t)interpolation];
    if (!value.empty()) out->SetAttribute("BakeInterpolation", value.data());
    out->SetAttribute("BakeDelay", bakeDelay);
  }

  bool is_frame_bake_serialized(const Element& frame, Flags flags)
  {
    return has_flag(flags, SERIALIZE_BAKE_SPECIAL_INTERPOLATED_FRAMES) && frame.type == ElementType::FRAME &&
           frame.interpolation != Interpolation::NONE && frame.interpolation != Interpolation::LINEAR;
  }

  XMLElement* element_to_xml(XMLDocument& document, const Element& element, ElementType parentType, Flags flags);

  void baked_frames_insert(XMLDocument& document, XMLElement* out, const Element& track, int index, Flags flags)
  {
    const auto& original = track.children[index];
    auto nextFrame = index + 1 < (int)track.children.size() && track.children[index + 1].type == ElementType::FRAME
                         ? track.children[index + 1]
                         : original;
    auto bakeDelay = std::max(original.duration, FRAME_DURATION_MIN);

    for (int bakeIndex = 0; bakeIndex < bakeDelay; ++bakeIndex)
    {
      auto baked = original;
      auto amount = interpolation_factor(original.interpolation, (float)bakeIndex / (float)bakeDelay);
      baked.duration = FRAME_DURATION_MIN;
      baked.interpolation = Interpolation::NONE;
      baked.rotation = glm::mix(original.rotation, nextFrame.rotation, amount);
      baked.position = glm::mix(original.position, nextFrame.position, amount);
      baked.scale = glm::mix(original.scale, nextFrame.scale, amount);
      baked.colorOffset = glm::mix(original.colorOffset, nextFrame.colorOffset, amount);
      baked.tint = glm::mix(original.tint, nextFrame.tint, amount);
      auto frame = element_to_xml(document, baked, track.type, flags);
      if (bakeIndex == 0) bake_attributes_write(frame, original.interpolation, bakeDelay);
      out->InsertEndChild(frame);
    }
  }

  void trigger_sounds_xml_insert(XMLDocument& document, XMLElement* out, const Element& element, Flags flags)
  {
    if (element.type != ElementType::TRIGGER || !has_flag(flags, SERIALIZE_SOUNDS)) return;

    for (auto soundId : element.soundIds)
    {
      if (soundId == -1) continue;
      auto sound = document.NewElement(element_tag_get(ElementType::SOUND_ELEMENT).data());
      sound->SetAttribute("Id", soundId);
      out->InsertEndChild(sound);
    }
  }

  XMLElement* element_to_xml(XMLDocument& document, const Element& element, ElementType parentType, Flags flags)
  {
    auto tag = element.type == ElementType::UNKNOWN ? std::string_view(element.tag) : element_tag_get(element.type);
    auto out = document.NewElement(tag.empty() ? element.tag.c_str() : tag.data());
    element_attributes_write(out, element, parentType, flags);
    trigger_sounds_xml_insert(document, out, element, flags);

    for (int i = 0; i < (int)element.children.size(); ++i)
    {
      const auto& child = element.children[i];
      if (element_write_skip(child, element.type, flags)) continue;

      if (is_track(element) && is_frame_bake_serialized(child, flags))
        baked_frames_insert(document, out, element, i, flags);
      else
        out->InsertEndChild(element_to_xml(document, child, element.type, flags));
    }

    return out;
  }

  XMLElement* element_to_xml(XMLDocument& document, const Element& element, Flags flags)
  {
    return element_to_xml(document, element, ElementType::UNKNOWN, flags);
  }

  std::string element_to_string(const Element& element, ElementType parentType, Flags flags)
  {
    XMLDocument document{};
    document.InsertEndChild(element_to_xml(document, element, parentType, flags));
    return xml::document_to_string(document);
  }

  std::string element_to_string(const Element& element, Flags flags)
  {
    return element_to_string(element, ElementType::UNKNOWN, flags);
  }

  constexpr std::uint64_t ANM2_HASH_OFFSET = 14695981039346656037ull;
  constexpr std::uint64_t ANM2_HASH_PRIME = 1099511628211ull;

  struct BakeAttributes
  {
    Interpolation interpolation{Interpolation::NONE};
    int delay{FRAME_DURATION_MIN};
  };

  void hash_byte_append(std::uint64_t& hash, unsigned char value)
  {
    hash ^= value;
    hash *= ANM2_HASH_PRIME;
  }

  void hash_bytes_append(std::uint64_t& hash, const char* data, std::size_t size)
  {
    for (std::size_t i = 0; i < size; ++i)
      hash_byte_append(hash, (unsigned char)data[i]);
  }

  void hash_string_append(std::uint64_t& hash, std::string_view value)
  {
    auto size = (std::uint64_t)value.size();
    hash_bytes_append(hash, reinterpret_cast<const char*>(&size), sizeof(size));
    hash_bytes_append(hash, value.data(), value.size());
  }

  void hash_attr_append(std::uint64_t& hash, std::string_view name, std::string_view value)
  {
    hash_byte_append(hash, 'A');
    hash_string_append(hash, name);
    hash_string_append(hash, value);
  }

  void hash_attr_append(std::uint64_t& hash, std::string_view name, int value)
  {
    char buffer[32]{};
    XMLUtil::ToStr(value, buffer, sizeof(buffer));
    hash_attr_append(hash, name, std::string_view(buffer));
  }

  void hash_attr_append(std::uint64_t& hash, std::string_view name, bool isValue)
  {
    char buffer[8]{};
    XMLUtil::ToStr(isValue, buffer, sizeof(buffer));
    hash_attr_append(hash, name, std::string_view(buffer));
  }

  void hash_attr_append(std::uint64_t& hash, std::string_view name, float value)
  {
    char buffer[64]{};
    XMLUtil::ToStr(value, buffer, sizeof(buffer));
    hash_attr_append(hash, name, std::string_view(buffer));
  }

  void hash_path_attr_append(std::uint64_t& hash, std::string_view name, const std::filesystem::path& pathValue)
  {
    if (pathValue.empty()) return;
    auto value = path::to_utf8(pathValue);
    hash_attr_append(hash, name, value);
  }

  void hash_interpolation_attr_append(std::uint64_t& hash, Interpolation interpolation)
  {
    if (interpolation == Interpolation::NONE || interpolation == Interpolation::LINEAR)
    {
      hash_attr_append(hash, "Interpolated", interpolation == Interpolation::LINEAR);
      return;
    }

    auto value = INTERPOLATION_VALUES[(std::size_t)interpolation];
    if (!value.empty()) hash_attr_append(hash, "Interpolated", value);
  }

  void hash_origin_attr_append(std::uint64_t& hash, const Element& element)
  {
    auto origin = ORIGIN_VALUES[(std::size_t)element.origin];
    if (!origin.empty())
    {
      hash_attr_append(hash, "Origin", origin);
      return;
    }

    hash_attr_append(hash, "XPivot", element.pivot.x);
    hash_attr_append(hash, "YPivot", element.pivot.y);
  }

  void frame_attributes_hash_append(std::uint64_t& hash, const Element& element, ElementType parentType, Flags flags)
  {
    if (parentType == ElementType::LAYER_ANIMATION)
    {
      bool isHasValidRegion = has_flag(flags, SERIALIZE_REGIONS) && element.regionId != -1;
      bool isWriteRegionValues = has_flag(flags, SERIALIZE_REDUNDANT_FRAME_REGION_VALUES) || !isHasValidRegion;

      if (isHasValidRegion) hash_attr_append(hash, "RegionId", element.regionId);
      if (isWriteRegionValues)
      {
        hash_attr_append(hash, "XPivot", element.pivot.x);
        hash_attr_append(hash, "YPivot", element.pivot.y);
        hash_attr_append(hash, "XCrop", element.crop.x);
        hash_attr_append(hash, "YCrop", element.crop.y);
        hash_attr_append(hash, "Width", element.size.x);
        hash_attr_append(hash, "Height", element.size.y);
      }
    }

    hash_attr_append(hash, "XPosition", element.position.x);
    hash_attr_append(hash, "YPosition", element.position.y);
    hash_attr_append(hash, "Delay", element.duration);
    hash_attr_append(hash, "Visible", element.isVisible);
    hash_attr_append(hash, "XScale", element.scale.x);
    hash_attr_append(hash, "YScale", element.scale.y);
    hash_attr_append(hash, "RedTint", color_write(element.tint.r));
    hash_attr_append(hash, "GreenTint", color_write(element.tint.g));
    hash_attr_append(hash, "BlueTint", color_write(element.tint.b));
    hash_attr_append(hash, "AlphaTint", color_write(element.tint.a));
    hash_attr_append(hash, "RedOffset", color_write(element.colorOffset.r));
    hash_attr_append(hash, "GreenOffset", color_write(element.colorOffset.g));
    hash_attr_append(hash, "BlueOffset", color_write(element.colorOffset.b));
    hash_attr_append(hash, "Rotation", element.rotation);
    hash_interpolation_attr_append(hash, element.interpolation);
  }

  void bake_attributes_hash_append(std::uint64_t& hash, Interpolation interpolation, int bakeDelay)
  {
    auto value = INTERPOLATION_VALUES[(std::size_t)interpolation];
    if (!value.empty()) hash_attr_append(hash, "BakeInterpolation", value);
    hash_attr_append(hash, "BakeDelay", bakeDelay);
  }

  void element_attributes_hash_append(std::uint64_t& hash, const Element& element, ElementType parentType, Flags flags)
  {
    if (element.type == ElementType::INFO)
    {
      hash_attr_append(hash, "CreatedBy", element.createdBy);
      hash_attr_append(hash, "CreatedOn", element.createdOn);
      hash_attr_append(hash, "Fps", element.fps);
      hash_attr_append(hash, "Version", element.version);
    }
    else if (element.type == ElementType::ANIMATIONS)
      hash_attr_append(hash, "DefaultAnimation", element.defaultAnimation);
    else if (element.type == ElementType::SPRITESHEET || element.type == ElementType::OVERLAY)
    {
      hash_attr_append(hash, "Id", element.id);
      hash_path_attr_append(hash, "Path", element.path);
      if (element.type == ElementType::OVERLAY && !element.isVisible)
        hash_attr_append(hash, "Visible", element.isVisible);
      if (element.type == ElementType::OVERLAY)
      {
        if (element.position != glm::vec2())
        {
          hash_attr_append(hash, "XOffset", element.position.x);
          hash_attr_append(hash, "YOffset", element.position.y);
        }
        if (color_write(element.tint.a) != 255) hash_attr_append(hash, "Alpha", color_write(element.tint.a));
        if (overlay_draw_order_get(element) == OVERLAY_DRAW_ORDER_BELOW)
          hash_attr_append(hash, "DrawOrder", OVERLAY_DRAW_ORDER_VALUES[OVERLAY_DRAW_ORDER_BELOW]);
      }
    }
    else if (element.type == ElementType::SHADER)
    {
      if (!element.isEnabled) hash_attr_append(hash, "Enabled", element.isEnabled);
      hash_path_attr_append(hash, "Vertex", element.vertex);
      hash_path_attr_append(hash, "Fragment", element.fragment);
    }
    else if (element.type == ElementType::UNIFORM)
    {
      hash_attr_append(hash, "Name", element.name);
      if (!element.binding.empty()) hash_attr_append(hash, "Binding", element.binding);
      if (!element.value.empty()) hash_attr_append(hash, "Value", element.value);
    }
    else if (element.type == ElementType::COMPONENT)
    {
      hash_attr_append(hash, "Index", element.index);
      if (!element.binding.empty()) hash_attr_append(hash, "Binding", element.binding);
      if (!element.value.empty()) hash_attr_append(hash, "Value", element.value);
    }
    else if (element.type == ElementType::REGION)
    {
      hash_attr_append(hash, "Id", element.id);
      hash_attr_append(hash, "Name", element.name);
      hash_attr_append(hash, "XCrop", element.crop.x);
      hash_attr_append(hash, "YCrop", element.crop.y);
      hash_attr_append(hash, "Width", element.size.x);
      hash_attr_append(hash, "Height", element.size.y);
      hash_origin_attr_append(hash, element);
    }
    else if (element.type == ElementType::LAYER_ELEMENT)
    {
      hash_attr_append(hash, "Id", element.id);
      hash_attr_append(hash, "Name", element.name);
      hash_attr_append(hash, "SpritesheetId", element.spritesheetId);
    }
    else if (element.type == ElementType::NULL_ELEMENT)
    {
      hash_attr_append(hash, "Id", element.id);
      hash_attr_append(hash, "Name", element.name);
      if (element.isShowRect) hash_attr_append(hash, "ShowRect", element.isShowRect);
    }
    else if (element.type == ElementType::GROUP)
    {
      if (!is_nested_group_parent(parentType, flags)) hash_attr_append(hash, "Id", element.id);
      hash_attr_append(hash, "Name", element.name);
      hash_attr_append(hash, "IsExpanded", element.isExpanded);
      hash_attr_append(hash, "Visible", element.isVisible);
      if ((parentType == ElementType::LAYER_ANIMATION_GROUPS || parentType == ElementType::NULL_ANIMATION_GROUPS) &&
          element.index != -1)
        hash_attr_append(hash, "Index", element.index);
    }
    else if (element.type == ElementType::EVENT_ELEMENT)
    {
      hash_attr_append(hash, "Id", element.id);
      hash_attr_append(hash, "Name", element.name);
    }
    else if (element.type == ElementType::SOUND_ELEMENT)
    {
      hash_attr_append(hash, "Id", element.id);
      if (parentType != ElementType::TRIGGER) hash_path_attr_append(hash, "Path", element.path);
    }
    else if (element.type == ElementType::ANIMATION)
    {
      hash_attr_append(hash, "Name", element.name);
      hash_attr_append(hash, "FrameNum", element.frameNum);
      hash_attr_append(hash, "Loop", element.isLoop);
      if (element.groupId != -1 && is_group_id_serialized(flags)) hash_attr_append(hash, "GroupId", element.groupId);
    }
    else if (element.type == ElementType::LAYER_ANIMATION)
    {
      hash_attr_append(hash, "LayerId", element.layerId);
      hash_attr_append(hash, "Visible", element.isVisible);
      if (element.groupId != -1 && is_group_id_serialized(flags)) hash_attr_append(hash, "GroupId", element.groupId);
    }
    else if (element.type == ElementType::NULL_ANIMATION)
    {
      hash_attr_append(hash, "NullId", element.nullId);
      hash_attr_append(hash, "Visible", element.isVisible);
      if (element.groupId != -1 && is_group_id_serialized(flags)) hash_attr_append(hash, "GroupId", element.groupId);
    }
    else if (element.type == ElementType::FRAME)
      frame_attributes_hash_append(hash, element, parentType, flags);
    else if (element.type == ElementType::TRIGGER)
    {
      if (element.eventId != -1) hash_attr_append(hash, "EventId", element.eventId);
      hash_attr_append(hash, "AtFrame", element.atFrame);
    }
  }

  void element_hash_append(std::uint64_t& hash, const Element& element, ElementType parentType, Flags flags,
                           std::optional<BakeAttributes> bakeAttributes = std::nullopt);

  void trigger_sounds_hash_append(std::uint64_t& hash, const Element& element, Flags flags)
  {
    if (element.type != ElementType::TRIGGER || !has_flag(flags, SERIALIZE_SOUNDS)) return;

    for (auto soundId : element.soundIds)
    {
      if (soundId == -1) continue;
      auto sound = element_make(ElementType::SOUND_ELEMENT);
      sound.id = soundId;
      element_hash_append(hash, sound, ElementType::TRIGGER, flags);
    }
  }

  void baked_frames_hash_append(std::uint64_t& hash, const Element& track, int index, Flags flags)
  {
    const auto& original = track.children[index];
    auto nextFrame = index + 1 < (int)track.children.size() && track.children[index + 1].type == ElementType::FRAME
                         ? track.children[index + 1]
                         : original;
    auto bakeDelay = std::max(original.duration, FRAME_DURATION_MIN);

    for (int bakeIndex = 0; bakeIndex < bakeDelay; ++bakeIndex)
    {
      auto baked = original;
      auto amount = interpolation_factor(original.interpolation, (float)bakeIndex / (float)bakeDelay);
      baked.duration = FRAME_DURATION_MIN;
      baked.interpolation = Interpolation::NONE;
      baked.rotation = glm::mix(original.rotation, nextFrame.rotation, amount);
      baked.position = glm::mix(original.position, nextFrame.position, amount);
      baked.scale = glm::mix(original.scale, nextFrame.scale, amount);
      baked.colorOffset = glm::mix(original.colorOffset, nextFrame.colorOffset, amount);
      baked.tint = glm::mix(original.tint, nextFrame.tint, amount);
      element_hash_append(hash, baked, track.type, flags,
                          bakeIndex == 0 ? std::optional<BakeAttributes>({original.interpolation, bakeDelay})
                                         : std::nullopt);
    }
  }

  void element_hash_append(std::uint64_t& hash, const Element& element, ElementType parentType, Flags flags,
                           std::optional<BakeAttributes> bakeAttributes)
  {
    auto tag = element.type == ElementType::UNKNOWN ? std::string_view(element.tag) : element_tag_get(element.type);
    hash_byte_append(hash, 'E');
    hash_string_append(hash, tag.empty() ? std::string_view(element.tag) : tag);

    element_attributes_hash_append(hash, element, parentType, flags);
    if (bakeAttributes) bake_attributes_hash_append(hash, bakeAttributes->interpolation, bakeAttributes->delay);

    hash_byte_append(hash, '[');
    trigger_sounds_hash_append(hash, element, flags);
    for (int i = 0; i < (int)element.children.size(); ++i)
    {
      const auto& child = element.children[i];
      if (element_write_skip(child, element.type, flags)) continue;

      if (is_track(element) && is_frame_bake_serialized(child, flags))
        baked_frames_hash_append(hash, element, i, flags);
      else
        element_hash_append(hash, child, element.type, flags);
    }
    hash_byte_append(hash, ']');
  }

  std::uint64_t element_hash(const Element& element, Flags flags)
  {
    auto hash = ANM2_HASH_OFFSET;
    element_hash_append(hash, element, ElementType::UNKNOWN, flags);
    return hash;
  }

  int child_index_get(const Element& element, ElementType type)
  {
    for (int i = 0; i < (int)element.children.size(); ++i)
      if (element.children[i].type == type) return i;
    return -1;
  }

  ElementType track_container_track_type_get(ElementType containerType)
  {
    if (containerType == ElementType::LAYER_ANIMATIONS) return ElementType::LAYER_ANIMATION;
    if (containerType == ElementType::NULL_ANIMATIONS) return ElementType::NULL_ANIMATION;
    return ElementType::UNKNOWN;
  }

  ElementType track_container_group_container_type_get(ElementType containerType)
  {
    if (containerType == ElementType::LAYER_ANIMATIONS) return ElementType::LAYER_ANIMATION_GROUPS;
    if (containerType == ElementType::NULL_ANIMATIONS) return ElementType::NULL_ANIMATION_GROUPS;
    return ElementType::UNKNOWN;
  }

  int track_group_insert_index_get(const Element& container, ElementType trackType, int groupId)
  {
    for (int i = 0; i < (int)container.children.size(); ++i)
      if (container.children[i].type == trackType && container.children[i].groupId == groupId) return i;
    return (int)container.children.size();
  }

  void group_metadata_embed(Element& animation, ElementType trackContainerType)
  {
    auto groupContainerType = track_container_group_container_type_get(trackContainerType);
    auto trackType = track_container_track_type_get(trackContainerType);
    auto groupContainerIndex = child_index_get(animation, groupContainerType);
    if (groupContainerIndex < 0 || trackType == ElementType::UNKNOWN) return;

    std::vector<Element> groups{};
    for (const auto& child : animation.children[groupContainerIndex].children)
      if (child.type == ElementType::GROUP) groups.push_back(child);

    if (!groups.empty())
    {
      auto trackContainerIndex = child_index_get(animation, trackContainerType);
      if (trackContainerIndex < 0)
      {
        animation.children.push_back(element_make(trackContainerType));
        trackContainerIndex = (int)animation.children.size() - 1;
      }

      auto& container = animation.children[trackContainerIndex];
      std::set<int> groupIds{};
      for (const auto& group : groups)
        if (group.id >= 0) groupIds.insert(group.id);
      std::erase_if(container.children, [&](const Element& child)
                    { return child.type == ElementType::GROUP && groupIds.contains(child.id); });

      std::stable_sort(groups.begin(), groups.end(),
                       [](const Element& left, const Element& right)
                       {
                         if (left.index == -1 || right.index == -1) return left.index != -1;
                         return left.index < right.index;
                       });

      for (auto& group : groups)
      {
        auto insertIndex = group.index >= 0 ? std::min(group.index, (int)container.children.size())
                                            : track_group_insert_index_get(container, trackType, group.id);
        group.index = -1;
        container.children.insert(container.children.begin() + insertIndex, std::move(group));
      }
    }

    std::erase_if(animation.children, [&](const Element& child) { return child.type == groupContainerType; });
  }

  void group_metadata_embed(Element& element)
  {
    if (element.type == ElementType::ANIMATION)
    {
      group_metadata_embed(element, ElementType::LAYER_ANIMATIONS);
      group_metadata_embed(element, ElementType::NULL_ANIMATIONS);
    }

    for (auto& child : element.children)
      group_metadata_embed(child);
  }

  void group_metadata_extract(Element& animation, ElementType trackContainerType, Flags flags)
  {
    auto groupContainerType = track_container_group_container_type_get(trackContainerType);
    if (groupContainerType == ElementType::UNKNOWN) return;

    std::erase_if(animation.children, [&](const Element& child) { return child.type == groupContainerType; });
    if (!has_flag(flags, SERIALIZE_GROUPS)) return;

    auto trackContainerIndex = child_index_get(animation, trackContainerType);
    if (trackContainerIndex < 0) return;

    auto& container = animation.children[trackContainerIndex];
    auto groupContainer = element_make(groupContainerType);
    std::vector<Element> tracks{};
    tracks.reserve(container.children.size());

    for (int i = 0; i < (int)container.children.size(); ++i)
    {
      auto child = container.children[i];
      if (child.type == ElementType::GROUP)
      {
        child.index = i;
        groupContainer.children.push_back(std::move(child));
      }
      else
        tracks.push_back(std::move(child));
    }

    container.children = std::move(tracks);
    if (groupContainer.children.empty()) return;
    animation.children.insert(animation.children.begin() + trackContainerIndex + 1, std::move(groupContainer));
  }

  void group_metadata_extract(Element& element, Flags flags)
  {
    if (element.type == ElementType::ANIMATION)
    {
      group_metadata_extract(element, ElementType::LAYER_ANIMATIONS, flags);
      group_metadata_extract(element, ElementType::NULL_ANIMATIONS, flags);
    }

    for (auto& child : element.children)
      group_metadata_extract(child, flags);
  }

  void groups_flatten(Element& element)
  {
    auto container_flatten = [](Element& container, ElementType childType, bool isRootKept)
    {
      int nextGroupId{};
      for (const auto& item : container.children)
        if (item.type == ElementType::GROUP) nextGroupId = std::max(nextGroupId, item.id + 1);

      std::vector<Element> flattened{};
      for (auto item : container.children)
      {
        if (item.type != ElementType::GROUP)
        {
          flattened.push_back(item);
          continue;
        }

        if (item.id < 0) item.id = nextGroupId++;
        auto groupId = item.id;
        auto children = std::move(item.children);
        item.children.clear();

        if (isRootKept)
        {
          bool isRootFound{};

          for (auto child : children)
            if (child.type == ElementType::ROOT_ANIMATION)
            {
              if (isRootFound) continue;
              item.children.push_back(child);
              isRootFound = true;
            }

          if (!isRootFound) item.children.push_back(root_animation_make());
        }

        flattened.push_back(item);

        for (auto child : children)
        {
          if (child.type != childType) continue;
          child.groupId = groupId;
          flattened.push_back(child);
        }
      }

      std::set<int> groupIds{};
      for (const auto& item : flattened)
        if (item.type == ElementType::GROUP) groupIds.insert(item.id);

      for (auto& item : flattened)
        if (item.type == childType && !groupIds.contains(item.groupId)) item.groupId = -1;

      container.children = std::move(flattened);
    };

    if (element.type == ElementType::ANIMATIONS)
      container_flatten(element, ElementType::ANIMATION, false);
    else if (element.type == ElementType::LAYER_ANIMATIONS)
      container_flatten(element, ElementType::LAYER_ANIMATION, true);
    else if (element.type == ElementType::NULL_ANIMATIONS)
      container_flatten(element, ElementType::NULL_ANIMATION, true);

    for (auto& child : element.children)
      groups_flatten(child);
  }

  void group_children_nest(Element& container, ElementType childType)
  {
    if (childType == ElementType::UNKNOWN) return;

    std::set<int> groupIds{};
    for (const auto& child : container.children)
      if (child.type == ElementType::GROUP && child.id >= 0) groupIds.insert(child.id);
    if (groupIds.empty()) return;

    std::unordered_map<int, std::vector<Element>> groupedChildren{};
    for (const auto& child : container.children)
    {
      if (child.type != childType || !groupIds.contains(child.groupId)) continue;
      auto groupedChild = child;
      groupedChild.groupId = -1;
      groupedChildren[child.groupId].push_back(std::move(groupedChild));
    }

    std::vector<Element> nested{};
    nested.reserve(container.children.size());
    for (auto child : container.children)
    {
      if (child.type == childType && groupIds.contains(child.groupId)) continue;
      if (child.type == ElementType::GROUP)
      {
        auto groupId = child.id;
        child.index = -1;
        if (auto grouped = groupedChildren.find(groupId); grouped != groupedChildren.end())
          child.children.insert(child.children.end(), std::make_move_iterator(grouped->second.begin()),
                                std::make_move_iterator(grouped->second.end()));
      }
      nested.push_back(std::move(child));
    }

    container.children = std::move(nested);
  }

  void groups_nest(Element& element)
  {
    if (element.type == ElementType::ANIMATIONS)
      group_children_nest(element, ElementType::ANIMATION);
    else if (element.type == ElementType::LAYER_ANIMATIONS)
      group_children_nest(element, ElementType::LAYER_ANIMATION);
    else if (element.type == ElementType::NULL_ANIMATIONS)
      group_children_nest(element, ElementType::NULL_ANIMATION);

    for (auto& child : element.children)
      groups_nest(child);
  }

  void source_document_erase(Element& element)
  {
    std::erase_if(element.children, [](const Element& child) { return is_source_document_tag(child.tag); });
    for (auto& child : element.children)
      source_document_erase(child);
  }

  Element* child_first_get(Element& element, ElementType type)
  {
    for (auto& child : element.children)
      if (child.type == type) return &child;
    return nullptr;
  }

  const Element* child_first_get(const Element& element, ElementType type)
  {
    for (const auto& child : element.children)
      if (child.type == type) return &child;
    return nullptr;
  }

  Element* element_child_first_get(Element& element, ElementType type) { return child_first_get(element, type); }

  const Element* element_child_first_get(const Element& element, ElementType type)
  {
    return child_first_get(element, type);
  }

  Element* shader_uniform_get(Element& shader, std::string_view name, bool isCreate)
  {
    for (auto& child : shader.children)
      if (child.type == ElementType::UNIFORM && child.name == name) return &child;
    if (!isCreate) return nullptr;

    shader.children.push_back(element_make(ElementType::UNIFORM));
    shader.children.back().name = std::string(name);
    return &shader.children.back();
  }

  const Element* shader_uniform_get(const Element& shader, std::string_view name)
  {
    for (const auto& child : shader.children)
      if (child.type == ElementType::UNIFORM && child.name == name) return &child;
    return nullptr;
  }

  Element* shader_uniform_component_get(Element& uniform, int index, bool isCreate)
  {
    for (auto& child : uniform.children)
      if (child.type == ElementType::COMPONENT && child.index == index) return &child;
    if (!isCreate) return nullptr;

    uniform.children.push_back(element_make(ElementType::COMPONENT));
    uniform.children.back().index = index;
    return &uniform.children.back();
  }

  const Element* shader_uniform_component_get(const Element& uniform, int index)
  {
    for (const auto& child : uniform.children)
      if (child.type == ElementType::COMPONENT && child.index == index) return &child;
    return nullptr;
  }

  Element* child_id_get(Element& element, ElementType type, int id)
  {
    for (auto& child : element.children)
      if (child.type == type && child.id == id) return &child;
    return nullptr;
  }

  const Element* child_id_get(const Element& element, ElementType type, int id)
  {
    for (const auto& child : element.children)
      if (child.type == type && child.id == id) return &child;
    return nullptr;
  }

  Element* element_child_id_get(Element& element, ElementType type, int id) { return child_id_get(element, type, id); }

  const Element* element_child_id_get(const Element& element, ElementType type, int id)
  {
    return child_id_get(element, type, id);
  }

  int element_child_max_id_get(const Element& element, ElementType type)
  {
    int maxId{-1};
    for (const auto& child : element.children)
      if (child.type == type) maxId = std::max(maxId, child.id);
    return maxId;
  }

  int element_child_next_id_get(const Element& element, ElementType type)
  {
    return element_child_max_id_get(element, type) + 1;
  }

  bool element_child_id_erase(Element& element, ElementType type, int id)
  {
    for (auto it = element.children.begin(); it != element.children.end(); ++it)
    {
      if (it->type != type || it->id != id) continue;
      element.children.erase(it);
      return true;
    }
    return false;
  }

  Element* element_first_get(Element& element, ElementType type)
  {
    if (element.type == type) return &element;
    for (auto& child : element.children)
      if (auto result = element_first_get(child, type); result) return result;
    return nullptr;
  }

  const Element* element_first_get(const Element& element, ElementType type)
  {
    if (element.type == type) return &element;
    for (const auto& child : element.children)
      if (auto result = element_first_get(child, type); result) return result;
    return nullptr;
  }

  float interpolation_factor(Interpolation interpolation, float value)
  {
    value = glm::clamp(value, 0.0f, 1.0f);
    if (interpolation == Interpolation::LINEAR) return value;
    if (interpolation == Interpolation::EASE_IN) return value * value;
    if (interpolation == Interpolation::EASE_OUT) return 1.0f - ((1.0f - value) * (1.0f - value));
    if (interpolation == Interpolation::EASE_IN_OUT)
      return value < 0.5f ? (2.0f * value * value) : (1.0f - std::pow(-2.0f * value + 2.0f, 2.0f) * 0.5f);
    return 0.0f;
  }

  bool is_track(const Element& element)
  {
    return element.type == ElementType::ROOT_ANIMATION || element.type == ElementType::LAYER_ANIMATION ||
           element.type == ElementType::NULL_ANIMATION || element.type == ElementType::TRIGGERS;
  }

  bool is_track_child_valid(ElementType parentType, ElementType childType)
  {
    if (parentType == ElementType::TRIGGERS) return childType == ElementType::TRIGGER;
    if (parentType == ElementType::TRIGGER) return childType == ElementType::SOUND_ELEMENT;
    if (parentType == ElementType::LAYER_ANIMATION_GROUPS || parentType == ElementType::NULL_ANIMATION_GROUPS)
      return childType == ElementType::GROUP;
    if (parentType == ElementType::ROOT_ANIMATION || parentType == ElementType::LAYER_ANIMATION ||
        parentType == ElementType::NULL_ANIMATION)
      return childType == ElementType::FRAME;
    return true;
  }

  ElementType track_frame_type_get(const Element& track)
  {
    return track.type == ElementType::TRIGGERS ? ElementType::TRIGGER : ElementType::FRAME;
  }

  ElementType item_type_to_track_type_get(ItemType type)
  {
    if (type == ItemType::ROOT) return ElementType::ROOT_ANIMATION;
    if (type == ItemType::LAYER) return ElementType::LAYER_ANIMATION;
    if (type == ItemType::NULL_) return ElementType::NULL_ANIMATION;
    if (type == ItemType::TRIGGER) return ElementType::TRIGGERS;
    return ElementType::UNKNOWN;
  }

  ElementType item_type_to_container_type_get(ItemType type)
  {
    if (type == ItemType::LAYER) return ElementType::LAYER_ANIMATIONS;
    if (type == ItemType::NULL_) return ElementType::NULL_ANIMATIONS;
    return ElementType::UNKNOWN;
  }

  Element* animation_group_get(Element& animation, int groupType, int groupId)
  {
    if (groupId < 0) return nullptr;
    auto containerType = item_type_to_container_type_get(static_cast<ItemType>(groupType));
    auto container = child_first_get(animation, containerType);
    return container ? child_id_get(*container, ElementType::GROUP, groupId) : nullptr;
  }

  const Element* animation_group_get(const Element& animation, int groupType, int groupId)
  {
    if (groupId < 0) return nullptr;
    auto containerType = item_type_to_container_type_get(static_cast<ItemType>(groupType));
    auto container = child_first_get(animation, containerType);
    return container ? child_id_get(*container, ElementType::GROUP, groupId) : nullptr;
  }

  Element* animation_group_root_get(Element& animation, int groupType, int groupId)
  {
    auto group = animation_group_get(animation, groupType, groupId);
    return group ? child_first_get(*group, ElementType::ROOT_ANIMATION) : nullptr;
  }

  const Element* animation_group_root_get(const Element& animation, int groupType, int groupId)
  {
    auto group = animation_group_get(animation, groupType, groupId);
    return group ? child_first_get(*group, ElementType::ROOT_ANIMATION) : nullptr;
  }

  Element* animation_group_root_ensure(Element& animation, int groupType, int groupId)
  {
    auto group = animation_group_get(animation, groupType, groupId);
    if (!group) return nullptr;
    if (auto root = child_first_get(*group, ElementType::ROOT_ANIMATION)) return root;
    group->children.push_back(root_animation_make());
    return &group->children.back();
  }

  int track_id_get(const Element& track)
  {
    if (track.type == ElementType::LAYER_ANIMATION) return track.layerId;
    if (track.type == ElementType::NULL_ANIMATION) return track.nullId;
    return -1;
  }

  template <typename Callback> void tracks_each(Element& parent, ElementType trackType, Callback&& callback)
  {
    for (auto& child : parent.children)
    {
      if (child.type == trackType)
        callback(child);
      else if (child.type == ElementType::GROUP)
        tracks_each(child, trackType, callback);
    }
  }

  template <typename Callback> void tracks_each(const Element& parent, ElementType trackType, Callback&& callback)
  {
    for (const auto& child : parent.children)
    {
      if (child.type == trackType)
        callback(child);
      else if (child.type == ElementType::GROUP)
        tracks_each(child, trackType, callback);
    }
  }

  Element* track_find(Element& parent, ElementType trackType, int id)
  {
    for (auto& child : parent.children)
    {
      if (child.type == trackType && track_id_get(child) == id) return &child;
      if (child.type == ElementType::GROUP)
        if (auto found = track_find(child, trackType, id)) return found;
    }
    return nullptr;
  }

  const Element* track_find(const Element& parent, ElementType trackType, int id)
  {
    for (const auto& child : parent.children)
    {
      if (child.type == trackType && track_id_get(child) == id) return &child;
      if (child.type == ElementType::GROUP)
        if (auto found = track_find(child, trackType, id)) return found;
    }
    return nullptr;
  }

  Element* track_group_find(Element& parent, ElementType trackType, int id, int groupId)
  {
    for (auto& child : parent.children)
    {
      if (child.type == trackType && track_id_get(child) == id && child.groupId == groupId) return &child;
      if (child.type == ElementType::GROUP)
        if (auto found = track_group_find(child, trackType, id, groupId)) return found;
    }
    return nullptr;
  }

  const Element* track_group_find(const Element& parent, ElementType trackType, int id, int groupId)
  {
    for (const auto& child : parent.children)
    {
      if (child.type == trackType && track_id_get(child) == id && child.groupId == groupId) return &child;
      if (child.type == ElementType::GROUP)
        if (auto found = track_group_find(child, trackType, id, groupId)) return found;
    }
    return nullptr;
  }

  void region_frame_ids_repair(Element& root)
  {
    auto content = child_first_get(root, ElementType::CONTENT);
    auto layers = content ? child_first_get(*content, ElementType::LAYERS) : nullptr;
    auto spritesheets = content ? child_first_get(*content, ElementType::SPRITESHEETS) : nullptr;
    auto animations = element_first_get(root, ElementType::ANIMATIONS);
    if (!layers || !spritesheets || !animations) return;

    for (auto& animation : animations->children)
    {
      if (animation.type != ElementType::ANIMATION) continue;
      auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS);
      if (!layerAnimations) continue;

      tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                  [&](Element& layerAnimation)
                  {
                    auto layer = child_id_get(*layers, ElementType::LAYER_ELEMENT, layerAnimation.layerId);
                    auto spritesheet =
                        layer ? child_id_get(*spritesheets, ElementType::SPRITESHEET, layer->spritesheetId) : nullptr;
                    if (!spritesheet) return;

                    for (auto& frame : layerAnimation.children)
                    {
                      if (frame.type != ElementType::FRAME) continue;

                      auto frameCrop = glm::ivec2(frame.crop);
                      auto frameSize = glm::ivec2(frame.size);
                      auto framePivot = glm::ivec2(frame.pivot);
                      auto is_region_match = [&](const Element& region)
                      {
                        return region.type == ElementType::REGION && glm::ivec2(region.crop) == frameCrop &&
                               glm::ivec2(region.size) == frameSize && glm::ivec2(region.pivot) == framePivot;
                      };

                      auto region = frame.regionId == -1
                                        ? nullptr
                                        : child_id_get(*spritesheet, ElementType::REGION, frame.regionId);
                      if (region && is_region_match(*region)) continue;

                      for (const auto& candidate : spritesheet->children)
                        if (is_region_match(candidate))
                        {
                          frame.regionId = candidate.id;
                          break;
                        }
                    }
                  });
    }
  }

  void region_ids_remap(Element& root)
  {
    auto content = child_first_get(root, ElementType::CONTENT);
    auto layers = content ? child_first_get(*content, ElementType::LAYERS) : nullptr;
    auto spritesheets = content ? child_first_get(*content, ElementType::SPRITESHEETS) : nullptr;
    auto animations = element_first_get(root, ElementType::ANIMATIONS);
    if (!spritesheets) return;

    std::unordered_map<int, std::unordered_map<int, int>> remaps{};
    for (auto& spritesheet : spritesheets->children)
    {
      if (spritesheet.type != ElementType::SPRITESHEET) continue;

      std::unordered_map<int, int> remap{};
      int nextId{};
      for (auto& region : spritesheet.children)
      {
        if (region.type != ElementType::REGION) continue;
        remap[region.id] = nextId;
        region.id = nextId++;
      }
      if (!remap.empty()) remaps[spritesheet.id] = std::move(remap);
    }

    if (!layers || !animations || remaps.empty()) return;

    std::unordered_map<int, int> layerSpritesheets{};
    for (const auto& layer : layers->children)
      if (layer.type == ElementType::LAYER_ELEMENT) layerSpritesheets[layer.id] = layer.spritesheetId;

    for (auto& animation : animations->children)
    {
      if (animation.type != ElementType::ANIMATION) continue;
      auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS);
      if (!layerAnimations) continue;

      tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                  [&](Element& layerAnimation)
                  {
                    auto layer = layerSpritesheets.find(layerAnimation.layerId);
                    if (layer == layerSpritesheets.end()) return;
                    auto remap = remaps.find(layer->second);
                    if (remap == remaps.end()) return;

                    for (auto& frame : layerAnimation.children)
                    {
                      if (frame.type != ElementType::FRAME || frame.regionId == -1) continue;
                      if (auto it = remap->second.find(frame.regionId); it != remap->second.end())
                        frame.regionId = it->second;
                      else
                        frame.regionId = -1;
                    }
                  });
    }
  }

  bool is_track_group_visible(const Element& container, const Element& track)
  {
    if (track.groupId == -1) return true;
    for (const auto& child : container.children)
      if (child.type == ElementType::GROUP && child.id == track.groupId) return child.isVisible;
    return true;
  }

  Element* animation_item_get(Element& animation, ItemType type, int id, int groupType, int groupId)
  {
    auto trackType = item_type_to_track_type_get(type);
    if (type == ItemType::ROOT && groupType != NONE && groupId != -1)
      return animation_group_root_get(animation, groupType, groupId);
    if (type == ItemType::ROOT || type == ItemType::TRIGGER) return child_first_get(animation, trackType);

    auto container = child_first_get(animation, item_type_to_container_type_get(type));
    if (!container) return nullptr;
    return groupType != NONE && groupId != -1 ? track_group_find(*container, trackType, id, groupId)
                                              : track_find(*container, trackType, id);
  }

  Element* animation_item_get(Element& animation, ItemType type, int id)
  {
    return animation_item_get(animation, type, id, NONE, -1);
  }

  const Element* animation_item_get(const Element& animation, ItemType type, int id, int groupType, int groupId)
  {
    auto trackType = item_type_to_track_type_get(type);
    if (type == ItemType::ROOT && groupType != NONE && groupId != -1)
      return animation_group_root_get(animation, groupType, groupId);
    if (type == ItemType::ROOT || type == ItemType::TRIGGER) return child_first_get(animation, trackType);

    auto container = child_first_get(animation, item_type_to_container_type_get(type));
    if (!container) return nullptr;
    return groupType != NONE && groupId != -1 ? track_group_find(*container, trackType, id, groupId)
                                              : track_find(*container, trackType, id);
  }

  const Element* animation_item_get(const Element& animation, ItemType type, int id)
  {
    return animation_item_get(animation, type, id, NONE, -1);
  }

  int track_frame_child_index_get(const Element& track, int index)
  {
    if (index < 0) return -1;
    auto frameType = track_frame_type_get(track);
    int frameIndex{};
    for (int i = 0; i < (int)track.children.size(); ++i)
    {
      if (track.children[i].type != frameType) continue;
      if (frameIndex == index) return i;
      ++frameIndex;
    }
    return -1;
  }

  int track_frames_count_get(const Element& track)
  {
    auto frameType = track_frame_type_get(track);
    int count{};
    for (const auto& child : track.children)
      if (child.type == frameType) ++count;
    return count;
  }

  int track_frame_insert_child_index_get(const Element& track, int index)
  {
    auto targetFrameIndex = std::max(0, index);
    auto frameType = track_frame_type_get(track);
    int frameIndex{};
    for (int i = 0; i < (int)track.children.size(); ++i)
    {
      if (track.children[i].type != frameType) continue;
      if (frameIndex == targetFrameIndex) return i;
      ++frameIndex;
    }
    return (int)track.children.size();
  }

  Element* track_frame_get(Element& track, int index)
  {
    auto childIndex = track_frame_child_index_get(track, index);
    return childIndex == -1 ? nullptr : &track.children[childIndex];
  }

  const Element* track_frame_get(const Element& track, int index)
  {
    auto childIndex = track_frame_child_index_get(track, index);
    return childIndex == -1 ? nullptr : &track.children[childIndex];
  }

  Element frame_generate(const Element& track, float time)
  {
    auto frame = element_make(track_frame_type_get(track));
    frame.isVisible = false;
    if (track.children.empty()) return frame;

    time = std::max(time, 0.0f);

    const Element* frameNext = nullptr;
    int durationCurrent{};
    int durationNext{};
    auto frameType = track_frame_type_get(track);

    for (int i = 0; i < (int)track.children.size(); ++i)
    {
      const auto& iFrame = track.children[i];
      if (iFrame.type != frameType) continue;

      if (frameType == ElementType::TRIGGER)
      {
        if ((int)time == iFrame.atFrame)
        {
          frame = iFrame;
          break;
        }
        continue;
      }

      frame = iFrame;
      durationNext += frame.duration;

      if (time >= durationCurrent && time < durationNext)
      {
        for (int next = i + 1; next < (int)track.children.size(); ++next)
          if (track.children[next].type == ElementType::FRAME)
          {
            frameNext = &track.children[next];
            break;
          }
        break;
      }

      durationCurrent += frame.duration;
    }

    if (frameType != ElementType::TRIGGER && frame.interpolation != Interpolation::NONE && frameNext &&
        frame.duration > 1)
    {
      auto amount =
          interpolation_factor(frame.interpolation, (time - durationCurrent) / (durationNext - durationCurrent));
      frame.rotation = glm::mix(frame.rotation, frameNext->rotation, amount);
      frame.position = glm::mix(frame.position, frameNext->position, amount);
      frame.scale = glm::mix(frame.scale, frameNext->scale, amount);
      frame.colorOffset = glm::mix(frame.colorOffset, frameNext->colorOffset, amount);
      frame.tint = glm::mix(frame.tint, frameNext->tint, amount);
    }

    return frame;
  }

  int frame_index_from_at_frame_get(const Element& track, int atFrame)
  {
    int index{};
    for (const auto& frame : track.children)
    {
      if (frame.type != ElementType::TRIGGER) continue;
      if (frame.atFrame == atFrame) return index;
      ++index;
    }
    return -1;
  }

  int frame_index_from_time_get(const Element& track, float time)
  {
    if (track.type == ElementType::TRIGGERS) return frame_index_from_at_frame_get(track, (int)time);

    auto frameType = track_frame_type_get(track);
    int frameCount{};
    for (const auto& frame : track.children)
      if (frame.type == frameType) ++frameCount;
    if (frameCount == 0) return -1;
    if (time <= 0.0f) return 0;

    float duration{};
    int index{};
    for (const auto& frame : track.children)
    {
      if (frame.type != frameType) continue;
      duration += frame.duration;
      if (time < duration) return index;
      ++index;
    }

    return frameCount - 1;
  }

  float frame_time_from_index_get(const Element& track, int index)
  {
    if (index < 0) return 0.0f;
    auto frameType = track_frame_type_get(track);
    float time{};
    int frameIndex{};
    for (const auto& frame : track.children)
    {
      if (frame.type != frameType) continue;
      if (frameIndex == index) return frameType == ElementType::TRIGGER ? (float)frame.atFrame : time;
      time += frame.duration;
      ++frameIndex;
    }
    return 0.0f;
  }

  void frame_bake(Element& track, int index, int interval, bool isRoundScale, bool isRoundRotation)
  {
    auto childIndex = track_frame_child_index_get(track, index);
    if (childIndex == -1) return;

    auto frame = &track.children[childIndex];

    auto original = *frame;
    if (original.duration <= FRAME_DURATION_MIN)
    {
      frame->interpolation = Interpolation::NONE;
      return;
    }

    auto nextFrame = track_frame_get(track, index + 1);
    auto next = nextFrame ? *nextFrame : original;
    int duration{};
    interval = std::max(interval, FRAME_DURATION_MIN);
    std::vector<Element> bakedFrames{};
    bakedFrames.reserve((original.duration + interval - 1) / interval);

    while (duration < original.duration)
    {
      auto baked = original;
      auto amount = interpolation_factor(original.interpolation, (float)duration / original.duration);
      baked.duration = std::min(interval, original.duration - duration);
      baked.interpolation = Interpolation::NONE;
      baked.rotation = glm::mix(original.rotation, next.rotation, amount);
      baked.position = glm::mix(original.position, next.position, amount);
      baked.scale = glm::mix(original.scale, next.scale, amount);
      baked.colorOffset = glm::mix(original.colorOffset, next.colorOffset, amount);
      baked.tint = glm::mix(original.tint, next.tint, amount);
      if (isRoundScale) baked.scale = glm::round(baked.scale);
      if (isRoundRotation) baked.rotation = std::round(baked.rotation);

      bakedFrames.push_back(baked);
      duration += baked.duration;
    }

    if (bakedFrames.empty()) return;
    track.children[childIndex] = std::move(bakedFrames.front());
    track.children.insert(track.children.begin() + childIndex + 1, std::make_move_iterator(bakedFrames.begin() + 1),
                          std::make_move_iterator(bakedFrames.end()));
  }

  void frames_generate_from_grid(Element& track, glm::ivec2 startPosition, glm::ivec2 size, glm::vec2 pivot,
                                 int columns, int count, int duration)
  {
    for (int i = 0; i < count; ++i)
    {
      auto frame = element_make(ElementType::FRAME);
      frame.duration = duration;
      frame.pivot = pivot;
      frame.size = size;
      frame.crop = startPosition + glm::ivec2(size.x * (i % columns), size.y * (i / columns));
      track.children.emplace_back(frame);
    }
  }

  void frames_sort_by_at_frame(Element& track)
  {
    std::sort(track.children.begin(), track.children.end(),
              [](const Element& a, const Element& b) { return a.atFrame < b.atFrame; });
  }

  bool frames_deserialize(Element& track, const std::string& string, int start, std::set<int>& indices,
                          std::string* errorString)
  {
    XMLDocument document{};
    if (document.Parse(string.c_str()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }

    auto frameType = track_frame_type_get(track);
    auto first =
        frameType == ElementType::TRIGGER ? document.FirstChildElement("Trigger") : document.FirstChildElement("Frame");
    if (!first)
    {
      if (errorString) *errorString = frameType == ElementType::TRIGGER ? "No valid trigger(s)." : "No valid frame(s).";
      return false;
    }

    if (frameType == ElementType::FRAME)
    {
      start = std::clamp(start, 0, track_frames_count_get(track));
      std::vector<Element> frames{};
      for (auto element = first; element; element = element->NextSiblingElement("Frame"))
      {
        auto frame = element_read(element);
        if (frame.type != ElementType::FRAME) continue;
        frames.push_back(std::move(frame));
      }
      if (frames.empty()) return false;
      auto count = (int)frames.size();
      track.children.reserve(track.children.size() + frames.size());
      auto childIndex = track_frame_insert_child_index_get(track, start);
      track.children.insert(track.children.begin() + childIndex, std::make_move_iterator(frames.begin()),
                            std::make_move_iterator(frames.end()));
      for (int offset = 0; offset < count; ++offset)
        indices.insert(start + offset);
      return !indices.empty();
    }

    auto has_conflict = [&](int value)
    {
      for (auto& trigger : track.children)
        if (trigger.type == ElementType::TRIGGER && trigger.atFrame == value) return true;
      return false;
    };

    std::vector<int> atFrames{};
    int count{};
    for (auto element = first; element; element = element->NextSiblingElement("Trigger"))
    {
      auto trigger = element_read(element);
      if (trigger.type != ElementType::TRIGGER) continue;
      trigger.atFrame = start + count;
      while (has_conflict(trigger.atFrame))
        ++trigger.atFrame;
      atFrames.push_back(trigger.atFrame);
      track.children.push_back(trigger);
      ++count;
    }
    frames_sort_by_at_frame(track);
    for (auto atFrame : atFrames)
      if (auto index = frame_index_from_at_frame_get(track, atFrame); index != -1) indices.insert(index);
    return !indices.empty();
  }

  void frames_change(Element& track, FrameChange change, ItemType itemType, ChangeType changeType,
                     const std::set<int>& selection)
  {
    const auto clamp_identity = [](auto value) { return value; };
    const auto clamp_duration = [](int value) { return std::max(FRAME_DURATION_MIN, value); };

    if (selection.empty()) return;

    auto apply_scalar_with_clamp = [&](auto& target, const auto& optionalValue, auto clampFunc)
    {
      if (!optionalValue) return;
      auto value = *optionalValue;

      switch (changeType)
      {
        case ChangeType::ADJUST:
          target = clampFunc(value);
          break;
        case ChangeType::ADD:
          target = clampFunc(target + value);
          break;
        case ChangeType::SUBTRACT:
          target = clampFunc(target - value);
          break;
        case ChangeType::MULTIPLY:
          target = clampFunc(target * value);
          break;
        case ChangeType::DIVIDE:
          if (value == decltype(value){}) return;
          target = clampFunc(target / value);
          break;
      }
    };

    auto apply_scalar = [&](auto& target, const auto& optionalValue)
    { apply_scalar_with_clamp(target, optionalValue, clamp_identity); };

    for (auto index : selection)
    {
      auto frame = track_frame_get(track, index);
      if (!frame) continue;

      if (change.isVisible) frame->isVisible = *change.isVisible;
      if (change.interpolation) frame->interpolation = *change.interpolation;
      if (change.isFlipX) frame->scale.x = -frame->scale.x;
      if (change.isFlipY) frame->scale.y = -frame->scale.y;

      apply_scalar(frame->rotation, change.rotation);
      apply_scalar_with_clamp(frame->duration, change.duration, clamp_duration);

      if (itemType == ItemType::LAYER)
      {
        apply_scalar(frame->crop.x, change.cropX);
        apply_scalar(frame->crop.y, change.cropY);
        apply_scalar(frame->pivot.x, change.pivotX);
        apply_scalar(frame->pivot.y, change.pivotY);
        apply_scalar(frame->size.x, change.sizeX);
        apply_scalar(frame->size.y, change.sizeY);
        if (change.regionId) frame->regionId = *change.regionId;
      }

      apply_scalar(frame->position.x, change.positionX);
      apply_scalar(frame->position.y, change.positionY);
      apply_scalar(frame->scale.x, change.scaleX);
      apply_scalar(frame->scale.y, change.scaleY);
      apply_scalar(frame->colorOffset.x, change.colorOffsetR);
      apply_scalar(frame->colorOffset.y, change.colorOffsetG);
      apply_scalar(frame->colorOffset.z, change.colorOffsetB);
      apply_scalar(frame->tint.x, change.tintR);
      apply_scalar(frame->tint.y, change.tintG);
      apply_scalar(frame->tint.z, change.tintB);
      apply_scalar(frame->tint.w, change.tintA);
    }
  }

  bool is_special_interpolated_frames(const Element& element)
  {
    if (element.type == ElementType::FRAME && element.interpolation != Interpolation::NONE &&
        element.interpolation != Interpolation::LINEAR)
      return true;

    for (const auto& child : element.children)
      if (is_special_interpolated_frames(child)) return true;
    return false;
  }

  enum class InterpolationBakeType
  {
    SPECIAL,
    ALL
  };

  bool is_frame_interpolation_baked(const Element& frame, InterpolationBakeType type)
  {
    if (frame.interpolation == Interpolation::NONE) return false;
    return type == InterpolationBakeType::ALL || frame.interpolation != Interpolation::LINEAR;
  }

  void track_frames_bake(Element& track, int interval, bool isRoundScale, bool isRoundRotation,
                         InterpolationBakeType type)
  {
    for (int index = (int)track.children.size() - 1; index >= 0; --index)
    {
      auto original = track.children[index];
      if (original.type != ElementType::FRAME) continue;
      if (!is_frame_interpolation_baked(original, type)) continue;

      if (original.duration <= FRAME_DURATION_MIN)
      {
        track.children[index].interpolation = Interpolation::NONE;
        continue;
      }

      auto nextFrame = index + 1 < (int)track.children.size() && track.children[index + 1].type == ElementType::FRAME
                           ? track.children[index + 1]
                           : original;
      int duration{};
      int insertIndex = index;

      while (duration < original.duration)
      {
        auto baked = original;
        float amount = interpolation_factor(original.interpolation, (float)duration / original.duration);
        baked.duration = std::min(interval, original.duration - duration);
        baked.interpolation = Interpolation::NONE;
        baked.rotation = glm::mix(original.rotation, nextFrame.rotation, amount);
        baked.position = glm::mix(original.position, nextFrame.position, amount);
        baked.scale = glm::mix(original.scale, nextFrame.scale, amount);
        baked.colorOffset = glm::mix(original.colorOffset, nextFrame.colorOffset, amount);
        baked.tint = glm::mix(original.tint, nextFrame.tint, amount);
        if (isRoundScale) baked.scale = glm::round(baked.scale);
        if (isRoundRotation) baked.rotation = std::round(baked.rotation);

        if (insertIndex == index)
          track.children[insertIndex] = baked;
        else
          track.children.insert(track.children.begin() + insertIndex, baked);

        duration += baked.duration;
        ++insertIndex;
      }
    }
  }

  void interpolated_frames_bake(Element& element, int interval, bool isRoundScale, bool isRoundRotation,
                                InterpolationBakeType type)
  {
    if (is_track(element))
    {
      track_frames_bake(element, interval, isRoundScale, isRoundRotation, type);
      return;
    }

    for (auto& child : element.children)
      interpolated_frames_bake(child, interval, isRoundScale, isRoundRotation, type);
  }

  void special_interpolated_frames_bake(Element& element, int interval, bool isRoundScale, bool isRoundRotation)
  {
    interpolated_frames_bake(element, interval, isRoundScale, isRoundRotation, InterpolationBakeType::SPECIAL);
  }

  void all_interpolated_frames_bake(Element& element, int interval, bool isRoundScale, bool isRoundRotation)
  {
    interpolated_frames_bake(element, interval, isRoundScale, isRoundRotation, InterpolationBakeType::ALL);
  }

  void layer_animation_ids_remap(Element& element, const std::unordered_map<int, int>& remap)
  {
    if (element.type == ElementType::LAYER_ANIMATION)
      if (auto it = remap.find(element.layerId); it != remap.end()) element.layerId = it->second;

    for (auto& child : element.children)
      layer_animation_ids_remap(child, remap);
  }

  Anm2::Anm2()
  {
    root = element_make(ElementType::ANIMATED_ACTOR);

    auto info = element_make(ElementType::INFO);
    info.createdOn = time::get("%m/%d/%Y %I:%M:%S %p");

    auto content = element_make(ElementType::CONTENT);
    content.children.push_back(element_make(ElementType::SPRITESHEETS));
    content.children.push_back(element_make(ElementType::LAYERS));
    content.children.push_back(element_make(ElementType::NULLS));
    content.children.push_back(element_make(ElementType::EVENTS));

    root.children.push_back(std::move(info));
    root.children.push_back(std::move(content));
    root.children.push_back(element_make(ElementType::ANIMATIONS));
  }

  Anm2::Anm2(const std::filesystem::path& path, std::string* errorString) : Anm2() { load(path, errorString); }

  bool Anm2::load(const std::filesystem::path& path, std::string* errorString)
  {
    XMLDocument document{};
    File file(path, "rb");
    if (!file)
    {
      if (errorString) *errorString = "File not found.";
      isValid = false;
      return false;
    }

    if (document.LoadFile(file.get()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      isValid = false;
      return false;
    }

    return anm2_document_load(*this, document, errorString);
  }

  bool Anm2::load_string(std::string_view string, std::string* errorString)
  {
    XMLDocument document{};
    if (document.Parse(string.data(), string.size()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      isValid = false;
      return false;
    }

    return anm2_document_load(*this, document, errorString);
  }

  bool Anm2::save(const std::filesystem::path& path, std::string* errorString, Options options) const
  {
    XMLDocument document{};
    document.InsertFirstChild(to_element(document, options));

    File file(path, "wb");
    if (!file)
    {
      if (errorString) *errorString = "File permissions.";
      return false;
    }

    if (document.SaveFile(file.get()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }

    return true;
  }

  std::string Anm2::to_string(Options options) const
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document, options));
    return xml::document_to_string(document);
  }

  XMLElement* Anm2::to_element(XMLDocument& document, Options options) const
  {
    if (options.isExtendedFormat)
    {
      auto editor = normalized_for_serialize(SERIALIZE_ANM2ED_DEFAULT);
      editor.region_frames_sync(true);
      return element_to_xml(document, editor.root, ElementType::UNKNOWN, SERIALIZE_ANM2ED_DEFAULT);
    }

    auto isaac = normalized_for_serialize(SERIALIZE_ISAAC_DEFAULT);
    isaac.region_frames_sync(true);
    auto out = element_to_xml(document, isaac.root, ElementType::UNKNOWN, SERIALIZE_ISAAC_DEFAULT);

    auto editor = normalized_for_serialize(SERIALIZE_ANM2ED_DEFAULT);
    editor.region_frames_sync(true);
    auto extension = element_to_xml(document, editor.root, ElementType::UNKNOWN, SERIALIZE_ANM2ED_DEFAULT);
    extension->SetName(SOURCE_DOCUMENT_TAG.data());
    out->InsertEndChild(extension);

    return out;
  }

  std::uint64_t Anm2::hash(Options options) const
  {
    auto hash = ANM2_HASH_OFFSET;
    hash_byte_append(hash, options.isExtendedFormat ? 'E' : 'A');
    element_hash_append(hash, root, ElementType::UNKNOWN, SERIALIZE_ANM2ED_DEFAULT);
    return hash;
  }

  bool Anm2::is_special_interpolated_frames() const { return ::anm2ed::is_special_interpolated_frames(root); }

  void Anm2::special_interpolated_frames_bake(int interval, bool isRoundScale, bool isRoundRotation)
  {
    ::anm2ed::special_interpolated_frames_bake(root, std::max(interval, FRAME_DURATION_MIN), isRoundScale,
                                               isRoundRotation);
  }

  void Anm2::region_frames_sync(bool isClearInvalid)
  {
    auto content = child_first_get(root, ElementType::CONTENT);
    auto layers = content ? child_first_get(*content, ElementType::LAYERS) : nullptr;
    auto spritesheets = content ? child_first_get(*content, ElementType::SPRITESHEETS) : nullptr;
    auto animations = element_first_get(root, ElementType::ANIMATIONS);
    if (!layers || !spritesheets || !animations) return;

    for (auto& animation : animations->children)
    {
      if (animation.type != ElementType::ANIMATION) continue;
      auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS);
      if (!layerAnimations) continue;

      tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                  [&](Element& layerAnimation)
                  {
                    auto layer = child_id_get(*layers, ElementType::LAYER_ELEMENT, layerAnimation.layerId);
                    auto spritesheet =
                        layer ? child_id_get(*spritesheets, ElementType::SPRITESHEET, layer->spritesheetId) : nullptr;
                    if (!spritesheet) return;

                    for (auto& frame : layerAnimation.children)
                    {
                      if (frame.type != ElementType::FRAME || frame.regionId == -1) continue;
                      auto region = child_id_get(*spritesheet, ElementType::REGION, frame.regionId);
                      if (!region)
                      {
                        if (isClearInvalid) frame.regionId = -1;
                        continue;
                      }

                      frame.crop = region->crop;
                      frame.size = region->size;
                      frame.pivot = region->pivot;
                    }
                  });
    }
  }

  Anm2 Anm2::normalized_for_serialize(Flags flags) const
  {
    auto normalized = *this;
    source_document_erase(normalized.root);
    group_frames_restore(normalized.root);
    groups_flatten(normalized.root);
    if (has_flag(flags, SERIALIZE_BAKE_GROUP_FRAMES)) group_frames_bake(normalized.root);
    if (has_flag(flags, SERIALIZE_FLATTEN_SPECIAL_INTERPOLATED_FRAMES))
      all_interpolated_frames_bake(normalized.root, FRAME_DURATION_MIN, false, false);
    region_ids_remap(normalized.root);
    if (has_flag(flags, SERIALIZE_GROUPS) && has_flag(flags, SERIALIZE_NESTED_GROUPS))
      groups_nest(normalized.root);
    else
      group_metadata_extract(normalized.root, flags);

    auto content = child_first_get(normalized.root, ElementType::CONTENT);
    if (content) std::erase_if(content->children, [](const Element& element) { return element.tag == "Groups"; });
    if (auto layers = content ? child_first_get(*content, ElementType::LAYERS) : nullptr)
    {
      std::unordered_map<int, int> remap{};
      int nextId{};
      for (auto& layer : layers->children)
      {
        if (layer.type != ElementType::LAYER_ELEMENT) continue;
        remap[layer.id] = nextId;
        layer.id = nextId++;
      }
      layer_animation_ids_remap(normalized.root, remap);
    }

    return normalized;
  }

  Element* Anm2::element_get(ElementType type)
  {
    if (type == ElementType::ANIMATED_ACTOR) return &root;
    if (type == ElementType::ANIMATIONS) return element_first_get(root, type);

    auto content = child_first_get(root, ElementType::CONTENT);
    if (!content) return nullptr;
    if (type == ElementType::CONTENT) return content;
    if (auto container = child_first_get(*content, type)) return container;

    switch (type)
    {
      case ElementType::SPRITESHEETS:
      case ElementType::LAYERS:
      case ElementType::NULLS:
      case ElementType::EVENTS:
      case ElementType::SOUNDS:
        content->children.push_back(element_make(type));
        return &content->children.back();
      default:
        break;
    }

    return element_first_get(root, type);
  }

  const Element* Anm2::element_get(ElementType type) const
  {
    if (type == ElementType::ANIMATED_ACTOR) return &root;
    if (type == ElementType::ANIMATIONS) return element_first_get(root, type);

    auto content = child_first_get(root, ElementType::CONTENT);
    if (!content) return nullptr;
    if (type == ElementType::CONTENT) return content;
    if (auto container = child_first_get(*content, type)) return container;
    return element_first_get(root, type);
  }

  Element* Anm2::element_get(ElementType type, int id)
  {
    if (type == ElementType::ANIMATION)
    {
      auto animations = element_get(ElementType::ANIMATIONS);
      if (!animations || id < 0) return nullptr;
      int current{};
      for (auto& animation : animations->children)
      {
        if (animation.type != ElementType::ANIMATION) continue;
        if (current == id) return &animation;
        ++current;
      }
      return nullptr;
    }

    auto container = element_get(element_container_type_get(type));
    return container ? child_id_get(*container, type, id) : nullptr;
  }

  const Element* Anm2::element_get(ElementType type, int id) const
  {
    if (type == ElementType::ANIMATION)
    {
      auto animations = element_get(ElementType::ANIMATIONS);
      if (!animations || id < 0) return nullptr;
      int current{};
      for (const auto& animation : animations->children)
      {
        if (animation.type != ElementType::ANIMATION) continue;
        if (current == id) return &animation;
        ++current;
      }
      return nullptr;
    }

    auto container = element_get(element_container_type_get(type));
    return container ? child_id_get(*container, type, id) : nullptr;
  }

  Element* Anm2::element_get(int animationIndex, ItemType type, int id)
  {
    auto animation = element_get(ElementType::ANIMATION, animationIndex);
    return animation ? animation_item_get(*animation, type, id) : nullptr;
  }

  const Element* Anm2::element_get(int animationIndex, ItemType type, int id) const
  {
    auto animation = element_get(ElementType::ANIMATION, animationIndex);
    return animation ? animation_item_get(*animation, type, id) : nullptr;
  }

  Element* Anm2::element_get(int animationIndex, ItemType type, int id, int groupType, int groupId)
  {
    auto animation = element_get(ElementType::ANIMATION, animationIndex);
    return animation ? animation_item_get(*animation, type, id, groupType, groupId) : nullptr;
  }

  const Element* Anm2::element_get(int animationIndex, ItemType type, int id, int groupType, int groupId) const
  {
    auto animation = element_get(ElementType::ANIMATION, animationIndex);
    return animation ? animation_item_get(*animation, type, id, groupType, groupId) : nullptr;
  }

  Element* Anm2::element_get(int animationIndex, ItemType type, int frameIndex, int id)
  {
    auto item = element_get(animationIndex, type, id);
    return item ? track_frame_get(*item, frameIndex) : nullptr;
  }

  const Element* Anm2::element_get(int animationIndex, ItemType type, int frameIndex, int id) const
  {
    auto item = element_get(animationIndex, type, id);
    return item ? track_frame_get(*item, frameIndex) : nullptr;
  }

  Element* Anm2::element_get(Reference reference)
  {
    auto itemType = static_cast<ItemType>(reference.itemType);
    auto item =
        element_get(reference.animationIndex, itemType, reference.itemID, reference.groupType, reference.groupId);
    if (reference.frameIndex < 0) return item;
    return item ? track_frame_get(*item, reference.frameIndex) : nullptr;
  }

  const Element* Anm2::element_get(Reference reference) const
  {
    auto itemType = static_cast<ItemType>(reference.itemType);
    auto item =
        element_get(reference.animationIndex, itemType, reference.itemID, reference.groupType, reference.groupId);
    if (reference.frameIndex < 0) return item;
    return item ? track_frame_get(*item, reference.frameIndex) : nullptr;
  }

  std::set<int> Anm2::element_unused(ElementType type) const
  {
    std::set<int> used{};

    if (type == ElementType::SPRITESHEET)
    {
      if (auto layers = element_get(ElementType::LAYERS))
        for (const auto& layer : layers->children)
          if (layer.type == ElementType::LAYER_ELEMENT && layer.spritesheetId != -1) used.insert(layer.spritesheetId);
    }
    else if (auto animations = element_get(ElementType::ANIMATIONS))
      for (const auto& animation : animations->children)
      {
        if (animation.type != ElementType::ANIMATION) continue;
        if (type == ElementType::EVENT_ELEMENT)
        {
          auto triggers = child_first_get(animation, ElementType::TRIGGERS);
          if (!triggers) continue;
          for (const auto& trigger : triggers->children)
            if (trigger.type == ElementType::TRIGGER && trigger.eventId != -1) used.insert(trigger.eventId);
        }
        else if (type == ElementType::SOUND_ELEMENT)
        {
          auto triggers = child_first_get(animation, ElementType::TRIGGERS);
          if (!triggers) continue;
          for (const auto& trigger : triggers->children)
          {
            if (trigger.type != ElementType::TRIGGER) continue;
            for (auto id : trigger.soundIds)
              used.insert(id);
          }
        }
        else if (type == ElementType::LAYER_ELEMENT || type == ElementType::NULL_ELEMENT)
        {
          auto containerType =
              type == ElementType::LAYER_ELEMENT ? ElementType::LAYER_ANIMATIONS : ElementType::NULL_ANIMATIONS;
          auto tracks = child_first_get(animation, containerType);
          if (!tracks) continue;
          auto trackType =
              type == ElementType::LAYER_ELEMENT ? ElementType::LAYER_ANIMATION : ElementType::NULL_ANIMATION;
          tracks_each(*tracks, trackType,
                      [&](const Element& track)
                      {
                        if (track.type == ElementType::LAYER_ANIMATION)
                          used.insert(track.layerId);
                        else if (track.type == ElementType::NULL_ANIMATION)
                          used.insert(track.nullId);
                      });
        }
      }

    std::set<int> unused{};
    if (auto container = element_get(element_container_type_get(type)))
      for (const auto& element : container->children)
        if (element.type == type && !used.contains(element.id)) unused.insert(element.id);
    return unused;
  }

  std::set<int> Anm2::element_unused(ElementType type, const Element& animation) const
  {
    if (type != ElementType::LAYER_ELEMENT && type != ElementType::NULL_ELEMENT) return {};

    std::set<int> used{};
    auto containerType =
        type == ElementType::LAYER_ELEMENT ? ElementType::LAYER_ANIMATIONS : ElementType::NULL_ANIMATIONS;
    if (auto tracks = child_first_get(animation, containerType))
    {
      auto trackType = type == ElementType::LAYER_ELEMENT ? ElementType::LAYER_ANIMATION : ElementType::NULL_ANIMATION;
      tracks_each(*tracks, trackType,
                  [&](const Element& track)
                  {
                    if (track.type == ElementType::LAYER_ANIMATION)
                      used.insert(track.layerId);
                    else if (track.type == ElementType::NULL_ANIMATION)
                      used.insert(track.nullId);
                  });
    }

    std::set<int> unused{};
    if (auto container = element_get(element_container_type_get(type)))
      for (const auto& element : container->children)
        if (element.type == type && !used.contains(element.id)) unused.insert(element.id);
    return unused;
  }

  std::set<int> Anm2::element_unused(ElementType type, int parentId) const
  {
    if (type != ElementType::REGION) return {};

    std::set<int> used{};
    auto animations = element_first_get(root, ElementType::ANIMATIONS);
    if (animations)
      for (const auto& animation : animations->children)
      {
        if (animation.type != ElementType::ANIMATION) continue;
        auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS);
        if (!layerAnimations) continue;
        tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                    [&](const Element& layerAnimation)
                    {
                      auto layer = element_get(ElementType::LAYER_ELEMENT, layerAnimation.layerId);
                      if (!layer || layer->spritesheetId != parentId) return;
                      for (const auto& frame : layerAnimation.children)
                        if (frame.type == ElementType::FRAME && frame.regionId != -1) used.insert(frame.regionId);
                    });
      }

    std::set<int> unused{};
    if (auto spritesheet = element_get(ElementType::SPRITESHEET, parentId))
      for (const auto& region : spritesheet->children)
        if (region.type == ElementType::REGION && !used.contains(region.id)) unused.insert(region.id);
    return unused;
  }

  bool Anm2::deserialize(ElementType type, const std::string& string, bool isAppend, std::string* errorString,
                         const std::filesystem::path& directory)
  {
    XMLDocument document{};
    if (document.Parse(string.c_str()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }

    auto tag = element_tag_get(type);
    if (tag.empty() || !document.FirstChildElement(tag.data()))
    {
      if (errorString) *errorString = std::format("No valid {}(s).", tag);
      return false;
    }

    auto containerType = element_container_type_get(type);
    auto container = element_get(containerType);
    if (!container)
    {
      if (errorString) *errorString = std::format("No {} container.", element_tag_get(containerType));
      return false;
    }

    std::optional<WorkingDirectory> workingDirectory{};
    if ((type == ElementType::SOUND_ELEMENT || type == ElementType::SPRITESHEET) && !directory.empty())
      workingDirectory.emplace(directory);

    for (auto xmlElement = document.FirstChildElement(tag.data()); xmlElement;
         xmlElement = xmlElement->NextSiblingElement(tag.data()))
    {
      auto element = element_read(xmlElement);
      if (element.type != type) continue;
      if (isAppend)
        element.id = element_child_next_id_get(*container, type);
      else
        element_child_id_erase(*container, type, element.id);
      if (type == ElementType::SOUND_ELEMENT || type == ElementType::SPRITESHEET)
        element.path = path::backslash_handle(element.path);
      container->children.push_back(element);
    }

    return true;
  }

  bool Anm2::regions_deserialize(int spritesheetId, const std::string& string, bool isAppend, std::string* errorString)
  {
    XMLDocument document{};
    if (document.Parse(string.c_str()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }

    if (!document.FirstChildElement("Region"))
    {
      if (errorString) *errorString = "No valid region(s).";
      return false;
    }

    auto spritesheet = element_get(ElementType::SPRITESHEET, spritesheetId);
    if (!spritesheet)
    {
      if (errorString) *errorString = "No spritesheet.";
      return false;
    }

    for (auto element = document.FirstChildElement("Region"); element; element = element->NextSiblingElement("Region"))
    {
      auto region = element_read(element);
      if (region.type != ElementType::REGION) continue;
      if (isAppend)
        region.id = element_child_next_id_get(*spritesheet, ElementType::REGION);
      else
        element_child_id_erase(*spritesheet, ElementType::REGION, region.id);
      spritesheet->children.push_back(region);
    }

    return true;
  }

  Element Anm2::frame_effective(int layerId, const Element& frame) const
  {
    auto resolved = frame;
    if (frame.regionId == -1) return resolved;

    auto layer = element_get(ElementType::LAYER_ELEMENT, layerId);
    if (!layer) return resolved;

    auto spritesheet = element_get(ElementType::SPRITESHEET, layer->spritesheetId);
    if (!spritesheet) return resolved;

    auto region = element_child_id_get(*spritesheet, ElementType::REGION, frame.regionId);
    if (!region) return resolved;

    resolved.crop = region->crop;
    resolved.size = region->size;
    resolved.pivot = region->pivot;
    return resolved;
  }

  glm::vec4 Anm2::animation_rect(const Element& animation, bool isRootTransform) const
  {
    constexpr glm::ivec2 CORNERS[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

    float minX = std::numeric_limits<float>::infinity();
    float minY = std::numeric_limits<float>::infinity();
    float maxX = -std::numeric_limits<float>::infinity();
    float maxY = -std::numeric_limits<float>::infinity();
    bool isAny{};

    for (float t = 0.0f; t < (float)animation.frameNum; t += 1.0f)
    {
      glm::mat4 transform(1.0f);

      auto root = animation_item_get(animation, ItemType::ROOT);
      if (isRootTransform && root)
      {
        auto rootFrame = frame_generate(*root, t);
        transform *= math::quad_model_parent_get(rootFrame.position, {}, math::percent_to_unit(rootFrame.scale),
                                                 rootFrame.rotation);
      }

      auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS);
      if (!layerAnimations) continue;

      tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                  [&](const Element& layerAnimation)
                  {
                    if (!layerAnimation.isVisible || !is_track_group_visible(*layerAnimations, layerAnimation)) return;

                    auto itemTransform = transform;
                    if (isRootTransform && layerAnimation.groupId != -1)
                      if (auto group = child_id_get(*layerAnimations, ElementType::GROUP, layerAnimation.groupId))
                        if (auto groupRoot = child_first_get(*group, ElementType::ROOT_ANIMATION))
                        {
                          auto groupRootFrame = frame_generate(*groupRoot, t);
                          itemTransform *= math::quad_model_parent_get(groupRootFrame.position, {},
                                                                       math::percent_to_unit(groupRootFrame.scale),
                                                                       groupRootFrame.rotation);
                        }

                    auto frame = frame_effective(layerAnimation.layerId, frame_generate(layerAnimation, t));
                    if (frame.size == glm::vec2() || !frame.isVisible) return;

                    auto layerTransform =
                        itemTransform * math::quad_model_get(frame.size, frame.position, frame.pivot,
                                                             math::percent_to_unit(frame.scale), frame.rotation);
                    for (auto& corner : CORNERS)
                    {
                      auto world = layerTransform * glm::vec4(corner, 0.0f, 1.0f);
                      minX = std::min(minX, world.x);
                      minY = std::min(minY, world.y);
                      maxX = std::max(maxX, world.x);
                      maxY = std::max(maxY, world.y);
                      isAny = true;
                    }
                  });
    }

    if (!isAny) return glm::vec4(-1.0f);
    return {minX, minY, maxX - minX, maxY - minY};
  }

  Element* animation_container_get(Element& animation, ElementType type)
  {
    if (auto container = child_first_get(animation, type)) return container;
    animation.children.push_back(element_make(type));
    return &animation.children.back();
  }

  int Anm2::layer_animation_add(int animationIndex, int id, int insertBeforeId, std::string name, int spritesheetId,
                                types::destination::Type destination)
  {
    auto layers = element_get(ElementType::LAYERS);
    if (!layers) return -1;

    id = id == -1 ? element_child_next_id_get(*layers, ElementType::LAYER_ELEMENT) : id;
    auto layer = element_child_id_get(*layers, ElementType::LAYER_ELEMENT, id);
    if (!layer)
    {
      layers->children.push_back(element_make(ElementType::LAYER_ELEMENT));
      layer = &layers->children.back();
      layer->id = id;
    }

    if (!name.empty()) layer->name = name;
    layer->spritesheetId = element_get(ElementType::SPRITESHEET, spritesheetId) ? spritesheetId : 0;

    auto add = [&](Element& animation)
    {
      if (animation_item_get(animation, ItemType::LAYER, id)) return;
      auto layerAnimations = animation_container_get(animation, ElementType::LAYER_ANIMATIONS);
      auto item = element_make(ElementType::LAYER_ANIMATION);
      item.layerId = id;

      if (insertBeforeId != -1)
        for (auto it = layerAnimations->children.begin(); it != layerAnimations->children.end(); ++it)
          if (it->type == ElementType::LAYER_ANIMATION && it->layerId == insertBeforeId)
          {
            layerAnimations->children.insert(it, item);
            return;
          }

      layerAnimations->children.push_back(item);
    };

    if (destination == types::destination::ALL)
    {
      if (auto animations = element_get(ElementType::ANIMATIONS))
        for (auto& animation : animations->children)
          if (animation.type == ElementType::ANIMATION) add(animation);
    }
    else if (auto animation = element_get(ElementType::ANIMATION, animationIndex))
      add(*animation);

    return id;
  }

  int Anm2::null_animation_add(int animationIndex, int id, std::string name, bool isShowRect,
                               types::destination::Type destination)
  {
    auto nulls = element_get(ElementType::NULLS);
    if (!nulls) return -1;

    id = id == -1 ? element_child_next_id_get(*nulls, ElementType::NULL_ELEMENT) : id;
    auto null = element_child_id_get(*nulls, ElementType::NULL_ELEMENT, id);
    if (!null)
    {
      nulls->children.push_back(element_make(ElementType::NULL_ELEMENT));
      null = &nulls->children.back();
      null->id = id;
    }

    if (!name.empty()) null->name = name;
    null->isShowRect = isShowRect;

    auto add = [&](Element& animation)
    {
      if (animation_item_get(animation, ItemType::NULL_, id)) return;
      auto nullAnimations = animation_container_get(animation, ElementType::NULL_ANIMATIONS);
      auto item = element_make(ElementType::NULL_ANIMATION);
      item.nullId = id;
      nullAnimations->children.push_back(item);
    };

    if (destination == types::destination::ALL)
    {
      if (auto animations = element_get(ElementType::ANIMATIONS))
        for (auto& animation : animations->children)
          if (animation.type == ElementType::ANIMATION) add(animation);
    }
    else if (auto animation = element_get(ElementType::ANIMATION, animationIndex))
      add(*animation);

    return id;
  }

  bool Anm2::animations_deserialize(const std::string& string, int start, std::set<int>& indices,
                                    std::string* errorString, std::set<int>* groupIds)
  {
    XMLDocument document{};
    if (document.Parse(string.c_str()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }

    auto animations = element_get(ElementType::ANIMATIONS);
    if (!animations)
    {
      if (errorString) *errorString = "No animations container.";
      return false;
    }

    start = std::clamp(start, 0, animations_count_get(*animations));

    std::vector<Element> items{};
    std::map<int, int> groupRemap{};
    auto nextGroupId = element_child_next_id_get(*animations, ElementType::GROUP);
    for (auto element = document.FirstChildElement(); element; element = element->NextSiblingElement())
    {
      auto item = element_read(element);
      if (item.type != ElementType::ANIMATION && item.type != ElementType::GROUP) continue;

      if (item.type == ElementType::GROUP)
      {
        auto sourceGroupId = item.id;
        if (!groupRemap.contains(sourceGroupId)) groupRemap[sourceGroupId] = nextGroupId++;
        item.id = groupRemap[sourceGroupId];
        if (groupIds) groupIds->insert(item.id);
      }

      items.push_back(item);
    }

    if (items.empty())
    {
      if (errorString) *errorString = "No valid animation(s).";
      return false;
    }

    int count{};
    for (auto& item : items)
    {
      if (item.type == ElementType::ANIMATION)
      {
        if (auto mapped = groupRemap.find(item.groupId); mapped != groupRemap.end()) item.groupId = mapped->second;
        indices.insert(start + count);
        ++count;
      }
    }

    auto childIndex = animations_child_insert_index_get(*animations, start);
    animations->children.insert(animations->children.begin() + childIndex, items.begin(), items.end());
    return true;
  }

  int track_length_get(const Element& track)
  {
    int length{};
    if (track.type == ElementType::TRIGGERS)
    {
      for (const auto& trigger : track.children)
        if (trigger.type == ElementType::TRIGGER) length = std::max(length, trigger.atFrame);
      return length;
    }

    for (const auto& frame : track.children)
      if (frame.type == ElementType::FRAME) length += frame.duration;
    return length;
  }

  int animation_length_get(const Element& animation)
  {
    int length{};
    auto group_roots_length_apply = [&](const Element& container)
    {
      for (const auto& child : container.children)
        if (child.type == ElementType::GROUP)
          if (auto rootAnimation = child_first_get(child, ElementType::ROOT_ANIMATION))
            length = std::max(length, track_length_get(*rootAnimation));
    };

    if (auto rootAnimation = child_first_get(animation, ElementType::ROOT_ANIMATION))
      length = std::max(length, track_length_get(*rootAnimation));
    if (auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS))
    {
      group_roots_length_apply(*layerAnimations);
      tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                  [&](const Element& track) { length = std::max(length, track_length_get(track)); });
    }
    if (auto nullAnimations = child_first_get(animation, ElementType::NULL_ANIMATIONS))
    {
      group_roots_length_apply(*nullAnimations);
      tracks_each(*nullAnimations, ElementType::NULL_ANIMATION,
                  [&](const Element& track) { length = std::max(length, track_length_get(track)); });
    }
    if (auto triggers = child_first_get(animation, ElementType::TRIGGERS))
      length = std::max(length, track_length_get(*triggers));
    return std::max(length, FRAME_DURATION_MIN);
  }

  Element* track_get(Element& tracks, const Element& source)
  {
    for (auto& track : tracks.children)
    {
      if (track.type == ElementType::GROUP)
        if (auto result = track_get(track, source)) return result;
      if (track.type != source.type) continue;
      if (track.type == ElementType::LAYER_ANIMATION && track.layerId == source.layerId) return &track;
      if (track.type == ElementType::NULL_ANIMATION && track.nullId == source.nullId) return &track;
    }
    return nullptr;
  }

  void track_merge(Element& destination, const Element& source, types::merge::Type type)
  {
    switch (type)
    {
      case types::merge::APPEND:
        destination.children.insert(destination.children.end(), source.children.begin(), source.children.end());
        break;
      case types::merge::PREPEND:
        destination.children.insert(destination.children.begin(), source.children.begin(), source.children.end());
        break;
      case types::merge::REPLACE:
        if (destination.children.size() < source.children.size()) destination.children.resize(source.children.size());
        for (int i = 0; i < (int)source.children.size(); ++i)
          destination.children[i] = source.children[i];
        break;
      case types::merge::IGNORE:
      default:
        break;
    }
  }

  void animation_tracks_merge(Element& destination, const Element& source, ElementType containerType,
                              types::merge::Type type)
  {
    auto sourceTracks = child_first_get(source, containerType);
    if (!sourceTracks) return;

    auto destinationTracks = child_first_get(destination, containerType);
    if (!destinationTracks)
    {
      destination.children.push_back(element_make(containerType));
      destinationTracks = &destination.children.back();
    }

    for (const auto& sourceTrack : sourceTracks->children)
    {
      if (sourceTrack.type == ElementType::GROUP)
        destinationTracks->children.push_back(sourceTrack);
      else if (auto destinationTrack = track_get(*destinationTracks, sourceTrack))
        track_merge(*destinationTrack, sourceTrack, type);
      else
        destinationTracks->children.push_back(sourceTrack);
    }
  }

  int Anm2::animations_merge(int target, std::set<int>& sources, types::merge::Type type, bool isDeleteAfter)
  {
    auto animations = element_get(ElementType::ANIMATIONS);
    auto targetAnimation = element_get(ElementType::ANIMATION, target);
    if (!animations || !targetAnimation) return target;

    if (!targetAnimation->name.ends_with(ANIMATION_MERGED_SUFFIX))
      targetAnimation->name += std::string(ANIMATION_MERGED_SUFFIX);

    for (auto index : sources)
    {
      if (index == target) continue;
      auto source = element_get(ElementType::ANIMATION, index);
      targetAnimation = element_get(ElementType::ANIMATION, target);
      if (!source || !targetAnimation) continue;

      if (auto sourceRoot = child_first_get(*source, ElementType::ROOT_ANIMATION))
      {
        auto targetRoot = child_first_get(*targetAnimation, ElementType::ROOT_ANIMATION);
        if (!targetRoot)
        {
          targetAnimation->children.push_back(element_make(ElementType::ROOT_ANIMATION));
          targetRoot = &targetAnimation->children.back();
        }
        track_merge(*targetRoot, *sourceRoot, type);
      }

      animation_tracks_merge(*targetAnimation, *source, ElementType::LAYER_ANIMATIONS, type);
      animation_tracks_merge(*targetAnimation, *source, ElementType::NULL_ANIMATIONS, type);

      if (auto sourceTriggers = child_first_get(*source, ElementType::TRIGGERS))
      {
        auto targetTriggers = child_first_get(*targetAnimation, ElementType::TRIGGERS);
        if (!targetTriggers)
        {
          targetAnimation->children.push_back(element_make(ElementType::TRIGGERS));
          targetTriggers = &targetAnimation->children.back();
        }
        track_merge(*targetTriggers, *sourceTriggers, type);
      }
    }

    int finalIndex = target;
    if (isDeleteAfter)
      for (auto it = sources.rbegin(); it != sources.rend(); ++it)
      {
        auto source = *it;
        auto sourceChildIndex = animations_child_index_get(*animations, source);
        if (source == target || sourceChildIndex == -1) continue;
        animations->children.erase(animations->children.begin() + sourceChildIndex);
        if (source < finalIndex) --finalIndex;
      }

    if (auto finalAnimation = element_get(ElementType::ANIMATION, finalIndex))
      finalAnimation->frameNum = animation_length_get(*finalAnimation);
    return finalIndex;
  }
}
