#include "frame.h"

#include "math_.h"
#include "xml_.h"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  Frame::Frame(XMLElement* element, Type type)
  {
    if (type != TRIGGER)
    {
      element->QueryFloatAttribute("XPosition", &position.x);
      element->QueryFloatAttribute("YPosition", &position.y);
      if (type == LAYER)
      {
        element->QueryFloatAttribute("XPivot", &pivot.x);
        element->QueryFloatAttribute("YPivot", &pivot.y);
        element->QueryFloatAttribute("XCrop", &crop.x);
        element->QueryFloatAttribute("YCrop", &crop.y);
        element->QueryFloatAttribute("Width", &size.x);
        element->QueryFloatAttribute("Height", &size.y);
      }
      element->QueryFloatAttribute("XScale", &scale.x);
      element->QueryFloatAttribute("YScale", &scale.y);
      element->QueryIntAttribute("Delay", &duration);
      element->QueryBoolAttribute("Visible", &isVisible);
      xml::query_color_attribute(element, "RedTint", tint.r);
      xml::query_color_attribute(element, "GreenTint", tint.g);
      xml::query_color_attribute(element, "BlueTint", tint.b);
      xml::query_color_attribute(element, "AlphaTint", tint.a);
      xml::query_color_attribute(element, "RedOffset", colorOffset.r);
      xml::query_color_attribute(element, "GreenOffset", colorOffset.g);
      xml::query_color_attribute(element, "BlueOffset", colorOffset.b);
      element->QueryFloatAttribute("Rotation", &rotation);
      element->QueryBoolAttribute("Interpolated", &isInterpolated);
    }
    else
    {
      element->QueryIntAttribute("EventId", &eventID);
      element->QueryIntAttribute("SoundId", &soundID);
      element->QueryIntAttribute("AtFrame", &atFrame);
    }
  }

  XMLElement* Frame::to_element(XMLDocument& document, Type type)
  {
    auto element = document.NewElement(type == TRIGGER ? "Trigger" : "Frame");

    switch (type)
    {
      case ROOT:
      case NULL_:
        element->SetAttribute("XPosition", position.x);
        element->SetAttribute("YPosition", position.y);
        element->SetAttribute("Delay", duration);
        element->SetAttribute("Visible", isVisible);
        element->SetAttribute("XScale", scale.x);
        element->SetAttribute("YScale", scale.y);
        element->SetAttribute("RedTint", math::float_to_uint8(tint.r));
        element->SetAttribute("GreenTint", math::float_to_uint8(tint.g));
        element->SetAttribute("BlueTint", math::float_to_uint8(tint.b));
        element->SetAttribute("AlphaTint", math::float_to_uint8(tint.a));
        element->SetAttribute("RedOffset", math::float_to_uint8(colorOffset.r));
        element->SetAttribute("GreenOffset", math::float_to_uint8(colorOffset.g));
        element->SetAttribute("BlueOffset", math::float_to_uint8(colorOffset.b));
        element->SetAttribute("Rotation", rotation);
        element->SetAttribute("Interpolated", isInterpolated);
        break;
      case LAYER:
        element->SetAttribute("XPosition", position.x);
        element->SetAttribute("YPosition", position.y);
        element->SetAttribute("XPivot", pivot.x);
        element->SetAttribute("YPivot", pivot.y);
        element->SetAttribute("XCrop", crop.x);
        element->SetAttribute("YCrop", crop.y);
        element->SetAttribute("Width", size.x);
        element->SetAttribute("Height", size.y);
        element->SetAttribute("XScale", scale.x);
        element->SetAttribute("YScale", scale.y);
        element->SetAttribute("Delay", duration);
        element->SetAttribute("Visible", isVisible);
        element->SetAttribute("RedTint", math::float_to_uint8(tint.r));
        element->SetAttribute("GreenTint", math::float_to_uint8(tint.g));
        element->SetAttribute("BlueTint", math::float_to_uint8(tint.b));
        element->SetAttribute("AlphaTint", math::float_to_uint8(tint.a));
        element->SetAttribute("RedOffset", math::float_to_uint8(colorOffset.r));
        element->SetAttribute("GreenOffset", math::float_to_uint8(colorOffset.g));
        element->SetAttribute("BlueOffset", math::float_to_uint8(colorOffset.b));
        element->SetAttribute("Rotation", rotation);
        element->SetAttribute("Interpolated", isInterpolated);
        break;
      case TRIGGER:
        element->SetAttribute("EventId", eventID);
        element->SetAttribute("SoundId", soundID);
        element->SetAttribute("AtFrame", atFrame);
        break;
      default:
        break;
    }

    return element;
  }

  void Frame::serialize(XMLDocument& document, XMLElement* parent, Type type)
  {
    parent->InsertEndChild(to_element(document, type));
  }

  std::string Frame::to_string(Type type)
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document, type));
    return xml::document_to_string(document);
  }

  void Frame::shorten() { duration = glm::clamp(--duration, FRAME_DURATION_MIN, FRAME_DURATION_MAX); }

  void Frame::extend() { duration = glm::clamp(++duration, FRAME_DURATION_MIN, FRAME_DURATION_MAX); }

  bool Frame::is_visible(Type type)
  {
    if (type == TRIGGER)
      return isVisible && eventID > -1;
    else
      return isVisible;
  }
}