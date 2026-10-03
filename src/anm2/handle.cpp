#include "internal.hpp"

#include <unordered_set>

#include "uid.hpp"

namespace anm2ed
{
  void Anm2::uids_repair()
  {
    std::unordered_set<std::uint64_t> seen{};
    element_each(root,
                 [&](Element& element)
                 {
                   if (element.uid == 0 || !seen.insert(element.uid).second) element.uid = util::uid_next();
                 });
  }

  Handle Anm2::handle_get(Reference reference, bool isGroup) const
  {
    Handle handle{};
    auto animation = element_get(ElementType::ANIMATION, reference.animationIndex);
    if (!animation) return handle;
    handle.animation = animation->uid;

    auto item = isGroup ? animation_group_get(*animation, reference.itemType, reference.itemID)
                : reference.itemType == NONE
                    ? nullptr
                    : animation_item_get(*animation, (ItemType)reference.itemType, reference.itemID,
                                         reference.groupType, reference.groupId);
    if (!item) return handle;
    handle.item = item->uid;

    if (auto frame = isGroup ? nullptr : track_frame_get(*item, reference.frameIndex)) handle.frame = frame->uid;
    return handle;
  }

  void track_locations_add(std::unordered_map<std::uint64_t, Reference>& locations, const Element& track,
                           Reference reference)
  {
    locations[track.uid] = reference;
    auto frameType = track_frame_type_get(track);
    for (const auto& frame : track.children)
      if (frame.type == frameType)
      {
        ++reference.frameIndex;
        locations[frame.uid] = reference;
      }
  }

  UidIndex::UidIndex(const Anm2& anm2)
  {
    auto animations = anm2.element_get(ElementType::ANIMATIONS);
    if (!animations) return;

    int animationIndex{};
    for (const auto& animation : animations->children)
    {
      if (animation.type != ElementType::ANIMATION) continue;
      locations[animation.uid] = {animationIndex};

      for (const auto& child : animation.children)
      {
        if (child.type == ElementType::ROOT_ANIMATION) track_locations_add(locations, child, {animationIndex, ROOT});
        if (child.type == ElementType::TRIGGERS) track_locations_add(locations, child, {animationIndex, TRIGGER});

        auto row = track_container_get(child.type);
        if (!row || row->container != child.type) continue;
        auto type = (int)row->itemType;

        auto tracks_add = [&](this auto&& self, const Element& parent, int groupId) -> void
        {
          for (const auto& track : parent.children)
            if (track.type == ElementType::GROUP)
            {
              locations[track.uid] = {animationIndex, type, track.id};
              self(track, track.id);
            }
            else if (track.type == ElementType::ROOT_ANIMATION && groupId != -1)
              track_locations_add(locations, track, {animationIndex, ROOT, -1, -1, type, groupId});
            else if (track.type == row->track)
            {
              auto trackGroupId = groupId != -1 ? groupId : track.groupId;
              track_locations_add(
                  locations, track,
                  {animationIndex, type, track_id_get(track), -1, trackGroupId == -1 ? NONE : type, trackGroupId});
            }
        };
        tracks_add(child, -1);
      }
      ++animationIndex;
    }
  }

  std::optional<Reference> UidIndex::reference_get(Handle handle, Reference reference) const
  {
    auto location_get = [&](std::uint64_t uid) -> const Reference*
    {
      auto it = locations.find(uid);
      return it == locations.end() ? nullptr : &it->second;
    };

    // Resolve the most specific element captured; if it no longer exists, neither does the reference.
    auto uid = handle.frame ? handle.frame : handle.item ? handle.item : handle.animation;
    if (!uid) return reference;
    auto location = location_get(uid);
    if (!location) return std::nullopt;

    auto isGroupAgnostic = reference.groupId == -1 && reference.itemType != ROOT;
    if (uid == handle.animation)
      reference.animationIndex = location->animationIndex;
    else
      reference = *location;
    // References made without group context stay that way, wherever the track now lives.
    if (isGroupAgnostic && reference.itemType != ROOT)
    {
      reference.groupType = NONE;
      reference.groupId = -1;
    }
    return reference;
  }
}
