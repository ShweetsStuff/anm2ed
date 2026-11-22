#include "edit/edit.h"

#include <algorithm>
#include <format>
#include <limits>
#include <unordered_map>

#include "model/frames.h"
#include "pack.h"
#include "path.h"

// Spritesheet pixel edits: the model changes in place, and a new image is returned for the document to store.
bool regions_trim(Model& model, int spritesheetId, const std::set<int>& ids, const Image& source)
{
    auto spritesheet = item_get(model.content.spritesheets, spritesheetId);
    if (!spritesheet || !source.is_valid() || source.pixels.empty())
        return false;

    bool isChanged = false;
    for (auto id : ids)
    {
        auto region = item_get(spritesheet->regions, id);
        if (!region)
            continue;

        auto minPoint = vec_max(IVec2(vec_min(region->crop, region->crop + region->size)), IVec2(0));
        auto maxPoint = vec_min(IVec2(vec_max(region->crop, region->crop + region->size)), source.size);
        auto contentMin = IVec2(std::numeric_limits<int>::max());
        auto contentMax = IVec2(std::numeric_limits<int>::min());

        for (int y = minPoint.y; y < maxPoint.y; ++y)
            for (int x = minPoint.x; x < maxPoint.x; ++x)
            {
                auto index = ((std::size_t)y * source.size.x + x) * IMAGE_CHANNELS;
                if (index + IMAGE_CHANNELS > source.pixels.size())
                    continue;
                if (std::all_of(source.pixels.begin() + index, source.pixels.begin() + index + IMAGE_CHANNELS,
                                [](auto channel) { return channel == 0; }))
                    continue;
                contentMin = vec_min(contentMin, IVec2(x, y));
                contentMax = vec_max(contentMax, IVec2(x, y));
            }

        if (contentMin.x == std::numeric_limits<int>::max())
            continue;

        auto newCrop = Vec2(contentMin);
        auto newSize = Vec2(contentMax - contentMin + 1);
        if (region->crop == newCrop && region->size == newSize)
            continue;

        auto previousCrop = region->crop;
        region->crop = newCrop;
        region->size = newSize;
        region->pivot = region->origin == Origin::CUSTOM ? region->pivot - (region->crop - previousCrop)
                                                         : region_pivot_get(*region);
        isChanged = true;
    }

    return isChanged;
}

std::optional<Image> spritesheet_pack(Model& model, int id, const Image& sheet, int padding)
{
    struct PackItem
    {
        int regionId = -1;
        IVec2 source{};
        IVec2 size{};
    };

    auto spritesheet = item_get(model.content.spritesheets, id);
    if (!spritesheet || !sheet.is_valid() || sheet.pixels.empty())
        return {};

    padding = std::max(0, padding);
    std::vector<PackItem> items;
    for (auto& region : spritesheet->regions)
    {
        auto minPoint = IVec2(vec_min(region.crop, region.crop + region.size));
        auto maxPoint = IVec2(vec_max(region.crop, region.crop + region.size));
        items.push_back({region.id, minPoint, vec_max(maxPoint - minPoint, IVec2(1))});
    }

    std::sort(items.begin(), items.end(), [](const PackItem& a, const PackItem& b) {
        auto areaA = a.size.x * a.size.y;
        auto areaB = b.size.x * b.size.y;
        return areaA != areaB ? areaA > areaB : a.regionId < b.regionId;
    });

    std::vector<IVec2> sizes{};
    for (auto& item : items)
        sizes.push_back(item.size + padding * 2);

    IVec2 packedSize{};
    std::vector<IVec2> positions{};
    if (!rects_pack(sizes, packedSize, positions) || packedSize.x <= 0 || packedSize.y <= 0)
        return {};

    std::vector<ImagePart> parts;
    for (int i = 0; i < (int)items.size(); ++i)
        parts.push_back({&sheet, items[i].source, items[i].size, positions[i] + padding});
    for (auto& region : spritesheet->regions)
        for (int i = 0; i < (int)items.size(); ++i)
            if (items[i].regionId == region.id)
                region.crop = parts[i].destination;
    return Image::compose(parts, packedSize, sheet.is_vector());
}

