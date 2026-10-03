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

  // Attributes read into Element members (or retired by older anm2ed builds) are not passed through.
#define X(attribute, member) attribute,
  constexpr std::string_view KNOWN_ATTRIBUTES[] = {ANM2_STRING_ATTRIBUTES ANM2_PATH_ATTRIBUTES ANM2_INT_ATTRIBUTES
                                                       ANM2_BOOL_ATTRIBUTES ANM2_FLOAT_ATTRIBUTES ANM2_COLOR_ATTRIBUTES
                                                   "Interpolated",
                                                   "Origin",
                                                   "BakeInterpolation",
                                                   "BakeDelay",
                                                   "BakeCount",
                                                   "OriginalDelay"};
#undef X

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

  Origin origin_read(const XMLElement* element)
  {
    auto value = element ? element->Attribute("Origin") : nullptr;
    if (!value) return Origin::CUSTOM;

    for (std::size_t i = 0; i < ORIGIN_VALUES.size(); ++i)
      if (!ORIGIN_VALUES[i].empty() && ORIGIN_VALUES[i] == value) return (Origin)i;

    return Origin::CUSTOM;
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

    for (auto attribute = element->FirstAttribute(); attribute; attribute = attribute->Next())
      if (!std::ranges::contains(KNOWN_ATTRIBUTES, std::string_view(attribute->Name())))
        out.extraAttributes.emplace_back(attribute->Name(), attribute->Value());
    if (auto text = element->GetText()) out.text = text;

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

  struct WriteRule
  {
    ElementType type;
    Flag flag;
    bool isEmptySkipped;
  };

  constexpr WriteRule WRITE_RULES[] = {{ElementType::SHADERS, SERIALIZE_EXTENSIONS, true},
                                       {ElementType::SHADER, SERIALIZE_EXTENSIONS, false},
                                       {ElementType::UNIFORM, SERIALIZE_EXTENSIONS, false},
                                       {ElementType::COMPONENT, SERIALIZE_EXTENSIONS, false},
                                       {ElementType::SOUNDS, SERIALIZE_SOUNDS, true},
                                       {ElementType::LAYER_ANIMATION_GROUPS, SERIALIZE_GROUPS, true},
                                       {ElementType::NULL_ANIMATION_GROUPS, SERIALIZE_GROUPS, true},
                                       {ElementType::REGION, SERIALIZE_REGIONS, false},
                                       {ElementType::GROUP, SERIALIZE_GROUPS, false}};

  struct XmlSink : ElementSink
  {
    XMLDocument& document;
    std::vector<XMLElement*> stack{};
    XMLElement* root{};

    explicit XmlSink(XMLDocument& document) : document(document) {}

    void open(std::string_view tag) override
    {
      auto element = document.NewElement(std::string(tag).c_str());
      if (stack.empty())
        root = element;
      else
        stack.back()->InsertEndChild(element);
      stack.push_back(element);
    }

    void attribute(const char* name, const char* value) override { stack.back()->SetAttribute(name, value); }
    void attribute(const char* name, int value) override { stack.back()->SetAttribute(name, value); }
    void attribute(const char* name, bool value) override { stack.back()->SetAttribute(name, value); }
    void attribute(const char* name, float value) override { stack.back()->SetAttribute(name, value); }
    void text(const char* value) override { stack.back()->SetText(value); }
    void close() override { stack.pop_back(); }
  };

  bool is_nested_group_parent(ElementType parentType, Flags flags)
  {
    return has_flag(flags, SERIALIZE_NESTED_GROUPS) && group_child_type_get(parentType) != ElementType::UNKNOWN;
  }

  bool is_group_id_serialized(Flags flags)
  {
    return has_flag(flags, SERIALIZE_GROUPS) && !has_flag(flags, SERIALIZE_NESTED_GROUPS);
  }

  bool element_write_skip(const Element& element, ElementType parentType, Flags flags)
  {
    if (!is_track_child_valid(parentType, element.type)) return true;
    if (element.type == ElementType::SOUND_ELEMENT && parentType == ElementType::TRIGGER) return true;
    for (const auto& rule : WRITE_RULES)
      if (rule.type == element.type)
        return !has_flag(flags, rule.flag) || (rule.isEmptySkipped && element.children.empty());
    return false;
  }

  bool is_frame_bake_serialized(const Element& frame, Flags flags)
  {
    return has_flag(flags, SERIALIZE_BAKE_SPECIAL_INTERPOLATED_FRAMES) && frame.type == ElementType::FRAME &&
           frame.interpolation != Interpolation::NONE && frame.interpolation != Interpolation::LINEAR;
  }

  void path_emit(ElementSink& sink, const char* name, const std::filesystem::path& value)
  {
    if (!value.empty()) sink.attribute(name, path::to_utf8(value).c_str());
  }

  void interpolation_emit(ElementSink& sink, const char* name, Interpolation interpolation)
  {
    auto value = INTERPOLATION_VALUES[(std::size_t)interpolation];
    if (interpolation == Interpolation::NONE || interpolation == Interpolation::LINEAR)
      sink.attribute(name, interpolation == Interpolation::LINEAR);
    else if (!value.empty())
      sink.attribute(name, value.data());
  }

  void id_name_emit(ElementSink& sink, const Element& element)
  {
    sink.attribute("Id", element.id);
    sink.attribute("Name", element.name.c_str());
  }

  void binding_value_emit(ElementSink& sink, const Element& element)
  {
    if (!element.binding.empty()) sink.attribute("Binding", element.binding.c_str());
    if (!element.value.empty()) sink.attribute("Value", element.value.c_str());
  }

  void group_id_emit(ElementSink& sink, const Element& element, Flags flags)
  {
    if (element.groupId != -1 && is_group_id_serialized(flags)) sink.attribute("GroupId", element.groupId);
  }

  void crop_size_emit(ElementSink& sink, const Element& element)
  {
    sink.attribute("XCrop", element.crop.x);
    sink.attribute("YCrop", element.crop.y);
    sink.attribute("Width", element.size.x);
    sink.attribute("Height", element.size.y);
  }

  void frame_attributes_emit(ElementSink& sink, const Element& element, ElementType parentType, Flags flags)
  {
    if (parentType == ElementType::LAYER_ANIMATION)
    {
      bool isHasValidRegion = has_flag(flags, SERIALIZE_REGIONS) && element.regionId != -1;
      if (isHasValidRegion) sink.attribute("RegionId", element.regionId);
      if (has_flag(flags, SERIALIZE_REDUNDANT_FRAME_REGION_VALUES) || !isHasValidRegion)
      {
        sink.attribute("XPivot", element.pivot.x);
        sink.attribute("YPivot", element.pivot.y);
        crop_size_emit(sink, element);
      }
    }

    sink.attribute("XPosition", element.position.x);
    sink.attribute("YPosition", element.position.y);
    sink.attribute("Delay", element.duration);
    sink.attribute("Visible", element.isVisible);
    sink.attribute("XScale", element.scale.x);
    sink.attribute("YScale", element.scale.y);
    if (has_flag(flags, SERIALIZE_EXTENSIONS))
    {
      sink.attribute("ShearX", element.shear.x);
      sink.attribute("ShearY", element.shear.y);
    }
    sink.attribute("RedTint", color_write(element.tint.r));
    sink.attribute("GreenTint", color_write(element.tint.g));
    sink.attribute("BlueTint", color_write(element.tint.b));
    sink.attribute("AlphaTint", color_write(element.tint.a));
    sink.attribute("RedOffset", color_write(element.colorOffset.r));
    sink.attribute("GreenOffset", color_write(element.colorOffset.g));
    sink.attribute("BlueOffset", color_write(element.colorOffset.b));
    sink.attribute("Rotation", element.rotation);
    interpolation_emit(sink, "Interpolated", element.interpolation);
    if (has_flag(flags, SERIALIZE_EXTENSIONS) && element.shaderId != -1) sink.attribute("ShaderId", element.shaderId);
  }

  void element_attributes_emit(ElementSink& sink, const Element& element, ElementType parentType, Flags flags)
  {
    switch (element.type)
    {
      case ElementType::INFO:
        sink.attribute("CreatedBy", element.createdBy.c_str());
        sink.attribute("CreatedOn", element.createdOn.c_str());
        sink.attribute("Fps", element.fps);
        sink.attribute("Version", element.version);
        break;
      case ElementType::ANIMATIONS:
        sink.attribute("DefaultAnimation", element.defaultAnimation.c_str());
        break;
      case ElementType::SPRITESHEET:
        sink.attribute("Id", element.id);
        path_emit(sink, "Path", element.path);
        break;
      case ElementType::SHADER:
        id_name_emit(sink, element);
        path_emit(sink, "Vertex", element.vertex);
        path_emit(sink, "Fragment", element.fragment);
        break;
      case ElementType::UNIFORM:
        sink.attribute("Name", element.name.c_str());
        binding_value_emit(sink, element);
        break;
      case ElementType::COMPONENT:
        sink.attribute("Index", element.index);
        binding_value_emit(sink, element);
        break;
      case ElementType::REGION:
      {
        id_name_emit(sink, element);
        crop_size_emit(sink, element);
        auto origin = ORIGIN_VALUES[(std::size_t)element.origin];
        if (!origin.empty())
          sink.attribute("Origin", origin.data());
        else
        {
          sink.attribute("XPivot", element.pivot.x);
          sink.attribute("YPivot", element.pivot.y);
        }
        break;
      }
      case ElementType::LAYER_ELEMENT:
        id_name_emit(sink, element);
        sink.attribute("SpritesheetId", element.spritesheetId);
        break;
      case ElementType::NULL_ELEMENT:
        id_name_emit(sink, element);
        if (element.isShowRect) sink.attribute("ShowRect", element.isShowRect);
        break;
      case ElementType::GROUP:
        if (!is_nested_group_parent(parentType, flags)) sink.attribute("Id", element.id);
        sink.attribute("Name", element.name.c_str());
        sink.attribute("IsExpanded", element.isExpanded);
        sink.attribute("Visible", element.isVisible);
        if ((parentType == ElementType::LAYER_ANIMATION_GROUPS || parentType == ElementType::NULL_ANIMATION_GROUPS) &&
            element.index != -1)
          sink.attribute("Index", element.index);
        break;
      case ElementType::EVENT_ELEMENT:
        id_name_emit(sink, element);
        break;
      case ElementType::SOUND_ELEMENT:
        sink.attribute("Id", element.id);
        if (parentType != ElementType::TRIGGER) path_emit(sink, "Path", element.path);
        break;
      case ElementType::ANIMATION:
        sink.attribute("Name", element.name.c_str());
        sink.attribute("FrameNum", element.frameNum);
        sink.attribute("Loop", element.isLoop);
        group_id_emit(sink, element, flags);
        break;
      case ElementType::LAYER_ANIMATION:
      case ElementType::NULL_ANIMATION:
        sink.attribute(element.type == ElementType::LAYER_ANIMATION ? "LayerId" : "NullId", track_id_get(element));
        sink.attribute("Visible", element.isVisible);
        group_id_emit(sink, element, flags);
        break;
      case ElementType::FRAME:
        frame_attributes_emit(sink, element, parentType, flags);
        break;
      case ElementType::TRIGGER:
        if (element.eventId != -1) sink.attribute("EventId", element.eventId);
        sink.attribute("AtFrame", element.atFrame);
        break;
      default:
        break;
    }
  }

  void element_emit(ElementSink& sink, const Element& element, ElementType parentType, Flags flags,
                    std::optional<BakeAttributes> bake)
  {
    auto tag = element.type == ElementType::UNKNOWN ? std::string_view(element.tag) : element_tag_get(element.type);
    sink.open(tag.empty() ? std::string_view(element.tag) : tag);
    element_attributes_emit(sink, element, parentType, flags);
    for (const auto& [name, value] : element.extraAttributes)
      sink.attribute(name.c_str(), value.c_str());
    if (!element.text.empty()) sink.text(element.text.c_str());
    if (bake)
    {
      auto value = INTERPOLATION_VALUES[(std::size_t)bake->interpolation];
      if (!value.empty()) sink.attribute("BakeInterpolation", value.data());
      sink.attribute("BakeDelay", bake->delay);
    }
    sink.body();

    if (element.type == ElementType::TRIGGER && has_flag(flags, SERIALIZE_SOUNDS))
      for (auto soundId : element.soundIds)
      {
        if (soundId == -1) continue;
        auto sound = element_make(ElementType::SOUND_ELEMENT);
        sound.id = soundId;
        element_emit(sink, sound, ElementType::TRIGGER, flags);
      }

    for (int i = 0; i < (int)element.children.size(); ++i)
    {
      const auto& child = element.children[i];
      if (element_write_skip(child, element.type, flags)) continue;
      if (!is_track(element) || !is_frame_bake_serialized(child, flags))
      {
        element_emit(sink, child, element.type, flags);
        continue;
      }

      auto isNextFrame = i + 1 < (int)element.children.size() && element.children[i + 1].type == ElementType::FRAME;
      auto baked =
          frame_bake_split(child, isNextFrame ? element.children[i + 1] : child, FRAME_DURATION_MIN, false, false);
      for (int bakeIndex = 0; bakeIndex < (int)baked.size(); ++bakeIndex)
        element_emit(sink, baked[bakeIndex], element.type, flags,
                     bakeIndex == 0 ? std::optional<BakeAttributes>({child.interpolation, (int)baked.size()})
                                    : std::nullopt);
    }
    sink.close();
  }

  XMLElement* element_to_xml(XMLDocument& document, const Element& element, ElementType parentType, Flags flags)
  {
    XmlSink sink(document);
    element_emit(sink, element, parentType, flags);
    return sink.root;
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

}
