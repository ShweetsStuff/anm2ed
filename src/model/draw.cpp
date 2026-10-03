#include "draw.hpp"

#include "frames.hpp"
#include "math.hpp"

using namespace anm2ed::util;

namespace anm2ed::model
{
  glm::mat4 draw_quad_model_get(const Draw& draw)
  {
    auto& frame = draw.frame;
    return math::quad_model_get(frame.size, frame.position, frame.pivot, math::percent_to_unit(frame.scale),
                                frame.rotation, math::percent_to_unit(frame.shear));
  }

  // When an item is drawn: now, and for each sample its time if the track has a frame there.
  std::vector<std::pair<int, float>> draw_times_get(const Animation& animation, const Track& track,
                                                    const DrawOptions& options)
  {
    std::vector<std::pair<int, float>> times{};
    for (int i = 0; i < (int)options.samples.size(); ++i)
    {
      auto& sample = options.samples[i];
      if (!options.isIndexSampled)
      {
        auto time = options.time + sample.timeOffset;
        if (time >= 0.0f && time <= (float)animation.frameNum) times.emplace_back(i, time);
        continue;
      }
      auto index = track.frames.empty() ? -1 : frame_index_from_time_get(track, options.time);
      if (index < 0 || index + sample.indexOffset < 0 || index + sample.indexOffset >= (int)track.frames.size())
        continue;
      times.emplace_back(i, frame_time_from_index_get(track, index + sample.indexOffset));
    }
    times.emplace_back(-1, options.time);
    return times;
  }

  std::vector<Draw> animation_draws_get(const Model& model, const Animation& animation, const DrawOptions& options)
  {
    std::vector<Draw> draws{};
    auto parent_get = [&](float time, const TrackGroup* group)
    {
      glm::mat4 parent(1.0f);
      if (!options.isRootTransform) return parent;
      parent *= frame_parent_model_get(frame_generate(animation.root, time));
      if (group) parent *= frame_parent_model_get(frame_generate(group->root, time));
      return parent;
    };
    auto track_draws_add = [&](const Track& track, DrawType type, int id, int groupType, const TrackGroup* group)
    {
      for (auto [sample, time] : draw_times_get(animation, track, options))
      {
        auto frame = frame_generate(track, time);
        if (!frame.isVisible) continue;
        auto parent = parent_get(time, type == DrawType::ROOT ? nullptr : group);
        if (type == DrawType::LAYER)
        {
          frame = model.frame_effective(id, frame);
          if (options.isRootTransform)
            for (auto* root : {&animation.root, group ? &group->root : nullptr})
              if (root)
              {
                auto rootFrame = frame_generate(*root, time);
                frame.tint *= rootFrame.tint;
                frame.colorOffset += rootFrame.colorOffset;
              }
        }
        draws.push_back({type, id, groupType, sample, time, frame, parent});
      }
    };

    if (animation.root.isVisible) track_draws_add(animation.root, DrawType::ROOT, -1, NONE, nullptr);
    for (auto [type, entries] : {std::pair{LAYER, &animation.layers}, std::pair{NULL_, &animation.nulls}})
    {
      auto drawType = type == LAYER ? DrawType::LAYER : DrawType::NULL_;
      auto track_add = [&](const Track& track, const TrackGroup* group)
      {
        auto isFound = type == LAYER ? item_get(model.content.layers, track.id) != nullptr
                                     : item_get(model.content.nulls, track.id) != nullptr;
        if (track.isVisible && isFound) track_draws_add(track, drawType, track.id, NONE, group);
      };
      for (const auto& entry : *entries)
      {
        if (auto track = std::get_if<Track>(&entry))
        {
          track_add(*track, nullptr);
          continue;
        }
        auto& group = std::get<TrackGroup>(entry);
        if (!group.isVisible) continue;
        if (group.root.isVisible) track_draws_add(group.root, DrawType::GROUP_ROOT, group.id, type, &group);
        for (const auto& groupTrack : group.tracks)
          track_add(groupTrack, &group);
      }
    }
    return draws;
  }
}
