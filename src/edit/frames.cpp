#include "edit.hpp"

#include <utility>

#include "math.hpp"
#include "uid.hpp"

using namespace anm2ed::util;

namespace anm2ed::edit
{
  Element element_clone(Element element)
  {
    element_each(element, [](Element& child) { child.uid = uid_next(); });
    return element;
  }

  Element* track_get(Anm2& anm2, Reference reference)
  {
    reference.frameIndex = -1;
    return anm2.element_get(reference);
  }

  std::map<Reference, std::set<int>> frames_by_track_get(const std::set<Reference>& frames)
  {
    std::map<Reference, std::set<int>> result{};
    for (auto frame : frames)
    {
      auto index = std::exchange(frame.frameIndex, -1);
      result[frame].insert(index);
    }
    return result;
  }

  Uids frames_uids_get(const Element& track, int start, int count)
  {
    Uids uids{};
    for (int i = start; i < start + count; ++i)
      if (auto frame = track_frame_get(track, i)) uids.push_back(frame->uid);
    return uids;
  }

  Uids frame_insert(Anm2& anm2, Reference reference, int time)
  {
    auto track = track_get(anm2, reference);
    if (!track) return {};

    if (reference.itemType == TRIGGER)
    {
      if (std::ranges::any_of(track->children, [&](const Element& trigger) { return trigger.atFrame == time; }))
        return {};
      auto trigger = element_clone(element_make(ElementType::TRIGGER));
      trigger.atFrame = time;
      track->children.push_back(trigger);
      frames_sort_by_at_frame(*track);
      return {trigger.uid};
    }

    auto count = track_frames_count_get(*track);
    auto source = track_frame_get(*track, reference.frameIndex);
    auto last = track_frame_get(*track, count - 1);
    auto frame = element_clone(source ? *source : last ? *last : element_make(ElementType::FRAME));
    auto index = source ? std::clamp(reference.frameIndex + 1, 0, count) : count;
    track->children.insert(track->children.begin() + track_frame_insert_child_index_get(*track, index), frame);
    return {frame.uid};
  }

  Uids frames_delete(Anm2& anm2, const std::set<Reference>& frames)
  {
    for (auto& [trackReference, indices] : frames_by_track_get(frames))
      if (auto track = track_get(anm2, trackReference))
        for (auto it = indices.rbegin(); it != indices.rend(); ++it)
          if (auto childIndex = track_frame_child_index_get(*track, *it); childIndex != -1)
            track->children.erase(track->children.begin() + childIndex);
    return {};
  }

  Uids frames_duplicate(Anm2& anm2, const std::set<Reference>& frames, Reference focus)
  {
    Uids uids{};
    std::uint64_t focusUid{};
    for (auto& [trackReference, indices] : frames_by_track_get(frames))
    {
      auto track = track_get(anm2, trackReference);
      if (!track) continue;

      std::vector<Element> copies{};
      int lastIndex{-1};
      for (auto index : indices)
        if (auto frame = track_frame_get(*track, index))
        {
          copies.push_back(element_clone(*frame));
          lastIndex = index;
          if (trackReference.animationIndex == focus.animationIndex && index == focus.frameIndex &&
              track_get(anm2, focus) == track)
            focusUid = copies.back().uid;
        }
      if (copies.empty()) continue;

      for (const auto& copy : copies)
        uids.push_back(copy.uid);
      auto childIndex = track_frame_insert_child_index_get(*track, lastIndex + 1);
      track->children.insert(track->children.begin() + childIndex, copies.begin(), copies.end());
    }

    if (auto it = std::ranges::find(uids, focusUid); focusUid && it != uids.end())
      std::rotate(uids.begin(), it, it + 1);
    return uids;
  }

  Uids frames_reverse(Anm2& anm2, const std::set<Reference>& frames)
  {
    for (auto& [trackReference, indices] : frames_by_track_get(frames))
    {
      auto track = track_get(anm2, trackReference);
      if (!track || indices.size() < 2) continue;

      std::vector<Element> reversed{};
      for (auto it = indices.rbegin(); it != indices.rend(); ++it)
        if (auto childIndex = track_frame_child_index_get(*track, *it); childIndex != -1)
        {
          reversed.push_back(std::move(track->children[childIndex]));
          track->children.erase(track->children.begin() + childIndex);
        }
      auto childIndex = track_frame_insert_child_index_get(*track, *indices.begin());
      track->children.insert(track->children.begin() + childIndex, reversed.begin(), reversed.end());
    }
    return {};
  }

