#include "edit.hpp"

#include <algorithm>
#include <utility>

#include "math.hpp"
#include "model/frames.hpp"
#include "model/xml.hpp"

using namespace anm2ed::util;
using namespace anm2ed::model;

namespace anm2ed::edit
{
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

  Uids frames_uids_get(const Track& track, int start, int count)
  {
    Uids uids{};
    for (int i = std::max(start, 0); i < std::min(start + count, (int)track.frames.size()); ++i)
      uids.push_back(track.frames[i].uid);
    return uids;
  }

  Uids frame_insert(Model& model, Reference reference, int time)
  {
    auto track = model.track_edit(reference);
    if (!track) return {};

    if (track->type == ItemType::TRIGGER)
    {
      if (std::ranges::any_of(track->frames, [&](const Frame& trigger) { return trigger.atFrame == time; })) return {};
      Frame trigger{};
      trigger.atFrame = time;
      track->frames.push_back(trigger);
      frames_sort_by_at_frame(*track);
      return {trigger.uid};
    }

    auto count = (int)track->frames.size();
    auto isSource = reference.frameIndex >= 0 && reference.frameIndex < count;
    auto frame = isSource ? frame_clone(track->frames[reference.frameIndex])
                 : count  ? frame_clone(track->frames.back())
                          : Frame{};
    auto index = isSource ? reference.frameIndex + 1 : count;
    track->frames.insert(track->frames.begin() + index, frame);
    return {frame.uid};
  }

  Uids frames_delete(Model& model, const std::set<Reference>& frames)
  {
    for (auto& [trackReference, indices] : frames_by_track_get(frames))
      if (auto track = model.track_edit(trackReference))
        for (auto it = indices.rbegin(); it != indices.rend(); ++it)
          if (*it >= 0 && *it < (int)track->frames.size()) track->frames.erase(track->frames.begin() + *it);
    return {};
  }

  Uids frames_duplicate(Model& model, const std::set<Reference>& frames, Reference focus)
  {
    Uids uids{};
    std::uint64_t focusUid{};
    for (auto& [trackReference, indices] : frames_by_track_get(frames))
    {
      auto track = model.track_edit(trackReference);
      if (!track) continue;

      std::vector<Frame> copies{};
      int lastIndex{-1};
      for (auto index : indices)
      {
        if (index < 0 || index >= (int)track->frames.size()) continue;
        copies.push_back(frame_clone(track->frames[index]));
        lastIndex = index;
        auto focusTrack = focus;
        focusTrack.frameIndex = -1;
        if (focusTrack == trackReference && index == focus.frameIndex) focusUid = copies.back().uid;
      }
      if (copies.empty()) continue;

      for (const auto& copy : copies)
        uids.push_back(copy.uid);
      track->frames.insert(track->frames.begin() + lastIndex + 1, copies.begin(), copies.end());
    }

    if (auto it = std::ranges::find(uids, focusUid); focusUid && it != uids.end())
      std::rotate(uids.begin(), it, it + 1);
    return uids;
  }

  // Reverses the selected frames of each track as one block placed where the first of them was.
  Uids frames_reverse(Model& model, const std::set<Reference>& frames)
  {
    for (auto& [trackReference, indices] : frames_by_track_get(frames))
    {
      auto track = model.track_edit(trackReference);
      if (!track || indices.size() < 2) continue;

      std::vector<Frame> reversed{};
      for (auto it = indices.rbegin(); it != indices.rend(); ++it)
        if (*it >= 0 && *it < (int)track->frames.size())
        {
          reversed.push_back(std::move(track->frames[*it]));
          track->frames.erase(track->frames.begin() + *it);
        }
      auto index = std::min(*indices.begin(), (int)track->frames.size());
      track->frames.insert(track->frames.begin() + index, reversed.begin(), reversed.end());
    }
    return {};
  }

  Uids frames_bake(Model& model, const std::set<Reference>& frames, int interval, bool isRoundScale,
                   bool isRoundRotation)
  {
    Uids uids{};
    for (auto& [trackReference, indices] : frames_by_track_get(frames))
    {
      auto track = model.track_edit(trackReference);
      if (!track) continue;

      int inserted{};
      for (auto originalIndex : indices)
      {
        auto index = originalIndex + inserted;
        if (index < 0 || index >= (int)track->frames.size()) continue;
        auto duration = track->frames[index].duration;
        auto count = duration <= FRAME_DURATION_MIN ? 1 : (int)std::ceil((float)duration / interval);
        frame_bake(*track, index, interval, isRoundScale, isRoundRotation);
        uids.append_range(frames_uids_get(*track, index, count));
        inserted += count - 1;
      }
    }
    return uids;
  }

