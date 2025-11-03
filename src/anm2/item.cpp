#include "item.h"
#include <ranges>

#include "vector_.h"
#include "xml_.h"

using namespace anm2ed::util;
using namespace tinyxml2;
using namespace glm;

namespace anm2ed::anm2
{
  Item::Item(XMLElement* element, Type type, int* id)
  {
    if (type == LAYER && id) element->QueryIntAttribute("LayerId", id);
    if (type == NULL_ && id) element->QueryIntAttribute("NullId", id);

    element->QueryBoolAttribute("Visible", &isVisible);

    for (auto child = type == TRIGGER ? element->FirstChildElement("Trigger") : element->FirstChildElement("Frame");
         child; child = type == TRIGGER ? child->NextSiblingElement("Trigger") : child->NextSiblingElement("Frame"))
      frames.push_back(Frame(child, type));
  }

  XMLElement* Item::to_element(XMLDocument& document, Type type, int id)
  {
    auto element = document.NewElement(TYPE_ANIMATION_STRINGS[type]);

    if (type == LAYER) element->SetAttribute("LayerId", id);
    if (type == NULL_) element->SetAttribute("NullId", id);
    if (type == LAYER || type == NULL_) element->SetAttribute("Visible", isVisible);

    for (auto& frame : frames)
      frame.serialize(document, element, type);

    return element;
  }

  void Item::serialize(XMLDocument& document, XMLElement* parent, Type type, int id)
  {
    parent->InsertEndChild(to_element(document, type, id));
  }

  std::string Item::to_string(Type type, int id)
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document, type, id));
    return xml::document_to_string(document);
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
      frame.colorOffset = glm::mix(frame.colorOffset, frameNext->colorOffset, interpolation);
      frame.tint = glm::mix(frame.tint, frameNext->tint, interpolation);
    }

    return frame;
  }

  void Item::frames_change(FrameChange& change, ChangeType type, int start, int numberFrames)
  {
    auto useStart = numberFrames > -1 ? start : 0;
    auto end = numberFrames > -1 ? start + numberFrames : (int)frames.size();
    end = glm::clamp(end, start, (int)frames.size());

    for (int i = useStart; i < end; i++)
    {
      Frame& frame = frames[i];

      if (change.isVisible) frame.isVisible = *change.isVisible;
      if (change.isInterpolated) frame.isInterpolated = *change.isInterpolated;

      switch (type)
      {
        case ADJUST:
          if (change.rotation) frame.rotation = *change.rotation;
          if (change.delay) frame.delay = std::max(FRAME_DELAY_MIN, *change.delay);
          if (change.crop) frame.crop = *change.crop;
          if (change.pivot) frame.pivot = *change.pivot;
          if (change.position) frame.position = *change.position;
          if (change.size) frame.size = *change.size;
          if (change.scale) frame.scale = *change.scale;
          if (change.colorOffset) frame.colorOffset = glm::clamp(*change.colorOffset, 0.0f, 1.0f);
          if (change.tint) frame.tint = glm::clamp(*change.tint, 0.0f, 1.0f);
          break;

        case ADD:
          if (change.rotation) frame.rotation += *change.rotation;
          if (change.delay) frame.delay = std::max(FRAME_DELAY_MIN, frame.delay + *change.delay);
          if (change.crop) frame.crop += *change.crop;
          if (change.pivot) frame.pivot += *change.pivot;
          if (change.position) frame.position += *change.position;
          if (change.size) frame.size += *change.size;
          if (change.scale) frame.scale += *change.scale;
          if (change.colorOffset) frame.colorOffset = glm::clamp(frame.colorOffset + *change.colorOffset, 0.0f, 1.0f);
          if (change.tint) frame.tint = glm::clamp(frame.tint + *change.tint, 0.0f, 1.0f);
          break;

        case SUBTRACT:
          if (change.rotation) frame.rotation -= *change.rotation;
          if (change.delay) frame.delay = std::max(FRAME_DELAY_MIN, frame.delay - *change.delay);
          if (change.crop) frame.crop -= *change.crop;
          if (change.pivot) frame.pivot -= *change.pivot;
          if (change.position) frame.position -= *change.position;
          if (change.size) frame.size -= *change.size;
          if (change.scale) frame.scale -= *change.scale;
          if (change.colorOffset) frame.colorOffset = glm::clamp(frame.colorOffset - *change.colorOffset, 0.0f, 1.0f);
          if (change.tint) frame.tint = glm::clamp(frame.tint - *change.tint, 0.0f, 1.0f);
          break;
      }
    }
  }

  bool Item::frames_deserialize(const std::string& string, Type type, int start, std::set<int>& indices,
                                std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      if (!document.FirstChildElement("Frame"))
      {
        if (errorString) *errorString = "No valid frame(s).";
        return false;
      }

      int count{};
      for (auto element = document.FirstChildElement("Frame"); element; element = element->NextSiblingElement("Frame"))
      {
        auto index = start + count;
        frames.insert(frames.begin() + start + count, Frame(element, type));
        indices.insert(index);
        count++;
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

  void Item::frames_bake(int index, int interval, bool isRoundScale, bool isRoundRotation)
  {
    if (!vector::in_bounds(frames, index)) return;

    Frame& frame = frames[index];
    if (frame.delay == FRAME_DELAY_MIN) return;

    Frame frameNext = vector::in_bounds(frames, index + 1) ? frames[index + 1] : frame;

    int delay{};
    int i = index;

    while (delay < frame.delay)
    {
      Frame baked = frame;
      float interpolation = (float)delay / frame.delay;
      baked.delay = std::min(interval, frame.delay - delay);
      baked.isInterpolated = (i == index) ? frame.isInterpolated : false;
      baked.rotation = glm::mix(frame.rotation, frameNext.rotation, interpolation);
      baked.position = glm::mix(frame.position, frameNext.position, interpolation);
      baked.scale = glm::mix(frame.scale, frameNext.scale, interpolation);
      baked.colorOffset = glm::mix(frame.colorOffset, frameNext.colorOffset, interpolation);
      baked.tint = glm::mix(frame.tint, frameNext.tint, interpolation);
      if (isRoundScale) baked.scale = vec2(ivec2(baked.scale));
      if (isRoundRotation) baked.rotation = (int)baked.rotation;

      if (i == index)
        frames[i] = baked;
      else
        frames.insert(frames.begin() + i, baked);
      i++;

      delay += baked.delay;
    }
  }

  void Item::frames_generate_from_grid(ivec2 startPosition, ivec2 size, ivec2 pivot, int columns, int count, int delay)
  {
    for (int i = 0; i < count; i++)
    {
      Frame frame{};
      frame.delay = delay;
      frame.pivot = pivot;
      frame.size = size;
      frame.crop = startPosition + ivec2(size.x * (i % columns), size.y * (i / columns));

      frames.emplace_back(frame);
    }
  }
}