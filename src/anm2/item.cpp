#include "item.h"
#include <algorithm>
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
    auto element = document.NewElement(TYPE_ITEM_STRINGS[type]);

    if (type == LAYER) element->SetAttribute("LayerId", id);
    if (type == NULL_) element->SetAttribute("NullId", id);
    if (type == LAYER || type == NULL_) element->SetAttribute("Visible", isVisible);

    if (type == TRIGGER) frames_sort_by_at_frame();

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
        length += frame.duration;

    return length;
  }

  void Item::frames_sort_by_at_frame()
  {
    std::sort(frames.begin(), frames.end(), [](const Frame& a, const Frame& b) { return a.atFrame < b.atFrame; });
  }

  Frame Item::frame_generate(float time, Type type)
  {
    Frame frame{};
    frame.isVisible = false;

    if (frames.empty()) return frame;

    time = time < 0.0f ? 0.0f : time;

    Frame* frameNext = nullptr;
    int durationCurrent = 0;
    int durationNext = 0;

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

        durationNext += frame.duration;

        if (time >= durationCurrent && time < durationNext)
        {
          if (i + 1 < (int)frames.size())
            frameNext = &frames[i + 1];
          else
            frameNext = nullptr;
          break;
        }

        durationCurrent += frame.duration;
      }
    }

    if (type != TRIGGER && frame.isInterpolated && frameNext && frame.duration > 1)
    {
      auto interpolation = (time - durationCurrent) / (durationNext - durationCurrent);

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
          if (change.duration) frame.duration = std::max(FRAME_DURATION_MIN, *change.duration);
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
          if (change.duration) frame.duration = std::max(FRAME_DURATION_MIN, frame.duration + *change.duration);
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
          if (change.duration) frame.duration = std::max(FRAME_DURATION_MIN, frame.duration - *change.duration);
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
      int count{};
      if (document.FirstChildElement("Frame") && type != anm2::TRIGGER)
      {
        start = std::clamp(start, 0, (int)frames.size());
        for (auto element = document.FirstChildElement("Frame"); element;
             element = element->NextSiblingElement("Frame"))
        {
          auto index = start + count;
          frames.insert(frames.begin() + start + count, Frame(element, type));
          indices.insert(index);
          count++;
        }

        return true;
      }
      else if (document.FirstChildElement("Trigger") && type == anm2::TRIGGER)
      {
        auto has_conflict = [&](int value)
        {
          for (auto& trigger : frames)
            if (trigger.atFrame == value) return true;
          return false;
        };

        for (auto element = document.FirstChildElement("Trigger"); element;
             element = element->NextSiblingElement("Trigger"))
        {
          Frame trigger(element, type);
          trigger.atFrame = start + count;
          while (has_conflict(trigger.atFrame))
            trigger.atFrame++;
          frames.push_back(trigger);
          indices.insert(trigger.atFrame);
          count++;
        }

        frames_sort_by_at_frame();
        return true;
      }
      else
      {
        if (errorString) *errorString = type == anm2::TRIGGER ? "No valid trigger(s)." : "No valid frame(s).";
        return false;
      }
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

  void Item::frames_bake(int index, int interval, bool isRoundScale, bool isRoundRotation)
  {
    if (!vector::in_bounds(frames, index)) return;

    auto original = frames[index];
    if (original.duration == FRAME_DURATION_MIN) return;

    auto nextFrame = vector::in_bounds(frames, index + 1) ? frames[index + 1] : original;

    int duration{};
    int i = index;

    while (duration < original.duration)
    {
      Frame baked = original;
      float interpolation = (float)duration / original.duration;
      baked.duration = std::min(interval, original.duration - duration);
      baked.isInterpolated = (i == index) ? original.isInterpolated : false;
      baked.rotation = glm::mix(original.rotation, nextFrame.rotation, interpolation);
      baked.position = glm::mix(original.position, nextFrame.position, interpolation);
      baked.scale = glm::mix(original.scale, nextFrame.scale, interpolation);
      baked.colorOffset = glm::mix(original.colorOffset, nextFrame.colorOffset, interpolation);
      baked.tint = glm::mix(original.tint, nextFrame.tint, interpolation);
      if (isRoundScale) baked.scale = vec2(ivec2(baked.scale));
      if (isRoundRotation) baked.rotation = (int)baked.rotation;

      if (i == index)
        frames[i] = baked;
      else
        frames.insert(frames.begin() + i, baked);
      i++;

      duration += baked.duration;
    }
  }

  void Item::frames_generate_from_grid(ivec2 startPosition, ivec2 size, ivec2 pivot, int columns, int count,
                                       int duration)
  {
    for (int i = 0; i < count; i++)
    {
      Frame frame{};
      frame.duration = duration;
      frame.pivot = pivot;
      frame.size = size;
      frame.crop = startPosition + ivec2(size.x * (i % columns), size.y * (i / columns));

      frames.emplace_back(frame);
    }
  }

  int Item::frame_index_from_at_frame_get(int atFrame)
  {
    for (auto [i, frame] : std::views::enumerate(frames))
      if (frame.atFrame == atFrame) return i;
    return -1;
  }

  float Item::frame_time_from_index_get(int index)
  {
    if (!vector::in_bounds(frames, index)) return 0.0f;

    float time{};
    for (auto [i, frame] : std::views::enumerate(frames))
    {
      if (i == index) return time;
      time += frame.duration;
    }

    return time;
  }
}
