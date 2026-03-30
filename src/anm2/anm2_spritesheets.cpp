#include "anm2.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "map_.hpp"
#include "path_.hpp"
#include "working_directory.hpp"

using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  Spritesheet* Anm2::spritesheet_get(int id) { return map::find(content.spritesheets, id); }

  bool Anm2::spritesheet_add(const std::filesystem::path& directory, const std::filesystem::path& path, int& id)
  {
    Spritesheet spritesheet(directory, path);
    if (!spritesheet.is_valid()) return false;
    id = map::next_id_get(content.spritesheets);
    content.spritesheets[id] = std::move(spritesheet);
    return true;
  }

  bool Anm2::spritesheet_pack(int id, int padding)
  {
    const int packingPadding = std::max(0, padding);

    struct RectI
    {
      int x{};
      int y{};
      int w{};
      int h{};
    };

    struct PackItem
    {
      int regionID{-1};
      int srcX{};
      int srcY{};
      int width{};
      int height{};
      int packWidth{};
      int packHeight{};
    };

    class MaxRectsPacker
    {
      int width{};
      int height{};
      std::vector<RectI> freeRects{};

      static bool intersects(const RectI& a, const RectI& b)
      {
        return !(b.x >= a.x + a.w || b.x + b.w <= a.x || b.y >= a.y + a.h || b.y + b.h <= a.y);
      }

      static bool contains(const RectI& a, const RectI& b)
      {
        return b.x >= a.x && b.y >= a.y && b.x + b.w <= a.x + a.w && b.y + b.h <= a.y + a.h;
      }

      void split_free_rects(const RectI& used)
      {
        std::vector<RectI> next{};
        next.reserve(freeRects.size() * 2);

        for (auto& free : freeRects)
        {
          if (!intersects(free, used))
          {
            next.push_back(free);
            continue;
          }

          if (used.x > free.x) next.push_back({free.x, free.y, used.x - free.x, free.h});
          if (used.x + used.w < free.x + free.w)
            next.push_back({used.x + used.w, free.y, free.x + free.w - (used.x + used.w), free.h});
          if (used.y > free.y) next.push_back({free.x, free.y, free.w, used.y - free.y});
          if (used.y + used.h < free.y + free.h)
            next.push_back({free.x, used.y + used.h, free.w, free.y + free.h - (used.y + used.h)});
        }

        freeRects = std::move(next);
      }

      void prune_free_rects()
      {
        for (int i = 0; i < (int)freeRects.size(); i++)
        {
          if (freeRects[i].w <= 0 || freeRects[i].h <= 0)
          {
            freeRects.erase(freeRects.begin() + i--);
            continue;
          }

          for (int j = i + 1; j < (int)freeRects.size();)
          {
            if (contains(freeRects[i], freeRects[j]))
              freeRects.erase(freeRects.begin() + j);
            else if (contains(freeRects[j], freeRects[i]))
            {
              freeRects.erase(freeRects.begin() + i);
              i--;
              break;
            }
            else
              j++;
          }
        }
      }

    public:
      MaxRectsPacker(int width, int height) : width(width), height(height), freeRects({{0, 0, width, height}}) {}

      bool insert(int width, int height, RectI& result)
      {
        int bestShort = std::numeric_limits<int>::max();
        int bestLong = std::numeric_limits<int>::max();
        RectI best{};
        bool found{};

        for (auto& free : freeRects)
        {
          if (width > free.w || height > free.h) continue;

          int leftOverW = free.w - width;
          int leftOverH = free.h - height;
          int shortSide = std::min(leftOverW, leftOverH);
          int longSide = std::max(leftOverW, leftOverH);

          if (shortSide < bestShort || (shortSide == bestShort && longSide < bestLong))
          {
            bestShort = shortSide;
            bestLong = longSide;
            best = {free.x, free.y, width, height};
            found = true;
          }
        }

        if (!found) return false;

        result = best;
        split_free_rects(best);
        prune_free_rects();
        return true;
      }
    };

    auto pack_regions = [&](const std::vector<PackItem>& items, int& packedWidth, int& packedHeight,
                            std::unordered_map<int, RectI>& packedRects)
    {
      if (items.empty()) return false;

      int maxWidth{};
      int maxHeight{};
      int sumWidth{};
      int sumHeight{};
      int64_t totalArea{};
      for (auto& item : items)
      {
        maxWidth = std::max(maxWidth, item.packWidth);
        maxHeight = std::max(maxHeight, item.packHeight);
        sumWidth += item.packWidth;
        sumHeight += item.packHeight;
        totalArea += (int64_t)item.packWidth * item.packHeight;
      }

      if (maxWidth <= 0 || maxHeight <= 0) return false;

      int bestSquareDelta = std::numeric_limits<int>::max();
      int bestArea = std::numeric_limits<int>::max();
      int bestWidth{};
      int bestHeight{};
      std::unordered_map<int, RectI> bestRects{};

      int startWidth = maxWidth;
      int endWidth = std::max(startWidth, sumWidth);
      int step = std::max(1, (endWidth - startWidth) / 512);

      for (int candidateWidth = startWidth; candidateWidth <= endWidth; candidateWidth += step)
      {
        int candidateHeightMin = std::max(maxHeight, (int)std::ceil((double)totalArea / candidateWidth));
        bool isValid{};
        int usedWidth{};
        int usedHeight{};
        std::unordered_map<int, RectI> candidateRects{};

        // Grow candidate height until this width can actually fit all rectangles.
        for (int candidateHeight = candidateHeightMin; candidateHeight <= sumHeight; candidateHeight++)
        {
          MaxRectsPacker packer(candidateWidth, candidateHeight);
          candidateRects.clear();
          isValid = true;
          usedWidth = 0;
          usedHeight = 0;

          for (auto& item : items)
          {
            RectI rect{};
            if (!packer.insert(item.packWidth, item.packHeight, rect))
            {
              isValid = false;
              break;
            }

            candidateRects[item.regionID] = rect;
            usedWidth = std::max(usedWidth, rect.x + rect.w);
            usedHeight = std::max(usedHeight, rect.y + rect.h);
          }

          if (isValid) break;
        }

        if (!isValid) continue;

        int area = usedWidth * usedHeight;
        int squareDelta = std::abs(usedWidth - usedHeight);
        if (squareDelta < bestSquareDelta || (squareDelta == bestSquareDelta && area < bestArea))
        {
          bestSquareDelta = squareDelta;
          bestArea = area;
          bestWidth = usedWidth;
          bestHeight = usedHeight;
          bestRects = std::move(candidateRects);
          if (bestArea == totalArea && bestSquareDelta == 0) break;
        }
      }

      if (bestArea == std::numeric_limits<int>::max()) return false;

      packedWidth = bestWidth;
      packedHeight = bestHeight;
      packedRects = std::move(bestRects);
      return true;
    };

    if (!content.spritesheets.contains(id)) return false;
    auto& spritesheet = content.spritesheets.at(id);
    if (!spritesheet.texture.is_valid() || spritesheet.texture.pixels.empty()) return false;
    if (spritesheet.regions.empty()) return false;

    std::vector<PackItem> items{};
    items.reserve(spritesheet.regions.size());

    for (auto& [regionID, region] : spritesheet.regions)
    {
      auto minPoint = glm::ivec2(glm::min(region.crop, region.crop + region.size));
      auto maxPoint = glm::ivec2(glm::max(region.crop, region.crop + region.size));
      auto size = glm::max(maxPoint - minPoint, glm::ivec2(1));
      int packWidth = size.x + packingPadding * 2;
      int packHeight = size.y + packingPadding * 2;
      items.push_back({regionID, minPoint.x, minPoint.y, size.x, size.y, packWidth, packHeight});
    }

    std::sort(items.begin(), items.end(), [](const PackItem& a, const PackItem& b)
    {
      int areaA = a.width * a.height;
      int areaB = b.width * b.height;
      if (areaA != areaB) return areaA > areaB;
      return a.regionID < b.regionID;
    });

    int packedWidth{};
    int packedHeight{};
    std::unordered_map<int, RectI> packedRects{};
    if (!pack_regions(items, packedWidth, packedHeight, packedRects)) return false;
    if (packedWidth <= 0 || packedHeight <= 0) return false;

    auto textureSize = spritesheet.texture.size;
    auto& sourcePixels = spritesheet.texture.pixels;
    std::vector<uint8_t> packedPixels((size_t)packedWidth * packedHeight * resource::texture::CHANNELS, 0);

    for (auto& item : items)
    {
      if (!packedRects.contains(item.regionID)) continue;
      auto destinationRect = packedRects.at(item.regionID);

      for (int y = 0; y < item.height; y++)
      {
        for (int x = 0; x < item.width; x++)
        {
          int sourceX = item.srcX + x;
          int sourceY = item.srcY + y;
          int destinationX = destinationRect.x + packingPadding + x;
          int destinationY = destinationRect.y + packingPadding + y;

          if (sourceX < 0 || sourceY < 0 || sourceX >= textureSize.x || sourceY >= textureSize.y) continue;
          if (destinationX < 0 || destinationY < 0 || destinationX >= packedWidth || destinationY >= packedHeight)
            continue;

          auto sourceIndex = ((size_t)sourceY * textureSize.x + sourceX) * resource::texture::CHANNELS;
          auto destinationIndex =
              ((size_t)destinationY * packedWidth + destinationX) * resource::texture::CHANNELS;
          std::copy_n(sourcePixels.data() + sourceIndex, resource::texture::CHANNELS,
                      packedPixels.data() + destinationIndex);
        }
      }
    }

    spritesheet.texture = resource::Texture(packedPixels.data(), {packedWidth, packedHeight});

    for (auto& [regionID, region] : spritesheet.regions)
      if (packedRects.contains(regionID))
      {
        auto& rect = packedRects.at(regionID);
        region.crop = {rect.x + packingPadding, rect.y + packingPadding};
      }

    return true;
  }

  bool Anm2::regions_trim(int spritesheetID, const std::set<int>& ids)
  {
    auto spritesheet = spritesheet_get(spritesheetID);
    if (!spritesheet || !spritesheet->texture.is_valid() || spritesheet->texture.pixels.empty() || ids.empty())
      return false;

    auto& texture = spritesheet->texture;
    bool changed{};

    for (auto id : ids)
    {
      if (!spritesheet->regions.contains(id)) continue;
      auto& region = spritesheet->regions.at(id);

      auto minPoint = glm::ivec2(glm::min(region.crop, region.crop + region.size));
      auto maxPoint = glm::ivec2(glm::max(region.crop, region.crop + region.size));

      int minX = std::max(0, minPoint.x);
      int minY = std::max(0, minPoint.y);
      int maxX = std::min(texture.size.x, maxPoint.x);
      int maxY = std::min(texture.size.y, maxPoint.y);

      if (minX >= maxX || minY >= maxY) continue;

      int contentMinX = std::numeric_limits<int>::max();
      int contentMinY = std::numeric_limits<int>::max();
      int contentMaxX = std::numeric_limits<int>::min();
      int contentMaxY = std::numeric_limits<int>::min();

      for (int y = minY; y < maxY; y++)
      {
        for (int x = minX; x < maxX; x++)
        {
          auto index = ((size_t)y * texture.size.x + x) * resource::texture::CHANNELS;
          if (index + resource::texture::CHANNELS > texture.pixels.size()) continue;

          auto r = texture.pixels[index + 0];
          auto g = texture.pixels[index + 1];
          auto b = texture.pixels[index + 2];
          auto a = texture.pixels[index + 3];
          if (r == 0 && g == 0 && b == 0 && a == 0) continue;

          contentMinX = std::min(contentMinX, x);
          contentMinY = std::min(contentMinY, y);
          contentMaxX = std::max(contentMaxX, x);
          contentMaxY = std::max(contentMaxY, y);
        }
      }

      if (contentMinX == std::numeric_limits<int>::max()) continue;

      auto newCrop = glm::vec2(contentMinX, contentMinY);
      auto newSize = glm::vec2(contentMaxX - contentMinX + 1, contentMaxY - contentMinY + 1);
      if (region.crop != newCrop || region.size != newSize)
      {
        auto previousCrop = region.crop;
        region.crop = newCrop;
        region.size = newSize;
        if (region.origin == Spritesheet::Region::TOP_LEFT)
          region.pivot = {};
        else if (region.origin == Spritesheet::Region::ORIGIN_CENTER)
          region.pivot = {static_cast<int>(region.size.x / 2.0f), static_cast<int>(region.size.y / 2.0f)};
        else
          // Preserve the same texture-space pivot location when trimming shifts region crop.
          region.pivot -= (region.crop - previousCrop);
        changed = true;
      }
    }

    return changed;
  }

  std::set<int> Anm2::spritesheets_unused()
  {
    std::set<int> used{};
    for (auto& layer : content.layers | std::views::values)
      if (layer.is_spritesheet_valid()) used.insert(layer.spritesheetID);

    std::set<int> unused{};
    for (auto& id : content.spritesheets | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  bool Anm2::spritesheets_merge(const std::set<int>& ids, SpritesheetMergeOrigin mergeOrigin, bool isMakeRegions,
                                bool isMakePrimaryRegion, origin::Type regionOrigin)
  {
    if (ids.size() < 2) return false;

    auto baseId = *ids.begin();
    if (!content.spritesheets.contains(baseId)) return false;
    for (auto id : ids)
      if (!content.spritesheets.contains(id)) return false;

    auto& base = content.spritesheets.at(baseId);
    if (!base.texture.is_valid()) return false;

    std::unordered_map<int, glm::ivec2> offsets{};
    offsets[baseId] = {};

    auto baseTextureSize = base.texture.size;
    auto mergedTexture = base.texture;
    for (auto id : ids)
    {
      if (id == baseId) continue;

      auto& spritesheet = content.spritesheets.at(id);
      if (!spritesheet.texture.is_valid()) return false;

      offsets[id] = mergeOrigin == APPEND_RIGHT ? glm::ivec2(mergedTexture.size.x, 0)
                                                : glm::ivec2(0, mergedTexture.size.y);
      mergedTexture = resource::Texture::merge_append(mergedTexture, spritesheet.texture,
                                                      mergeOrigin == APPEND_RIGHT);
    }
    base.texture = std::move(mergedTexture);

    std::unordered_map<int, std::unordered_map<int, int>> regionIdMap{};

    if (isMakeRegions)
    {
      if (base.regionOrder.size() != base.regions.size())
      {
        base.regionOrder.clear();
        base.regionOrder.reserve(base.regions.size());
        for (auto id : base.regions | std::views::keys)
          base.regionOrder.push_back(id);
      }

      if (isMakePrimaryRegion)
      {
        auto baseLocationRegionID = map::next_id_get(base.regions);
        auto baseFilename = path::to_utf8(base.path.stem());
        auto baseLocationRegionName = baseFilename.empty() ? std::format("#{}", baseId) : baseFilename;
        auto baseLocationRegionPivot =
            regionOrigin == origin::ORIGIN_CENTER ? glm::vec2(baseTextureSize) * 0.5f : glm::vec2();
        base.regions[baseLocationRegionID] = {
            .name = baseLocationRegionName,
            .crop = {},
            .pivot = glm::ivec2(baseLocationRegionPivot),
            .size = baseTextureSize,
            .origin = regionOrigin,
        };
        base.regionOrder.push_back(baseLocationRegionID);
      }

      for (auto id : ids)
      {
        if (id == baseId) continue;

        auto& source = content.spritesheets.at(id);
        auto sheetOffset = offsets.at(id);

        auto locationRegionID = map::next_id_get(base.regions);
        auto sourceFilename = path::to_utf8(source.path.stem());
        auto locationRegionName = sourceFilename.empty() ? std::format("#{}", id) : sourceFilename;
        auto locationRegionPivot =
            regionOrigin == origin::ORIGIN_CENTER ? glm::vec2(source.texture.size) * 0.5f : glm::vec2();
        base.regions[locationRegionID] = {
            .name = locationRegionName,
            .crop = sheetOffset,
            .pivot = glm::ivec2(locationRegionPivot),
            .size = source.texture.size,
            .origin = regionOrigin,
        };
        base.regionOrder.push_back(locationRegionID);

        for (auto& [sourceRegionID, sourceRegion] : source.regions)
        {
          auto destinationRegionID = map::next_id_get(base.regions);
          auto destinationRegion = sourceRegion;
          destinationRegion.crop += sheetOffset;
          base.regions[destinationRegionID] = destinationRegion;
          base.regionOrder.push_back(destinationRegionID);
          regionIdMap[id][sourceRegionID] = destinationRegionID;
        }
      }
    }

    std::unordered_map<int, int> layerSpritesheetBefore{};
    for (auto& [layerID, layer] : content.layers)
    {
      if (!ids.contains(layer.spritesheetID)) continue;
      layerSpritesheetBefore[layerID] = layer.spritesheetID;
      layer.spritesheetID = baseId;
    }

    for (auto& animation : animations.items)
    {
      for (auto& [layerID, item] : animation.layerAnimations)
      {
        if (!layerSpritesheetBefore.contains(layerID)) continue;
        auto sourceSpritesheetID = layerSpritesheetBefore.at(layerID);
        if (sourceSpritesheetID == baseId) continue;

        for (auto& frame : item.frames)
        {
          if (frame.regionID == -1) continue;

          if (isMakeRegions && regionIdMap.contains(sourceSpritesheetID) &&
              regionIdMap.at(sourceSpritesheetID).contains(frame.regionID))
            frame.regionID = regionIdMap.at(sourceSpritesheetID).at(frame.regionID);
          else
            frame.regionID = -1;
        }
      }
    }

    for (auto id : ids)
      if (id != baseId) content.spritesheets.erase(id);

    return true;
  }

  std::vector<std::string> Anm2::spritesheet_labels_get()
  {
    std::vector<std::string> labels{};
    for (auto& [id, spritesheet] : content.spritesheets)
    {
      auto pathString = path::to_utf8(spritesheet.path);
      labels.emplace_back(std::vformat(localize.get(FORMAT_SPRITESHEET), std::make_format_args(id, pathString)));
    }
    return labels;
  }

  std::vector<int> Anm2::spritesheet_ids_get()
  {
    std::vector<int> ids{};
    for (auto& [id, spritesheet] : content.spritesheets)
      ids.emplace_back(id);
    return ids;
  }

  std::vector<std::string> Anm2::region_labels_get(Spritesheet& spritesheet)
  {
    auto rebuild_order = [&]()
    {
      spritesheet.regionOrder.clear();
      spritesheet.regionOrder.reserve(spritesheet.regions.size());
      for (auto id : spritesheet.regions | std::views::keys)
        spritesheet.regionOrder.push_back(id);
    };
    if (spritesheet.regionOrder.size() != spritesheet.regions.size())
      rebuild_order();
    else
    {
      bool isOrderValid = true;
      for (auto id : spritesheet.regionOrder)
        if (!spritesheet.regions.contains(id))
        {
          isOrderValid = false;
          break;
        }
      if (!isOrderValid) rebuild_order();
    }

    std::vector<std::string> labels{};
    labels.emplace_back(localize.get(BASIC_NONE));
    for (auto id : spritesheet.regionOrder)
      labels.emplace_back(spritesheet.regions.at(id).name);
    return labels;
  }

  std::vector<int> Anm2::region_ids_get(Spritesheet& spritesheet)
  {
    auto rebuild_order = [&]()
    {
      spritesheet.regionOrder.clear();
      spritesheet.regionOrder.reserve(spritesheet.regions.size());
      for (auto id : spritesheet.regions | std::views::keys)
        spritesheet.regionOrder.push_back(id);
    };
    if (spritesheet.regionOrder.size() != spritesheet.regions.size())
      rebuild_order();
    else
    {
      bool isOrderValid = true;
      for (auto id : spritesheet.regionOrder)
        if (!spritesheet.regions.contains(id))
        {
          isOrderValid = false;
          break;
        }
      if (!isOrderValid) rebuild_order();
    }

    std::vector<int> ids{};
    ids.emplace_back(-1);
    for (auto id : spritesheet.regionOrder)
      ids.emplace_back(id);
    return ids;
  }

  std::set<int> Anm2::regions_unused(Spritesheet& spritesheet)
  {
    std::set<int> used{};

    for (auto& animation : animations.items)
    {
      for (auto& layerAnimation : animation.layerAnimations | std::views::values)
      {
        for (auto& frame : layerAnimation.frames)
          if (frame.regionID != -1) used.insert(frame.regionID);
      }
    }

    std::set<int> unused{};
    for (auto& id : spritesheet.regions | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  void Anm2::scan_and_set_regions()
  {
    for (auto& animation : animations.items)
    {
      for (auto& [layerID, item] : animation.layerAnimations)
      {
        auto layer = map::find(content.layers, layerID);
        if (!layer) continue;

        auto spritesheet = spritesheet_get(layer->spritesheetID);
        if (!spritesheet || spritesheet->regions.empty()) continue;

        for (auto& frame : item.frames)
        {
          if (frame.regionID != -1) continue;

          auto frameCrop = glm::ivec2(frame.crop);
          auto frameSize = glm::ivec2(frame.size);
          auto framePivot = glm::ivec2(frame.pivot);

          for (auto& [regionID, region] : spritesheet->regions)
          {
            if (glm::ivec2(region.crop) == frameCrop && glm::ivec2(region.size) == frameSize &&
                glm::ivec2(region.pivot) == framePivot)
            {
              frame.regionID = regionID;
              break;
            }
          }
        }
      }
    }
  }

  bool Anm2::spritesheets_deserialize(const std::string& string, const std::filesystem::path& directory,
                                      merge::Type type, std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      int id{};

      if (!document.FirstChildElement("Spritesheet"))
      {
        if (errorString) *errorString = "No valid spritesheet(s).";
        return false;
      }

      WorkingDirectory workingDirectory(directory);

      for (auto element = document.FirstChildElement("Spritesheet"); element;
           element = element->NextSiblingElement("Spritesheet"))
      {
        auto spritesheet = Spritesheet(element, id);
        if (type == merge::APPEND) id = map::next_id_get(content.spritesheets);
        content.spritesheets[id] = std::move(spritesheet);
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }
}
