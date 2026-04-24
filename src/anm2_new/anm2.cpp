#include "anm2.hpp"

#include <tinyxml2/tinyxml2.h>

#include "file_.hpp"
#include "path_.hpp"
#include "xml_.hpp"

namespace anm2ed::anm2_new
{
  using namespace tinyxml2;

  template <typename Enum, std::size_t N>
  constexpr std::string_view enum_string_get(const std::array<std::pair<Enum, std::string_view>, N>& mappings,
                                             Enum value)
  {
    for (const auto& [mappedValue, mappedString] : mappings)
      if (mappedValue == value) return mappedString;
    return {};
  }

  template <typename Enum, std::size_t N>
  constexpr Enum string_enum_get(const std::array<std::pair<Enum, std::string_view>, N>& mappings,
                                 std::string_view value, Enum fallback)
  {
    for (const auto& [mappedValue, mappedString] : mappings)
      if (mappedString == value) return mappedValue;
    return fallback;
  }

  constexpr bool is_flag_set(Anm2::Flags flags, Anm2::Flag flag) { return (flags & flag) != 0; }

  std::filesystem::path asset_path_resolve(const std::filesystem::path& directory,
                                           const std::filesystem::path& assetPath)
  {
    if (assetPath.empty()) return {};

    auto resolved = assetPath.is_absolute() ? assetPath : directory / assetPath;
    resolved = util::path::lower_case_backslash_handle(resolved);
    if (util::path::is_exist(resolved)) return resolved;
    return util::path::lower_case_backslash_handle(assetPath);
  }

  std::string_view Anm2::Element::tag_get(Type type) { return enum_string_get(TYPE_TAGS, type); }

  Anm2::Element::Type Anm2::Element::type_get(std::string_view tag, Type parentType)
  {
    (void)parentType;
    return string_enum_get(TYPE_TAGS, tag, UNKNOWN);
  }

  Anm2::Element::Element(const std::filesystem::path& path, std::string* errorString)
  {
    XMLDocument document{};
    if (!document_load(document, path, errorString)) return;
    if (auto* root = document.RootElement()) *this = from_xml(root);
  }

  Anm2::Element::Element(std::string_view xml, Source, std::string* errorString)
  {
    XMLDocument document{};
    if (!document_parse(document, xml, errorString)) return;
    if (auto* root = document.RootElement()) *this = from_xml(root);
  }

