#include "edit.hpp"

#include <format>
#include <unordered_map>

#include "model/frames.hpp"
#include "model/xml.hpp"

using namespace anm2ed::model;

namespace anm2ed::edit
{
  std::string region_name_get(const std::string& format, int number)
  {
    try
    {
      return std::vformat(format, std::make_format_args(number));
    }
    catch (const std::format_error&)
    {
      return format;
    }
  }

  bool is_region_matched(const Region& region, const Frame& frame)
  {
    return glm::ivec2(region.crop) == glm::ivec2(frame.crop) && glm::ivec2(region.size) == glm::ivec2(frame.size) &&
           glm::ivec2(region.pivot) == glm::ivec2(frame.pivot);
  }

  // Every frame of every layer track whose layer has a spritesheet, with that spritesheet's id.
  template <class Callback> void layer_frames_each(Model& model, int animationIndex, Callback&& callback)
  {
    auto animation = model.animation_get(animationIndex);
    if (!animation) return;
    std::vector<std::pair<int, int>> tracks{};
    tracks_each(animation->layers,
                [&](const Track& track, const TrackGroup* group)
                {
                  if (model.layer_spritesheet_get(track.id)) tracks.emplace_back(track.id, group ? group->id : -1);
                });
    for (auto [id, groupId] : tracks)
    {
      auto spritesheetId = model.layer_spritesheet_get(id)->id;
      auto track = model.track_edit({animationIndex, LAYER, id, -1, groupId == -1 ? NONE : LAYER, groupId});
      for (auto& frame : track->frames)
        callback(frame, spritesheetId);
    }
  }

  // Makes a region for each distinct crop/size/pivot among the frames (and points frames at them when mapping is SET).
  Uids regions_generate(Model& model, const std::set<int>& animationIndices, const std::set<Reference>& frames,
                        const std::string& format, RegionFrameMapping mapping)
  {
    std::unordered_map<int, int> regionNumbers{};
    auto frame_apply = [&](Frame& frame, int spritesheetId)
    {
      auto& regions = item_get(model.content.spritesheets, spritesheetId)->regions;
      auto region =
          std::ranges::find_if(regions, [&](const Region& region) { return is_region_matched(region, frame); });
      if (region == regions.end())
      {
        regions.push_back({.id = item_next_id_get(regions),
                           .name = region_name_get(format, ++regionNumbers[spritesheetId]),
                           .crop = frame.crop,
                           .size = frame.size,
                           .pivot = frame.pivot});
        region = regions.end() - 1;
      }
      if (mapping == RegionFrameMapping::SET) frame.regionId = region->id;
    };

    for (auto index : animationIndices)
      layer_frames_each(model, index, frame_apply);

    for (auto reference : frames)
    {
      auto spritesheet = reference.itemType == LAYER ? model.layer_spritesheet_get(reference.itemID) : nullptr;
      auto source = model.frame_get(reference);
      if (spritesheet && source && source->regionId == -1) frame_apply(*model.frame_edit(reference), spritesheet->id);
    }
    return {};
  }

  // Points frames without a region at a region of their spritesheet with the same crop/size/pivot.
  Uids regions_scan(Model& model)
  {
    for (int i = 0; i < model.animations_count_get(); ++i)
      layer_frames_each(model, i,
                        [&](Frame& frame, int spritesheetId)
                        {
                          if (frame.regionId != -1) return;
                          for (const auto& region : item_get(model.content.spritesheets, spritesheetId)->regions)
                            if (is_region_matched(region, frame))
                            {
                              frame.regionId = region.id;
                              break;
                            }
                        });
    return {};
  }

  // Copies each layer frame's region crop/size/pivot onto it; frames pointing at a missing region lose it.
  Uids region_frames_sync(Model& model)
  {
    for (int i = 0; i < model.animations_count_get(); ++i)
      layer_frames_each(model, i,
                        [&](Frame& frame, int spritesheetId)
                        {
                          if (frame.regionId == -1) return;
                          auto region =
                              item_get(item_get(model.content.spritesheets, spritesheetId)->regions, frame.regionId);
                          if (!region)
                          {
                            frame.regionId = -1;
                            return;
                          }
                          frame.crop = region->crop;
                          frame.size = region->size;
                          frame.pivot = region->pivot;
                        });
    return {};
  }