  Uids frame_split(Model& model, Reference reference, int time)
  {
    auto source = model.frame_get(reference);
    if (!source || reference.itemType == TRIGGER || source->duration <= 1) return {};
    auto duration = source->duration;
    auto firstDuration =
        time - (int)std::round(frame_time_from_index_get(*model.track_get(reference), reference.frameIndex)) + 1;
    if (firstDuration <= 0 || firstDuration >= duration) return {};

    auto& track = *model.track_edit(reference);
    auto& frame = track.frames[reference.frameIndex];
    auto split = frame_clone(frame);
    split.duration = duration - firstDuration;
    if (reference.frameIndex + 1 < (int)track.frames.size() && frame.interpolation != Interpolation::NONE)
      frame_mix(split, track.frames[reference.frameIndex + 1],
                interpolation_factor(frame.interpolation, (float)firstDuration / (float)duration));

    frame.duration = firstDuration;
    auto uid = frame.uid;
    track.frames.insert(track.frames.begin() + reference.frameIndex + 1, split);
    return {uid};
  }

  // Moves frames (from any tracks) into one track before the frame at the given index.
  Uids frames_move(Model& model, const std::set<Reference>& frames, Reference target, int insertIndex)
  {
    target.frameIndex = -1;
    auto targetTrack = model.track_get(target);
    if (!targetTrack) return {};
    auto targetUid = targetTrack->uid;
    insertIndex = std::clamp(insertIndex, 0, (int)targetTrack->frames.size());

    std::vector<Frame> moved{};
    for (auto& [trackReference, indices] : frames_by_track_get(frames))
    {
      auto track = model.track_edit(trackReference);
      if (!track) continue;
      std::vector<Frame> trackMoved{};
      for (auto it = indices.rbegin(); it != indices.rend(); ++it)
      {
        if (*it < 0 || *it >= (int)track->frames.size()) continue;
        trackMoved.insert(trackMoved.begin(), std::move(track->frames[*it]));
        track->frames.erase(track->frames.begin() + *it);
        if (track->uid == targetUid && *it < insertIndex) --insertIndex;
      }
      moved.append_range(trackMoved);
    }
    if (moved.empty()) return {};

    auto& destination = *model.track_edit(target);
    insertIndex = std::clamp(insertIndex, 0, (int)destination.frames.size());
    destination.frames.insert(destination.frames.begin() + insertIndex, moved.begin(), moved.end());
    return frames_uids_get(destination, insertIndex, (int)moved.size());
  }

  Uids frame_durations_set(Model& model, const std::map<Reference, int>& durations)
  {
    for (auto [reference, duration] : durations)
      if (auto frame = model.frame_edit(reference))
        frame->duration = std::clamp(duration, FRAME_DURATION_MIN, FRAME_DURATION_MAX);
    return {};
  }

  // Places a trigger at a frame, stepping it back past any other trigger already there.
  Uids trigger_at_frame_set(Model& model, Reference reference, int atFrame)
  {
    auto trigger = model.frame_edit(reference);
    if (!trigger) return {};
    trigger->atFrame = atFrame;
    for (const auto& other : model.track_get(reference)->frames)
      if (&other != trigger && other.atFrame == trigger->atFrame) trigger->atFrame--;
    return {};
  }

  Uids triggers_sort(Model& model, Reference reference)
  {
    if (auto track = model.track_edit(reference)) frames_sort_by_at_frame(*track);
    return {};
  }

