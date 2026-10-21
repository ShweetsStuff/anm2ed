#include "edit.hpp"

#include <algorithm>
#include <format>
#include <limits>
#include <unordered_map>

#include "model/frames.hpp"
#include "pack.hpp"
#include "path.hpp"

using namespace anm2ed::model;
using namespace anm2ed::resource;
using namespace anm2ed::util;

// Spritesheet pixel edits: the model changes in place, and a new image is returned for the document to store.
namespace anm2ed::edit
{
  bool regions_trim(Model& model, int spritesheetId, const std::set<int>& ids, const Image& source)
  {
    auto spritesheet = item_get(model.content.spritesheets, spritesheetId);
    if (!spritesheet || !source.is_valid() || source.pixels.empty()) return false;

    bool isChanged{};
    for (auto id : ids)
    {
      auto region = item_get(spritesheet->regions, id);
      if (!region) continue;

      auto minPoint = glm::max(glm::ivec2(glm::min(region->crop, region->crop + region->size)), glm::ivec2(0));
      auto maxPoint = glm::min(glm::ivec2(glm::max(region->crop, region->crop + region->size)), source.size);
      auto contentMin = glm::ivec2(std::numeric_limits<int>::max());
      auto contentMax = glm::ivec2(std::numeric_limits<int>::min());

      for (int y = minPoint.y; y < maxPoint.y; ++y)
        for (int x = minPoint.x; x < maxPoint.x; ++x)
        {
          auto index = ((std::size_t)y * source.size.x + x) * image::CHANNELS;
          if (index + image::CHANNELS > source.pixels.size()) continue;
          if (std::all_of(source.pixels.begin() + index, source.pixels.begin() + index + image::CHANNELS,
                          [](auto channel) { return channel == 0; }))
            continue;
          contentMin = glm::min(contentMin, glm::ivec2(x, y));
          contentMax = glm::max(contentMax, glm::ivec2(x, y));
        }

      if (contentMin.x == std::numeric_limits<int>::max()) continue;

      auto newCrop = glm::vec2(contentMin);
      auto newSize = glm::vec2(contentMax - contentMin + 1);
      if (region->crop == newCrop && region->size == newSize) continue;

      auto previousCrop = region->crop;
      region->crop = newCrop;
      region->size = newSize;
      if (region->origin == Origin::TOP_LEFT)
        region->pivot = {};
      else if (region->origin == Origin::CENTER)
        region->pivot = region->size * 0.5f;
      else
        region->pivot -= region->crop - previousCrop;
      isChanged = true;
    }

    return isChanged;
  }

  std::optional<Image> spritesheet_pack(Model& model, int id, const Image& sheet, int padding)
  {
    struct PackItem
    {
      int regionId{-1};
      glm::ivec2 source{};
      glm::ivec2 size{};
    };

    auto spritesheet = item_get(model.content.spritesheets, id);
    if (!spritesheet || !sheet.is_valid() || sheet.pixels.empty()) return {};

    padding = std::max(0, padding);
    std::vector<PackItem> items{};
    for (auto& region : spritesheet->regions)
    {
      auto minPoint = glm::ivec2(glm::min(region.crop, region.crop + region.size));
      auto maxPoint = glm::ivec2(glm::max(region.crop, region.crop + region.size));
      items.push_back({region.id, minPoint, glm::max(maxPoint - minPoint, glm::ivec2(1))});
    }

    std::sort(items.begin(), items.end(),
              [](const PackItem& a, const PackItem& b)
              {
                auto areaA = a.size.x * a.size.y;
                auto areaB = b.size.x * b.size.y;
                return areaA != areaB ? areaA > areaB : a.regionId < b.regionId;
              });

    std::vector<glm::ivec2> sizes{};
    for (auto& item : items)
      sizes.push_back(item.size + padding * 2);

    glm::ivec2 packedSize{};
    std::vector<glm::ivec2> positions{};
    if (!util::pack::rects_pack(sizes, packedSize, positions) || packedSize.x <= 0 || packedSize.y <= 0) return {};

    std::vector<ImagePart> parts{};
    for (int i = 0; i < (int)items.size(); ++i)
      parts.push_back({&sheet, items[i].source, items[i].size, positions[i] + padding});
    for (auto& region : spritesheet->regions)
      for (int i = 0; i < (int)items.size(); ++i)
        if (items[i].regionId == region.id) region.crop = parts[i].destination;
    return Image::compose(parts, packedSize, sheet.is_vector());
  }

