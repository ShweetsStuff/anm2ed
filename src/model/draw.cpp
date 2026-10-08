#include "draw.hpp"

#include <algorithm>

#include "frames.hpp"
#include "math.hpp"

using namespace anm2ed::util;

namespace anm2ed::model
{
  glm::mat4 draw_quad_model_get(const Draw& draw)
  {
    auto& frame = draw.frame;
    return math::quad_model_get(frame.size, frame.position, frame.pivot, math::percent_to_unit(frame.scale),
                                frame.rotation, math::percent_to_unit(frame.skew));
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
    // Each layer draw's place in the drawing order: its track's position among the layers plus its frame's depth; a
    // moved layer lands past the one already at that place (below it when moved down, above it when moved up).
    std::vector<std::pair<int, int>> layerOrders{};
    int layerOrder{};
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
        layerOrders.emplace_back(layerOrder + frame.depth, frame.depth);
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
        if (type == LAYER) ++layerOrder;
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

    // Layers draw one after another, so reordering them by depth only moves them among each other.
    std::vector<std::size_t> layerIndices{};
    for (std::size_t i = 0; i < draws.size(); ++i)
      if (draws[i].type == DrawType::LAYER) layerIndices.push_back(i);
    auto sorted = layerIndices;
    std::ranges::stable_sort(sorted, {}, [&](std::size_t i) { return layerOrders[i]; });
    std::vector<Draw> layerDraws{};
    for (auto i : sorted)
      layerDraws.push_back(draws[i]);
    for (std::size_t i = 0; i < layerIndices.size(); ++i)
      draws[layerIndices[i]] = std::move(layerDraws[i]);
    return draws;
  }
}
