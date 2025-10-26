#include "anm2.h"

#include <ranges>

#include "filesystem.h"
#include "math.h"
#include "texture.h"
#include "util.h"
#include "xml.h"

using namespace tinyxml2;
using namespace anm2ed::filesystem;
using namespace anm2ed::texture;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::anm2
{

  void Reference::previous_frame(int max)
  {
    frameIndex = glm::clamp(--frameIndex, 0, max);
  }

  void Reference::next_frame(int max)
  {
    frameIndex = glm::clamp(++frameIndex, 0, max);
  }

  Info::Info() = default;

  Info::Info(XMLElement* element)
  {
    if (!element) return;
    xml::query_string_attribute(element, "CreatedBy", &createdBy);
    xml::query_string_attribute(element, "CreatedOn", &createdOn);
    element->QueryIntAttribute("Fps", &fps);
    element->QueryIntAttribute("Version", &version);
  }

  void Info::serialize(XMLDocument& document, XMLElement* parent)
  {
    auto infoElement = document.NewElement("Info");
    infoElement->SetAttribute("CreatedBy", createdBy.c_str());
    infoElement->SetAttribute("CreatedOn", createdOn.c_str());
    infoElement->SetAttribute("Fps", fps);
    infoElement->SetAttribute("Version", version);
    parent->InsertEndChild(infoElement);
  }

  Spritesheet::Spritesheet() = default;

  Spritesheet::Spritesheet(XMLElement* element, int& id)
  {
    if (!element) return;
    element->QueryIntAttribute("Id", &id);
    xml::query_path_attribute(element, "Path", &path);
    // Spritesheet paths from Isaac Rebirth are made with the assumption that
    // the paths are case-insensitive (as the game was developed on Windows)
    // However when using the resource dumper, the spritesheet paths are all lowercase (on Linux anyway)
    // If the check doesn't work, set the spritesheet path to lowercase
    // If that check doesn't work, replace backslashes with slashes
    // At the minimum this should make all textures be able to be loaded on Linux
    // If it doesn't work beyond that then that's on the user :^)
    if (!path_is_exist(path)) path = string::to_lower(path);
    if (!path_is_exist(path)) path = string::replace_backslash(path);
    texture = Texture(path);
  }

  Spritesheet::Spritesheet(const std::string& directory, const std::string& path)
  {
    this->path = !path.empty() ? path : this->path.string();
    WorkingDirectory workingDirectory(directory);
    texture = Texture(this->path);
  }

  bool Spritesheet::save(const std::string& directory, const std::string& path)
  {
    this->path = !path.empty() ? path : this->path.string();
    WorkingDirectory workingDirectory(directory);
    return texture.write_png(this->path);
  }

  void Spritesheet::serialize(XMLDocument& document, XMLElement* parent, int id)
  {
    auto element = document.NewElement("Spritesheet");
    element->SetAttribute("Id", id);
    element->SetAttribute("Path", path.c_str());
    parent->InsertEndChild(element);
  }

  void Spritesheet::reload(const std::string& directory)
  {
    *this = Spritesheet(directory, this->path);
  }

  bool Spritesheet::is_valid()
  {
    return texture.is_valid();
  }

  std::string Spritesheet::to_string(int id)
  {
    XMLDocument document{};

    auto* element = document.NewElement("Spritesheet");

    element->SetAttribute("Id", id);
    element->SetAttribute("Path", path.c_str());

    document.InsertFirstChild(element);

    XMLPrinter printer;
    document.Print(&printer);
    return std::string(printer.CStr());
  }

  Layer::Layer() = default;

  Layer::Layer(XMLElement* element, int& id)
  {
    if (!element) return;
    element->QueryIntAttribute("Id", &id);
    xml::query_string_attribute(element, "Name", &name);
    element->QueryIntAttribute("SpritesheetId", &spritesheetID);
  }

  void Layer::serialize(XMLDocument& document, XMLElement* parent, int id)
  {
    auto element = document.NewElement("Layer");
    element->SetAttribute("Id", id);
    element->SetAttribute("Name", name.c_str());
    element->SetAttribute("SpritesheetId", spritesheetID);
    parent->InsertEndChild(element);
  }

  std::string Layer::to_string(int id)
  {
    XMLDocument document{};

    auto* element = document.NewElement("Layer");

    element->SetAttribute("Id", id);
    element->SetAttribute("Name", name.c_str());
    element->SetAttribute("SpritesheetId", spritesheetID);

    document.InsertFirstChild(element);

    XMLPrinter printer;
    document.Print(&printer);
    return std::string(printer.CStr());
  }

  Null::Null() = default;

  Null::Null(XMLElement* element, int& id)
  {
    if (!element) return;
    element->QueryIntAttribute("Id", &id);
    xml::query_string_attribute(element, "Name", &name);
    element->QueryBoolAttribute("ShowRect", &isShowRect);
  }

  void Null::serialize(XMLDocument& document, XMLElement* parent, int id)
  {
    auto element = document.NewElement("Null");
    element->SetAttribute("Id", id);
    element->SetAttribute("Name", name.c_str());
    if (isShowRect) element->SetAttribute("ShowRect", isShowRect);

    parent->InsertEndChild(element);
  }

  std::string Null::to_string(int id)
  {
    XMLDocument document{};

    auto* element = document.NewElement("Null");

    element->SetAttribute("Id", id);
    element->SetAttribute("Name", name.c_str());

    document.InsertFirstChild(element);

    XMLPrinter printer;
    document.Print(&printer);
    return std::string(printer.CStr());
  }

  Event::Event() = default;

  Event::Event(XMLElement* element, int& id)
  {
    if (!element) return;
    element->QueryIntAttribute("Id", &id);
    xml::query_string_attribute(element, "Name", &name);
  }

  void Event::serialize(XMLDocument& document, XMLElement* parent, int id)
  {
    auto element = document.NewElement("Event");
    element->SetAttribute("Id", id);
    element->SetAttribute("Name", name.c_str());
    parent->InsertEndChild(element);
  }

  std::string Event::to_string(int id)
  {
    XMLDocument document{};

    auto* element = document.NewElement("Event");

    element->SetAttribute("Id", id);
    element->SetAttribute("Name", name.c_str());

    document.InsertFirstChild(element);

    XMLPrinter printer;
    document.Print(&printer);
    return std::string(printer.CStr());
  }

  Content::Content() = default;

  void Content::serialize(XMLDocument& document, XMLElement* parent)
  {
    auto element = document.NewElement("Content");

    auto spritesheetsElement = document.NewElement("Spritesheets");
    for (auto& [id, spritesheet] : spritesheets)
      spritesheet.serialize(document, spritesheetsElement, id);
    element->InsertEndChild(spritesheetsElement);

    auto layersElement = document.NewElement("Layers");
    for (auto& [id, layer] : layers)
      layer.serialize(document, layersElement, id);
    element->InsertEndChild(layersElement);

    auto nullsElement = document.NewElement("Nulls");
    for (auto& [id, null] : nulls)
      null.serialize(document, nullsElement, id);
    element->InsertEndChild(nullsElement);

    auto eventsElement = document.NewElement("Events");
    for (auto& [id, event] : events)
      event.serialize(document, eventsElement, id);
    element->InsertEndChild(eventsElement);

    parent->InsertEndChild(element);
  }

  Content::Content(XMLElement* element)
  {
    int id{};

    if (auto spritesheetsElement = element->FirstChildElement("Spritesheets"))
    {
      for (auto child = spritesheetsElement->FirstChildElement("Spritesheet"); child;
           child = child->NextSiblingElement("Spritesheet"))
        spritesheets[id] = Spritesheet(child, id);
    }

    if (auto layersElement = element->FirstChildElement("Layers"))
    {
      for (auto child = layersElement->FirstChildElement("Layer"); child; child = child->NextSiblingElement("Layer"))
        layers[id] = Layer(child, id);
    }

    if (auto nullsElement = element->FirstChildElement("Nulls"))
    {
      for (auto child = nullsElement->FirstChildElement("Null"); child; child = child->NextSiblingElement("Null"))
        nulls[id] = Null(child, id);
    }

    if (auto eventsElement = element->FirstChildElement("Events"))
    {
      for (auto child = eventsElement->FirstChildElement("Event"); child; child = child->NextSiblingElement("Event"))
        events[id] = Event(child, id);
    }
  }

  std::set<int> Content::spritesheets_unused()
  {
    std::set<int> used;
    for (auto& layer : layers | std::views::values)
      if (layer.spritesheetID != -1) used.insert(layer.spritesheetID);

    std::set<int> unused;
    for (auto& id : spritesheets | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  void Content::layer_add(int& id)
  {
    id = map::next_id_get(layers);
    layers[id] = Layer();
  }

  void Content::null_add(int& id)
  {
    id = map::next_id_get(nulls);
    nulls[id] = Null();
  }

  void Content::event_add(int& id)
  {
    id = map::next_id_get(events);
    events[id] = Event();
  }

  bool Content::spritesheets_deserialize(const std::string& string, const std::string& directory, merge::Type type,
                                         std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      int id{};

      if (!document.FirstChildElement("Spritesheet"))
      {
        if (errorString) *errorString = "No valid spritesheet(s).";
        return false;
      }

      WorkingDirectory workingDirectory(directory);

      for (auto element = document.FirstChildElement("Spritesheet"); element;
           element = element->NextSiblingElement("Spritesheet"))
      {
        auto spritesheet = Spritesheet(element, id);

        if (type == merge::APPEND) id = map::next_id_get(spritesheets);

        spritesheets[id] = std::move(spritesheet);
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

  bool Content::layers_deserialize(const std::string& string, merge::Type type, std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      int id{};

      if (!document.FirstChildElement("Layer"))
      {
        if (errorString) *errorString = "No valid layer(s).";
        return false;
      }

      for (auto element = document.FirstChildElement("Layer"); element; element = element->NextSiblingElement("Layer"))
      {
        auto layer = Layer(element, id);

        if (type == merge::APPEND) id = map::next_id_get(layers);

        layers[id] = layer;
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

  bool Content::nulls_deserialize(const std::string& string, merge::Type type, std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      int id{};

      if (!document.FirstChildElement("Null"))
      {
        if (errorString) *errorString = "No valid null(s).";
        return false;
      }

      for (auto element = document.FirstChildElement("Null"); element; element = element->NextSiblingElement("Null"))
      {
        auto layer = Null(element, id);

        if (type == merge::APPEND) id = map::next_id_get(nulls);

        nulls[id] = layer;
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

  bool Content::events_deserialize(const std::string& string, merge::Type type, std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      int id{};

      if (!document.FirstChildElement("Event"))
      {
        if (errorString) *errorString = "No valid event(s).";
        return false;
      }

      for (auto element = document.FirstChildElement("Event"); element; element = element->NextSiblingElement("Event"))
      {
        auto layer = Event(element, id);

        if (type == merge::APPEND) id = map::next_id_get(events);

        events[id] = layer;
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

  Frame::Frame() = default;

  Frame::Frame(XMLElement* element, Type type)
  {
    if (!element) return;

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
      element->QueryIntAttribute("Delay", &delay);
      element->QueryBoolAttribute("Visible", &isVisible);
      xml::query_color_attribute(element, "RedTint", tint.r);
      xml::query_color_attribute(element, "GreenTint", tint.g);
      xml::query_color_attribute(element, "BlueTint", tint.b);
      xml::query_color_attribute(element, "AlphaTint", tint.a);
      xml::query_color_attribute(element, "RedOffset", offset.r);
      xml::query_color_attribute(element, "GreenOffset", offset.g);
      xml::query_color_attribute(element, "BlueOffset", offset.b);
      element->QueryFloatAttribute("Rotation", &rotation);
      element->QueryBoolAttribute("Interpolated", &isInterpolated);
    }
    else
    {
      element->QueryIntAttribute("EventId", &eventID);
      element->QueryIntAttribute("AtFrame", &atFrame);
    }
  }

  void Frame::serialize(XMLDocument& document, XMLElement* parent, Type type)
  {
    auto element = document.NewElement(type == TRIGGER ? "Trigger" : "Frame");

    switch (type)
    {
      case ROOT:
      case NULL_:
        element->SetAttribute("XPosition", position.x);
        element->SetAttribute("YPosition", position.y);
        element->SetAttribute("Delay", delay);
        element->SetAttribute("Visible", isVisible);
        element->SetAttribute("XScale", scale.x);
        element->SetAttribute("YScale", scale.y);
        element->SetAttribute("RedTint", math::float_to_uint8(tint.r));
        element->SetAttribute("GreenTint", math::float_to_uint8(tint.g));
        element->SetAttribute("BlueTint", math::float_to_uint8(tint.b));
        element->SetAttribute("AlphaTint", math::float_to_uint8(tint.a));
        element->SetAttribute("RedOffset", math::float_to_uint8(offset.r));
        element->SetAttribute("GreenOffset", math::float_to_uint8(offset.g));
        element->SetAttribute("BlueOffset", math::float_to_uint8(offset.b));
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
        element->SetAttribute("Delay", delay);
        element->SetAttribute("Visible", isVisible);
        element->SetAttribute("RedTint", math::float_to_uint8(tint.r));
        element->SetAttribute("GreenTint", math::float_to_uint8(tint.g));
        element->SetAttribute("BlueTint", math::float_to_uint8(tint.b));
        element->SetAttribute("AlphaTint", math::float_to_uint8(tint.a));
        element->SetAttribute("RedOffset", math::float_to_uint8(offset.r));
        element->SetAttribute("GreenOffset", math::float_to_uint8(offset.g));
        element->SetAttribute("BlueOffset", math::float_to_uint8(offset.b));
        element->SetAttribute("Rotation", rotation);
        element->SetAttribute("Interpolated", isInterpolated);
        break;
      case TRIGGER:
        element->SetAttribute("EventId", eventID);
        element->SetAttribute("AtFrame", atFrame);
        break;
      default:
        break;
    }

    parent->InsertEndChild(element);
  }

  void Frame::shorten()
  {
    delay = glm::clamp(--delay, FRAME_DELAY_MIN, FRAME_DELAY_MAX);
  }

  void Frame::extend()
  {
    delay = glm::clamp(++delay, FRAME_DELAY_MIN, FRAME_DELAY_MAX);
  }

  Item::Item() = default;

  Item::Item(XMLElement* element, Type type, int* id)
  {
    if (type == LAYER && id) element->QueryIntAttribute("LayerId", id);
    if (type == NULL_ && id) element->QueryIntAttribute("NullId", id);

    element->QueryBoolAttribute("Visible", &isVisible);

    for (auto child = type == TRIGGER ? element->FirstChildElement("Trigger") : element->FirstChildElement("Frame");
         child; child = type == TRIGGER ? child->NextSiblingElement("Trigger") : child->NextSiblingElement("Frame"))
      frames.push_back(Frame(child, type));
  }

  void Item::serialize(XMLDocument& document, XMLElement* parent, Type type, int id)
  {
    auto typeString = type == ROOT    ? "RootAnimation"
                      : type == LAYER ? "LayerAnimation"
                      : type == NULL_ ? "NullAnimation"
                                      : "Triggers";

    auto element = document.NewElement(typeString);

    if (type == LAYER) element->SetAttribute("LayerId", id);
    if (type == NULL_) element->SetAttribute("NullId", id);
    if (type == LAYER || type == NULL_) element->SetAttribute("Visible", isVisible);

    for (auto& frame : frames)
      frame.serialize(document, element, type);
    parent->InsertEndChild(element);
  }

  int Item::length(Type type)
  {
    int length{};

    if (type == TRIGGER)
      for (auto& frame : frames)
        length = frame.atFrame > length ? frame.atFrame : length;
    else
      for (auto& frame : frames)
        length += frame.delay;

    return length;
  }

  Frame Item::frame_generate(float time, Type type)
  {
    Frame frame{};
    frame.isVisible = false;

    if (frames.empty()) return frame;

    Frame* frameNext = nullptr;
    int delayCurrent = 0;
    int delayNext = 0;

    for (auto [i, iFrame] : std::views::enumerate(frames))
    {
      if (type == TRIGGER)
      {
        if ((int)time == iFrame.atFrame)
        {
          frame = iFrame;
          break;
        }
      }
      else
      {
        frame = iFrame;

        delayNext += frame.delay;

        if (time >= delayCurrent && time < delayNext)
        {
          if (i + 1 < (int)frames.size())
            frameNext = &frames[i + 1];
          else
            frameNext = nullptr;
          break;
        }

        delayCurrent += frame.delay;
      }
    }

    if (type != TRIGGER && frame.isInterpolated && frameNext && frame.delay > 1)
    {
      auto interpolation = (time - delayCurrent) / (delayNext - delayCurrent);

      frame.rotation = glm::mix(frame.rotation, frameNext->rotation, interpolation);
      frame.position = glm::mix(frame.position, frameNext->position, interpolation);
      frame.scale = glm::mix(frame.scale, frameNext->scale, interpolation);
      frame.offset = glm::mix(frame.offset, frameNext->offset, interpolation);
      frame.tint = glm::mix(frame.tint, frameNext->tint, interpolation);
    }

    return frame;
  }

  Animation::Animation() = default;

  Animation::Animation(XMLElement* element)
  {
    int id{};

    xml::query_string_attribute(element, "Name", &name);
    element->QueryIntAttribute("FrameNum", &frameNum);
    element->QueryBoolAttribute("Loop", &isLoop);

    if (auto rootAnimationElement = element->FirstChildElement("RootAnimation"))
      rootAnimation = Item(rootAnimationElement, ROOT);

    if (auto layerAnimationsElement = element->FirstChildElement("LayerAnimations"))
    {
      for (auto child = layerAnimationsElement->FirstChildElement("LayerAnimation"); child;
           child = child->NextSiblingElement("LayerAnimation"))
      {
        layerAnimations[id] = Item(child, LAYER, &id);
        layerOrder.push_back(id);
      }
    }

    if (auto nullAnimationsElement = element->FirstChildElement("NullAnimations"))
    {
      for (auto child = nullAnimationsElement->FirstChildElement("NullAnimation"); child;
           child = child->NextSiblingElement("NullAnimation"))
        nullAnimations[id] = Item(child, NULL_, &id);
    }

    if (auto triggersElement = element->FirstChildElement("Triggers")) triggers = Item(triggersElement, TRIGGER);
  }

  Item* Animation::item_get(Type type, int id)
  {
    switch (type)
    {
      case ROOT:
        return &rootAnimation;
      case LAYER:
        return unordered_map::find(layerAnimations, id);
      case NULL_:
        return map::find(nullAnimations, id);
      case TRIGGER:
        return &triggers;
      default:
        return nullptr;
    }
    return nullptr;
  }

  void Animation::item_remove(Type type, int id)
  {
    switch (type)
    {
      case LAYER:
        layerAnimations.erase(id);
        for (auto [i, value] : std::views::enumerate(layerOrder))
          if (value == id) layerOrder.erase(layerOrder.begin() + i);
        break;
      case NULL_:
        nullAnimations.erase(id);
        break;
      case ROOT:
      case TRIGGER:
      default:
        break;
    }
  }

  void Animation::serialize(XMLDocument& document, XMLElement* parent)
  {
    auto element = document.NewElement("Animation");
    element->SetAttribute("Name", name.c_str());
    element->SetAttribute("FrameNum", frameNum);
    element->SetAttribute("Loop", isLoop);

    rootAnimation.serialize(document, element, ROOT);

    auto layerAnimationsElement = document.NewElement("LayerAnimations");
    for (auto& i : layerOrder)
    {
      Item& layerAnimation = layerAnimations.at(i);
      layerAnimation.serialize(document, layerAnimationsElement, LAYER, i);
    }
    element->InsertEndChild(layerAnimationsElement);

    auto nullAnimationsElement = document.NewElement("NullAnimations");
    for (auto& [id, nullAnimation] : nullAnimations)
      nullAnimation.serialize(document, nullAnimationsElement, NULL_, id);
    element->InsertEndChild(nullAnimationsElement);

    triggers.serialize(document, element, TRIGGER);

    parent->InsertEndChild(element);
  }

  int Animation::length()
  {
    int length{};

    if (int rootAnimationLength = rootAnimation.length(ROOT); rootAnimationLength > length)
      length = rootAnimationLength;

    for (auto& layerAnimation : layerAnimations | std::views::values)
      if (int layerAnimationLength = layerAnimation.length(LAYER); layerAnimationLength > length)
        length = layerAnimationLength;

    for (auto& nullAnimation : nullAnimations | std::views::values)
      if (int nullAnimationLength = nullAnimation.length(NULL_); nullAnimationLength > length)
        length = nullAnimationLength;

    if (int triggersLength = triggers.length(TRIGGER); triggersLength > length) length = triggersLength;

    return length;
  }

  std::string Animation::to_string()
  {
    XMLDocument document{};

    auto* element = document.NewElement("Animation");
    document.InsertFirstChild(element);

    element->SetAttribute("Name", name.c_str());
    element->SetAttribute("FrameNum", frameNum);
    element->SetAttribute("Loop", isLoop);

    rootAnimation.serialize(document, element, ROOT);

    auto layerAnimationsElement = document.NewElement("LayerAnimations");
    for (auto& i : layerOrder)
    {
      Item& layerAnimation = layerAnimations.at(i);
      layerAnimation.serialize(document, layerAnimationsElement, LAYER, i);
    }
    element->InsertEndChild(layerAnimationsElement);

    auto nullAnimationsElement = document.NewElement("NullAnimations");
    for (auto& [id, nullAnimation] : nullAnimations)
      nullAnimation.serialize(document, nullAnimationsElement, NULL_, id);
    element->InsertEndChild(nullAnimationsElement);

    triggers.serialize(document, element, TRIGGER);

    XMLPrinter printer;
    document.Print(&printer);
    return std::string(printer.CStr());
  }

  vec4 Animation::rect(bool isRootTransform)
  {
    f32 minX = std::numeric_limits<f32>::infinity();
    f32 minY = std::numeric_limits<f32>::infinity();
    f32 maxX = -std::numeric_limits<f32>::infinity();
    f32 maxY = -std::numeric_limits<f32>::infinity();
    bool any = false;

    constexpr ivec2 CORNERS[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

    for (float t = 0.0f; t < (float)frameNum; t += 1.0f)
    {
      mat4 transform(1.0f);

      if (isRootTransform)
      {
        auto root = rootAnimation.frame_generate(t, anm2::ROOT);
        transform *= math::quad_model_parent_get(root.position, {}, math::percent_to_unit(root.scale), root.rotation);
      }

      for (auto& [id, layerAnimation] : layerAnimations)
      {
        auto frame = layerAnimation.frame_generate(t, anm2::LAYER);

        if (frame.size == vec2() || !frame.isVisible) continue;

        auto layerTransform = transform * math::quad_model_get(frame.size, frame.position, frame.pivot,
                                                               math::percent_to_unit(frame.scale), frame.rotation);
        for (auto& corner : CORNERS)
        {
          vec4 world = layerTransform * vec4(corner, 0.0f, 1.0f);
          minX = std::min(minX, world.x);
          minY = std::min(minY, world.y);
          maxX = std::max(maxX, world.x);
          maxY = std::max(maxY, world.y);
          any = true;
        }
      }
    }

    if (!any) return vec4(-1.0f);
    return {minX, minY, maxX - minX, maxY - minY};
  }

  Animations::Animations() = default;

  Animations::Animations(XMLElement* element)
  {
    xml::query_string_attribute(element, "DefaultAnimation", &defaultAnimation);

    for (auto child = element->FirstChildElement("Animation"); child; child = child->NextSiblingElement("Animation"))
      items.push_back(Animation(child));
  }

  void Animations::serialize(XMLDocument& document, XMLElement* parent)
  {
    auto element = document.NewElement("Animations");
    element->SetAttribute("DefaultAnimation", defaultAnimation.c_str());
    for (auto& animation : items)
      animation.serialize(document, element);
    parent->InsertEndChild(element);
  }

  int Animations::length()
  {
    int length{};

    for (auto& animation : items)
      if (int animationLength = animation.length(); animationLength > length) length = animationLength;

    return length;
  }

  int Animations::merge(int target, std::set<int>& sources, merge::Type type, bool isDeleteAfter)
  {
    Animation& animation = items.at(target);

    if (!animation.name.ends_with(MERGED_STRING)) animation.name = animation.name + " " + MERGED_STRING;

    auto merge_item = [&](Item& destination, Item& source)
    {
      switch (type)
      {
        case merge::APPEND:
          destination.frames.insert(destination.frames.end(), source.frames.begin(), source.frames.end());
          break;
        case merge::PREPEND:
          destination.frames.insert(destination.frames.begin(), source.frames.begin(), source.frames.end());
          break;
        case merge::REPLACE:
          if (destination.frames.size() < source.frames.size()) destination.frames.resize(source.frames.size());
          for (int i = 0; i < (int)source.frames.size(); i++)
            destination.frames[i] = source.frames[i];
          break;
        case merge::IGNORE:
        default:
          break;
      }
    };

    for (auto& i : sources)
    {
      if (i == target) continue;
      if (i < 0 || i >= (int)items.size()) continue;

      auto& source = items.at(i);

      merge_item(animation.rootAnimation, source.rootAnimation);

      for (auto& [id, layerAnimation] : source.layerAnimations)
      {
        if (!animation.layerAnimations.contains(id))
        {
          animation.layerAnimations[id] = layerAnimation;
          animation.layerOrder.emplace_back(id);
        }
        merge_item(animation.layerAnimations[id], layerAnimation);
      }

      for (auto& [id, nullAnimation] : source.nullAnimations)
      {
        if (!animation.nullAnimations.contains(id)) animation.nullAnimations[id] = nullAnimation;
        merge_item(animation.nullAnimations[id], nullAnimation);
      }

      merge_item(animation.triggers, source.triggers);
    }

    if (isDeleteAfter)
    {
      for (auto& source : std::ranges::reverse_view(sources))
      {
        if (source == target) continue;
        items.erase(items.begin() + source);
      }
    }

    int finalIndex = target;

    if (isDeleteAfter)
    {
      int numDeletedBefore = 0;
      for (auto& idx : sources)
      {
        if (idx == target) continue;
        if (idx >= 0 && idx < target) ++numDeletedBefore;
      }
      finalIndex -= numDeletedBefore;
    }

    return finalIndex;
  }

  bool Animations::animations_deserialize(const std::string& string, int start, std::set<int>& indices,
                                          std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      if (!document.FirstChildElement("Animation"))
      {
        if (errorString) *errorString = "No valid animation(s).";
        return false;
      }

      int count{};
      for (auto element = document.FirstChildElement("Animation"); element;
           element = element->NextSiblingElement("Animation"))
      {
        auto index = start + count;
        items.insert(items.begin() + start + count, Animation(element));
        indices.insert(index);
        count++;
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

  Anm2::Anm2()
  {
    info.createdOn = time::get("%d-%B-%Y %I:%M:%S");
  }

  bool Anm2::serialize(const std::string& path, std::string* errorString)
  {
    XMLDocument document;

    auto* element = document.NewElement("AnimatedActor");
    document.InsertFirstChild(element);

    info.serialize(document, element);
    content.serialize(document, element);
    animations.serialize(document, element);

    if (document.SaveFile(path.c_str()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return false;
    }
    return true;
  }

  std::string Anm2::to_string()
  {
    XMLDocument document;

    auto* element = document.NewElement("AnimatedActor");
    document.InsertFirstChild(element);

    info.serialize(document, element);
    content.serialize(document, element);
    animations.serialize(document, element);

    XMLPrinter printer;
    document.Print(&printer);
    return std::string(printer.CStr());
  }

  Anm2::Anm2(const std::string& path, std::string* errorString)
  {
    XMLDocument document;

    if (document.LoadFile(path.c_str()) != XML_SUCCESS)
    {
      if (errorString) *errorString = document.ErrorStr();
      return;
    }

    WorkingDirectory workingDirectory(path, true);

    const XMLElement* element = document.RootElement();

    if (auto infoElement = element->FirstChildElement("Info")) info = Info((XMLElement*)infoElement);

    if (auto contentElement = element->FirstChildElement("Content")) content = Content((XMLElement*)contentElement);

    if (auto animationsElement = element->FirstChildElement("Animations"))
      animations = Animations((XMLElement*)animationsElement);
  }

  uint64_t Anm2::hash()
  {
    return std::hash<std::string>{}(to_string());
  }

  Animation* Anm2::animation_get(Reference& reference)
  {
    return vector::find(animations.items, reference.animationIndex);
  }

  Item* Anm2::item_get(Reference& reference)
  {
    if (Animation* animation = animation_get(reference))
    {
      switch (reference.itemType)
      {
        case ROOT:
          return &animation->rootAnimation;
        case LAYER:
          return unordered_map::find(animation->layerAnimations, reference.itemID);
        case NULL_:
          return map::find(animation->nullAnimations, reference.itemID);
        case TRIGGER:
          return &animation->triggers;
        default:
          return nullptr;
      }
    }
    return nullptr;
  }

  Frame* Anm2::frame_get(Reference& reference)
  {
    Item* item = item_get(reference);
    if (!item) return nullptr;
    return vector::find(item->frames, reference.frameIndex);
    return nullptr;
  }

  bool Anm2::spritesheet_add(const std::string& directory, const std::string& path, int& id)
  {
    Spritesheet spritesheet(directory, path);
    if (!spritesheet.is_valid()) return false;
    id = map::next_id_get(content.spritesheets);
    content.spritesheets[id] = std::move(spritesheet);
    return true;
  }

  void Anm2::spritesheet_remove(int id)
  {
    content.spritesheets.erase(id);
  }

  Spritesheet* Anm2::spritesheet_get(int id)
  {
    return map::find(content.spritesheets, id);
  }

  std::set<int> Anm2::spritesheets_unused()
  {
    return content.spritesheets_unused();
  }

  Reference Anm2::layer_add(Reference reference, std::string name, int spritesheetID, locale::Type locale)
  {
    auto id = reference.itemID == -1 ? map::next_id_get(content.layers) : reference.itemID;
    auto& layer = content.layers[id];

    layer.name = !name.empty() ? name : layer.name;
    layer.spritesheetID = content.spritesheets.contains(spritesheetID) ? spritesheetID : 0;

    auto add = [&](Animation* animation, int id)
    {
      animation->layerAnimations[id] = Item();
      animation->layerOrder.push_back(id);
    };

    if (locale == locale::GLOBAL)
    {
      for (auto& animation : animations.items)
        if (!animation.layerAnimations.contains(id)) add(&animation, id);
    }
    else if (locale == locale::LOCAL)
    {
      if (auto animation = animation_get(reference))
        if (!animation->layerAnimations.contains(id)) add(animation, id);
    }

    return {reference.animationIndex, LAYER, id};
  }

  Reference Anm2::null_add(Reference reference, std::string name, locale::Type locale)
  {
    auto id = reference.itemID == -1 ? map::next_id_get(content.nulls) : reference.itemID;
    auto& null = content.nulls[id];

    null.name = !name.empty() ? name : null.name;

    auto add = [&](Animation* animation, int id) { animation->nullAnimations[id] = Item(); };

    if (locale == locale::GLOBAL)
    {
      for (auto& animation : animations.items)
        if (!animation.nullAnimations.contains(id)) add(&animation, id);
    }
    else if (locale == locale::LOCAL)
    {
      if (auto animation = animation_get(reference))
        if (!animation->nullAnimations.contains(id)) add(animation, id);
    }

    return {reference.animationIndex, LAYER, id};
  }

  void Anm2::event_add(int& id)
  {
    content.event_add(id);
  }

  std::set<int> Anm2::events_unused(Reference reference)
  {
    std::set<int> used{};
    std::set<int> unused{};

    if (auto animation = animation_get(reference); animation)
      for (auto& frame : animation->triggers.frames)
        used.insert(frame.eventID);
    else
      for (auto& animation : animations.items)
        for (auto& frame : animation.triggers.frames)
          used.insert(frame.eventID);

    for (auto& id : content.events | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  std::set<int> Anm2::layers_unused(Reference reference)
  {
    std::set<int> used{};
    std::set<int> unused{};

    if (auto animation = animation_get(reference); animation)
      for (auto& id : animation->layerAnimations | std::views::keys)
        used.insert(id);
    else
      for (auto& animation : animations.items)
        for (auto& id : animation.layerAnimations | std::views::keys)
          used.insert(id);

    for (auto& id : content.layers | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  std::set<int> Anm2::nulls_unused(Reference reference)
  {
    std::set<int> used{};
    std::set<int> unused{};

    if (auto animation = animation_get(reference); animation)
      for (auto& id : animation->nullAnimations | std::views::keys)
        used.insert(id);
    else
      for (auto& animation : animations.items)
        for (auto& id : animation.nullAnimations | std::views::keys)
          used.insert(id);

    for (auto& id : content.nulls | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  std::vector<std::string> Anm2::spritesheet_names_get()
  {
    std::vector<std::string> spritesheets{};
    for (auto& [id, spritesheet] : content.spritesheets)
      spritesheets.push_back(std::format("#{} {}", id, spritesheet.path.c_str()));
    return spritesheets;
  }

  void Anm2::bake(Reference reference, int interval, bool isRoundScale, bool isRoundRotation)
  {
    Item* item = item_get(reference);
    if (!item) return;

    Frame* frame = frame_get(reference);
    if (!frame) return;

    if (frame->delay == FRAME_DELAY_MIN) return;

    Reference referenceNext = reference;
    referenceNext.frameIndex = reference.frameIndex + 1;

    Frame* frameNext = frame_get(referenceNext);
    if (!frameNext) frameNext = frame;

    Frame baseFrame = *frame;
    Frame baseFrameNext = *frameNext;

    int delay{};
    int index = reference.frameIndex;

    while (delay < baseFrame.delay)
    {
      float interpolation = (float)delay / baseFrame.delay;

      Frame baked = baseFrame;
      baked.delay = std::min(interval, baseFrame.delay - delay);
      baked.isInterpolated = (index == reference.frameIndex) ? baseFrame.isInterpolated : false;

      baked.rotation = glm::mix(baseFrame.rotation, baseFrameNext.rotation, interpolation);
      baked.position = glm::mix(baseFrame.position, baseFrameNext.position, interpolation);
      baked.scale = glm::mix(baseFrame.scale, baseFrameNext.scale, interpolation);
      baked.offset = glm::mix(baseFrame.offset, baseFrameNext.offset, interpolation);
      baked.tint = glm::mix(baseFrame.tint, baseFrameNext.tint, interpolation);

      if (isRoundScale) baked.scale = vec2(ivec2(baked.scale));
      if (isRoundRotation) baked.rotation = (int)baked.rotation;

      if (index == reference.frameIndex)
        item->frames[index] = baked;
      else
        item->frames.insert(item->frames.begin() + index, baked);
      index++;

      delay += baked.delay;
    }
  }
}
