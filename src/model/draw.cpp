#include "draw.hpp"

#include <algorithm>
#include <cmath>

#include "frames.hpp"
#include "math.hpp"

using namespace anm2ed::util;

namespace anm2ed::model
{
  constexpr unsigned RANDOM_MULTIPLIER_A = 73856093u;
  constexpr unsigned RANDOM_MULTIPLIER_B = 19349663u;

  glm::mat4 draw_quad_model_get(const Draw& draw)
  {
    auto& frame = draw.frame;
    return math::quad_model_get(frame.size, frame.position, frame.pivot, math::percent_to_unit(frame.scale),
                                frame.rotation, math::percent_to_unit(frame.skew));
  }

  // How many canvas pixels one spritesheet pixel of a layer draw covers (along its longer axis).
  float draw_scale_get(const Draw& draw)
  {
    auto model = draw.parent * draw_quad_model_get(draw);
    auto size = glm::max(draw.frame.size, glm::vec2(1.0f));
    return std::max(glm::length(glm::vec2(model[0])) / size.x, glm::length(glm::vec2(model[1])) / size.y);
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

  // A keyframe's start frame in its child animation; a random start is fixed per layer and keyframe.
  float child_start_get(const Frame& frame, int length, int seed, int index)
  {
    if (!frame.isChildStartRandom) return (float)frame.childStart;
    return (float)((((unsigned)seed * RANDOM_MULTIPLIER_A) ^ ((unsigned)index * RANDOM_MULTIPLIER_B)) %
                   (unsigned)length);
  }

  // Where an animation layer's child animation is at `time`. Its clock runs from the last keyframe that restarted it
  // (the first, one set to restart, or one that changes the animation), from that keyframe's start frame; the keyframe
  // playing then loops the child, stops it at its end, or holds its own start frame.
  float child_time_get(const Track& track, float time, int length, int seed)
  {
    float start{};
    float clockStart{};
    float clockOffset{};
    for (int i = 0; i < (int)track.frames.size(); ++i)
    {
      auto& frame = track.frames[i];
      if (i == 0 || frame.isChildRestart || frame.childAnimation != track.frames[i - 1].childAnimation)
      {
        clockStart = start;
        clockOffset = child_start_get(frame, length, seed, i);
      }
      start += frame.duration;
      if (time >= start && i + 1 < (int)track.frames.size()) continue;
      if (frame.childMode == ChildMode::HOLD) return child_start_get(frame, length, seed, i);
      auto childTime = clockOffset + std::max(time, 0.0f) - clockStart;
      if (frame.childMode == ChildMode::ONCE) return std::min(childTime, (float)(length - 1));
      return std::fmod(childTime, (float)length);
    }
    return 0.0f;
  }

  // `playing` holds the animations being drawn into one another, so an animation never draws itself.
  std::vector<Draw> animation_draws_get(const Model& model, const Animation& animation, const DrawOptions& options,
                                        std::vector<const Animation*>& playing)
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
    // An animation layer's child animation, placed by the layer's frame (its pivot is the child's origin).
    auto child_draws_add =
        [&](const Track& track, const Layer& layer, int sample, float time, const Frame& frame, const glm::mat4& parent)
    {
      auto childModel = layer.libraryId == -1 ? &model
                        : options.library_get ? options.library_get(model, layer.libraryId)
                                              : nullptr;
      auto childAnimation =
          childModel
              ? childModel->animation_named_get(frame.childAnimation.empty() ? layer.animation : frame.childAnimation)
              : nullptr;
      if (!childAnimation || childAnimation->frameNum < FRAME_NUM_MIN || std::ranges::contains(playing, childAnimation))
        return;
      playing.push_back(childAnimation);
      auto transform = parent * math::quad_model_get(glm::vec2(1.0f), frame.position, frame.pivot,
                                                     math::percent_to_unit(frame.scale), frame.rotation,
                                                     math::percent_to_unit(frame.skew));
      DrawOptions childOptions{.time = child_time_get(track, time, childAnimation->frameNum, layer.id),
                               .isRootTransform = true,
                               .library_get = options.library_get};
      for (auto child : animation_draws_get(*childModel, *childAnimation, childOptions, playing))
      {
        if (child.type != DrawType::LAYER) continue;
        child.id = layer.id;
        child.sample = sample;
        child.time = time;
        child.parent = transform * child.parent;
        child.frame.tint *= frame.tint;
        child.frame.colorOffset += frame.colorOffset;
        child.frame.blur = std::max(child.frame.blur, frame.blur); // Each part blurs on its own, as the game draws it.
        if (child.source != &model) child.frame.shaderId = -1;
        layerOrders.emplace_back(layerOrder + frame.depth, frame.depth);
        draws.push_back(std::move(child));
      }
      playing.pop_back();
    };
    auto track_draws_add = [&](const Track& track, DrawType type, int id, int groupType, const TrackGroup* group)
    {
      auto layer = type == DrawType::LAYER ? item_get(model.content.layers, id) : nullptr;
      for (auto [sample, time] : draw_times_get(animation, track, options))
      {
        auto frame = frame_generate(track, time);
        if (!frame.isVisible) continue;
        auto parent = parent_get(time, type == DrawType::ROOT ? nullptr : group);
        if (layer)
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
          if (layer->is_animation())
          {
            child_draws_add(track, *layer, sample, time, frame, parent);
            continue;
          }
        }
        layerOrders.emplace_back(layerOrder + frame.depth, frame.depth);
        draws.push_back({type, id, groupType, sample, time, frame, parent, &model, layer ? layer->spritesheetId : -1});
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

  std::vector<Draw> animation_draws_get(const Model& model, const Animation& animation, const DrawOptions& options)
  {
    std::vector<const Animation*> playing{&animation};
    return animation_draws_get(model, animation, options, playing);
  }
}
