#include "internal.hpp"

namespace anm2ed
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

  bool is_region_matched(const Element& region, const Element& frame)
  {
    return region.type == ElementType::REGION && glm::ivec2(region.crop) == glm::ivec2(frame.crop) &&
           glm::ivec2(region.size) == glm::ivec2(frame.size) && glm::ivec2(region.pivot) == glm::ivec2(frame.pivot);
  }

  Element* region_match_get(Element& spritesheet, const Element& frame)
  {
    return child_find(spritesheet, [&](const Element& region) { return is_region_matched(region, frame); });
  }

  bool Anm2::regions_generate(const std::set<int>& animationIndices, const std::set<Reference>& frameReferences,
                              const std::string& format, RegionFrameMapping mapping)
  {
    std::vector<std::pair<Element*, Element*>> frames{};
    for (auto animationIndex : animationIndices)
      if (auto animation = element_get(ElementType::ANIMATION, animationIndex))
        layer_frames_each(root, *animation,
                          [&](Element& frame, Element& spritesheet) { frames.emplace_back(&frame, &spritesheet); });

    for (auto frameReference : frameReferences)
    {
      if (frameReference.itemType != LAYER || frameReference.frameIndex < 0) continue;
      auto frame = element_get(frameReference);
      auto layer = element_get(ElementType::LAYER_ELEMENT, frameReference.itemID);
      auto spritesheet = layer ? element_get(ElementType::SPRITESHEET, layer->spritesheetId) : nullptr;
      if (frame && frame->type == ElementType::FRAME && frame->regionId == -1 && spritesheet)
        frames.emplace_back(frame, spritesheet);
    }

    std::unordered_map<int, int> regionNumbers{};
    bool isChanged{};
    for (auto [frame, spritesheet] : frames)
    {
      auto region = region_match_get(*spritesheet, *frame);
      if (!region)
      {
        auto id = element_child_next_id_get(*spritesheet, ElementType::REGION);
        region = &spritesheet->children.emplace_back(element_make(ElementType::REGION));
        region->id = id;
        region->name = region_name_get(format, ++regionNumbers[spritesheet->id]);
        region->crop = frame->crop;
        region->size = frame->size;
        region->pivot = frame->pivot;
        isChanged = true;
      }
      if (mapping == RegionFrameMapping::SET && frame->regionId != region->id)
      {
        frame->regionId = region->id;
        isChanged = true;
      }
    }

    return isChanged;
  }

  void Anm2::regions_scan()
  {
    layer_frames_each(root,
                      [](Element& frame, Element& spritesheet)
                      {
                        if (frame.regionId != -1) return;
                        if (auto region = region_match_get(spritesheet, frame)) frame.regionId = region->id;
                      });
  }
}