  // Appends sheets to the first, right or below. The result is an SVG (holding any PNG sheets as images) when any of
  // them is one; the first sheet's path takes the result's extension.
  std::optional<Image> spritesheets_merge(Model& model, const std::set<int>& ids, const ImageGet& image_get,
                                          bool isAppendRight, bool isMakeRegions, bool isMakePrimaryRegion,
                                          Origin origin)
  {
    if (ids.size() < 2) return {};
    for (auto id : ids)
      if (!item_get(model.content.spritesheets, id) || !image_get(id) || !image_get(id)->is_valid()) return {};

    auto baseId = *ids.begin();
    auto base = item_get(model.content.spritesheets, baseId);
    auto mergedSize = image_get(baseId)->size;
    std::vector<ImagePart> parts{{image_get(baseId), {}, mergedSize, {}}};
    std::unordered_map<int, std::unordered_map<int, int>> regionIdMap{};

    auto location_region_add = [&](int sourceId, glm::ivec2 crop, glm::ivec2 size)
    {
      auto source = item_get(model.content.spritesheets, sourceId);
      Region region{};
      region.id = item_next_id_get(base->regions);
      auto stem = path::to_utf8(source->path.stem());
      region.name = stem.empty() ? std::format("#{}", sourceId) : stem;
      region.crop = crop;
      region.size = size;
      region.pivot = origin == Origin::CENTER ? glm::vec2(size) * 0.5f : glm::vec2();
      region.origin = origin;
      base->regions.push_back(region);
    };

    if (isMakeRegions && isMakePrimaryRegion) location_region_add(baseId, {}, mergedSize);
    for (auto id : ids)
    {
      if (id == baseId) continue;
      auto texture = image_get(id);
      auto offset = isAppendRight ? glm::ivec2(mergedSize.x, 0) : glm::ivec2(0, mergedSize.y);
      mergedSize = isAppendRight ? glm::ivec2(mergedSize.x + texture->size.x, std::max(mergedSize.y, texture->size.y))
                                 : glm::ivec2(std::max(mergedSize.x, texture->size.x), mergedSize.y + texture->size.y);
      parts.push_back({texture, {}, texture->size, offset});
      if (!isMakeRegions) continue;

      location_region_add(id, offset, texture->size);
      for (auto sourceRegion : item_get(model.content.spritesheets, id)->regions)
      {
        auto sourceRegionId = sourceRegion.id;
        sourceRegion.id = item_next_id_get(base->regions);
        sourceRegion.uid = util::uid_next();
        sourceRegion.crop += offset;
        base->regions.push_back(sourceRegion);
        regionIdMap[id][sourceRegionId] = sourceRegion.id;
      }
    }

    std::unordered_map<int, int> layerSpritesheetBefore{};
    for (auto& layer : model.content.layers)
      if (ids.contains(layer.spritesheetId))
      {
        layerSpritesheetBefore[layer.id] = layer.spritesheetId;
        layer.spritesheetId = baseId;
      }

    for (int i = 0; i < model.animations_count_get(); ++i)
      tracks_each(model.animation_edit(i)->layers,
                  [&](Track& track, auto*)
                  {
                    auto before = layerSpritesheetBefore.find(track.id);
                    if (before == layerSpritesheetBefore.end() || before->second == baseId) return;
                    auto& remap = regionIdMap[before->second];
                    for (auto& frame : track.frames)
                      if (frame.regionId != -1)
                        frame.regionId = remap.contains(frame.regionId) ? remap.at(frame.regionId) : -1;
                  });

    auto isVector = std::ranges::any_of(parts, [](const ImagePart& part) { return part.image->is_vector(); });
    if (isVector != path::is_extension(base->path, image::SVG_EXTENSION))
      base->path.replace_extension(isVector ? image::SVG_EXTENSION : image::PNG_EXTENSION);
    std::erase_if(model.content.spritesheets, [&](const Spritesheet& spritesheet)
                  { return spritesheet.id != baseId && ids.contains(spritesheet.id); });
    return Image::compose(parts, mergedSize, isVector);
  }
}