  // Clears frame references to the spritesheet's unused regions, then removes them.
  Uids regions_remove_unused(Model& model, int spritesheetId)
  {
    auto unused = model.region_unused_get(spritesheetId);
    auto spritesheet = item_get(model.content.spritesheets, spritesheetId);
    if (unused.empty() || !spritesheet) return {};
    std::erase_if(spritesheet->regions, [&](const Region& region) { return unused.contains(region.id); });
    return {};
  }

  // Pastes regions into a spritesheet at a position; they get fresh ids after the existing ones.
  Uids regions_paste(Model& model, int spritesheetId, const std::string& text, int insertIndex,
                     std::string* errorString)
  {
    auto spritesheet = item_get(model.content.spritesheets, spritesheetId);
    if (!spritesheet) return {};
    auto regions = items_from_string<Region>(text, errorString);
    for (auto& region : regions)
      region.id = item_next_id_get(spritesheet->regions) + (int)(&region - regions.data());
    insertIndex = std::clamp(insertIndex, 0, (int)spritesheet->regions.size());
    spritesheet->regions.insert(spritesheet->regions.begin() + insertIndex, regions.begin(), regions.end());
    return {};
  }

  // Appends grid frames to each layer track (optionally making a region per frame) and fits the animation length.
  Uids animation_grid_generate(Model& model, const std::vector<Reference>& tracks, const GridOptions& options)
  {
    Uids uids{};
    for (auto reference : tracks)
    {
      if (!model.track_get(reference)) continue;
      auto spritesheetId =
          model.layer_spritesheet_get(reference.itemID) ? model.layer_spritesheet_get(reference.itemID)->id : -1;
      auto& track = *model.track_edit(reference);
      auto start = (int)track.frames.size();
      frames_generate_from_grid(track, options.start, options.size, options.pivot, options.columns, options.count,
                                options.delay);
      auto spritesheet = item_get(model.content.spritesheets, spritesheetId);
      for (int i = start; i < (int)track.frames.size(); ++i)
      {
        auto& frame = track.frames[i];
        uids.push_back(frame.uid);
        if (!options.isMakeRegions || !spritesheet) continue;
        auto& region =
            spritesheet->regions.emplace_back(Region{.id = item_next_id_get(spritesheet->regions),
                                                     .name = region_name_get(options.regionNameFormat, i - start),
                                                     .crop = frame.crop,
                                                     .size = frame.size,
                                                     .pivot = frame.pivot});
        frame.regionId = region.id;
      }
      auto& animation = *model.animation_edit(reference.animationIndex);
      animation.frameNum = animation_length_get(animation);
    }
    return uids;
  }

  // Applies a frame change to explicit frames, to whole tracks (frameIndex -1), and to every matching track of the
  // given animations.
  Uids frames_change_apply(Model& model, const ChangeTargets& targets, FrameChange change, ChangeType type)
  {
    std::map<Reference, std::set<int>> frames{};
    std::set<Reference> wholeTracks{};
    for (auto reference : targets.references)
    {
      auto index = std::exchange(reference.frameIndex, -1);
      index < 0 ? (void)wholeTracks.insert(reference) : (void)frames[reference].insert(index);
    }

    auto whole_apply = [&](Track& track)
    {
      std::set<int> indices{};
      for (int i = 0; i < (int)track.frames.size(); ++i)
        indices.insert(i);
      frames_change(track, change, track.type, type, indices);
    };

    for (auto& [reference, indices] : frames)
      if (auto track = model.track_edit(reference)) frames_change(*track, change, track->type, type, indices);
    for (auto reference : wholeTracks)
      if (auto track = model.track_edit(reference)) whole_apply(*track);
    for (auto index : targets.animations)
    {
      auto animation = model.animation_edit(index);
      if (!animation) continue;
      if (targets.isRoot) whole_apply(animation->root);
      if (targets.isLayers) tracks_each(animation->layers, [&](Track& track, auto*) { whole_apply(track); });
      if (targets.isNulls) tracks_each(animation->nulls, [&](Track& track, auto*) { whole_apply(track); });
    }
    return {};
  }

