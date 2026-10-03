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

  glm::mat4 frame_parent_model_get(const Element& frame)
  {
    return math::quad_model_parent_get(frame.position, {}, math::percent_to_unit(frame.scale), frame.rotation,
                                       math::percent_to_unit(frame.shear));
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
    auto frameCount = track_frames_count_get(track);
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

    auto original = track.children[childIndex];
    if (original.duration <= FRAME_DURATION_MIN)
    {
      track.children[childIndex].interpolation = Interpolation::NONE;
      return;
    }

    auto nextFrame = track_frame_get(track, index + 1);
    auto baked = frame_bake_split(original, nextFrame ? *nextFrame : original, interval, isRoundScale, isRoundRotation);
    track.children.erase(track.children.begin() + childIndex);
    track.children.insert(track.children.begin() + childIndex, std::make_move_iterator(baked.begin()),
                          std::make_move_iterator(baked.end()));
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
    auto tag = element_tag_get(frameType).data();
    auto first = document.FirstChildElement(tag);
    if (!first)
    {
      if (errorString) *errorString = std::format("No valid {}(s).", tag);
      return false;
    }

    if (frameType == ElementType::FRAME)
    {
      start = std::clamp(start, 0, track_frames_count_get(track));
      std::vector<Element> frames{};
      for (auto element = first; element; element = element->NextSiblingElement(tag))
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
    for (auto element = first; element; element = element->NextSiblingElement(tag))
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
    auto apply = [&](auto& target, const auto& optionalValue)
    {
      if (!optionalValue) return;
      auto value = *optionalValue;
      if (changeType == ChangeType::ADJUST) target = value;
      if (changeType == ChangeType::ADD) target += value;
      if (changeType == ChangeType::SUBTRACT) target -= value;
      if (changeType == ChangeType::MULTIPLY) target *= value;
      if (changeType == ChangeType::DIVIDE && value != decltype(value){}) target /= value;
    };

    for (auto index : selection)
    {
      auto frame = track_frame_get(track, index);
      if (!frame) continue;

      if (change.isVisible) frame->isVisible = *change.isVisible;
      if (change.interpolation) frame->interpolation = *change.interpolation;
      if (change.isFlipX) frame->scale.x = -frame->scale.x;
      if (change.isFlipY) frame->scale.y = -frame->scale.y;

      apply(frame->duration, change.duration);
      if (change.duration) frame->duration = std::max(FRAME_DURATION_MIN, frame->duration);
      if (itemType == ItemType::LAYER && change.regionId) frame->regionId = *change.regionId;
      if (itemType == ItemType::LAYER && change.shaderId) frame->shaderId = *change.shaderId;
#define X(name, member, isLayerOnly, isColor, isEnabled, value)                                                        \
  if (!isLayerOnly || itemType == ItemType::LAYER) apply(frame->member, change.name);
      FRAME_CHANGE_SCALARS
#undef X
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

  void special_interpolated_frames_bake(Element& element, int interval, bool isRoundScale, bool isRoundRotation)
  {
    interpolated_frames_bake(element, interval, isRoundScale, isRoundRotation, InterpolationBakeType::SPECIAL);
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
    for (const auto& row : TRACK_CONTAINERS)
      if (auto container = child_first_get(animation, row.container))
      {
        group_roots_length_apply(*container);
        tracks_each(*container, row.track,
                    [&](const Element& track) { length = std::max(length, track_length_get(track)); });
      }
    if (auto triggers = child_first_get(animation, ElementType::TRIGGERS))
      length = std::max(length, track_length_get(*triggers));
    return std::max(length, FRAME_DURATION_MIN);
  }
}
