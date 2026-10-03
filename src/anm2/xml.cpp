#include "internal.hpp"

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
  X("ShaderId", shaderId)                                                                                              \
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
  X("YScale", scale.y)                                                                                                 \
  X("ShearX", shear.x)                                                                                                 \
  X("ShearY", shear.y)

#define ANM2_COLOR_ATTRIBUTES                                                                                          \
  X("RedTint", tint.r)                                                                                                 \
  X("GreenTint", tint.g)                                                                                               \
  X("BlueTint", tint.b)                                                                                                \
  X("AlphaTint", tint.a)                                                                                               \
  X("RedOffset", colorOffset.r)                                                                                        \
  X("GreenOffset", colorOffset.g)                                                                                      \
  X("BlueOffset", colorOffset.b)

  constexpr std::string_view LEGACY_OVERLAY_TAG = "Overlay";


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
      auto childName = std::string_view(child->Name() ? child->Name() : "");
      if (childName == LEGACY_OVERLAY_TAG)
      {
        child = child->NextSiblingElement();
        continue;
      }

      auto childType = element_type_get(childName);
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
    if ((element.type == ElementType::SHADERS || element.type == ElementType::SHADER ||
         element.type == ElementType::UNIFORM || element.type == ElementType::COMPONENT) &&
        !has_flag(flags, SERIALIZE_EXTENSIONS))
      return true;
    if (element.type == ElementType::SHADERS && element.children.empty()) return true;
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
    if (has_flag(flags, SERIALIZE_EXTENSIONS))
    {
      out->SetAttribute("ShearX", element.shear.x);
      out->SetAttribute("ShearY", element.shear.y);
    }
    out->SetAttribute("RedTint", color_write(element.tint.r));
    out->SetAttribute("GreenTint", color_write(element.tint.g));
    out->SetAttribute("BlueTint", color_write(element.tint.b));
    out->SetAttribute("AlphaTint", color_write(element.tint.a));
    out->SetAttribute("RedOffset", color_write(element.colorOffset.r));
    out->SetAttribute("GreenOffset", color_write(element.colorOffset.g));
    out->SetAttribute("BlueOffset", color_write(element.colorOffset.b));
    out->SetAttribute("Rotation", element.rotation);
    interpolation_write(out, element.interpolation);
    if (has_flag(flags, SERIALIZE_EXTENSIONS) && element.shaderId != -1)
      out->SetAttribute("ShaderId", element.shaderId);
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
    else if (element.type == ElementType::SPRITESHEET)
    {
      out->SetAttribute("Id", element.id);
      path_set(out, "Path", element.path);
    }
    else if (element.type == ElementType::SHADER)
    {
      out->SetAttribute("Id", element.id);
      out->SetAttribute("Name", element.name.c_str());
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
      baked.shear = glm::mix(original.shear, nextFrame.shear, amount);
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
}
