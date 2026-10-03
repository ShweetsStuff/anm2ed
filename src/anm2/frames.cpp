#include "internal.hpp"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed
{
  enum class InterpolationBakeType
  {
    SPECIAL,
    ALL
  };

  void frame_mix(Element& frame, const Element& next, float amount)
  {
    frame.rotation = glm::mix(frame.rotation, next.rotation, amount);
    frame.position = glm::mix(frame.position, next.position, amount);
    frame.scale = glm::mix(frame.scale, next.scale, amount);
    frame.shear = glm::mix(frame.shear, next.shear, amount);
    frame.colorOffset = glm::mix(frame.colorOffset, next.colorOffset, amount);
    frame.tint = glm::mix(frame.tint, next.tint, amount);
  }

  std::vector<Element> frame_bake_split(const Element& original, const Element& next, int interval, bool isRoundScale,
                                        bool isRoundRotation)
  {
    auto total = std::max(original.duration, FRAME_DURATION_MIN);
    interval = std::max(interval, FRAME_DURATION_MIN);
    std::vector<Element> frames{};
    frames.reserve((total + interval - 1) / interval);
    for (int duration = 0; duration < total; duration += interval)
    {
      auto& baked = frames.emplace_back(original);
      frame_mix(baked, next, interpolation_factor(original.interpolation, (float)duration / total));
      baked.duration = std::min(interval, total - duration);
      baked.interpolation = Interpolation::NONE;
      if (isRoundScale) baked.scale = glm::round(baked.scale);
      if (isRoundRotation) baked.rotation = std::round(baked.rotation);
    }
    return frames;
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
      frame_mix(frame, *frameNext,
                interpolation_factor(frame.interpolation, (time - durationCurrent) / (durationNext - durationCurrent)));
    }

    return frame;
  }

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
      if (original.type != ElementType::FRAME || !is_frame_interpolation_baked(original, type)) continue;

      if (original.duration <= FRAME_DURATION_MIN)
      {
        track.children[index].interpolation = Interpolation::NONE;
        continue;
      }

      auto isNextFrame = index + 1 < (int)track.children.size() && track.children[index + 1].type == ElementType::FRAME;
      auto baked = frame_bake_split(original, isNextFrame ? track.children[index + 1] : original, interval,
                                    isRoundScale, isRoundRotation);
      track.children.erase(track.children.begin() + index);
      track.children.insert(track.children.begin() + index, std::make_move_iterator(baked.begin()),
                            std::make_move_iterator(baked.end()));
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

  void all_interpolated_frames_bake(Element& element, int interval, bool isRoundScale, bool isRoundRotation)
  {
    interpolated_frames_bake(element, interval, isRoundScale, isRoundRotation, InterpolationBakeType::ALL);
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

}