  Uids frames_bake(Anm2& anm2, const std::set<Reference>& frames, int interval, bool isRoundScale, bool isRoundRotation)
  {
    Uids uids{};
    for (auto& [trackReference, indices] : frames_by_track_get(frames))
    {
      auto track = track_get(anm2, trackReference);
      if (!track) continue;

      int inserted{};
      for (auto originalIndex : indices)
      {
        auto index = originalIndex + inserted;
        auto frame = track_frame_get(*track, index);
        if (!frame) continue;

        auto count = frame->duration <= FRAME_DURATION_MIN ? 1 : (int)std::ceil((float)frame->duration / interval);
        frame_bake(*track, index, interval, isRoundScale, isRoundRotation);
        anm2.uids_repair();
        uids.append_range(frames_uids_get(*track, index, count));
        inserted += count - 1;
      }
    }
    return uids;
  }

  Uids frame_split(Anm2& anm2, Reference reference, int time)
  {
    auto track = reference.itemType == TRIGGER ? nullptr : track_get(anm2, reference);
    auto frame = track ? track_frame_get(*track, reference.frameIndex) : nullptr;
    if (!frame || frame->duration <= 1) return {};

    auto duration = frame->duration;
    auto firstDuration = time - (int)std::round(frame_time_from_index_get(*track, reference.frameIndex)) + 1;
    if (firstDuration <= 0 || firstDuration >= duration) return {};

    auto split = element_clone(*frame);
    split.duration = duration - firstDuration;
    if (auto next = track_frame_get(*track, reference.frameIndex + 1);
        next && frame->interpolation != Interpolation::NONE)
      frame_mix(split, *next, interpolation_factor(frame->interpolation, (float)firstDuration / (float)duration));

    frame->duration = firstDuration;
    auto uid = frame->uid;
    track->children.insert(
        track->children.begin() + track_frame_insert_child_index_get(*track, reference.frameIndex + 1), split);
    return {uid};
  }

  // Moves frames (from any tracks) into one track before the frame at the given index.
  Uids frames_move(Anm2& anm2, const std::set<Reference>& frames, Reference target, int insertIndex)
  {
    auto targetTrack = track_get(anm2, target);
    if (!targetTrack) return {};
    insertIndex = std::clamp(insertIndex, 0, track_frames_count_get(*targetTrack));
    target.frameIndex = -1;

    std::vector<Element> moved{};
    for (auto& [trackReference, indices] : frames_by_track_get(frames))
    {
      auto track = track_get(anm2, trackReference);
      if (!track) continue;
      std::vector<Element> trackMoved{};
      for (auto it = indices.rbegin(); it != indices.rend(); ++it)
        if (auto childIndex = track_frame_child_index_get(*track, *it); childIndex != -1)
        {
          trackMoved.insert(trackMoved.begin(), std::move(track->children[childIndex]));
          track->children.erase(track->children.begin() + childIndex);
          if (track == targetTrack && *it < insertIndex) --insertIndex;
        }
      moved.append_range(trackMoved);
    }
    if (moved.empty()) return {};

    insertIndex = std::clamp(insertIndex, 0, track_frames_count_get(*targetTrack));
    auto childIndex = track_frame_insert_child_index_get(*targetTrack, insertIndex);
    targetTrack->children.insert(targetTrack->children.begin() + childIndex, moved.begin(), moved.end());
    return frames_uids_get(*targetTrack, insertIndex, (int)moved.size());
  }

  Uids frame_durations_set(Anm2& anm2, const std::map<Reference, int>& durations)
  {
    for (auto [reference, duration] : durations)
      if (auto frame = anm2.element_get(reference))
        frame->duration = std::clamp(duration, FRAME_DURATION_MIN, FRAME_DURATION_MAX);
    return {};
  }

  // Places a trigger at a frame, stepping it back past any other trigger already there.
  Uids trigger_at_frame_set(Anm2& anm2, Reference reference, int atFrame)
  {
    auto track = track_get(anm2, reference);
    auto trigger = track ? track_frame_get(*track, reference.frameIndex) : nullptr;
    if (!trigger) return {};
    trigger->atFrame = atFrame;
    for (const auto& other : track->children)
      if (&other != trigger && other.atFrame == trigger->atFrame) trigger->atFrame--;
    return {};
  }

