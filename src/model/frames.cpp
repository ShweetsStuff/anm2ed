#include "frames.hpp"

#include <algorithm>
#include <limits>

#include "math.hpp"

using namespace anm2ed::util;

namespace anm2ed::model
{
  void frame_mix(Frame& frame, const Frame& next, float amount)
  {
    frame.rotation = glm::mix(frame.rotation, next.rotation, amount);
    frame.position = glm::mix(frame.position, next.position, amount);
    frame.scale = glm::mix(frame.scale, next.scale, amount);
    frame.shear = glm::mix(frame.shear, next.shear, amount);
    frame.colorOffset = glm::mix(frame.colorOffset, next.colorOffset, amount);
    frame.tint = glm::mix(frame.tint, next.tint, amount);
  }

  // Splits an interpolated frame into uninterpolated frames of at most `interval`; the first keeps the frame's uid.
  std::vector<Frame> frame_bake_split(const Frame& original, const Frame& next, int interval, bool isRoundScale,
                                      bool isRoundRotation)
  {
    auto total = std::max(original.duration, FRAME_DURATION_MIN);
    interval = std::max(interval, FRAME_DURATION_MIN);
    std::vector<Frame> frames{};
    for (int duration = 0; duration < total; duration += interval)
    {
      auto& baked = frames.emplace_back(duration == 0 ? original : frame_clone(original));
      frame_mix(baked, next, interpolation_factor(original.interpolation, (float)duration / total));
      baked.duration = std::min(interval, total - duration);
      baked.interpolation = Interpolation::NONE;
      if (isRoundScale) baked.scale = glm::round(baked.scale);
      if (isRoundRotation) baked.rotation = std::round(baked.rotation);
    }
    return frames;
  }

  glm::mat4 frame_parent_model_get(const Frame& frame)
  {
    return math::quad_model_parent_get(frame.position, {}, math::percent_to_unit(frame.scale), frame.rotation,
                                       math::percent_to_unit(frame.shear));
  }

  // The track's state at a time: the frame playing then, mixed toward the next one when interpolated. A trigger is
  // returned only on its exact frame; otherwise the result is invisible.
  Frame frame_generate(const Track& track, float time)
  {
    Frame frame{};
    frame.isVisible = false;
    if (track.frames.empty()) return frame;
    time = std::max(time, 0.0f);

    if (track.type == ItemType::TRIGGER)
    {
      for (const auto& trigger : track.frames)
        if ((int)time == trigger.atFrame) return trigger;
      return frame;
    }

    const Frame* next{};
    int durationCurrent{};
    int durationNext{};
    for (int i = 0; i < (int)track.frames.size(); ++i)
    {
      frame = track.frames[i];
      durationNext += frame.duration;
      if (time >= durationCurrent && time < durationNext)
      {
        if (i + 1 < (int)track.frames.size()) next = &track.frames[i + 1];
        break;
      }
      durationCurrent += frame.duration;
    }

    if (frame.interpolation != Interpolation::NONE && next && frame.duration > 1)
      frame_mix(frame, *next,
                interpolation_factor(frame.interpolation, (time - durationCurrent) / (durationNext - durationCurrent)));
    return frame;
  }

  int frame_index_from_at_frame_get(const Track& track, int atFrame)
  {
    for (int i = 0; i < (int)track.frames.size(); ++i)
      if (track.frames[i].atFrame == atFrame) return i;
    return -1;
  }

  int frame_index_from_time_get(const Track& track, float time)
  {
    if (track.type == ItemType::TRIGGER) return frame_index_from_at_frame_get(track, (int)time);
    if (track.frames.empty()) return -1;
    if (time <= 0.0f) return 0;

    float duration{};
    for (int i = 0; i < (int)track.frames.size(); ++i)
    {
      duration += track.frames[i].duration;
      if (time < duration) return i;
    }
    return (int)track.frames.size() - 1;
  }

  float frame_time_from_index_get(const Track& track, int index)
  {
    if (index < 0 || index >= (int)track.frames.size()) return 0.0f;
    if (track.type == ItemType::TRIGGER) return (float)track.frames[index].atFrame;
    float time{};
    for (int i = 0; i < index; ++i)
      time += track.frames[i].duration;
    return time;
  }

  void frame_bake(Track& track, int index, int interval, bool isRoundScale, bool isRoundRotation)
  {
    if (index < 0 || index >= (int)track.frames.size()) return;
    auto original = track.frames[index];
    if (original.duration <= FRAME_DURATION_MIN)
    {
      track.frames[index].interpolation = Interpolation::NONE;
      return;
    }

    auto next = index + 1 < (int)track.frames.size() ? track.frames[index + 1] : original;
    auto baked = frame_bake_split(original, next, interval, isRoundScale, isRoundRotation);
    track.frames.erase(track.frames.begin() + index);
    track.frames.insert(track.frames.begin() + index, baked.begin(), baked.end());
  }