  bool Anm2::Element::document_load(XMLDocument& document, const std::filesystem::path& path, std::string* errorString)
  {
    util::File file(path, "rb");
    if (!file)
    {
      if (errorString) *errorString = "Could not open file.";
      return false;
    }

    if (document.LoadFile(file.get()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }

    return true;
  }

  bool Anm2::Element::document_parse(XMLDocument& document, std::string_view xml, std::string* errorString)
  {
    if (document.Parse(xml.data(), xml.size()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }

    return true;
  }

  void Anm2::Element::attributes_read(const XMLElement* element, Type parentType)
  {
    if (!element) return;

    util::xml::query_string_attribute(element, "CreatedBy", createdBy);
    util::xml::query_string_attribute(element, "CreatedOn", createdOn);
    util::xml::query_string_attribute(element, "DefaultAnimation", defaultAnimation);
    util::xml::query_string_attribute(element, "Name", name);
    util::xml::query_path_attribute(element, "Path", path);

    util::xml::query_int_attribute(element, "Id", id);
    util::xml::query_int_attribute(element, "LayerId", layerId);
    util::xml::query_int_attribute(element, "NullId", nullId);
    util::xml::query_int_attribute(element, "SpritesheetId", spritesheetId);
    util::xml::query_int_attribute(element, "EventId", eventId);
    util::xml::query_int_attribute(element, "RegionId", regionId);
    util::xml::query_int_attribute(element, "AtFrame", atFrame);
    util::xml::query_int_attribute(element, "FrameNum", frameNum);
    util::xml::query_int_attribute(element, "Fps", fps);
    util::xml::query_int_attribute(element, "Version", version);
    util::xml::query_int_attribute(element, "Delay", delay);

    util::xml::query_float_attribute(element, "XCrop", crop.x);
    util::xml::query_float_attribute(element, "YCrop", crop.y);
    util::xml::query_float_attribute(element, "Width", size.x);
    util::xml::query_float_attribute(element, "Height", size.y);
    util::xml::query_float_attribute(element, "XPivot", pivot.x);
    util::xml::query_float_attribute(element, "YPivot", pivot.y);
    util::xml::query_float_attribute(element, "XPosition", position.x);
    util::xml::query_float_attribute(element, "YPosition", position.y);
    util::xml::query_float_attribute(element, "XScale", scale.x);
    util::xml::query_float_attribute(element, "YScale", scale.y);
    util::xml::query_float_attribute(element, "Rotation", rotation);

    util::xml::query_color_attribute(element, "RedTint", tint.r);
    util::xml::query_color_attribute(element, "GreenTint", tint.g);
    util::xml::query_color_attribute(element, "BlueTint", tint.b);
    util::xml::query_color_attribute(element, "AlphaTint", tint.a);
    util::xml::query_color_attribute(element, "RedOffset", colorOffset.r);
    util::xml::query_color_attribute(element, "GreenOffset", colorOffset.g);
    util::xml::query_color_attribute(element, "BlueOffset", colorOffset.b);

    util::xml::query_bool_attribute(element, "Visible", isVisible);
    util::xml::query_bool_attribute(element, "ShowRect", isShowRect);
    util::xml::query_bool_attribute(element, "Loop", isLoop);

    if (const char* value = element->Attribute("Origin")) origin = string_enum_get(ORIGIN_STRINGS, value, CUSTOM);

    if (const char* value = element->Attribute("Interpolated"))
    {
      auto view = std::string_view(value);
      if (view == "true" || view == "1")
        interpolation = LINEAR;
      else if (view == "false" || view == "0")
        interpolation = NONE;
      else
        interpolation = string_enum_get(INTERPOLATION_STRINGS, view, NONE);
    }

    if (type == SOUND && parentType == TRIGGER) path.clear();
  }

  void Anm2::Element::attributes_write(XMLElement* element, Type parentType, Flags flags) const
  {
    if (!element) return;

    switch (type)
    {
      case INFO:
        util::xml::set_string_attribute(element, "CreatedBy", createdBy);
        util::xml::set_string_attribute(element, "CreatedOn", createdOn);
        util::xml::set_int_attribute(element, "Fps", fps);
        util::xml::set_int_attribute(element, "Version", version);
        break;

      case SPRITESHEET:
        if (id > -1) util::xml::set_int_attribute(element, "Id", id);
        util::xml::set_path_attribute(element, "Path", path);
        break;

      case REGION:
        if (id > -1) util::xml::set_int_attribute(element, "Id", id);
        util::xml::set_string_attribute(element, "Name", name);
        util::xml::set_float_attribute(element, "XCrop", crop.x);
        util::xml::set_float_attribute(element, "YCrop", crop.y);
        util::xml::set_float_attribute(element, "Width", size.x);
        util::xml::set_float_attribute(element, "Height", size.y);

        if (origin == CUSTOM)
        {
          util::xml::set_float_attribute(element, "XPivot", pivot.x);
          util::xml::set_float_attribute(element, "YPivot", pivot.y);
        }
        else if (auto value = enum_string_get(ORIGIN_STRINGS, origin); !value.empty())
          element->SetAttribute("Origin", value.data());
        break;

      case LAYER:
        if (id > -1) util::xml::set_int_attribute(element, "Id", id);
        util::xml::set_string_attribute(element, "Name", name);
        if (spritesheetId > -1) util::xml::set_int_attribute(element, "SpritesheetId", spritesheetId);
        break;

      case NULL_ELEMENT:
        if (id > -1) util::xml::set_int_attribute(element, "Id", id);
        util::xml::set_string_attribute(element, "Name", name);
        util::xml::set_bool_attribute(element, "ShowRect", isShowRect);
        break;

      case EVENT:
        if (id > -1) util::xml::set_int_attribute(element, "Id", id);
        util::xml::set_string_attribute(element, "Name", name);
        break;

      case SOUND:
        if (id > -1)
        {
          if (parentType == TRIGGER)
            util::xml::set_int_attribute(element, "Id", id);
          else
          {
            util::xml::set_int_attribute(element, "Id", id);
            util::xml::set_path_attribute(element, "Path", path);
          }
        }
        break;
      case ANIMATIONS:
        util::xml::set_string_attribute(element, "DefaultAnimation", defaultAnimation);
        break;
      case ANIMATION:
        util::xml::set_string_attribute(element, "Name", name);
        util::xml::set_int_attribute(element, "FrameNum", frameNum);
        util::xml::set_bool_attribute(element, "Loop", isLoop);
        break;

      case LAYER_ANIMATION:
        if (layerId > -1) util::xml::set_int_attribute(element, "LayerId", layerId);
        util::xml::set_bool_attribute(element, "Visible", isVisible);
        break;

      case NULL_ANIMATION:
        if (nullId > -1) util::xml::set_int_attribute(element, "NullId", nullId);
        util::xml::set_bool_attribute(element, "Visible", isVisible);
        break;

      case TRIGGER:
        if (eventId > -1) util::xml::set_int_attribute(element, "EventId", eventId);
        util::xml::set_int_attribute(element, "AtFrame", atFrame);
        break;

      case FRAME:
      {
        bool isNoRegions = is_flag_set(flags, NO_REGIONS);
        bool isFrameNoRegionValues = is_flag_set(flags, FRAME_NO_REGION_VALUES);
        bool isRegionValid = parentType == LAYER_ANIMATION && !isNoRegions && regionId != -1;
        bool isWriteRegionValues = parentType == LAYER_ANIMATION && (!isFrameNoRegionValues || !isRegionValid);

        if (isRegionValid) util::xml::set_int_attribute(element, "RegionId", regionId);
        util::xml::set_float_attribute(element, "XPosition", position.x);
        util::xml::set_float_attribute(element, "YPosition", position.y);

        if (isWriteRegionValues)
        {
          util::xml::set_float_attribute(element, "XPivot", pivot.x);
          util::xml::set_float_attribute(element, "YPivot", pivot.y);
          util::xml::set_float_attribute(element, "XCrop", crop.x);
          util::xml::set_float_attribute(element, "YCrop", crop.y);
          util::xml::set_float_attribute(element, "Width", size.x);
          util::xml::set_float_attribute(element, "Height", size.y);
        }

        util::xml::set_float_attribute(element, "XScale", scale.x);
        util::xml::set_float_attribute(element, "YScale", scale.y);
        util::xml::set_int_attribute(element, "Delay", delay);
        util::xml::set_bool_attribute(element, "Visible", isVisible);
        util::xml::set_color_attribute(element, "RedTint", tint.r);
        util::xml::set_color_attribute(element, "GreenTint", tint.g);
        util::xml::set_color_attribute(element, "BlueTint", tint.b);
        util::xml::set_color_attribute(element, "AlphaTint", tint.a);
        util::xml::set_color_attribute(element, "RedOffset", colorOffset.r);
        util::xml::set_color_attribute(element, "GreenOffset", colorOffset.g);
        util::xml::set_color_attribute(element, "BlueOffset", colorOffset.b);
        util::xml::set_float_attribute(element, "Rotation", rotation);

        if (is_flag_set(flags, INTERPOLATION_BOOL_ONLY) || interpolation == LINEAR || interpolation == NONE)
          element->SetAttribute("Interpolated", interpolation == LINEAR);
        else if (auto value = enum_string_get(INTERPOLATION_STRINGS, interpolation); !value.empty())
          element->SetAttribute("Interpolated", value.data());
        break;
      }

      case ACTOR:
      case CONTENT:
      case SPRITESHEETS:
      case LAYERS:
      case NULLS:
      case EVENTS:
      case SOUNDS:
      case ROOT_ANIMATION:
      case LAYER_ANIMATIONS:
      case NULL_ANIMATIONS:
      case TRIGGERS:
      case UNKNOWN:
      case TYPE_COUNT:
        break;
    }
  }

  Anm2::Element Anm2::Element::from_xml(const XMLElement* element, Type parentType)
  {
    Element parsed{};
    if (!element) return parsed;

    parsed.type = type_get(element->Name(), parentType);
    if (parsed.type == UNKNOWN) return parsed;

    parsed.attributes_read(element, parentType);

    for (const XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
      if (auto parsedChild = from_xml(child, parsed.type); parsedChild.type != UNKNOWN)
        parsed.children.push_back(std::move(parsedChild));

    return parsed;
  }

  XMLElement* Anm2::Element::to_xml(XMLDocument& document, Type parentType, Flags flags) const
  {
    auto tagName = tag_get(type);
    if (tagName.empty()) return nullptr;

    auto* element = document.NewElement(tagName.data());
    attributes_write(element, parentType, flags);

    for (const auto& child : children)
    {
      if (is_flag_set(flags, NO_SOUNDS) && (child.type == SOUNDS || child.type == SOUND)) continue;
      if (is_flag_set(flags, NO_REGIONS) && child.type == REGION) continue;
      if (auto* childElement = child.to_xml(document, type, flags)) element->InsertEndChild(childElement);
    }

    return element;
  }

  bool Anm2::Element::serialize(const std::filesystem::path& path, std::string* errorString,
                                Compatibility compatibility) const
  {
    XMLDocument document{};
    if (auto* element = to_xml(document, UNKNOWN, Anm2::flags_get(compatibility)))
      document.InsertFirstChild(element);
    else
    {
      if (errorString) *errorString = "No supported element to serialize.";
      return false;
    }

    util::File file(path, "wb");
    if (!file)
    {
      if (errorString) *errorString = "Could not open file for writing.";
      return false;
    }

    if (document.SaveFile(file.get()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }

    return true;
  }

  std::string Anm2::Element::to_string(Compatibility compatibility) const
  {
    XMLDocument document{};
    if (auto* element = to_xml(document, UNKNOWN, Anm2::flags_get(compatibility))) document.InsertFirstChild(element);
    return util::xml::document_to_string(document);
  }

  Anm2::Element* Anm2::Element::child_get(Type childType, std::size_t index)
  {
    std::size_t matchIndex{};
    for (auto& child : children)
      if (child.type == childType && matchIndex++ == index) return &child;
    return nullptr;
  }

  const Anm2::Element* Anm2::Element::child_get(Type childType, std::size_t index) const
  {
    std::size_t matchIndex{};
    for (const auto& child : children)
      if (child.type == childType && matchIndex++ == index) return &child;
    return nullptr;
  }

  std::vector<Anm2::Element*> Anm2::Element::children_get(Type childType)
  {
    std::vector<Element*> matches{};
    for (auto& child : children)
      if (child.type == childType) matches.push_back(&child);
    return matches;
  }

  std::vector<const Anm2::Element*> Anm2::Element::children_get(Type childType) const
  {
    std::vector<const Element*> matches{};
    for (const auto& child : children)
      if (child.type == childType) matches.push_back(&child);
    return matches;
  }

  Anm2::Anm2(const std::filesystem::path& filepath, std::string* errorString)
      : path(filepath), root(filepath, errorString)
  {
    isValid = root.type == Element::ACTOR;
    assets_reload();
  }

  bool Anm2::serialize(const std::filesystem::path& filepath, std::string* errorString,
                       Compatibility compatibility) const
  {
    return root.serialize(filepath, errorString, compatibility);
  }

  std::string Anm2::to_string(Compatibility compatibility) const { return root.to_string(compatibility); }

  void Anm2::assets_reload()
  {
    spritesheets.clear();
    sounds.clear();

    auto directory = path.parent_path();
    for (auto* element : elements_get(Element::SPRITESHEET))
      if (element->id > -1 && !element->path.empty())
        spritesheets[element->id] = resource::Texture(asset_path_resolve(directory, element->path));

    for (auto* element : elements_get(Element::SOUND))
      if (element->id > -1 && !element->path.empty())
        sounds[element->id] = resource::Audio(asset_path_resolve(directory, element->path));
  }

  Anm2::Element* Anm2::element_get(Element::Type type, std::size_t index)
  {
    auto matches = elements_get(type);
    return index < matches.size() ? matches[index] : nullptr;
  }

  const Anm2::Element* Anm2::element_get(Element::Type type, std::size_t index) const
  {
    auto matches = elements_get(type);
    return index < matches.size() ? matches[index] : nullptr;
  }

  std::vector<Anm2::Element*> Anm2::elements_get(Element::Type type)
  {
    std::vector<Element*> matches{};
    elements_collect(root, type, matches);
    return matches;
  }

  std::vector<const Anm2::Element*> Anm2::elements_get(Element::Type type) const
  {
    std::vector<const Element*> matches{};
    elements_collect(root, type, matches);
    return matches;
  }

  void Anm2::elements_collect(Element& element, Element::Type type, std::vector<Element*>& out)
  {
    if (element.type == type) out.push_back(&element);
    for (auto& child : element.children)
      elements_collect(child, type, out);
  }

  void Anm2::elements_collect(const Element& element, Element::Type type, std::vector<const Element*>& out)
  {
    if (element.type == type) out.push_back(&element);
    for (const auto& child : element.children)
      elements_collect(child, type, out);
  }
}
