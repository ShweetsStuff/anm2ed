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
      auto amount =
          interpolation_factor(frame.interpolation, (time - durationCurrent) / (durationNext - durationCurrent));
      frame.rotation = glm::mix(frame.rotation, frameNext->rotation, amount);
      frame.position = glm::mix(frame.position, frameNext->position, amount);
      frame.scale = glm::mix(frame.scale, frameNext->scale, amount);
      frame.shear = glm::mix(frame.shear, frameNext->shear, amount);
      frame.colorOffset = glm::mix(frame.colorOffset, frameNext->colorOffset, amount);
      frame.tint = glm::mix(frame.tint, frameNext->tint, amount);
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
    int frameCount{};
    for (const auto& frame : track.children)
      if (frame.type == frameType) ++frameCount;
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

    auto frame = &track.children[childIndex];

    auto original = *frame;
    if (original.duration <= FRAME_DURATION_MIN)
    {
      frame->interpolation = Interpolation::NONE;
      return;
    }

    auto nextFrame = track_frame_get(track, index + 1);
    auto next = nextFrame ? *nextFrame : original;
    int duration{};
    interval = std::max(interval, FRAME_DURATION_MIN);
    std::vector<Element> bakedFrames{};
    bakedFrames.reserve((original.duration + interval - 1) / interval);

    while (duration < original.duration)
    {
      auto baked = original;
      auto amount = interpolation_factor(original.interpolation, (float)duration / original.duration);
      baked.duration = std::min(interval, original.duration - duration);
      baked.interpolation = Interpolation::NONE;
      baked.rotation = glm::mix(original.rotation, next.rotation, amount);
      baked.position = glm::mix(original.position, next.position, amount);
      baked.scale = glm::mix(original.scale, next.scale, amount);
      baked.shear = glm::mix(original.shear, next.shear, amount);
      baked.colorOffset = glm::mix(original.colorOffset, next.colorOffset, amount);
      baked.tint = glm::mix(original.tint, next.tint, amount);
      if (isRoundScale) baked.scale = glm::round(baked.scale);
      if (isRoundRotation) baked.rotation = std::round(baked.rotation);

      bakedFrames.push_back(baked);
      duration += baked.duration;
    }

    if (bakedFrames.empty()) return;
    track.children[childIndex] = std::move(bakedFrames.front());
    track.children.insert(track.children.begin() + childIndex + 1, std::make_move_iterator(bakedFrames.begin() + 1),
                          std::make_move_iterator(bakedFrames.end()));
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
    auto first =
        frameType == ElementType::TRIGGER ? document.FirstChildElement("Trigger") : document.FirstChildElement("Frame");
    if (!first)
    {
      if (errorString) *errorString = frameType == ElementType::TRIGGER ? "No valid trigger(s)." : "No valid frame(s).";
      return false;
    }

    if (frameType == ElementType::FRAME)
    {
      start = std::clamp(start, 0, track_frames_count_get(track));
      std::vector<Element> frames{};
      for (auto element = first; element; element = element->NextSiblingElement("Frame"))
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
    for (auto element = first; element; element = element->NextSiblingElement("Trigger"))
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
    const auto clamp_identity = [](auto value) { return value; };
    const auto clamp_duration = [](int value) { return std::max(FRAME_DURATION_MIN, value); };

    if (selection.empty()) return;

    auto apply_scalar_with_clamp = [&](auto& target, const auto& optionalValue, auto clampFunc)
    {
      if (!optionalValue) return;
      auto value = *optionalValue;

      switch (changeType)
      {
        case ChangeType::ADJUST:
          target = clampFunc(value);
          break;
        case ChangeType::ADD:
          target = clampFunc(target + value);
          break;
        case ChangeType::SUBTRACT:
          target = clampFunc(target - value);
          break;
        case ChangeType::MULTIPLY:
          target = clampFunc(target * value);
          break;
        case ChangeType::DIVIDE:
          if (value == decltype(value){}) return;
          target = clampFunc(target / value);
          break;
      }
    };

    auto apply_scalar = [&](auto& target, const auto& optionalValue)
    { apply_scalar_with_clamp(target, optionalValue, clamp_identity); };

    for (auto index : selection)
    {
      auto frame = track_frame_get(track, index);
      if (!frame) continue;

      if (change.isVisible) frame->isVisible = *change.isVisible;
      if (change.interpolation) frame->interpolation = *change.interpolation;
      if (change.isFlipX) frame->scale.x = -frame->scale.x;
      if (change.isFlipY) frame->scale.y = -frame->scale.y;

      apply_scalar(frame->rotation, change.rotation);
      apply_scalar_with_clamp(frame->duration, change.duration, clamp_duration);

      if (itemType == ItemType::LAYER)
      {
        apply_scalar(frame->crop.x, change.cropX);
        apply_scalar(frame->crop.y, change.cropY);
        apply_scalar(frame->pivot.x, change.pivotX);
        apply_scalar(frame->pivot.y, change.pivotY);
        apply_scalar(frame->size.x, change.sizeX);
        apply_scalar(frame->size.y, change.sizeY);
        if (change.regionId) frame->regionId = *change.regionId;
        if (change.shaderId) frame->shaderId = *change.shaderId;
      }

      apply_scalar(frame->position.x, change.positionX);
      apply_scalar(frame->position.y, change.positionY);
      apply_scalar(frame->scale.x, change.scaleX);
      apply_scalar(frame->scale.y, change.scaleY);
      apply_scalar(frame->shear.x, change.shearX);
      apply_scalar(frame->shear.y, change.shearY);
      apply_scalar(frame->colorOffset.x, change.colorOffsetR);
      apply_scalar(frame->colorOffset.y, change.colorOffsetG);
      apply_scalar(frame->colorOffset.z, change.colorOffsetB);
      apply_scalar(frame->tint.x, change.tintR);
      apply_scalar(frame->tint.y, change.tintG);
      apply_scalar(frame->tint.z, change.tintB);
      apply_scalar(frame->tint.w, change.tintA);
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
      if (original.type != ElementType::FRAME) continue;
      if (!is_frame_interpolation_baked(original, type)) continue;

      if (original.duration <= FRAME_DURATION_MIN)
      {
        track.children[index].interpolation = Interpolation::NONE;
        continue;
      }

      auto nextFrame = index + 1 < (int)track.children.size() && track.children[index + 1].type == ElementType::FRAME
                           ? track.children[index + 1]
                           : original;
      int duration{};
      int insertIndex = index;

      while (duration < original.duration)
      {
        auto baked = original;
        float amount = interpolation_factor(original.interpolation, (float)duration / original.duration);
        baked.duration = std::min(interval, original.duration - duration);
        baked.interpolation = Interpolation::NONE;
        baked.rotation = glm::mix(original.rotation, nextFrame.rotation, amount);
        baked.position = glm::mix(original.position, nextFrame.position, amount);
        baked.scale = glm::mix(original.scale, nextFrame.scale, amount);
        baked.shear = glm::mix(original.shear, nextFrame.shear, amount);
        baked.colorOffset = glm::mix(original.colorOffset, nextFrame.colorOffset, amount);
        baked.tint = glm::mix(original.tint, nextFrame.tint, amount);
        if (isRoundScale) baked.scale = glm::round(baked.scale);
        if (isRoundRotation) baked.rotation = std::round(baked.rotation);

        if (insertIndex == index)
          track.children[insertIndex] = baked;
        else
          track.children.insert(track.children.begin() + insertIndex, baked);

        duration += baked.duration;
        ++insertIndex;
      }
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

  void layer_animation_ids_remap(Element& element, const std::unordered_map<int, int>& remap)
  {
    if (element.type == ElementType::LAYER_ANIMATION)
      if (auto it = remap.find(element.layerId); it != remap.end()) element.layerId = it->second;

    for (auto& child : element.children)
      layer_animation_ids_remap(child, remap);
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
    if (auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS))
    {
      group_roots_length_apply(*layerAnimations);
      tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                  [&](const Element& track) { length = std::max(length, track_length_get(track)); });
    }
    if (auto nullAnimations = child_first_get(animation, ElementType::NULL_ANIMATIONS))
    {
      group_roots_length_apply(*nullAnimations);
      tracks_each(*nullAnimations, ElementType::NULL_ANIMATION,
                  [&](const Element& track) { length = std::max(length, track_length_get(track)); });
    }
    if (auto triggers = child_first_get(animation, ElementType::TRIGGERS))
      length = std::max(length, track_length_get(*triggers));
    return std::max(length, FRAME_DURATION_MIN);
  }
}