  void frames_generate_from_grid(Track& track, glm::ivec2 startPosition, glm::ivec2 size, glm::vec2 pivot, int columns,
                                 int count, int duration)
  {
    for (int i = 0; i < count; ++i)
    {
      auto& frame = track.frames.emplace_back();
      frame.duration = duration;
      frame.pivot = pivot;
      frame.size = size;
      frame.crop = startPosition + glm::ivec2(size.x * (i % columns), size.y * (i / columns));
    }
  }

  void frames_sort_by_at_frame(Track& track)
  {
    std::stable_sort(track.frames.begin(), track.frames.end(),
                     [](const Frame& a, const Frame& b) { return a.atFrame < b.atFrame; });
  }

  void frames_change(Track& track, FrameChange change, ItemType itemType, ChangeType changeType,
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
      if (index < 0 || index >= (int)track.frames.size()) continue;
      auto& frame = track.frames[index];
      if (change.isVisible) frame.isVisible = *change.isVisible;
      if (change.interpolation) frame.interpolation = *change.interpolation;
      if (change.isFlipX) frame.scale.x = -frame.scale.x;
      if (change.isFlipY) frame.scale.y = -frame.scale.y;

      apply(frame.duration, change.duration);
      if (change.duration) frame.duration = std::max(FRAME_DURATION_MIN, frame.duration);
      if (itemType == ItemType::LAYER && change.regionId) frame.regionId = *change.regionId;
      if (itemType == ItemType::LAYER && change.shaderId) frame.shaderId = *change.shaderId;
#define X(name, member, isLayerOnly, isColor, isEnabled, value)                                                        \
  if (!isLayerOnly || itemType == ItemType::LAYER) apply(frame.member, change.name);
      FRAME_CHANGE_SCALARS
#undef X
    }
  }

  // Bakes every ease-in/ease-out frame (not linear ones) in every track into single-step frames.
  void special_interpolated_frames_bake(Model& model, int interval, bool isRoundScale, bool isRoundRotation)
  {
    for (int i = 0; i < model.animations_count_get(); ++i)
      animation_tracks_each(*model.animation_edit(i),
                            [&](Track& track)
                            {
                              if (track.type == ItemType::TRIGGER) return;
                              for (int index = (int)track.frames.size() - 1; index >= 0; --index)
                              {
                                auto interpolation = track.frames[index].interpolation;
                                if (interpolation == Interpolation::NONE || interpolation == Interpolation::LINEAR)
                                  continue;
                                frame_bake(track, index, interval, isRoundScale, isRoundRotation);
                              }
                            });
  }

  // Bounds of all visible layer frames over the animation's length, or -1 when nothing is drawn.
  glm::vec4 animation_rect(const Model& model, const Animation& animation, bool isRootTransform)
  {
    constexpr glm::ivec2 CORNERS[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    glm::vec2 min{std::numeric_limits<float>::infinity()};
    glm::vec2 max{-std::numeric_limits<float>::infinity()};
    bool isAny{};

    for (float t = 0.0f; t < (float)animation.frameNum; t += 1.0f)
    {
      glm::mat4 transform(1.0f);
      if (isRootTransform) transform *= frame_parent_model_get(frame_generate(animation.root, t));

      tracks_each(animation.layers,
                  [&](const Track& track, const TrackGroup* group)
                  {
                    if (!track.isVisible || (group && !group->isVisible)) return;
                    auto itemTransform = transform;
                    if (isRootTransform && group)
                      itemTransform *= frame_parent_model_get(frame_generate(group->root, t));

                    auto frame = model.frame_effective(track.id, frame_generate(track, t));
                    if (frame.size == glm::vec2() || !frame.isVisible) return;

                    auto layerTransform =
                        itemTransform * math::quad_model_get(frame.size, frame.position, frame.pivot,
                                                             math::percent_to_unit(frame.scale), frame.rotation,
                                                             math::percent_to_unit(frame.shear));
                    for (auto& corner : CORNERS)
                    {
                      auto world = glm::vec2(layerTransform * glm::vec4(corner, 0.0f, 1.0f));
                      min = glm::min(min, world);
                      max = glm::max(max, world);
                      isAny = true;
                    }
                  });
    }

    if (!isAny) return glm::vec4(-1.0f);
    return {min.x, min.y, max.x - min.x, max.y - min.y};
  }
}
