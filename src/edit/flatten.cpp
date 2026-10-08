#include "edit.hpp"

#include <cmath>
#include <map>
#include <tuple>

#include "math.hpp"
#include "model/frames.hpp"

using namespace anm2ed::model;
using namespace anm2ed::util;

namespace anm2ed::edit
{
  // A draw as a plain layer frame of one frame: its transform (parents included) taken apart into position, scale and
  // rotation. Skew has no place in a plain frame, and is dropped.
  Frame draw_frame_bake(const Draw& draw)
  {
    auto frame = draw.frame;
    auto size = glm::max(frame.size, glm::vec2(1.0f));
    auto model = draw.parent * draw_quad_model_get(draw);
    auto column0 = glm::vec2(model[0]) / size.x;
    auto column1 = glm::vec2(model[1]) / size.y;
    auto scaleX = glm::length(column0);
    auto determinant = column0.x * column1.y - column0.y * column1.x;
    frame.position = glm::vec2(model * glm::vec4(frame.pivot / size, 0.0f, 1.0f));
    frame.scale = math::unit_to_percent(glm::vec2(scaleX, scaleX > 0.0f ? determinant / scaleX : 0.0f));
    frame.rotation = glm::degrees(std::atan2(column0.y, column0.x));
    frame.skew = {};
    frame.depth = 0;
    frame.regionId = -1;
    frame.duration = FRAME_DURATION_MIN;
    frame.interpolation = Interpolation::NONE;
    frame.isVisible = true;
    return frame;
  }

  bool is_frame_baked_equal(Frame a, const Frame& b)
  {
    a.uid = b.uid;
    a.duration = b.duration;
    return a == b;
  }

  // The game's copy of a document: each animation layer's track becomes plain layer tracks (one per layer its child
  // animation draws), baked a frame at a time. A library's spritesheets join the copy, at paths from `path_get`.
  Model animation_layers_flatten(Model model, const LibraryGet& library_get, const SpritesheetPathGet& path_get)
  {
    auto& content = model.content;
    if (std::ranges::none_of(content.layers, &Layer::is_animation)) return model;

    std::map<std::pair<const Model*, int>, int> spritesheetIds{};
    auto spritesheet_id_get = [&](const Model* source, int id)
    {
      if (source == &model) return id;
      auto [it, isNew] = spritesheetIds.try_emplace({source, id});
      if (!isNew) return it->second;
      auto spritesheet = item_get(source->content.spritesheets, id);
      it->second = item_next_id_get(content.spritesheets);
      content.spritesheets.push_back(
          {.id = it->second, .path = spritesheet ? path_get(*source, spritesheet->path) : std::filesystem::path{}});
      return it->second;
    };

    // (animation layer, source, source layer) -> the plain layer standing in for it.
    std::map<std::tuple<int, const Model*, int>, int> layerIds{};
    auto layer_id_get = [&](const Draw& draw)
    {
      auto [it, isNew] = layerIds.try_emplace({draw.id, draw.source, draw.sourceId});
      if (!isNew) return it->second;
      auto parent = item_get(content.layers, draw.id);
      auto sourceLayer = item_get(draw.source->content.layers, draw.sourceId);
      it->second = item_next_id_get(content.layers);
      content.layers.push_back({.id = it->second,
                                .name = parent->name + "/" + (sourceLayer ? sourceLayer->name : std::string{}),
                                .spritesheetId = spritesheet_id_get(draw.source, draw.spritesheetId)});
      return it->second;
    };

    for (int index = 0; index < model.animations_count_get(); ++index)
    {
      // Drawn as stored, so an animation layer playing its own animation is still caught.
      auto& source = *model.animation_get(index);
      auto tracks_flatten = [&](const Track& track)
      {
        auto layer = item_get(content.layers, track.id);
        if (!layer || !layer->is_animation()) return std::vector{track};
        std::map<int, std::vector<Frame>> frames{};
        std::vector<int> order{};
        for (int time = 0; time < source.frameNum; ++time)
          for (const auto& draw : animation_draws_get(model, source, {.time = (float)time, .library_get = library_get}))
          {
            if (draw.type != DrawType::LAYER || draw.id != track.id || draw.frame.size == glm::vec2()) continue;
            auto id = layer_id_get(draw);
            auto [it, isNew] = frames.try_emplace(id, source.frameNum, Frame{.isVisible = false});
            if (isNew) order.push_back(id);
            it->second[time] = draw_frame_bake(draw);
          }
        std::vector<Track> tracks{};
        for (auto id : order)
        {
          auto& baked = tracks.emplace_back(Track{.type = ItemType::LAYER, .id = id, .isVisible = track.isVisible});
          for (auto& frame : frames[id])
            if (!baked.frames.empty() && is_frame_baked_equal(frame, baked.frames.back()))
              ++baked.frames.back().duration;
            else
              baked.frames.push_back(frame);
        }
        return tracks;
      };

      std::vector<TrackEntry> entries{};
      for (auto& entry : source.layers)
        if (auto track = std::get_if<Track>(&entry))
          for (auto& flat : tracks_flatten(*track))
            entries.emplace_back(std::move(flat));
        else
        {
          auto group = std::get<TrackGroup>(entry);
          group.tracks.clear();
          for (auto& groupTrack : std::get<TrackGroup>(entry).tracks)
            for (auto& flat : tracks_flatten(groupTrack))
              group.tracks.push_back(std::move(flat));
          entries.emplace_back(std::move(group));
        }
      // The children's triggers join the animation's own, on the frames they fire (their events found by name).
      auto triggers = source.triggers;
      for (int time = 0; time < source.frameNum; ++time)
      {
        auto fires = animation_triggers_get(model, source, time, library_get);
        auto isOwn = source.triggers.isVisible && frame_generate(source.triggers, (float)time).isVisible;
        std::vector<int> eventIds{};
        for (int i = isOwn ? 1 : 0; i < (int)fires.size(); ++i)
          for (auto eventId : fires[i].trigger.eventIds)
            if (auto event = item_get(fires[i].source->content.events, eventId))
            {
              auto existing = std::ranges::find(content.events, event->name, &Event::name);
              eventIds.push_back(existing != content.events.end()
                                     ? existing->id
                                     : content.events.emplace_back(Event{.id = item_next_id_get(content.events),
                                                                         .name = event->name})
                                           .id);
            }
        if (eventIds.empty()) continue;
        auto trigger = std::ranges::find(triggers.frames, time, &Frame::atFrame);
        if (trigger == triggers.frames.end())
          trigger = triggers.frames.insert(triggers.frames.end(), Frame{.atFrame = time});
        trigger->eventIds.insert(trigger->eventIds.end(), eventIds.begin(), eventIds.end());
      }
      frames_sort_by_at_frame(triggers);

      auto flattened = model.animation_edit(index);
      flattened->layers = std::move(entries);
      flattened->triggers = std::move(triggers);
    }
    std::erase_if(content.layers, [](const Layer& layer) { return layer.is_animation(); });
    return model;
  }
}