// Appends sheets to the first, right or below. The result is an SVG (holding any PNG sheets as images) when any of
// them is one. The first sheet's path takes the result's extension.
std::optional<Image> spritesheets_merge(Model& model, const std::set<int>& ids, const ImageGet& image_get,
                                        SpritesheetMergeOptions options)
{
    auto [isAppendRight, isMakeRegions, isMakePrimaryRegion, origin] = options;
    if (ids.size() < 2)
        return {};
    for (auto id : ids)
        if (!item_get(model.content.spritesheets, id) || !image_get(id) || !image_get(id)->is_valid())
            return {};

    auto baseId = *ids.begin();
    auto base = item_get(model.content.spritesheets, baseId);
    auto mergedSize = image_get(baseId)->size;
    std::vector<ImagePart> parts{{image_get(baseId), {}, mergedSize, {}}};
    std::unordered_map<int, std::unordered_map<int, int>> regionIdMap;

    auto location_region_add = [&](int sourceId, IVec2 crop, IVec2 size) {
        auto source = item_get(model.content.spritesheets, sourceId);
        Region region = {};
        region.id = item_next_id_get(base->regions);
        auto stem = path_to_utf8(source->path.stem());
        region.name = stem.empty() ? std::format("#{}", sourceId) : stem;
        region.crop = crop;
        region.size = size;
        region.pivot = origin == Origin::CENTER ? Vec2(size) * 0.5f : Vec2();
        region.origin = origin;
        base->regions.push_back(region);
    };

    if (isMakeRegions && isMakePrimaryRegion)
        location_region_add(baseId, {}, mergedSize);
    for (auto id : ids)
    {
        if (id == baseId)
            continue;
        auto texture = image_get(id);
        auto offset = isAppendRight ? IVec2(mergedSize.x, 0) : IVec2(0, mergedSize.y);
        mergedSize = isAppendRight ? IVec2(mergedSize.x + texture->size.x, std::max(mergedSize.y, texture->size.y))
                                   : IVec2(std::max(mergedSize.x, texture->size.x), mergedSize.y + texture->size.y);
        parts.push_back({texture, {}, texture->size, offset});
        if (!isMakeRegions)
            continue;

        location_region_add(id, offset, texture->size);
        for (auto sourceRegion : item_get(model.content.spritesheets, id)->regions)
        {
            auto sourceRegionId = sourceRegion.id;
            sourceRegion.id = item_next_id_get(base->regions);
            sourceRegion.uid = uid_next();
            sourceRegion.crop += offset;
            base->regions.push_back(sourceRegion);
            regionIdMap[id][sourceRegionId] = sourceRegion.id;
        }
    }

    std::unordered_map<int, int> layerSpritesheetBefore;
    for (auto& layer : model.content.layers)
        if (ids.contains(layer.spritesheetId))
        {
            layerSpritesheetBefore[layer.id] = layer.spritesheetId;
            layer.spritesheetId = baseId;
        }

    for (int i = 0; i < model.animations_count_get(); ++i)
        items_each(model.animation_edit(i)->layers, [&](Track& track, auto*) {
            auto before = layerSpritesheetBefore.find(track.id);
            if (before == layerSpritesheetBefore.end() || before->second == baseId)
                return;
            auto& remap = regionIdMap[before->second];
            for (auto& frame : track.frames)
                if (frame.regionId != -1)
                    frame.regionId = remap.contains(frame.regionId) ? remap.at(frame.regionId) : -1;
        });

    auto isVector = std::ranges::any_of(parts, [](const ImagePart& part) { return part.image->is_vector(); });
    if (isVector != is_path_extension(base->path, SVG_EXTENSION))
        base->path.replace_extension(isVector ? SVG_EXTENSION : PNG_EXTENSION);
    std::erase_if(model.content.spritesheets, [&](const Spritesheet& spritesheet) {
        return spritesheet.id != baseId && ids.contains(spritesheet.id);
    });
    return Image::compose(parts, mergedSize, isVector);
}
