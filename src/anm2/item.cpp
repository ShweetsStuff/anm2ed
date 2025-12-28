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

  void Item::frames_change(FrameChange change, ChangeType type, std::set<int>& selection)
  {
    const auto clamp_identity = [](auto value) { return value; };
    const auto clamp01 = [](auto value) { return glm::clamp(value, 0.0f, 1.0f); };
    const auto clamp_duration = [](int value) { return std::max(FRAME_DURATION_MIN, value); };

    if (selection.empty()) return;

    auto apply_scalar_with_clamp = [&](auto& target, const auto& optionalValue, auto clampFunc)
    {
      if (!optionalValue) return;
      auto value = *optionalValue;

      switch (type)
      {
        case ADJUST:
          target = clampFunc(value);
          break;
        case ADD:
          target = clampFunc(target + value);
          break;
        case SUBTRACT:
          target = clampFunc(target - value);
          break;
        case MULTIPLY:
          target = clampFunc(target * value);
          break;
        case DIVIDE:
          if (value == decltype(value){}) return;
          target = clampFunc(target / value);
          break;
      }
    };

    auto apply_scalar = [&](auto& target, const auto& optionalValue)
    { apply_scalar_with_clamp(target, optionalValue, clamp_identity); };

    for (auto i : selection)
    {
      if (!vector::in_bounds(frames, i)) continue;
      Frame& frame = frames[i];

      if (change.isVisible) frame.isVisible = *change.isVisible;
      if (change.isInterpolated) frame.isInterpolated = *change.isInterpolated;
      if (change.isFlipX) frame.scale.x = -frame.scale.x;
      if (change.isFlipY) frame.scale.y = -frame.scale.y;

      apply_scalar(frame.rotation, change.rotation);
      apply_scalar_with_clamp(frame.duration, change.duration, clamp_duration);

      apply_scalar(frame.crop.x, change.cropX);
      apply_scalar(frame.crop.y, change.cropY);

      apply_scalar(frame.pivot.x, change.pivotX);
      apply_scalar(frame.pivot.y, change.pivotY);

      apply_scalar(frame.position.x, change.positionX);
      apply_scalar(frame.position.y, change.positionY);

      apply_scalar(frame.size.x, change.sizeX);
      apply_scalar(frame.size.y, change.sizeY);

      apply_scalar(frame.scale.x, change.scaleX);
      apply_scalar(frame.scale.y, change.scaleY);

      apply_scalar_with_clamp(frame.colorOffset.x, change.colorOffsetR, clamp01);
      apply_scalar_with_clamp(frame.colorOffset.y, change.colorOffsetG, clamp01);
      apply_scalar_with_clamp(frame.colorOffset.z, change.colorOffsetB, clamp01);

      apply_scalar_with_clamp(frame.tint.x, change.tintR, clamp01);
      apply_scalar_with_clamp(frame.tint.y, change.tintG, clamp01);
      apply_scalar_with_clamp(frame.tint.z, change.tintB, clamp01);
      apply_scalar_with_clamp(frame.tint.w, change.tintA, clamp01);
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

  int Item::frame_index_from_time_get(float time)
  {
    if (frames.empty()) return -1;
    if (time <= 0.0f) return 0;

    float duration{};
    for (auto [i, frame] : std::views::enumerate(frames))
    {
      duration += frame.duration;
      if (time < duration) return (int)i;
    }

    return (int)frames.size() - 1;
  }
}
