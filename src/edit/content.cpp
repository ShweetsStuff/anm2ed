#include "edit.hpp"

#include "vector.hpp"

namespace anm2ed
{
  std::string region_name_get(const std::string&, int);
}

namespace anm2ed::edit
{
  // Clears frame references to the spritesheet's unused regions, then removes them.
  Uids regions_remove_unused(Anm2& anm2, int spritesheetId)
  {
    auto unused = anm2.region_unused(spritesheetId);
    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, spritesheetId);
    if (unused.empty() || !spritesheet) return {};

    animations_tracks_each(anm2.root, ElementType::LAYER_ANIMATION,
                           [&](Element& track)
                           {
                             auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, track.layerId);
                             if (!layer || layer->spritesheetId != spritesheetId) return;
                             for (auto& frame : track.children)
                               if (frame.type == ElementType::FRAME && unused.contains(frame.regionId))
                                 frame.regionId = -1;
                           });
    for (auto id : unused)
      element_child_id_erase(*spritesheet, ElementType::REGION, id);
    return {};
  }

  // Pastes regions into a spritesheet at a child position; they get fresh ids after the existing ones.
  Uids regions_paste(Anm2& anm2, int spritesheetId, const std::string& text, int insertIndex, std::string* errorString)
  {
    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, spritesheetId);
    if (!spritesheet) return {};
    auto maxId = element_child_max_id_get(*spritesheet, ElementType::REGION);
    if (!anm2.deserialize(ElementType::REGION, text, true, errorString, {}, spritesheetId)) return {};

    spritesheet = anm2.element_get(ElementType::SPRITESHEET, spritesheetId);
    std::vector<int> pasted{};
    for (int i = 0; i < (int)spritesheet->children.size(); i++)
      if (spritesheet->children[i].type == ElementType::REGION && spritesheet->children[i].id > maxId)
        pasted.push_back(i);
    util::vector::move_indices_to_position(spritesheet->children, pasted, insertIndex);
    return {};
  }

  // Appends grid frames to each layer track (optionally making a region per frame) and fits the animation length.
  Uids animation_grid_generate(Anm2& anm2, const std::vector<Reference>& tracks, const GridOptions& options)
  {
    Uids uids{};
    for (auto reference : tracks)
    {
      auto track = anm2.element_get(reference);
      auto animation = anm2.element_get(ElementType::ANIMATION, reference.animationIndex);
      auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, reference.itemID);
      auto spritesheet = layer ? anm2.element_get(ElementType::SPRITESHEET, layer->spritesheetId) : nullptr;
      if (!track || !animation) continue;

      auto start = (int)track->children.size();
      frames_generate_from_grid(*track, options.start, options.size, options.pivot, options.columns, options.count,
                                options.delay);
      for (int i = start; i < (int)track->children.size(); ++i)
      {
        auto& frame = track->children[i];
        if (frame.type != ElementType::FRAME) continue;
        if (!options.isMakeRegions || !spritesheet) continue;

        auto region = element_make(ElementType::REGION);
        region.id = element_child_next_id_get(*spritesheet, ElementType::REGION);
        region.name = region_name_get(options.regionNameFormat, i - start);
        region.crop = frame.crop;
        region.size = frame.size;
        region.pivot = frame.pivot;
        frame.regionId = region.id;
        spritesheet->children.push_back(region);
      }
      animation->frameNum = animation_length_get(*animation);

      anm2.uids_repair();
      for (int i = start; i < (int)track->children.size(); ++i)
        uids.push_back(track->children[i].uid);
    }
    return uids;
  }

  // Applies a frame change to explicit frames, to whole tracks (frameIndex -1), and to every matching track of the
  // given animations.
  Uids frames_change_apply(Anm2& anm2, const ChangeTargets& targets, FrameChange change, ChangeType type)
  {
    std::map<Reference, std::set<int>> frames{};
    std::set<Reference> wholeTracks{};
    for (auto reference : targets.references)
    {
      auto index = std::exchange(reference.frameIndex, -1);
      index < 0 ? (void)wholeTracks.insert(reference) : (void)frames[reference].insert(index);
    }

    auto whole_apply = [&](Element* track, ItemType itemType)
    {
      if (!track) return;
      std::set<int> indices{};
      for (int i = 0; i < track_frames_count_get(*track); ++i)
        indices.insert(i);
      frames_change(*track, change, itemType, type, indices);
    };

    for (auto& [reference, indices] : frames)
      if (auto track = anm2.element_get(reference))
        frames_change(*track, change, (ItemType)reference.itemType, type, indices);
    for (auto reference : wholeTracks)
      whole_apply(anm2.element_get(reference), (ItemType)reference.itemType);
    for (auto animationIndex : targets.animations)
    {
      auto animation = anm2.element_get(ElementType::ANIMATION, animationIndex);
      if (!animation) continue;
      if (targets.isRoot) whole_apply(animation_item_get(*animation, ItemType::ROOT), ItemType::ROOT);
      for (const auto& row : TRACK_CONTAINERS)
        if (auto container = child_first_get(*animation, row.container);
            container && (row.itemType == ItemType::LAYER ? targets.isLayers : targets.isNulls))
          tracks_each(*container, row.track, [&](Element& track) { whole_apply(&track, row.itemType); });
    }
    return {};
  }
}