  // Merges another anm2 file: content matched by name (or path for sounds) is replaced unless appending as new,
  // animations are added, replaced, or merged track by track depending on the preset.
  bool file_merge(Model& model, const std::filesystem::path& path, const std::filesystem::path& directory,
                  FileMergePreset preset)
  {
    Model source{};
    if (!model_load(source, path) || !source.isValid) return false;

    auto isAppendAsNew = preset == FILE_MERGE_PRESET_APPEND_AS_NEW;
    std::map<ElementType, std::unordered_map<int, int>> remaps{};
    auto& content = model.content;

    auto path_remap = [&](const std::filesystem::path& original)
    {
      if (directory.empty() || original.empty()) return original;
      std::error_code ec{};
      auto absolute =
          original.is_absolute() ? original : std::filesystem::weakly_canonical(path.parent_path() / original, ec);
      if (ec) absolute = path.parent_path() / original;
      auto relative = std::filesystem::relative(absolute, directory, ec);
      return ec ? original : relative;
    };

    auto id_remap = [&](ElementType type, int id)
    {
      auto it = remaps[type].find(id);
      return it == remaps[type].end() ? -1 : it->second;
    };

    auto name_find = [](auto& items, const std::string& name)
    {
      auto it = std::ranges::find_if(items, [&](const auto& item) { return item.name == name; });
      return it == items.end() ? nullptr : &*it;
    };

    auto name_unique_get = [&](auto& items, const std::string& name)
    {
      auto candidate = name;
      for (int i = 2; name_find(items, candidate); ++i)
        candidate = std::format("{} {}", name, i);
      return candidate;
    };

    auto named_merge = [&](auto& items, auto item)
    {
      auto existing = isAppendAsNew ? nullptr : name_find(items, item.name);
      if (existing)
      {
        item.id = existing->id;
        item.uid = existing->uid;
        *existing = item;
        return item.id;
      }
      item.id = item_next_id_get(items);
      if (isAppendAsNew) item.name = name_unique_get(items, item.name);
      items.push_back(item);
      return item.id;
    };

    auto spritesheet_import = [&](int sourceId)
    {
      auto& remap = remaps[ElementType::SPRITESHEET];
      if (remap.contains(sourceId)) return remap[sourceId];
      auto spritesheet = item_get(source.content.spritesheets, sourceId);
      if (sourceId < 0 || !spritesheet) return -1;
      auto imported = *spritesheet;
      imported.id = item_next_id_get(content.spritesheets);
      imported.path = path_remap(imported.path);
      content.spritesheets.push_back(imported);
      return remap[sourceId] = imported.id;
    };

    for (auto sound : source.content.sounds)
    {
      auto sourceId = sound.id;
      sound.path = path_remap(sound.path);
      auto existing = isAppendAsNew ? content.sounds.end()
                                    : std::ranges::find_if(content.sounds, [&](const Sound& other)
                                                           { return other.path == sound.path; });
      sound.id = existing != content.sounds.end() ? existing->id : item_next_id_get(content.sounds);
      if (existing != content.sounds.end())
        *existing = sound;
      else
        content.sounds.push_back(sound);
      remaps[ElementType::SOUND_ELEMENT][sourceId] = sound.id;
    }

    for (auto layer : source.content.layers)
    {
      auto existing = isAppendAsNew ? nullptr : name_find(content.layers, layer.name);
      auto sourceId = layer.id;
      layer.spritesheetId = existing ? existing->spritesheetId : spritesheet_import(layer.spritesheetId);
      remaps[ElementType::LAYER_ELEMENT][sourceId] = named_merge(content.layers, layer);
    }
    for (auto null : source.content.nulls)
      remaps[ElementType::NULL_ELEMENT][null.id] = named_merge(content.nulls, null);
    for (auto event : source.content.events)
      remaps[ElementType::EVENT_ELEMENT][event.id] = named_merge(content.events, event);

    auto track_remap = [&](Track& track)
    {
      for (auto& frame : track.frames)
      {
        for (auto& soundId : frame.soundIds)
          soundId = id_remap(ElementType::SOUND_ELEMENT, soundId);
        frame.eventId = id_remap(ElementType::EVENT_ELEMENT, frame.eventId);
      }
    };

    auto entries_remap = [&](std::vector<TrackEntry>& entries, ElementType type)
    {
      std::vector<TrackEntry> result{};
      auto track_keep = [&](Track& track)
      {
        track.id = id_remap(type, track.id);
        if (track.id < 0) return false;
        track_remap(track);
        return true;
      };
      for (auto& entry : entries)
        if (auto track = std::get_if<Track>(&entry))
        {
          if (track_keep(*track)) result.push_back(std::move(entry));
        }
        else
        {
          auto& group = std::get<TrackGroup>(entry);
          std::erase_if(group.tracks, [&](Track& groupTrack) { return !track_keep(groupTrack); });
          result.push_back(std::move(entry));
        }
      entries = std::move(result);
    };

    auto animation_build = [&](const Animation& incoming)
    {
      auto animation = animation_clone(incoming);
      animation.extras = animation.layersExtras = animation.nullsExtras = {};
      track_remap(animation.root);
      track_remap(animation.triggers);
      entries_remap(animation.layers, ElementType::LAYER_ELEMENT);
      entries_remap(animation.nulls, ElementType::NULL_ELEMENT);
      return animation;
    };

    auto animation_find = [&](const std::string& name) -> int
    {
      for (int i = 0; i < model.animations_count_get(); ++i)
        if (model.animation_get(i)->name == name) return i;
      return -1;
    };

    std::string defaultAnimationName{};
    for (auto [index, incoming] : source.animations_get())
    {
      auto processed = animation_build(*incoming);
      auto destination = animation_find(processed.name);
      if (isAppendAsNew)
      {
        auto candidate = processed.name;
        for (int i = 2; animation_find(candidate) != -1; ++i)
          candidate = std::format("{} {}", processed.name, i);
        processed.name = candidate;
      }
      if (incoming->name == source.animations.defaultAnimation) defaultAnimationName = processed.name;

      if (destination == -1 || isAppendAsNew)
      {
        model.animations.entries.emplace_back(std::make_shared<const Animation>(std::move(processed)));
        continue;
      }

      auto& target = *model.animation_edit(destination);
      if (preset == FILE_MERGE_PRESET_REPLACE_MATCHING)
      {
        processed.uid = target.uid;
        target = std::move(processed);
        continue;
      }

      target.isLoop = processed.isLoop;
      if (!processed.root.frames.empty()) target.root = processed.root;
      if (!processed.triggers.frames.empty()) target.triggers = processed.triggers;

      for (auto [targetEntries, sourceEntries] :
           {std::pair{&target.layers, &processed.layers}, std::pair{&target.nulls, &processed.nulls}})
      {
        auto nextGroupId = 0;
        groups_each(*targetEntries,
                    [&](const TrackGroup& group) { nextGroupId = std::max(nextGroupId, group.id + 1); });
        for (auto& entry : *sourceEntries)
        {
          if (auto group = std::get_if<TrackGroup>(&entry))
          {
            auto copy = *group;
            copy.id = nextGroupId++;
            std::erase_if(copy.tracks,
                          [&](const Track& track)
                          {
                            auto existing = animation_track_get(target, (int)track.type, track.id);
                            if (existing && !track.frames.empty()) *existing = track;
                            return existing != nullptr;
                          });
            targetEntries->push_back(std::move(copy));
            continue;
          }
          auto& track = std::get<Track>(entry);
          if (auto existing = animation_track_get(target, (int)track.type, track.id))
          {
            if (!track.frames.empty()) *existing = track;
          }
          else
            targetEntries->push_back(track);
        }
      }

      target.frameNum = std::max({target.frameNum, processed.frameNum, animation_length_get(target)});
    }

    if (model.animations.defaultAnimation.empty() && !source.animations.defaultAnimation.empty())
      model.animations.defaultAnimation =
          defaultAnimationName.empty() ? source.animations.defaultAnimation : defaultAnimationName;
    return true;
  }
}