  Uids frames_paste(Anm2& anm2, Reference reference, const std::set<Reference>& selection, const std::string& text,
                    int time, std::string* errorString)
  {
    auto track = track_get(anm2, reference);
    if (!track) return {};

    auto trackReference = reference;
    trackReference.frameIndex = -1;
    auto selected = frames_by_track_get(selection);
    auto count = (int)track->children.size();
    auto insertIndex = count;
    if (auto it = selected.find(trackReference); it != selected.end() && !it->second.empty())
      insertIndex = std::min(count, *it->second.rbegin() + 1);
    else if (reference.frameIndex >= 0 && reference.frameIndex < count)
      insertIndex = reference.frameIndex + 1;

    std::set<int> indices{};
    if (!frames_deserialize(*track, text, reference.itemType == TRIGGER ? time : insertIndex, indices, errorString))
      return {};

    auto layer = reference.itemType == LAYER ? anm2.element_get(ElementType::LAYER_ELEMENT, reference.itemID) : nullptr;
    auto spritesheet = layer ? anm2.element_get(ElementType::SPRITESHEET, layer->spritesheetId) : nullptr;
    for (auto index : indices)
    {
      auto frame = track_frame_get(*track, index);
      if (!frame) continue;
      if (reference.itemType != LAYER)
      {
        frame->regionId = -1;
        frame->crop = frame->size = frame->pivot = {};
      }
      else if (frame->regionId != -1 &&
               (!spritesheet || !child_id_get(*spritesheet, ElementType::REGION, frame->regionId)))
        frame->regionId = -1;
    }

    anm2.uids_repair();
    Uids uids{};
    for (auto index : indices)
      uids.append_range(frames_uids_get(*track, index, 1));
    return uids;
  }

  void frame_root_transform_apply(Element& frame, const Element& rootFrame, bool isRoundScale, bool isRoundRotation,
                                  bool isUseRootPivot)
  {
    auto rootScale = math::percent_to_unit(rootFrame.scale);
    auto pivot = isUseRootPivot ? rootFrame.position : glm::vec2();
    auto offset = (frame.position - pivot) * rootScale;
    auto radians = glm::radians(rootFrame.rotation);
    auto cos = std::cos(radians);
    auto sin = std::sin(radians);

    frame.position = rootFrame.position + glm::vec2(offset.x * cos - offset.y * sin, offset.x * sin + offset.y * cos);
    frame.scale *= rootScale;
    frame.rotation += rootFrame.rotation;
    frame.tint *= rootFrame.tint;
    frame.colorOffset += rootFrame.colorOffset;

    if (isRoundScale) frame.scale = glm::round(frame.scale);
    if (isRoundRotation) frame.rotation = std::round(frame.rotation);
  }

  // Applies the selected root frames' transforms to the target tracks' frames under them, then resets those root
  // frames to a default frame spanning the same time.
  Uids root_bake_into(Anm2& anm2, const std::set<Reference>& rootFrames, const std::set<Reference>& targets,
                      RootBakeOptions options)
  {
    auto byTrack = frames_by_track_get(rootFrames);
    if (byTrack.size() != 1 || targets.empty()) return {};
    auto& [rootReference, rootIndices] = *byTrack.begin();
    auto root = track_get(anm2, rootReference);
    if (!root) return {};

    std::vector<std::pair<int, int>> spans{};
    for (auto index : rootIndices)
      if (auto frame = track_frame_get(*root, index))
      {
        auto start = (int)frame_time_from_index_get(*root, index);
        spans.emplace_back(start, start + frame->duration);
      }
    if (spans.empty()) return {};
    auto is_spanned = [&](int time)
    { return std::ranges::any_of(spans, [&](auto span) { return time >= span.first && time < span.second; }); };

    for (auto target : targets)
    {
      auto track = track_get(anm2, target);
      if (!track) continue;
      auto frameType = track_frame_type_get(*track);

      if (options.isMatchRootInterpolation)
      {
        std::vector<int> bakeIndices{};
        int frameIndex{};
        for (const auto& frame : track->children)
          if (frame.type == frameType)
          {
            if (is_spanned((int)frame_time_from_index_get(*track, frameIndex))) bakeIndices.push_back(frameIndex);
            ++frameIndex;
          }
        for (auto it = bakeIndices.rbegin(); it != bakeIndices.rend(); ++it)
          frame_bake(*track, *it, FRAME_DURATION_MIN, options.isRoundScale, options.isRoundRotation);
      }

      int frameIndex{};
      for (auto& frame : track->children)
      {
        if (frame.type != frameType) continue;
        auto start = (int)frame_time_from_index_get(*track, frameIndex++);
        if (is_spanned(start))
          frame_root_transform_apply(frame, frame_generate(*root, (float)start), options.isRoundScale,
                                     options.isRoundRotation, options.isUseRootPivot);
      }
    }

    Uids uids{};
    std::vector<Element> rewritten{};
    int frameIndex{};
    int defaultDuration{};
    auto default_push = [&]()
    {
      if (defaultDuration <= 0) return;
      auto frame = element_clone(element_make(ElementType::FRAME));
      frame.duration = std::max(defaultDuration, FRAME_DURATION_MIN);
      uids.push_back(frame.uid);
      rewritten.push_back(frame);
      defaultDuration = 0;
    };

    for (const auto& frame : root->children)
    {
      if (frame.type == ElementType::FRAME && rootIndices.contains(frameIndex++))
      {
        defaultDuration += frame.duration;
        continue;
      }
      default_push();
      rewritten.push_back(frame);
    }
    default_push();

    root->children = std::move(rewritten);
    anm2.uids_repair();
    return uids;
  }
}
