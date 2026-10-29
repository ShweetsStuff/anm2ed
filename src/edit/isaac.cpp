#include "edit.hpp"

#include <algorithm>
#include <format>
#include <map>

#include "math.hpp"
#include "model/frames.hpp"

using namespace anm2ed::model;
using namespace anm2ed::util;

namespace anm2ed::edit
{
  // An alpha offset joins the alpha tint: exact on opaque pixels, which the offset can only fade.
  void alpha_offsets_bridge(Model& model)
  {
    auto is_offset = [](const Frame& frame) { return math::float_to_uint8(frame.colorOffset.a) != 0; };
    for (int i = 0; i < model.animations_count_get(); ++i)
    {
      auto isUsed = false;
      animation_tracks_each(*model.animation_get(i),
                            [&](const Track& track) { isUsed |= std::ranges::any_of(track.frames, is_offset); });
      if (!isUsed) continue;
      animation_tracks_each(*model.animation_edit(i),
                            [&](Track& track)
                            {
                              if (track.type == ItemType::TRIGGER) return;
                              for (auto& frame : track.frames)
                              {
                                frame.tint.a = std::clamp(frame.tint.a + frame.colorOffset.a, 0.0f, 1.0f);
                                frame.colorOffset.a = 0.0f;
                              }
                            });
    }
  }

  // A layer frame's depth becomes a copy of its layer, named for the depth, standing where the editor draws it (the
  // layer at the copy's place + depth, past the layer already there), and showing only the frames at that depth; the
  // layer itself shows the rest. Groups can't stand apart in the game, so their roots go into their tracks first.
  void depths_bridge(Model& model)
  {
    auto& content = model.content;
    std::map<std::pair<int, int>, int> copyIds{};
    auto copy_id_get = [&](int layerId, int depth)
    {
      auto [it, isNew] = copyIds.try_emplace({layerId, depth});
      if (!isNew) return it->second;
      auto layer = item_get(content.layers, layerId) ? *item_get(content.layers, layerId) : Layer{};
      layer.id = item_next_id_get(content.layers);
      layer.name = std::format("{} (depth {:+})", layer.name, depth);
      content.layers.push_back(layer);
      return it->second = layer.id;
    };
    auto is_depth = [](const Frame& frame) { return frame.depth != 0; };

    for (int i = 0; i < model.animations_count_get(); ++i)
    {
      auto isUsed = false;
      tracks_each(model.animation_get(i)->layers,
                  [&](const Track& track, auto*) { isUsed |= std::ranges::any_of(track.frames, is_depth); });
      if (!isUsed) continue;
      auto& animation = *model.animation_edit(i);

      std::vector<Track> tracks{};
      for (auto& entry : animation.layers)
        if (auto track = std::get_if<Track>(&entry))
          tracks.push_back(std::move(*track));
        else
        {
          auto& group = std::get<TrackGroup>(entry);
          for (auto& groupTrack : group.tracks)
          {
            track_group_root_bake(groupTrack, group.root, animation.frameNum);
            groupTrack.isVisible = groupTrack.isVisible && group.isVisible;
            tracks.push_back(std::move(groupTrack));
          }
        }

      // Each track's place in drawing order: (its position + depth, depth), kept in position order when equal.
      std::vector<std::pair<std::pair<int, int>, Track>> placed{};
      auto shown_set = [](Track& track, int depth)
      {
        for (auto& frame : track.frames)
        {
          frame.isVisible = frame.isVisible && frame.depth == depth;
          frame.depth = 0;
        }
      };
      for (int position = 0; position < (int)tracks.size(); ++position)
      {
        auto& track = tracks[position];
        std::set<int> depths{};
        for (const auto& frame : track.frames)
          if (frame.depth != 0) depths.insert(frame.depth);
        for (auto depth : depths)
        {
          auto copy = track;
          copy.id = copy_id_get(track.id, depth);
          shown_set(copy, depth);
          placed.push_back({{position + depth, depth}, std::move(copy)});
        }
        shown_set(track, 0);
        placed.push_back({{position, 0}, std::move(track)});
      }
      std::ranges::stable_sort(placed, {}, &std::pair<std::pair<int, int>, Track>::first);
      animation.layers.clear();
      for (auto& [order, track] : placed)
        animation.layers.emplace_back(std::move(track));
    }
  }

  // The game's copy with what the game can't show worked around (Convert and Save).
  Model isaac_bridge(Model model)
  {
    alpha_offsets_bridge(model);
    depths_bridge(model);
    return model;
  }
}
