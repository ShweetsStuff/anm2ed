#include "document.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <set>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <format>

#include "log.hpp"
#include "manager.hpp"
#include "path.hpp"
#include "strings.hpp"
#include "toast.hpp"
#include "working_directory.hpp"

using namespace anm2ed::imgui;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::document
{
  Options save_options_get(Compatibility compatibility, bool isBakeSpecialInterpolatedFrames, bool isRoundScale,
                           bool isRoundRotation)
  {
    return {.compatibility = compatibility,
            .isBakeSpecialInterpolatedFrames = isBakeSpecialInterpolatedFrames,
            .isRoundScale = isRoundScale,
            .isRoundRotation = isRoundRotation};
  }

  ItemType item_type_get(int type) { return static_cast<ItemType>(type); }

  int animation_count_get(const Anm2& data)
  {
    int count{};
    if (auto animations = data.element_get(ElementType::ANIMATIONS))
      for (auto& animation : animations->children)
        if (animation.type == ElementType::ANIMATION) ++count;
    return count;
  }

  int frame_count_get(const Element& item)
  {
    auto frameType = item.type == ElementType::TRIGGERS ? ElementType::TRIGGER : ElementType::FRAME;
    int count{};
    for (auto& frame : item.children)
      if (frame.type == frameType) ++count;
    return count;
  }

  std::vector<std::string> animation_labels_get(const Anm2& data)
  {
    std::vector<std::string> labels{"None"};
    if (auto animations = data.element_get(ElementType::ANIMATIONS))
      for (auto& animation : animations->children)
        if (animation.type == ElementType::ANIMATION) labels.emplace_back(animation.name);
    return labels;
  }

  std::vector<std::string> element_name_labels_get(const Element* container, ElementType type, bool isNone)
  {
    std::vector<std::string> labels{};
    if (isNone) labels.emplace_back(localize.get(BASIC_NONE));
    if (!container) return labels;
    for (auto& element : container->children)
      if (element.type == type) labels.emplace_back(element.name);
    return labels;
  }

  std::vector<int> element_ids_get(const Element* container, ElementType type, bool isNone)
  {
    std::vector<int> ids{};
    if (isNone) ids.emplace_back(-1);
    if (!container) return ids;
    for (auto& element : container->children)
      if (element.type == type) ids.emplace_back(element.id);
    return ids;
  }

  std::vector<std::string> spritesheet_labels_get(const Anm2& data)
  {
    std::vector<std::string> labels{};
    if (auto spritesheets = data.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
        if (spritesheet.type == ElementType::SPRITESHEET)
        {
          auto pathString = path::to_utf8(spritesheet.path);
          labels.emplace_back(
              std::vformat(localize.get(FORMAT_SPRITESHEET), std::make_format_args(spritesheet.id, pathString)));
        }
    return labels;
  }

  std::vector<int> spritesheet_ids_get(const Anm2& data)
  {
    std::vector<int> ids{};
    if (auto spritesheets = data.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
        if (spritesheet.type == ElementType::SPRITESHEET) ids.emplace_back(spritesheet.id);
    return ids;
  }

  std::vector<std::string> sound_labels_get(const Anm2& data)
  {
    std::vector<std::string> labels{localize.get(BASIC_NONE)};
    if (auto sounds = data.element_get(ElementType::SOUNDS))
      for (auto& sound : sounds->children)
        if (sound.type == ElementType::SOUND_ELEMENT)
        {
          auto pathString = path::to_utf8(sound.path);
          labels.emplace_back(std::vformat(localize.get(FORMAT_SOUND), std::make_format_args(sound.id, pathString)));
        }
    return labels;
  }

  std::vector<int> sound_ids_get(const Anm2& data)
  {
    std::vector<int> ids{-1};
    if (auto sounds = data.element_get(ElementType::SOUNDS))
      for (auto& sound : sounds->children)
        if (sound.type == ElementType::SOUND_ELEMENT) ids.emplace_back(sound.id);
    return ids;
  }

  uint64_t spritesheet_hash_get(const Element& spritesheet, const resource::Texture* texture)
  {
    auto hash_combine = [](std::size_t& seed, std::size_t value)
    { seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2); };

    std::size_t seed{};
    hash_combine(seed, std::hash<int>{}(texture ? texture->size.x : 0));
    hash_combine(seed, std::hash<int>{}(texture ? texture->size.y : 0));
    hash_combine(seed, std::hash<int>{}(texture ? texture->channels : 0));
    hash_combine(seed, std::hash<int>{}(texture ? texture->filter : 0));
    hash_combine(seed, std::hash<std::string>{}(path::to_utf8(spritesheet.path)));

    if (texture && !texture->pixels.empty())
    {
      std::string_view bytes(reinterpret_cast<const char*>(texture->pixels.data()), texture->pixels.size());
      hash_combine(seed, std::hash<std::string_view>{}(bytes));
    }
    else
      hash_combine(seed, 0);

    return static_cast<uint64_t>(seed);
  }

  void restored_snapshot_sanitize(Document& document)
  {
    auto& reference = document.reference;
    auto& selection = document.frames.selection;
    auto& frameReferences = document.frames.references;
    auto& itemReferences = document.items.references;

    auto animationCount = animation_count_get(document.anm2);
    if (animationCount <= 0)
    {
      reference = {};
      selection.clear();
      frameReferences.clear();
      itemReferences.clear();
      document.frameTime = 0.0f;
      return;
    }

    if (reference.animationIndex < 0 || reference.animationIndex >= animationCount)
      reference.animationIndex = std::clamp(reference.animationIndex, 0, animationCount - 1);

    auto item =
        document.anm2.element_get(reference.animationIndex, item_type_get(reference.itemType), reference.itemID);
    if (!item)
    {
      reference.itemType = (int)ItemType::ROOT;
      reference.itemID = -1;
      item = document.anm2.element_get(reference.animationIndex, item_type_get(reference.itemType), reference.itemID);
    }

    if (!item)
    {
      reference.frameIndex = -1;
      selection.clear();
      frameReferences.clear();
      document.frameTime = 0.0f;
      return;
    }

    for (auto it = itemReferences.begin(); it != itemReferences.end();)
    {
      auto item = document.anm2.element_get(it->animationIndex, item_type_get(it->itemType), it->itemID);
      if (!item)
        it = itemReferences.erase(it);
      else
        ++it;
    }

    for (auto it = frameReferences.begin(); it != frameReferences.end();)
    {
      auto item = document.anm2.element_get(it->animationIndex, item_type_get(it->itemType), it->itemID);
      auto frameCount = item ? frame_count_get(*item) : 0;
      if (!item || it->frameIndex < 0 || it->frameIndex >= frameCount)
        it = frameReferences.erase(it);
      else
        ++it;
    }

    auto frameCount = frame_count_get(*item);
    for (auto it = selection.begin(); it != selection.end();)
    {
      if (*it < 0 || *it >= frameCount)
        it = selection.erase(it);
      else
        ++it;
    }

    if (frameCount <= 0)
    {
      reference.frameIndex = -1;
      frameReferences.clear();
      document.frameTime = 0.0f;
      return;
    }

    if (reference.frameIndex < 0 || reference.frameIndex >= frameCount)
      reference.frameIndex =
          selection.empty() ? std::clamp(reference.frameIndex, 0, frameCount - 1) : *selection.begin();

    if (frameReferences.empty())
      for (auto frameIndex : selection)
        frameReferences.insert({reference.animationIndex, reference.itemType, reference.itemID, frameIndex});

    selection.clear();
    for (const auto& frameReference : frameReferences)
      if (frameReference.animationIndex == reference.animationIndex && frameReference.itemType == reference.itemType &&
          frameReference.itemID == reference.itemID)
        selection.insert(frameReference.frameIndex);

    document.frameTime = frame_time_from_index_get(*item, reference.frameIndex);
  }
}

namespace anm2ed
{
  Document::Document(const std::filesystem::path& path, bool isNew, std::string* errorString)
  {
    if (isNew)
    {
      anm2 = Anm2();
      if (!save(path, errorString))
      {
        isValid = false;
        this->path.clear();
        return;
      }
    }
    else
    {
      anm2 = Anm2(path, errorString);
      if (!anm2.isValid)
      {
        isValid = false;
        this->path.clear();
        return;
      }
    }

    this->path = path;
    isValid = anm2.isValid;
    clean();
    change(Document::ALL);
  }

  Document::Document(Document&& other) noexcept
      : path(std::move(other.path)), snapshots(std::move(other.snapshots)), current(snapshots.current),
        playback(current.playback), animation(current.animation), event(current.event), frames(current.frames),
        items(current.items), layer(current.layer), merge(current.merge), null(current.null), region(current.region),
        sound(current.sound), spritesheet(current.spritesheet), textures(current.textures), sounds(current.sounds),
        anm2(current.anm2), reference(current.reference), frameTime(current.frameTime), message(current.message),
        regionBySpritesheet(std::move(other.regionBySpritesheet)),
        changeAllFramePropertiesRegionId(other.changeAllFramePropertiesRegionId), previewZoom(other.previewZoom),
        previewPan(other.previewPan), editorPan(other.editorPan), editorZoom(other.editorZoom),
        overlayIndex(other.overlayIndex), hash(other.hash), saveHash(other.saveHash), autosaveHash(other.autosaveHash),
        lastAutosaveTime(other.lastAutosaveTime), isValid(other.isValid), isOpen(other.isOpen),
        isForceDirty(other.isForceDirty), spritesheetHashes(std::move(other.spritesheetHashes)),
        spritesheetSaveHashes(std::move(other.spritesheetSaveHashes)), texturePaths(std::move(other.texturePaths)),
        soundPaths(std::move(other.soundPaths)), isAnimationPreviewSet(other.isAnimationPreviewSet),
        isSpritesheetEditorSet(other.isSpritesheetEditorSet)
  {
  }

  Document& Document::operator=(Document&& other) noexcept
  {
    if (this != &other) this->~Document();
    new (this) Document(std::move(other));
    return *this;
  }

  bool Document::save(const std::filesystem::path& path, std::string* errorString, Compatibility compatibility,
                      bool isBakeSpecialInterpolatedFramesOnSave, bool isRoundScale, bool isRoundRotation)
  {
    auto absolutePath = !path.empty() ? path : this->path;
    auto absolutePathUtf8 = path::to_utf8(absolutePath);
    if (anm2.save(absolutePath, errorString,
                  document::save_options_get(compatibility, isBakeSpecialInterpolatedFramesOnSave, isRoundScale,
                                             isRoundRotation)))
    {
      this->path = absolutePath;
      toasts.push(std::vformat(localize.get(TOAST_SAVE_DOCUMENT), std::make_format_args(absolutePathUtf8)));
      logger.info(
          std::vformat(localize.get(TOAST_SAVE_DOCUMENT, anm2ed::ENGLISH), std::make_format_args(absolutePathUtf8)));
      clean();
      return true;
    }
    else if (errorString)
    {
      toasts.push(std::vformat(localize.get(TOAST_SAVE_DOCUMENT_FAILED),
                               std::make_format_args(absolutePathUtf8, *errorString)));
      logger.error(std::vformat(localize.get(TOAST_SAVE_DOCUMENT_FAILED, anm2ed::ENGLISH),
                                std::make_format_args(absolutePathUtf8, *errorString)));
    }

    return false;
  }

  std::filesystem::path Document::autosave_path_get()
  {
    auto fileNameUtf8 = path::to_utf8(filename_get());
    auto autosaveNameUtf8 = "." + fileNameUtf8 + ".autosave";
    return directory_get() / path::from_utf8(autosaveNameUtf8);
  }

  std::filesystem::path Document::path_from_autosave_get(const std::filesystem::path& path)
  {
    auto fileName = path::to_utf8(path.filename());
    if (!fileName.empty() && fileName.front() == '.') fileName.erase(fileName.begin());
    constexpr std::string_view autosaveExtension = ".autosave";
    if (fileName.ends_with(autosaveExtension)) fileName.erase(fileName.size() - autosaveExtension.size());

    auto restorePath = path.parent_path() / std::filesystem::path(std::u8string(fileName.begin(), fileName.end()));

    return restorePath;
  }

  bool Document::autosave(std::string* errorString, Compatibility compatibility,
                          bool isBakeSpecialInterpolatedFramesOnSave, bool isRoundScale, bool isRoundRotation)
  {
    auto autosavePath = autosave_path_get();
    auto autosavePathUtf8 = path::to_utf8(autosavePath);
    if (anm2.save(autosavePath, errorString,
                  document::save_options_get(compatibility, isBakeSpecialInterpolatedFramesOnSave, isRoundScale,
                                             isRoundRotation)))
    {
      autosaveHash = hash;
      lastAutosaveTime = 0.0f;
      toasts.push(localize.get(TOAST_AUTOSAVING));
      logger.info(localize.get(TOAST_AUTOSAVING, anm2ed::ENGLISH));
      logger.info(std::format("Autosaved document to: {}", autosavePathUtf8));
      return true;
    }
    else if (errorString)
    {
      toasts.push(
          std::vformat(localize.get(TOAST_AUTOSAVE_FAILED), std::make_format_args(autosavePathUtf8, *errorString)));
      logger.error(std::vformat(localize.get(TOAST_AUTOSAVE_FAILED, anm2ed::ENGLISH),
                                std::make_format_args(autosavePathUtf8, *errorString)));
    }

    return false;
  }

  void Document::anm2_change(ChangeType type) { change(type); }

  void Document::texture_change(int id)
  {
    auto texture = texture_get(id);
    if (!texture || !anm2.element_get(ElementType::SPRITESHEET, id)) return;
    change(SPRITESHEETS);
  }

  void Document::assets_sync(ChangeType type)
  {
    if (type == ALL || type == SPRITESHEETS || type == TEXTURES)
    {
      std::set<int> validIds{};
      util::WorkingDirectory workingDirectory(directory_get());
      if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
        for (auto& spritesheet : spritesheets->children)
        {
          if (spritesheet.type != ElementType::SPRITESHEET) continue;
          validIds.insert(spritesheet.id);
          auto isReload = !textures.contains(spritesheet.id) || !texturePaths.contains(spritesheet.id) ||
                          texturePaths.at(spritesheet.id) != spritesheet.path;
          if (isReload)
          {
            textures[spritesheet.id] = resource::Texture(spritesheet.path);
            texturePaths[spritesheet.id] = spritesheet.path;
          }
        }

      for (auto it = textures.begin(); it != textures.end();)
      {
        if (!validIds.contains(it->first))
        {
          texturePaths.erase(it->first);
          it = textures.erase(it);
        }
        else
          ++it;
      }
    }

    if (type == ALL || type == SOUNDS)
    {
      std::set<int> validIds{};
      util::WorkingDirectory workingDirectory(directory_get());
      if (auto soundItems = anm2.element_get(ElementType::SOUNDS))
        for (auto& sound : soundItems->children)
        {
          if (sound.type != ElementType::SOUND_ELEMENT) continue;
          validIds.insert(sound.id);
          auto isReload =
              !sounds.contains(sound.id) || !soundPaths.contains(sound.id) || soundPaths.at(sound.id) != sound.path;
          if (isReload)
          {
            sounds[sound.id] = resource::Audio(sound.path);
            soundPaths[sound.id] = sound.path;
          }
        }

      for (auto it = sounds.begin(); it != sounds.end();)
      {
        if (!validIds.contains(it->first))
        {
          soundPaths.erase(it->first);
          it = sounds.erase(it);
        }
        else
          ++it;
      }
    }
  }

  resource::Texture* Document::texture_get(int id)
  {
    auto it = textures.find(id);
    return it == textures.end() ? nullptr : &it->second;
  }

  const resource::Texture* Document::texture_get(int id) const
  {
    auto it = textures.find(id);
    return it == textures.end() ? nullptr : &it->second;
  }

  resource::Audio* Document::sound_get(int id)
  {
    auto it = sounds.find(id);
    return it == sounds.end() ? nullptr : &it->second;
  }

  const resource::Audio* Document::sound_get(int id) const
  {
    auto it = sounds.find(id);
    return it == sounds.end() ? nullptr : &it->second;
  }

  bool Document::regions_trim(int spritesheetId, const std::set<int>& ids)
  {
    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, spritesheetId);
    auto texture = texture_get(spritesheetId);
    if (!spritesheet || !texture || !texture->is_valid() || texture->pixels.empty() || ids.empty()) return false;

    bool isChanged{};
    for (auto id : ids)
    {
      auto region = element_child_id_get(*spritesheet, ElementType::REGION, id);
      if (!region) continue;

      auto minPoint = glm::ivec2(glm::min(region->crop, region->crop + region->size));
      auto maxPoint = glm::ivec2(glm::max(region->crop, region->crop + region->size));
      int minX = std::max(0, minPoint.x);
      int minY = std::max(0, minPoint.y);
      int maxX = std::min(texture->size.x, maxPoint.x);
      int maxY = std::min(texture->size.y, maxPoint.y);
      if (minX >= maxX || minY >= maxY) continue;

      int contentMinX = std::numeric_limits<int>::max();
      int contentMinY = std::numeric_limits<int>::max();
      int contentMaxX = std::numeric_limits<int>::min();
      int contentMaxY = std::numeric_limits<int>::min();

      for (int y = minY; y < maxY; ++y)
      {
        for (int x = minX; x < maxX; ++x)
        {
          auto index = ((std::size_t)y * texture->size.x + x) * resource::texture::CHANNELS;
          if (index + resource::texture::CHANNELS > texture->pixels.size()) continue;
          auto r = texture->pixels[index + 0];
          auto g = texture->pixels[index + 1];
          auto b = texture->pixels[index + 2];
          auto a = texture->pixels[index + 3];
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
      if (region->crop == newCrop && region->size == newSize) continue;

      auto previousCrop = region->crop;
      region->crop = newCrop;
      region->size = newSize;
      if (region->origin == Origin::TOP_LEFT)
        region->pivot = {};
      else if (region->origin == Origin::CENTER)
        region->pivot = {static_cast<int>(region->size.x / 2.0f), static_cast<int>(region->size.y / 2.0f)};
      else
        region->pivot -= region->crop - previousCrop;
      isChanged = true;
    }

    return isChanged;
  }

  bool Document::spritesheet_pack(int id, int padding)
  {
    struct RectI
    {
      int x{};
      int y{};
      int w{};
      int h{};
    };

    struct PackItem
    {
      int regionId{-1};
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
        for (int i = 0; i < (int)freeRects.size(); ++i)
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
              freeRects.erase(freeRects.begin() + i--);
              break;
            }
            else
              ++j;
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
        bool isFound{};
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
            isFound = true;
          }
        }
        if (!isFound) return false;
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
      std::int64_t totalArea{};
      for (auto& item : items)
      {
        maxWidth = std::max(maxWidth, item.packWidth);
        maxHeight = std::max(maxHeight, item.packHeight);
        sumWidth += item.packWidth;
        sumHeight += item.packHeight;
        totalArea += (std::int64_t)item.packWidth * item.packHeight;
      }
      if (maxWidth <= 0 || maxHeight <= 0) return false;

      int bestSquareDelta = std::numeric_limits<int>::max();
      int bestArea = std::numeric_limits<int>::max();
      std::unordered_map<int, RectI> bestRects{};
      int bestWidth{};
      int bestHeight{};
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
        for (int candidateHeight = candidateHeightMin; candidateHeight <= sumHeight; ++candidateHeight)
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
            candidateRects[item.regionId] = rect;
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

    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, id);
    auto texture = texture_get(id);
    if (!spritesheet || !texture || !texture->is_valid() || texture->pixels.empty()) return false;

    auto packingPadding = std::max(0, padding);
    std::vector<PackItem> items{};
    for (auto& region : spritesheet->children)
    {
      if (region.type != ElementType::REGION) continue;
      auto minPoint = glm::ivec2(glm::min(region.crop, region.crop + region.size));
      auto maxPoint = glm::ivec2(glm::max(region.crop, region.crop + region.size));
      auto size = glm::max(maxPoint - minPoint, glm::ivec2(1));
      items.push_back({region.id, minPoint.x, minPoint.y, size.x, size.y, size.x + packingPadding * 2,
                       size.y + packingPadding * 2});
    }
    if (items.empty()) return false;

    std::sort(items.begin(), items.end(),
              [](const PackItem& a, const PackItem& b)
              {
                int areaA = a.width * a.height;
                int areaB = b.width * b.height;
                if (areaA != areaB) return areaA > areaB;
                return a.regionId < b.regionId;
              });

    int packedWidth{};
    int packedHeight{};
    std::unordered_map<int, RectI> packedRects{};
    if (!pack_regions(items, packedWidth, packedHeight, packedRects)) return false;
    if (packedWidth <= 0 || packedHeight <= 0) return false;

    std::vector<uint8_t> packedPixels((std::size_t)packedWidth * packedHeight * resource::texture::CHANNELS, 0);
    for (auto& item : items)
    {
      if (!packedRects.contains(item.regionId)) continue;
      auto destinationRect = packedRects.at(item.regionId);
      for (int y = 0; y < item.height; ++y)
        for (int x = 0; x < item.width; ++x)
        {
          int sourceX = item.srcX + x;
          int sourceY = item.srcY + y;
          int destinationX = destinationRect.x + packingPadding + x;
          int destinationY = destinationRect.y + packingPadding + y;
          if (sourceX < 0 || sourceY < 0 || sourceX >= texture->size.x || sourceY >= texture->size.y) continue;
          if (destinationX < 0 || destinationY < 0 || destinationX >= packedWidth || destinationY >= packedHeight)
            continue;
          auto sourceIndex = ((std::size_t)sourceY * texture->size.x + sourceX) * resource::texture::CHANNELS;
          auto destinationIndex =
              ((std::size_t)destinationY * packedWidth + destinationX) * resource::texture::CHANNELS;
          std::copy_n(texture->pixels.data() + sourceIndex, resource::texture::CHANNELS,
                      packedPixels.data() + destinationIndex);
        }
    }

    textures[id] = resource::Texture(packedPixels.data(), {packedWidth, packedHeight});
    for (auto& region : spritesheet->children)
      if (region.type == ElementType::REGION && packedRects.contains(region.id))
      {
        auto& rect = packedRects.at(region.id);
        region.crop = {rect.x + packingPadding, rect.y + packingPadding};
      }

    assets_sync(SPRITESHEETS);
    return true;
  }

  bool Document::spritesheets_merge(const std::set<int>& ids, bool isAppendRight, bool isMakeRegions,
                                    bool isMakePrimaryRegion, origin::Type regionOrigin)
  {
    if (ids.size() < 2) return false;
    auto baseId = *ids.begin();
    auto base = anm2.element_get(ElementType::SPRITESHEET, baseId);
    auto baseTexture = texture_get(baseId);
    if (!base || !baseTexture || !baseTexture->is_valid()) return false;
    for (auto id : ids)
      if (!anm2.element_get(ElementType::SPRITESHEET, id) || !texture_get(id) || !texture_get(id)->is_valid())
        return false;

    auto origin = regionOrigin == origin::ORIGIN_CENTER ? Origin::CENTER : Origin::TOP_LEFT;
    auto baseTextureSize = baseTexture->size;
    auto mergedTexture = *baseTexture;
    std::unordered_map<int, glm::ivec2> offsets{{baseId, {}}};

    for (auto id : ids)
    {
      if (id == baseId) continue;
      auto texture = texture_get(id);
      offsets[id] = isAppendRight ? glm::ivec2(mergedTexture.size.x, 0) : glm::ivec2(0, mergedTexture.size.y);
      mergedTexture = resource::Texture::merge_append(mergedTexture, *texture, isAppendRight);
    }
    textures[baseId] = std::move(mergedTexture);

    std::unordered_map<int, std::unordered_map<int, int>> regionIdMap{};
    if (isMakeRegions)
    {
      auto add_location_region = [&](int sourceId, glm::ivec2 crop, glm::ivec2 size)
      {
        auto source = anm2.element_get(ElementType::SPRITESHEET, sourceId);
        if (!source) return;
        auto region = element_make(ElementType::REGION);
        region.id = element_child_next_id_get(*base, ElementType::REGION);
        auto stem = path::to_utf8(source->path.stem());
        region.name = stem.empty() ? std::format("#{}", sourceId) : stem;
        region.crop = crop;
        region.size = size;
        region.pivot = origin == Origin::CENTER ? glm::vec2(size) * 0.5f : glm::vec2();
        region.origin = origin;
        base->children.push_back(region);
      };

      if (isMakePrimaryRegion) add_location_region(baseId, {}, baseTextureSize);
      for (auto id : ids)
      {
        if (id == baseId) continue;
        auto source = anm2.element_get(ElementType::SPRITESHEET, id);
        auto sourceTexture = texture_get(id);
        auto sheetOffset = offsets.at(id);
        add_location_region(id, sheetOffset, sourceTexture->size);
        for (auto& sourceRegion : source->children)
        {
          if (sourceRegion.type != ElementType::REGION) continue;
          auto destinationRegion = sourceRegion;
          destinationRegion.id = element_child_next_id_get(*base, ElementType::REGION);
          destinationRegion.crop += sheetOffset;
          base->children.push_back(destinationRegion);
          regionIdMap[id][sourceRegion.id] = destinationRegion.id;
        }
      }
    }

    std::unordered_map<int, int> layerSpritesheetBefore{};
    if (auto layers = anm2.element_get(ElementType::LAYERS))
      for (auto& layer : layers->children)
        if (layer.type == ElementType::LAYER_ELEMENT && ids.contains(layer.spritesheetId))
        {
          layerSpritesheetBefore[layer.id] = layer.spritesheetId;
          layer.spritesheetId = baseId;
        }

    if (auto animations = anm2.element_get(ElementType::ANIMATIONS))
      for (auto& animation : animations->children)
      {
        if (animation.type != ElementType::ANIMATION) continue;
        auto layerAnimations = element_child_first_get(animation, ElementType::LAYER_ANIMATIONS);
        if (!layerAnimations) continue;
        auto layer_animation_update = [&](auto&& self, Element& layerAnimation) -> void
        {
          if (layerAnimation.type == ElementType::GROUP)
          {
            for (auto& child : layerAnimation.children)
              self(self, child);
            return;
          }
          if (layerAnimation.type != ElementType::LAYER_ANIMATION ||
              !layerSpritesheetBefore.contains(layerAnimation.layerId))
            return;
          auto sourceSpritesheetId = layerSpritesheetBefore.at(layerAnimation.layerId);
          if (sourceSpritesheetId == baseId) return;
          for (auto& frame : layerAnimation.children)
          {
            if (frame.type != ElementType::FRAME || frame.regionId == -1) continue;
            if (isMakeRegions && regionIdMap.contains(sourceSpritesheetId) &&
                regionIdMap.at(sourceSpritesheetId).contains(frame.regionId))
              frame.regionId = regionIdMap.at(sourceSpritesheetId).at(frame.regionId);
            else
              frame.regionId = -1;
          }
        };
        for (auto& layerAnimation : layerAnimations->children)
          layer_animation_update(layer_animation_update, layerAnimation);
      }

    auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS);
    if (!spritesheets) return false;
    for (auto id : ids)
      if (id != baseId)
      {
        element_child_id_erase(*spritesheets, ElementType::SPRITESHEET, id);
        textures.erase(id);
      }

    assets_sync(ALL);
    return true;
  }

  void Document::scan_and_set_regions()
  {
    auto animations = anm2.element_get(ElementType::ANIMATIONS);
    if (animations)
    {
      for (auto& animation : animations->children)
      {
        if (animation.type != ElementType::ANIMATION) continue;
        auto layerAnimations = element_child_first_get(animation, ElementType::LAYER_ANIMATIONS);
        if (!layerAnimations) continue;
        auto layer_animation_update = [&](auto&& self, Element& layerAnimation) -> void
        {
          if (layerAnimation.type == ElementType::GROUP)
          {
            for (auto& child : layerAnimation.children)
              self(self, child);
            return;
          }
          if (layerAnimation.type != ElementType::LAYER_ANIMATION) return;
          auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, layerAnimation.layerId);
          auto spritesheet = layer ? anm2.element_get(ElementType::SPRITESHEET, layer->spritesheetId) : nullptr;
          if (!spritesheet) return;
          for (auto& frame : layerAnimation.children)
          {
            if (frame.type != ElementType::FRAME || frame.regionId != -1) continue;
            auto frameCrop = glm::ivec2(frame.crop);
            auto frameSize = glm::ivec2(frame.size);
            auto framePivot = glm::ivec2(frame.pivot);
            for (const auto& region : spritesheet->children)
              if (region.type == ElementType::REGION && glm::ivec2(region.crop) == frameCrop &&
                  glm::ivec2(region.size) == frameSize && glm::ivec2(region.pivot) == framePivot)
              {
                frame.regionId = region.id;
                break;
              }
          }
        };
        for (auto& layerAnimation : layerAnimations->children)
          layer_animation_update(layer_animation_update, layerAnimation);
      }
    }
  }

  bool Document::file_merge(const std::filesystem::path& path)
  {
    Anm2 source(path);
    if (!source.isValid) return false;

    auto remap_path = [&](const std::filesystem::path& original) -> std::filesystem::path
    {
      if (directory_get().empty()) return original;
      std::error_code ec{};
      std::filesystem::path absolute{};
      bool isAbsolute{};

      if (!original.empty())
      {
        if (original.is_absolute())
        {
          absolute = original;
          isAbsolute = true;
        }
        else
        {
          absolute = std::filesystem::weakly_canonical(path.parent_path() / original, ec);
          if (ec)
          {
            ec.clear();
            absolute = path.parent_path() / original;
          }
          isAbsolute = true;
        }
      }

      if (!isAbsolute) return original;
      auto relative = std::filesystem::relative(absolute, directory_get(), ec);
      if (!ec) return relative;
      return original.empty() ? absolute : original;
    };

    auto remap_id = [](const std::unordered_map<int, int>& remap, int id)
    {
      auto it = remap.find(id);
      return it == remap.end() ? -1 : it->second;
    };

    auto find_by_name = [](Element& container, ElementType type, const std::string& name)
    {
      for (auto& child : container.children)
        if (child.type == type && child.name == name) return &child;
      return (Element*)nullptr;
    };

    auto child_set = [](Element& container, Element child, ElementType type)
    {
      if (auto existing = element_child_first_get(container, type))
        *existing = std::move(child);
      else
        container.children.push_back(std::move(child));
    };

    auto track_find = [](Element& container, const Element& source)
    {
      auto find = [](auto&& self, Element& parent, const Element& source) -> Element*
      {
        for (auto& track : parent.children)
        {
          if (track.type == ElementType::GROUP)
            if (auto result = self(self, track, source)) return result;
          if (track.type != source.type) continue;
          if (track.type == ElementType::LAYER_ANIMATION && track.layerId == source.layerId) return &track;
          if (track.type == ElementType::NULL_ANIMATION && track.nullId == source.nullId) return &track;
        }
        return (Element*)nullptr;
      };
      return find(find, container, source);
    };

    auto track_container_get = [](Element& animation, ElementType type)
    {
      if (auto container = element_child_first_get(animation, type)) return container;
      animation.children.push_back(element_make(type));
      return &animation.children.back();
    };

    std::unordered_map<int, int> spritesheetRemap{};
    std::unordered_map<int, int> layerRemap{};
    std::unordered_map<int, int> nullRemap{};
    std::unordered_map<int, int> eventRemap{};
    std::unordered_map<int, int> soundRemap{};

    if (auto sourceSpritesheets = source.element_get(ElementType::SPRITESHEETS))
      if (auto destinationSpritesheets = anm2.element_get(ElementType::SPRITESHEETS))
        for (auto spritesheet : sourceSpritesheets->children)
        {
          if (spritesheet.type != ElementType::SPRITESHEET) continue;
          auto sourceId = spritesheet.id;
          spritesheet.id = element_child_next_id_get(*destinationSpritesheets, ElementType::SPRITESHEET);
          spritesheet.path = remap_path(spritesheet.path);
          destinationSpritesheets->children.push_back(spritesheet);
          spritesheetRemap[sourceId] = spritesheet.id;
        }

    if (auto sourceSounds = source.element_get(ElementType::SOUNDS))
      if (auto destinationSounds = anm2.element_get(ElementType::SOUNDS))
        for (auto sound : sourceSounds->children)
        {
          if (sound.type != ElementType::SOUND_ELEMENT) continue;
          auto sourceId = sound.id;
          sound.path = remap_path(sound.path);
          int destinationId{-1};
          for (auto& existing : destinationSounds->children)
            if (existing.type == ElementType::SOUND_ELEMENT && existing.path == sound.path)
            {
              destinationId = existing.id;
              sound.id = destinationId;
              existing = sound;
              break;
            }
          if (destinationId == -1)
          {
            destinationId = element_child_next_id_get(*destinationSounds, ElementType::SOUND_ELEMENT);
            sound.id = destinationId;
            destinationSounds->children.push_back(sound);
          }
          soundRemap[sourceId] = destinationId;
        }

    if (auto sourceLayers = source.element_get(ElementType::LAYERS))
      if (auto destinationLayers = anm2.element_get(ElementType::LAYERS))
        for (auto layer : sourceLayers->children)
        {
          if (layer.type != ElementType::LAYER_ELEMENT) continue;
          auto sourceId = layer.id;
          layer.spritesheetId = remap_id(spritesheetRemap, layer.spritesheetId);
          if (auto existing = find_by_name(*destinationLayers, ElementType::LAYER_ELEMENT, layer.name))
          {
            layer.id = existing->id;
            *existing = layer;
          }
          else
          {
            layer.id = element_child_next_id_get(*destinationLayers, ElementType::LAYER_ELEMENT);
            destinationLayers->children.push_back(layer);
          }
          layerRemap[sourceId] = layer.id;
        }

    if (auto sourceNulls = source.element_get(ElementType::NULLS))
      if (auto destinationNulls = anm2.element_get(ElementType::NULLS))
        for (auto null : sourceNulls->children)
        {
          if (null.type != ElementType::NULL_ELEMENT) continue;
          auto sourceId = null.id;
          if (auto existing = find_by_name(*destinationNulls, ElementType::NULL_ELEMENT, null.name))
          {
            null.id = existing->id;
            *existing = null;
          }
          else
          {
            null.id = element_child_next_id_get(*destinationNulls, ElementType::NULL_ELEMENT);
            destinationNulls->children.push_back(null);
          }
          nullRemap[sourceId] = null.id;
        }

    if (auto sourceEvents = source.element_get(ElementType::EVENTS))
      if (auto destinationEvents = anm2.element_get(ElementType::EVENTS))
        for (auto event : sourceEvents->children)
        {
          if (event.type != ElementType::EVENT_ELEMENT) continue;
          auto sourceId = event.id;
          if (auto existing = find_by_name(*destinationEvents, ElementType::EVENT_ELEMENT, event.name))
          {
            event.id = existing->id;
            *existing = event;
          }
          else
          {
            event.id = element_child_next_id_get(*destinationEvents, ElementType::EVENT_ELEMENT);
            destinationEvents->children.push_back(event);
          }
          eventRemap[sourceId] = event.id;
        }

    auto item_remap = [&](Element& item)
    {
      for (auto& frame : item.children)
      {
        if (frame.type != ElementType::FRAME && frame.type != ElementType::TRIGGER) continue;
        for (auto& soundId : frame.soundIds)
          soundId = remap_id(soundRemap, soundId);
        frame.eventId = remap_id(eventRemap, frame.eventId);
      }
    };

    auto track_tree_remap = [&](auto&& self, Element item, int itemType, Element& container, int parentGroupId = -1) -> void
    {
      auto trackType = itemType == LAYER ? ElementType::LAYER_ANIMATION : ElementType::NULL_ANIMATION;
      if (item.type == ElementType::GROUP)
      {
        auto group = element_make(ElementType::GROUP);
        group.id = item.id == -1 ? element_child_next_id_get(container, ElementType::GROUP) : item.id;
        group.name = item.name;
        group.isExpanded = item.isExpanded;
        container.children.push_back(group);
        for (auto child : item.children)
          self(self, child, itemType, container, group.id);
        return;
      }

      if (item.type != trackType) return;
      if (itemType == LAYER)
        item.layerId = remap_id(layerRemap, item.layerId);
      else
        item.nullId = remap_id(nullRemap, item.nullId);
      if ((itemType == LAYER && item.layerId < 0) || (itemType == NULL_ && item.nullId < 0)) return;
      if (parentGroupId != -1) item.groupId = parentGroupId;
      item_remap(item);
      container.children.push_back(item);
    };

    auto animation_build = [&](const Element& incoming)
    {
      auto animation = element_make(ElementType::ANIMATION);
      animation.name = incoming.name;
      animation.frameNum = incoming.frameNum;
      animation.isLoop = incoming.isLoop;

      if (auto root = element_child_first_get(incoming, ElementType::ROOT_ANIMATION))
      {
        auto item = *root;
        item_remap(item);
        animation.children.push_back(item);
      }

      if (auto layerAnimations = element_child_first_get(incoming, ElementType::LAYER_ANIMATIONS))
      {
        auto container = element_make(ElementType::LAYER_ANIMATIONS);
        for (auto item : layerAnimations->children)
          track_tree_remap(track_tree_remap, item, LAYER, container);
        animation.children.push_back(container);
      }

      if (auto nullAnimations = element_child_first_get(incoming, ElementType::NULL_ANIMATIONS))
      {
        auto container = element_make(ElementType::NULL_ANIMATIONS);
        for (auto item : nullAnimations->children)
          track_tree_remap(track_tree_remap, item, NULL_, container);
        animation.children.push_back(container);
      }

      if (auto triggers = element_child_first_get(incoming, ElementType::TRIGGERS))
      {
        auto item = *triggers;
        item_remap(item);
        animation.children.push_back(item);
      }

      return animation;
    };

    auto track_merge = [&](Element& destinationContainer, const Element& sourceTrack)
    {
      if (auto destinationTrack = track_find(destinationContainer, sourceTrack))
      {
        if (!sourceTrack.children.empty()) *destinationTrack = sourceTrack;
      }
      else
        destinationContainer.children.push_back(sourceTrack);
    };

    auto track_container_merge = [&](Element& destinationContainer, const Element& sourceContainer, ElementType trackType)
    {
      std::unordered_map<int, int> groupRemap{};
      for (auto item : sourceContainer.children)
      {
        if (item.type != ElementType::GROUP) continue;
        auto sourceGroupId = item.id;
        item.id = element_child_next_id_get(destinationContainer, ElementType::GROUP);
        item.children.clear();
        destinationContainer.children.push_back(item);
        groupRemap[sourceGroupId] = item.id;
      }

      for (auto item : sourceContainer.children)
      {
        if (item.type != trackType) continue;
        if (item.groupId != -1)
          item.groupId = groupRemap.contains(item.groupId) ? groupRemap.at(item.groupId) : -1;
        track_merge(destinationContainer, item);
      }
    };

    if (auto sourceAnimations = source.element_get(ElementType::ANIMATIONS))
      if (auto destinationAnimations = anm2.element_get(ElementType::ANIMATIONS))
      {
        for (const auto& incoming : sourceAnimations->children)
        {
          if (incoming.type != ElementType::ANIMATION) continue;
          auto processed = animation_build(incoming);
          auto destination = find_by_name(*destinationAnimations, ElementType::ANIMATION, processed.name);
          if (!destination)
          {
            destinationAnimations->children.push_back(processed);
            continue;
          }

          destination->frameNum = std::max(destination->frameNum, processed.frameNum);
          destination->isLoop = processed.isLoop;
          if (auto root = element_child_first_get(processed, ElementType::ROOT_ANIMATION);
              root && !root->children.empty())
            child_set(*destination, *root, ElementType::ROOT_ANIMATION);
          if (auto triggers = element_child_first_get(processed, ElementType::TRIGGERS);
              triggers && !triggers->children.empty())
            child_set(*destination, *triggers, ElementType::TRIGGERS);

          if (auto layerAnimations = element_child_first_get(processed, ElementType::LAYER_ANIMATIONS))
          {
            auto destinationLayerAnimations = track_container_get(*destination, ElementType::LAYER_ANIMATIONS);
            track_container_merge(*destinationLayerAnimations, *layerAnimations, ElementType::LAYER_ANIMATION);
          }

          if (auto nullAnimations = element_child_first_get(processed, ElementType::NULL_ANIMATIONS))
          {
            auto destinationNullAnimations = track_container_get(*destination, ElementType::NULL_ANIMATIONS);
            track_container_merge(*destinationNullAnimations, *nullAnimations, ElementType::NULL_ANIMATION);
          }

          destination->frameNum = std::max(destination->frameNum, animation_length_get(*destination));
        }

        if (destinationAnimations->defaultAnimation.empty() && !sourceAnimations->defaultAnimation.empty())
          destinationAnimations->defaultAnimation = sourceAnimations->defaultAnimation;
      }

    assets_sync(ALL);
    return true;
  }

  void Document::hash_set() { hash = anm2.hash(); }

  void Document::clean()
  {
    assets_sync();
    saveHash = anm2.hash();
    hash = saveHash;
    lastAutosaveTime = 0.0f;
    isForceDirty = false;
  }

  void Document::spritesheet_hashes_reset()
  {
    spritesheetHashes.clear();
    spritesheetSaveHashes.clear();
    if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
        if (spritesheet.type == ElementType::SPRITESHEET)
        {
          auto currentHash = document::spritesheet_hash_get(spritesheet, texture_get(spritesheet.id));
          spritesheetHashes[spritesheet.id] = currentHash;
          spritesheetSaveHashes[spritesheet.id] = currentHash;
        }
  }

  void Document::spritesheet_hashes_sync()
  {
    std::set<int> validIds{};
    if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
        if (spritesheet.type == ElementType::SPRITESHEET) validIds.insert(spritesheet.id);

    for (auto it = spritesheetHashes.begin(); it != spritesheetHashes.end();)
    {
      if (!validIds.contains(it->first))
        it = spritesheetHashes.erase(it);
      else
        ++it;
    }

    for (auto it = spritesheetSaveHashes.begin(); it != spritesheetSaveHashes.end();)
    {
      if (!validIds.contains(it->first))
        it = spritesheetSaveHashes.erase(it);
      else
        ++it;
    }

    if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
        if (spritesheet.type == ElementType::SPRITESHEET)
        {
          auto currentHash = document::spritesheet_hash_get(spritesheet, texture_get(spritesheet.id));
          spritesheetHashes[spritesheet.id] = currentHash;
          if (!spritesheetSaveHashes.contains(spritesheet.id)) spritesheetSaveHashes[spritesheet.id] = currentHash;
        }
  }

  void Document::change(ChangeType type)
  {
    hash_set();
    assets_sync(type);

    auto events_set = [&]()
    {
      auto events = anm2.element_get(ElementType::EVENTS);
      event.labels_set(document::element_name_labels_get(events, ElementType::EVENT_ELEMENT, true),
                       document::element_ids_get(events, ElementType::EVENT_ELEMENT, true));
    };

    auto animations_set = [&]() { animation.labels_set(document::animation_labels_get(anm2)); };

    auto spritesheets_set = [&]()
    {
      spritesheet.labels_set(document::spritesheet_labels_get(anm2), document::spritesheet_ids_get(anm2));
      spritesheet_hashes_sync();
    };

    auto sounds_set = [&]() { sound.labels_set(document::sound_labels_get(anm2), document::sound_ids_get(anm2)); };

    auto regions_set = [&]()
    {
      regionBySpritesheet.clear();
      if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
        for (auto& spritesheet : spritesheets->children)
          if (spritesheet.type == ElementType::SPRITESHEET)
          {
            Storage storage{};
            storage.labels_set(document::element_name_labels_get(&spritesheet, ElementType::REGION, true),
                               document::element_ids_get(&spritesheet, ElementType::REGION, true));
            regionBySpritesheet.emplace(spritesheet.id, std::move(storage));
          }
    };

    switch (type)
    {
      case LAYERS:
        break;
      case NULLS:
        break;
      case EVENTS:
        events_set();
        break;
      case SPRITESHEETS:
        spritesheets_set();
        regions_set();
        break;
      case SOUNDS:
        sounds_set();
        break;
      case FRAMES:
        events_set();
        sounds_set();
        break;
      case ITEMS:
        spritesheets_set();
        break;
      case ANIMATIONS:
      case ALL:
        events_set();
        spritesheets_set();
        regions_set();
        animations_set();
        sounds_set();
        break;
      default:
        break;
    }
  }

  bool Document::is_dirty() const { return hash != saveHash; }
  bool Document::is_autosave_dirty() const { return hash != autosaveHash; }
  void Document::spritesheet_hash_update(int id)
  {
    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, id);
    if (!spritesheet) return;
    assets_sync(TEXTURES);
    spritesheetHashes[id] = document::spritesheet_hash_get(*spritesheet, texture_get(id));
  }

  void Document::spritesheet_hash_set_saved(int id)
  {
    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, id);
    if (!spritesheet) return;
    assets_sync(TEXTURES);
    auto currentHash = document::spritesheet_hash_get(*spritesheet, texture_get(id));
    spritesheetHashes[id] = currentHash;
    spritesheetSaveHashes[id] = currentHash;
  }

  bool Document::spritesheet_is_dirty(int id)
  {
    if (!anm2.element_get(ElementType::SPRITESHEET, id)) return false;
    if (!spritesheetHashes.contains(id)) spritesheet_hash_update(id);
    auto saveIt = spritesheetSaveHashes.find(id);
    if (saveIt == spritesheetSaveHashes.end()) return false;
    return spritesheetHashes.at(id) != saveIt->second;
  }

  bool Document::spritesheet_any_dirty()
  {
    if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
        if (spritesheet.type == ElementType::SPRITESHEET && spritesheet_is_dirty(spritesheet.id)) return true;
    return false;
  }
  std::filesystem::path Document::directory_get() const { return path.parent_path(); }
  std::filesystem::path Document::filename_get() const { return path.filename(); }
  bool Document::is_valid() const { return isValid && !path.empty(); }

  void Document::command_run(Manager& manager, Command& command)
  {
    if (command.run) command.run(manager, *this);
  }

  Element* Document::frame_get()
  {
    return anm2.element_get(reference.animationIndex, document::item_type_get(reference.itemType), reference.frameIndex,
                            reference.itemID);
  }

  Element* Document::item_get()
  {
    return anm2.element_get(reference.animationIndex, document::item_type_get(reference.itemType), reference.itemID);
  }
  Element* Document::animation_get() { return anm2.element_get(ElementType::ANIMATION, reference.animationIndex); }
  Element* Document::spritesheet_get() { return anm2.element_get(ElementType::SPRITESHEET, spritesheet.reference); }

  void Document::spritesheet_add(const std::filesystem::path& path)
  {
    spritesheets_add({path});
  }

  void Document::spritesheets_add(const std::vector<std::filesystem::path>& paths)
  {
    struct LoadedSpritesheet
    {
      std::filesystem::path path{};
      resource::Texture texture{};
    };

    auto items = anm2.element_get(ElementType::SPRITESHEETS);
    if (!items) return;

    std::vector<LoadedSpritesheet> loaded{};
    for (auto& path : paths)
    {
      auto pathCopy = path;
      WorkingDirectory workingDirectory(directory_get());
      auto loadPath = path::lower_case_backslash_handle(path);
      auto texture = resource::Texture(loadPath);
      if (!texture.is_valid())
      {
        auto pathUtf8 = path::to_utf8(pathCopy);
        toasts.push(std::vformat(localize.get(TOAST_SPRITESHEET_INIT_FAILED), std::make_format_args(pathUtf8)));
        logger.error(std::vformat(localize.get(TOAST_SPRITESHEET_INIT_FAILED, anm2ed::ENGLISH),
                                  std::make_format_args(pathUtf8)));
        continue;
      }

      loaded.push_back({.path = pathCopy, .texture = std::move(texture)});
    }
    if (loaded.empty()) return;

    snapshot(localize.get(EDIT_ADD_SPRITESHEET));

    std::set<int> added{};
    for (auto& loadedSpritesheet : loaded)
    {
      auto id = element_child_next_id_get(*items, ElementType::SPRITESHEET);
      auto spritesheet = element_make(ElementType::SPRITESHEET);
      spritesheet.id = id;
      spritesheet.path = path::lower_case_backslash_handle(path::make_relative(loadedSpritesheet.path));
      auto pathString = path::to_utf8(spritesheet.path);
      items->children.push_back(spritesheet);
      textures[id] = std::move(loadedSpritesheet.texture);
      texturePaths[id] = spritesheet.path;
      added.insert(id);
      this->spritesheet.reference = id;
      spritesheet_hash_set_saved(id);
      toasts.push(std::vformat(localize.get(TOAST_SPRITESHEET_INITIALIZED), std::make_format_args(id, pathString)));
      logger.info(std::vformat(localize.get(TOAST_SPRITESHEET_INITIALIZED, anm2ed::ENGLISH),
                               std::make_format_args(id, pathString)));
    }
    this->spritesheet.selection = added;
    change(Document::SPRITESHEETS);
  }

  void Document::sound_add(const std::filesystem::path& path)
  {
    sounds_add({path});
  }

  void Document::sounds_add(const std::vector<std::filesystem::path>& paths)
  {
    auto items = anm2.element_get(ElementType::SOUNDS);
    if (!items) return;

    std::vector<std::filesystem::path> validPaths{};
    for (auto& path : paths)
      if (!path.empty()) validPaths.push_back(path);
    if (validPaths.empty()) return;

    snapshot(localize.get(EDIT_ADD_SOUND));

    std::set<int> added{};
    for (auto& path : validPaths)
    {
      auto id = element_child_next_id_get(*items, ElementType::SOUND_ELEMENT);
      auto soundElement = element_make(ElementType::SOUND_ELEMENT);
      soundElement.id = id;
      {
        WorkingDirectory workingDirectory(directory_get());
        soundElement.path = path::lower_case_backslash_handle(path::make_relative(path));
        sounds[id] = resource::Audio(soundElement.path);
        soundPaths[id] = soundElement.path;
      }
      auto soundPath = path::to_utf8(soundElement.path);
      items->children.push_back(soundElement);
      added.insert(id);
      sound.reference = id;
      toasts.push(std::vformat(localize.get(TOAST_SOUND_INITIALIZED), std::make_format_args(id, soundPath)));
      logger.info(
          std::vformat(localize.get(TOAST_SOUND_INITIALIZED, anm2ed::ENGLISH), std::make_format_args(id, soundPath)));
    }
    sound.selection = added;
    change(Document::SOUNDS);
  }

  void Document::snapshot(const std::string& message)
  {
    this->message = message;
    snapshots.push(current);
  }

  void Document::undo()
  {
    if (!snapshots.undo()) return;
    document::restored_snapshot_sanitize(*this);
    toasts.push(std::vformat(localize.get(TOAST_UNDO), std::make_format_args(message)));
    logger.info(std::vformat(localize.get(TOAST_UNDO, anm2ed::ENGLISH), std::make_format_args(message)));
    change(Document::ALL);
  }

  void Document::redo()
  {
    if (!snapshots.redo()) return;
    document::restored_snapshot_sanitize(*this);
    toasts.push(std::vformat(localize.get(TOAST_REDO), std::make_format_args(message)));
    logger.info(std::vformat(localize.get(TOAST_REDO, anm2ed::ENGLISH), std::make_format_args(message)));
    change(Document::ALL);
  }

  bool Document::is_able_to_undo() { return !snapshots.undoStack.is_empty(); }
  bool Document::is_able_to_redo() { return !snapshots.redoStack.is_empty(); }
}