  // Pastes clipboard frames after the track's selected frames (or its current frame); triggers go at `time`, stepping
  // past occupied frames. Frames lose regions that do not exist on the target layer.
  Uids frames_paste(Model& model, Reference reference, const std::set<Reference>& selection, const std::string& text,
                    int time, std::string* errorString)
  {
    auto source = model.track_get(reference);
    if (!source) return {};
    auto frames = frames_from_string(text, source->type, errorString);
    if (frames.empty()) return {};

    auto trackReference = reference;
    trackReference.frameIndex = -1;
    auto selected = frames_by_track_get(selection);
    auto count = (int)source->frames.size();
    auto insertIndex = count;
    if (auto it = selected.find(trackReference); it != selected.end() && !it->second.empty())
      insertIndex = std::min(count, *it->second.rbegin() + 1);
    else if (reference.frameIndex >= 0 && reference.frameIndex < count)
      insertIndex = reference.frameIndex + 1;

    auto spritesheet = reference.itemType == LAYER ? model.layer_spritesheet_get(reference.itemID) : nullptr;
    for (auto& frame : frames)
      if (reference.itemType != LAYER && reference.itemType != TRIGGER)
      {
        frame.regionId = -1;
        frame.crop = frame.size = frame.pivot = {};
      }
      else if (frame.regionId != -1 && (!spritesheet || !item_get(spritesheet->regions, frame.regionId)))
        frame.regionId = -1;

    auto& track = *model.track_edit(reference);
    Uids uids{};
    if (track.type != ItemType::TRIGGER)
    {
      track.frames.insert(track.frames.begin() + insertIndex, frames.begin(), frames.end());
      return frames_uids_get(track, insertIndex, (int)frames.size());
    }

    for (int i = 0; i < (int)frames.size(); ++i)
    {
      auto& trigger = frames[i];
      trigger.atFrame = time + i;
      while (frame_index_from_at_frame_get(track, trigger.atFrame) != -1)
        ++trigger.atFrame;
      uids.push_back(trigger.uid);
      track.frames.push_back(trigger);
    }
    frames_sort_by_at_frame(track);
    return uids;
  }

  // Applies the selected root frames' transforms to the target tracks' frames under them, then resets those root
  // frames to a default frame spanning the same time.
  Uids root_bake_into(Model& model, const std::set<Reference>& rootFrames, const std::set<Reference>& targets,
                      RootBakeOptions options)
  {
    auto byTrack = frames_by_track_get(rootFrames);
    if (byTrack.size() != 1 || targets.empty()) return {};
    auto& [rootReference, rootIndices] = *byTrack.begin();
    auto rootTrack = model.track_get(rootReference);
    if (!rootTrack) return {};
    auto root = *rootTrack;

    std::vector<std::pair<int, int>> spans{};
    for (auto index : rootIndices)
      if (index >= 0 && index < (int)root.frames.size())
      {
        auto start = (int)frame_time_from_index_get(root, index);
        spans.emplace_back(start, start + root.frames[index].duration);
      }
    if (spans.empty()) return {};
    auto is_spanned = [&](int time)
    { return std::ranges::any_of(spans, [&](auto span) { return time >= span.first && time < span.second; }); };

    for (auto target : targets)
    {
      auto track = model.track_edit(target);
      if (!track) continue;

      if (options.isMatchRootInterpolation)
        for (int index = (int)track->frames.size() - 1; index >= 0; --index)
          if (is_spanned((int)frame_time_from_index_get(*track, index)))
            frame_bake(*track, index, FRAME_DURATION_MIN, options.isRoundScale, options.isRoundRotation);

      for (int index = 0; index < (int)track->frames.size(); ++index)
      {
        auto start = (int)frame_time_from_index_get(*track, index);
        if (is_spanned(start))
          frame_root_transform_apply(track->frames[index], frame_generate(root, (float)start), options.isRoundScale,
                                     options.isRoundRotation, options.isUseRootPivot);
      }
    }

    Uids uids{};
    std::vector<Frame> rewritten{};
    int defaultDuration{};
    auto default_push = [&]()
    {
      if (defaultDuration <= 0) return;
      Frame frame{};
      frame.duration = std::max(defaultDuration, FRAME_DURATION_MIN);
      uids.push_back(frame.uid);
      rewritten.push_back(frame);
      defaultDuration = 0;
    };

    for (int index = 0; index < (int)root.frames.size(); ++index)
    {
      if (rootIndices.contains(index))
      {
        defaultDuration += root.frames[index].duration;
        continue;
      }
      default_push();
      rewritten.push_back(root.frames[index]);
    }
    default_push();

    model.track_edit(rootReference)->frames = std::move(rewritten);
    return uids;
  }
}
