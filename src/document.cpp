#include "document.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <limits>
#include <new>
#include <optional>
#include <set>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "file.hpp"
#include "log.hpp"
#include "manager.hpp"
#include "pack.hpp"
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
  constexpr std::string_view AUTOSAVE_EXTENSION = ".autosave";
  constexpr std::uint64_t HASH_COMBINE_CONSTANT = 0x9e3779b97f4a7c15ULL;

  uint64_t document_tab_id_next()
  {
    static uint64_t next{1};
    return next++;
  }

  std::filesystem::path shader_absolute_path_get(Document& document, const std::filesystem::path& path)
  {
    auto loadPath = path::backslash_handle(path);
    if (loadPath.empty() || loadPath.is_absolute()) return loadPath;
    return document.directory_get() / loadPath;
  }

  void shader_status_append(const resource::ShaderCompileResult& result, std::string* status)
  {
    if (!status) return;

    *status += localize.get(result.isCompiled ? LABEL_SHADER_COMPILE_SUCCEEDED : LABEL_SHADER_COMPILE_FAILED);
    *status += "\n";
    if (result.isCompiled)
    {
      *status += localize.get(result.isLinked ? LABEL_SHADER_LINK_SUCCEEDED : LABEL_SHADER_LINK_FAILED);
      *status += "\n";
    }
    if (!result.output.empty()) *status += result.output;
  }

  std::string shader_source_lines_get(std::string_view label, std::string_view source)
  {
    std::string output = std::format("\n{}:\n", label);
    int lineNumber{1};
    std::size_t position{};
    while (position <= source.size())
    {
      auto end = source.find('\n', position);
      auto isEnd = end == std::string_view::npos;
      output += std::format("{:4}: {}\n", lineNumber++,
                            source.substr(position, isEnd ? std::string_view::npos : end - position));
      if (isEnd) break;
      position = end + 1;
    }
    return output;
  }

  void shader_status_format_append(std::string* status, StringType format, StringType argument)
  {
    if (!status) return;
    auto argumentString = std::string(localize.get(argument));
    *status += std::vformat(localize.get(format), std::make_format_args(argumentString)) + "\n";
  }

  bool shader_source_load(Document& document, const std::filesystem::path& shaderPath, std::string& source,
                          StringType label, std::string* status, const char* fallback = nullptr)
  {
    if (shaderPath.empty() && !fallback)
    {
      shader_status_format_append(status, LABEL_SHADER_PATH_EMPTY, label);
      return false;
    }
    if (shaderPath.empty())
    {
      source = fallback;
      return true;
    }

    auto absolute = path::case_insensitive_find(shader_absolute_path_get(document, shaderPath));
    if (file::read_to_string(absolute, &source)) return true;

    if (status)
    {
      auto pathString = path::to_utf8(shaderPath);
      *status += std::vformat(localize.get(LABEL_SHADER_READ_FAILED), std::make_format_args(pathString)) + "\n";
    }
    return false;
  }

  template <class Label>
  void storage_labels_set(Storage& storage, const Element* container, ElementType type, bool isNone, Label&& label_get)
  {
    std::vector<std::string> labels{};
    std::vector<int> ids{};
    if (isNone)
    {
      labels.emplace_back(localize.get(BASIC_NONE));
      ids.emplace_back(-1);
    }
    if (container)
      for (auto& element : container->children)
        if (element.type == type)
        {
          labels.emplace_back(label_get(element));
          ids.emplace_back(element.id);
        }
    storage.labels_set(std::move(labels), std::move(ids));
  }

  std::string element_name_get(const Element& element) { return element.name; }

  // Loads assets for elements that are new or whose path changed, and forgets those of removed elements.
  template <class Load>
  void resources_sync(Document& document, ElementType type, std::map<int, std::uint64_t>& keys,
                      std::unordered_map<int, std::filesystem::path>& paths, Load&& load)
  {
    std::set<int> validIds{};
    util::WorkingDirectory workingDirectory(document.directory_get());
    if (auto container = document.anm2.element_get(ELEMENT_CONTAINERS[(int)type]))
      for (auto& element : container->children)
      {
        if (element.type != type) continue;
        validIds.insert(element.id);
        auto path = paths.find(element.id);
        if (!keys.contains(element.id) || path == paths.end() || path->second != element.path)
        {
          load(element.id, path::case_insensitive_find(element.path));
          paths[element.id] = element.path;
        }
      }

    std::erase_if(keys, [&](const auto& pair) { return !validIds.contains(pair.first); });
    std::erase_if(paths, [&](const auto& pair) { return !validIds.contains(pair.first); });
  }

  // A spritesheet is dirty when its path or image asset differs from what was last saved.
  uint64_t spritesheet_hash_get(const Element& spritesheet, std::uint64_t imageKey)
  {
    auto seed = std::hash<std::string>{}(path::to_utf8(spritesheet.path));
    return seed ^ (imageKey + HASH_COMBINE_CONSTANT + (seed << 6) + (seed >> 2));
  }

  Reference item_reference_get(Reference reference)
  {
    reference.frameIndex = -1;
    return reference;
  }

  void frame_time_sync(Document& document)
  {
    auto reference = document.reference_get();
    auto item = document.anm2.element_get(item_reference_get(reference));
    auto frameIndex = reference.frameIndex;
    document.frameTime = item && frameIndex >= 0 ? frame_time_from_index_get(*item, frameIndex) : 0.0f;
  }
}

namespace anm2ed
{
  Document::Document(const std::filesystem::path& path, bool isNew, std::string* errorString)
  {
    tabId = document::document_tab_id_next();
    if (!isNew) anm2 = Anm2(path, errorString);
    if (isNew ? !save(path, errorString) : !anm2.isValid)
    {
      isValid = false;
      this->path.clear();
      return;
    }

    this->path = path;
    isValid = anm2.isValid;
    clean();
    change();
  }

  Document::Document(Document&& other) noexcept : DocumentData(std::move(other)), editTarget(other.editTarget) {}

  Document& Document::operator=(Document&& other) noexcept
  {
    if (this != &other) this->~Document();
    new (this) Document(std::move(other));
    return *this;
  }

  bool Document::save(const std::filesystem::path& path, std::string* errorString, Options options)
  {
    auto absolutePath = !path.empty() ? path : this->path;
    auto absolutePathUtf8 = path::to_utf8(absolutePath);
    if (anm2.save(absolutePath, errorString, options))
    {
      this->path = absolutePath;
      toast_log(Level::INFO, TOAST_SAVE_DOCUMENT, absolutePathUtf8);
      clean();
      return true;
    }
    if (errorString) toast_log(Level::ERROR, TOAST_SAVE_DOCUMENT_FAILED, absolutePathUtf8, *errorString);
    return false;
  }

  std::filesystem::path Document::autosave_path_get()
  {
    return directory_get() /
           path::from_utf8("." + path::to_utf8(filename_get()) + std::string(document::AUTOSAVE_EXTENSION));
  }

  std::filesystem::path Document::path_from_autosave_get(const std::filesystem::path& path)
  {
    auto fileName = path::to_utf8(path.filename());
    if (fileName.starts_with('.')) fileName.erase(fileName.begin());
    if (fileName.ends_with(document::AUTOSAVE_EXTENSION))
      fileName.erase(fileName.size() - document::AUTOSAVE_EXTENSION.size());
    return path.parent_path() / std::filesystem::path(std::u8string(fileName.begin(), fileName.end()));
  }

  bool Document::autosave(std::string* errorString, Options options)
  {
    auto autosavePath = autosave_path_get();
    auto autosavePathUtf8 = path::to_utf8(autosavePath);
    if (anm2.save(autosavePath, errorString, options))
    {
      autosaveHash = hash;
      lastAutosaveTime = 0.0f;
      toast_log(Level::INFO, TOAST_AUTOSAVING);
      logger.info(std::format("Autosaved document to: {}", autosavePathUtf8));
      return true;
    }
    if (errorString) toast_log(Level::ERROR, TOAST_AUTOSAVE_FAILED, autosavePathUtf8, *errorString);
    return false;
  }

  void Document::texture_change(int id)
  {
    if (texture_get(id) && anm2.element_get(ElementType::SPRITESHEET, id)) change();
  }

  bool Document::texture_reload(int id)
  {
    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, id);
    if (!spritesheet) return false;
    util::WorkingDirectory workingDirectory(directory_get());
    texture_set(id, resource::Image(path::case_insensitive_find(spritesheet->path)));
    texturePaths[id] = spritesheet->path;
    return true;
  }

  bool Document::sound_reload(int id)
  {
    auto sound = anm2.element_get(ElementType::SOUND_ELEMENT, id);
    if (!sound) return false;
    util::WorkingDirectory workingDirectory(directory_get());
    sound_set(id, resource::AudioData(path::case_insensitive_find(sound->path)));
    soundPaths[id] = sound->path;
    return true;
  }

  bool Document::shader_reload(int shaderId, std::string* status)
  {
    auto shader_erase = [&]()
    {
      shaders.erase(shaderId);
      shaderVertexPaths.erase(shaderId);
      shaderFragmentPaths.erase(shaderId);
      return false;
    };

    auto shaderElement = anm2.element_get(ElementType::SHADER, shaderId);
    if (!shaderElement)
    {
      document::shader_status_format_append(status, LABEL_SHADER_PATH_EMPTY, LABEL_FRAGMENT);
      return shader_erase();
    }

    std::string vertexSource{};
    std::string fragmentSource{};
    auto isVertexLoaded = document::shader_source_load(*this, shaderElement->vertex, vertexSource, LABEL_VERTEX, status,
                                                       resource::shader::TEXTURE_COMPATIBILITY_VERTEX);
    auto isFragmentLoaded =
        document::shader_source_load(*this, shaderElement->fragment, fragmentSource, LABEL_FRAGMENT, status);
    if (!isVertexLoaded || !isFragmentLoaded) return shader_erase();

    auto vertex = resource::shader::gles_vertex_convert(vertexSource);
    auto fragment = resource::shader::gles_fragment_convert(fragmentSource);
    auto result = resource::shader_compile(vertex.c_str(), fragment.c_str());
    document::shader_status_append(result, status);
    if (status && (!result.isCompiled || !result.isLinked))
    {
      *status += document::shader_source_lines_get("Converted vertex shader", vertex);
      *status += document::shader_source_lines_get("Converted fragment shader", fragment);
    }
    if (!result.isLinked || !result.id) return shader_erase();

    resource::Shader runtime{};
    runtime.id = result.id;
    runtime.uniforms = std::move(result.uniforms);
    if (resource::shader::uniform_configs_trim(*shaderElement, runtime.uniforms)) hash_set();
    resource::shader::uniform_configs_apply(*shaderElement, runtime.uniforms);
    shaders[shaderId] = std::move(runtime);
    shaderVertexPaths[shaderId] = shaderElement->vertex;
    shaderFragmentPaths[shaderId] = shaderElement->fragment;
    return true;
  }

  void Document::assets_sync()
  {
    document::resources_sync(*this, ElementType::SPRITESHEET, textures, texturePaths,
                             [&](int id, const std::filesystem::path& path)
                             { texture_set(id, resource::Image(path)); });
    document::resources_sync(*this, ElementType::SOUND_ELEMENT, sounds, soundPaths,
                             [&](int id, const std::filesystem::path& path)
                             { sound_set(id, resource::AudioData(path)); });

    std::set<int> validShaderIds{};
    util::WorkingDirectory workingDirectory(directory_get());
    if (auto shaderElements = anm2.element_get(ElementType::SHADERS))
      for (auto& shaderElement : shaderElements->children)
      {
        if (shaderElement.type != ElementType::SHADER) continue;
        auto id = shaderElement.id;
        validShaderIds.insert(id);
        auto isReload = !shaders.contains(id) || !shaderVertexPaths.contains(id) || !shaderFragmentPaths.contains(id) ||
                        shaderVertexPaths.at(id) != shaderElement.vertex ||
                        shaderFragmentPaths.at(id) != shaderElement.fragment;
        if (shaderElement.fragment.empty())
        {
          shaders.erase(id);
          shaderVertexPaths.erase(id);
          shaderFragmentPaths.erase(id);
        }
        else if (isReload)
          shader_reload(id);
        else if (auto shader = shader_get(id))
          resource::shader::uniform_configs_apply(shaderElement, shader->uniforms);
      }

    auto is_invalid = [&](const auto& pair) { return !validShaderIds.contains(pair.first); };
    std::erase_if(shaders, is_invalid);
    std::erase_if(shaderVertexPaths, is_invalid);
    std::erase_if(shaderFragmentPaths, is_invalid);
  }

  const resource::Image* Document::texture_get(int id) const
  {
    if (auto draft = textureDrafts.find(id); draft != textureDrafts.end()) return &draft->second;
    auto key = textures.find(id);
    return key == textures.end() ? nullptr : assets.image_get(key->second);
  }

  // A private copy of the spritesheet's image to draw on; it becomes a new asset on the next change().
  resource::Image* Document::texture_edit(int id)
  {
    auto texture = texture_get(id);
    if (!texture) return nullptr;
    return &textureDrafts.try_emplace(id, *texture).first->second;
  }

  void Document::texture_set(int id, resource::Image image)
  {
    textureDrafts.erase(id);
    textures[id] = assets.image_add(std::move(image));
  }

  const resource::AudioData* Document::sound_get(int id) const
  {
    auto key = sounds.find(id);
    return key == sounds.end() ? nullptr : assets.audio_get(key->second);
  }

  void Document::sound_set(int id, resource::AudioData data) { sounds[id] = assets.audio_add(std::move(data)); }

  resource::Shader* Document::shader_get(int shaderId)
  {
    auto it = shaders.find(shaderId);
    return it == shaders.end() || !it->second.is_valid() ? nullptr : &it->second;
  }

  bool Document::regions_trim(int spritesheetId, const std::set<int>& ids)
  {
    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, spritesheetId);
    auto texture = texture_get(spritesheetId);
    if (!spritesheet || !texture || !texture->is_valid() || texture->pixels.empty()) return false;

    bool isChanged{};
    for (auto id : ids)
    {
      auto region = child_id_get(*spritesheet, ElementType::REGION, id);
      if (!region) continue;

      auto minPoint = glm::max(glm::ivec2(glm::min(region->crop, region->crop + region->size)), glm::ivec2(0));
      auto maxPoint = glm::min(glm::ivec2(glm::max(region->crop, region->crop + region->size)), texture->size);
      auto contentMin = glm::ivec2(std::numeric_limits<int>::max());
      auto contentMax = glm::ivec2(std::numeric_limits<int>::min());

      for (int y = minPoint.y; y < maxPoint.y; ++y)
        for (int x = minPoint.x; x < maxPoint.x; ++x)
        {
          auto index = ((std::size_t)y * texture->size.x + x) * resource::image::CHANNELS;
          if (index + resource::image::CHANNELS > texture->pixels.size()) continue;
          if (std::all_of(texture->pixels.begin() + index, texture->pixels.begin() + index + resource::image::CHANNELS,
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

  bool Document::spritesheet_pack(int id, int padding)
  {
    struct PackItem
    {
      int regionId{-1};
      glm::ivec2 source{};
      glm::ivec2 size{};
    };

    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, id);
    auto texture = texture_get(id);
    if (!spritesheet || !texture || !texture->is_valid() || texture->pixels.empty()) return false;

    padding = std::max(0, padding);
    std::vector<PackItem> items{};
    for (auto& region : spritesheet->children)
    {
      if (region.type != ElementType::REGION) continue;
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
    if (!util::pack::rects_pack(sizes, packedSize, positions) || packedSize.x <= 0 || packedSize.y <= 0) return false;

    std::vector<uint8_t> packedPixels((std::size_t)packedSize.x * packedSize.y * resource::image::CHANNELS, 0);
    std::unordered_map<int, glm::ivec2> crops{};
    for (int i = 0; i < (int)items.size(); ++i)
    {
      auto& item = items[i];
      auto destination = positions[i] + padding;
      crops[item.regionId] = destination;
      for (int y = 0; y < item.size.y; ++y)
        for (int x = 0; x < item.size.x; ++x)
        {
          auto source = item.source + glm::ivec2(x, y);
          auto target = destination + glm::ivec2(x, y);
          if (glm::any(glm::lessThan(source, glm::ivec2(0))) ||
              glm::any(glm::greaterThanEqual(source, texture->size)) ||
              glm::any(glm::greaterThanEqual(target, packedSize)))
            continue;
          std::copy_n(
              texture->pixels.data() + ((std::size_t)source.y * texture->size.x + source.x) * resource::image::CHANNELS,
              resource::image::CHANNELS,
              packedPixels.data() + ((std::size_t)target.y * packedSize.x + target.x) * resource::image::CHANNELS);
        }
    }

    texture_set(id, resource::Image(packedPixels.data(), packedSize));
    for (auto& region : spritesheet->children)
      if (region.type == ElementType::REGION && crops.contains(region.id)) region.crop = crops.at(region.id);

    assets_sync();
    return true;
  }

  bool Document::spritesheets_merge(const std::set<int>& ids, bool isAppendRight, bool isMakeRegions,
                                    bool isMakePrimaryRegion, origin::Type regionOrigin)
  {
    if (ids.size() < 2) return false;
    for (auto id : ids)
      if (!anm2.element_get(ElementType::SPRITESHEET, id) || !texture_get(id) || !texture_get(id)->is_valid())
        return false;

    auto baseId = *ids.begin();
    auto base = anm2.element_get(ElementType::SPRITESHEET, baseId);
    auto origin = regionOrigin == origin::ORIGIN_CENTER ? Origin::CENTER : Origin::TOP_LEFT;
    auto mergedTexture = *texture_get(baseId);
    std::unordered_map<int, std::unordered_map<int, int>> regionIdMap{};

    auto location_region_add = [&](int sourceId, glm::ivec2 crop, glm::ivec2 size)
    {
      auto source = anm2.element_get(ElementType::SPRITESHEET, sourceId);
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

    if (isMakeRegions && isMakePrimaryRegion) location_region_add(baseId, {}, mergedTexture.size);
    for (auto id : ids)
    {
      if (id == baseId) continue;
      auto texture = texture_get(id);
      auto offset = isAppendRight ? glm::ivec2(mergedTexture.size.x, 0) : glm::ivec2(0, mergedTexture.size.y);
      mergedTexture = resource::Image::merge_append(mergedTexture, *texture, isAppendRight);
      if (!isMakeRegions) continue;

      location_region_add(id, offset, texture->size);
      for (auto sourceRegion : anm2.element_get(ElementType::SPRITESHEET, id)->children)
      {
        if (sourceRegion.type != ElementType::REGION) continue;
        auto sourceRegionId = sourceRegion.id;
        sourceRegion.id = element_child_next_id_get(*base, ElementType::REGION);
        sourceRegion.crop += offset;
        base->children.push_back(sourceRegion);
        regionIdMap[id][sourceRegionId] = sourceRegion.id;
      }
    }
    texture_set(baseId, std::move(mergedTexture));

    std::unordered_map<int, int> layerSpritesheetBefore{};
    if (auto layers = anm2.element_get(ElementType::LAYERS))
      for (auto& layer : layers->children)
        if (layer.type == ElementType::LAYER_ELEMENT && ids.contains(layer.spritesheetId))
        {
          layerSpritesheetBefore[layer.id] = layer.spritesheetId;
          layer.spritesheetId = baseId;
        }

    animations_tracks_each(anm2.root, ElementType::LAYER_ANIMATION,
                           [&](Element& track)
                           {
                             auto before = layerSpritesheetBefore.find(track.layerId);
                             if (before == layerSpritesheetBefore.end() || before->second == baseId) return;
                             auto& remap = regionIdMap[before->second];
                             for (auto& frame : track.children)
                               if (frame.type == ElementType::FRAME && frame.regionId != -1)
                                 frame.regionId = remap.contains(frame.regionId) ? remap.at(frame.regionId) : -1;
                           });

    auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS);
    for (auto id : ids)
      if (id != baseId)
      {
        element_child_id_erase(*spritesheets, ElementType::SPRITESHEET, id);
        textures.erase(id);
      }

    assets_sync();
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
    spritesheet_hashes_sync();
  }

  void Document::spritesheet_hashes_sync()
  {
    std::set<int> validIds{};
    if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
        if (spritesheet.type == ElementType::SPRITESHEET)
        {
          validIds.insert(spritesheet.id);
          auto currentHash = document::spritesheet_hash_get(
              spritesheet, textures.contains(spritesheet.id) ? textures.at(spritesheet.id) : 0);
          spritesheetHashes[spritesheet.id] = currentHash;
          spritesheetSaveHashes.try_emplace(spritesheet.id, currentHash);
        }

    auto is_invalid = [&](const auto& pair) { return !validIds.contains(pair.first); };
    std::erase_if(spritesheetHashes, is_invalid);
    std::erase_if(spritesheetSaveHashes, is_invalid);
  }

  // Commits the pending edit and rebuilds everything derived from the model.
  void Document::change()
  {
    for (auto& [id, draft] : std::exchange(textureDrafts, {}))
      textures[id] = assets.image_add(std::move(draft));
    hash_set();
    assets_sync();

    auto events_set = [&]()
    {
      document::storage_labels_set(event, anm2.element_get(ElementType::EVENTS), ElementType::EVENT_ELEMENT, true,
                                   document::element_name_get);
    };

    auto spritesheets_set = [&]()
    {
      document::storage_labels_set(
          spritesheet, anm2.element_get(ElementType::SPRITESHEETS), ElementType::SPRITESHEET, false,
          [](const Element& element)
          {
            auto pathString = path::to_utf8(element.path);
            return std::vformat(localize.get(FORMAT_SPRITESHEET), std::make_format_args(element.id, pathString));
          });
      spritesheet_hashes_sync();
    };

    auto sounds_set = [&]()
    {
      document::storage_labels_set(sound, anm2.element_get(ElementType::SOUNDS), ElementType::SOUND_ELEMENT, true,
                                   [](const Element& element)
                                   {
                                     auto pathString = path::to_utf8(element.path);
                                     return std::vformat(localize.get(FORMAT_SOUND),
                                                         std::make_format_args(element.id, pathString));
                                   });
    };

    auto regions_set = [&]()
    {
      regionBySpritesheet.clear();
      if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
        for (auto& element : spritesheets->children)
          if (element.type == ElementType::SPRITESHEET)
            document::storage_labels_set(regionBySpritesheet[element.id], &element, ElementType::REGION, true,
                                         document::element_name_get);
    };

    auto shaders_set = [&]()
    {
      document::storage_labels_set(shader, anm2.element_get(ElementType::SHADERS), ElementType::SHADER, true,
                                   document::element_name_get);
    };

    events_set();
    spritesheets_set();
    regions_set();
    shaders_set();
    sounds_set();

    snapshots.commit();
    index = UidIndex(anm2);
  }

  void Document::edit_begin(StringType label) { snapshots.push(localize.get(label)); }

  edit::Uids Document::edit_run(StringType label, const std::function<edit::Uids(Anm2&)>& operation)
  {
    edit_begin(label);
    auto uids = operation(anm2);
    change();
    return uids;
  }

  std::vector<Reference> Document::references_get(const edit::Uids& uids) const
  {
    UidIndex index(anm2);
    std::vector<Reference> references{};
    for (auto uid : uids)
      if (auto reference = index.reference_get(uid)) references.push_back(*reference);
    return references;
  }

  bool Document::is_dirty() const { return hash != saveHash; }
  bool Document::is_autosave_dirty() const { return hash != autosaveHash; }

  void Document::spritesheet_hash_update(int id)
  {
    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, id);
    if (!spritesheet) return;
    assets_sync();
    spritesheetHashes[id] = document::spritesheet_hash_get(*spritesheet, textures.contains(id) ? textures.at(id) : 0);
  }

  void Document::spritesheet_hash_set_saved(int id)
  {
    spritesheet_hash_update(id);
    if (spritesheetHashes.contains(id)) spritesheetSaveHashes[id] = spritesheetHashes[id];
  }

  bool Document::spritesheet_is_dirty(int id)
  {
    if (!anm2.element_get(ElementType::SPRITESHEET, id)) return false;
    if (!spritesheetHashes.contains(id)) spritesheet_hash_update(id);
    auto saveIt = spritesheetSaveHashes.find(id);
    return saveIt != spritesheetSaveHashes.end() && spritesheetHashes.at(id) != saveIt->second;
  }

  bool Document::spritesheet_any_dirty()
  {
    auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS);
    return spritesheets &&
           std::ranges::any_of(
               spritesheets->children, [&](const Element& spritesheet)
               { return spritesheet.type == ElementType::SPRITESHEET && spritesheet_is_dirty(spritesheet.id); });
  }

  std::filesystem::path Document::directory_get() const { return path.parent_path(); }
  std::filesystem::path Document::filename_get() const { return path.filename(); }
  bool Document::is_valid() const { return isValid && !path.empty(); }

  void Document::command_run(Manager& manager, Command& command)
  {
    if (command.run) command.run(manager, *this);
  }

  Reference Document::reference_get() const { return selection_focus_get(selection, index); }

  void Document::reference_set(Reference reference) { selection_focus_set(selection, anm2, reference); }

  std::set<Reference> Document::selected_get(SelectionKind kind) const
  {
    return selection_references_get(selection, index, kind);
  }

  void Document::selected_set(SelectionKind kind, const std::set<Reference>& references)
  {
    selection_references_set(selection, anm2, kind, references);
  }

  std::set<int> Document::animations_selected_get() const
  {
    std::set<int> indices{};
    for (auto reference : selected_get(SelectionKind::ANIMATIONS))
      indices.insert(reference.animationIndex);
    return indices;
  }

  void Document::animations_selected_set(const std::set<int>& indices)
  {
    std::set<Reference> references{};
    for (auto index : indices)
      references.insert({index});
    selected_set(SelectionKind::ANIMATIONS, references);
  }

  std::set<Reference> Document::item_frame_references_get(Reference itemReference) const
  {
    std::set<Reference> result{};
    itemReference.frameIndex = -1;
    auto item = itemReference.itemType == NONE ? nullptr : anm2.element_get(itemReference);
    for (int frameIndex = 0; item && frameIndex < track_frames_count_get(*item); ++frameIndex)
    {
      itemReference.frameIndex = frameIndex;
      result.insert(itemReference);
    }
    return result;
  }

  std::set<Reference> Document::selected_item_frame_references_get() const
  {
    auto selectedItems = selected_get(SelectionKind::TRACKS);
    if (auto reference = reference_get(); selectedItems.empty() && reference.itemType != NONE)
      selectedItems.insert(document::item_reference_get(reference));

    std::set<Reference> result{};
    for (auto itemReference : selectedItems)
      result.merge(item_frame_references_get(itemReference));
    return result;
  }

  std::set<Reference> Document::frame_references_get(FrameReferenceFallback fallback) const
  {
    auto result = selected_get(SelectionKind::FRAMES);
    auto reference = reference_get();
    if (result.empty() && fallback == FrameReferenceFallback::CURRENT && reference.frameIndex >= 0)
      result.insert(reference);
    return result;
  }

  // Selects frames together with their tracks; the focus moves to the first frame unless it is already selected.
  void Document::frame_references_set(std::set<Reference> frameReferences)
  {
    std::erase_if(frameReferences,
                  [&](const Reference& frame) { return frame.frameIndex < 0 || !anm2.element_get(frame); });
    std::set<Reference> tracks{};
    for (auto frame : frameReferences)
      tracks.insert(document::item_reference_get(frame));
    selected_set(SelectionKind::FRAMES, frameReferences);
    selected_set(SelectionKind::TRACKS, tracks);
    if (!frameReferences.empty() && !frameReferences.contains(reference_get())) reference_set(*frameReferences.begin());
  }

  void Document::frame_references_clear() { selection.uids[SelectionKind::FRAMES].clear(); }

  std::vector<Reference> Document::layer_references_get()
  {
    auto reference = reference_get();
    std::set<Reference> selectedReferences = selected_get(SelectionKind::TRACKS);
    if (selectedReferences.empty() && reference.itemType != NONE)
      selectedReferences.insert(document::item_reference_get(reference));

    std::vector<Reference> result{};
    for (auto itemReference : selectedReferences)
    {
      if (itemReference.itemType != LAYER) return {};
      result.push_back(itemReference);
    }
    return result;
  }

  Storage* Document::layer_regions_get(int layerId)
  {
    auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, layerId);
    auto regions = layer ? regionBySpritesheet.find(layer->spritesheetId) : regionBySpritesheet.end();
    return regions == regionBySpritesheet.end() ? nullptr : &regions->second;
  }

  Element* Document::frame_get()
  {
    auto reference = reference_get();
    return reference.frameIndex < 0 ? nullptr : anm2.element_get(reference);
  }
  Element* Document::item_get() { return anm2.element_get(document::item_reference_get(reference_get())); }
  Element* Document::spritesheet_get() { return anm2.element_get(ElementType::SPRITESHEET, spritesheet.reference); }

  void Document::spritesheets_add(const std::vector<std::filesystem::path>& paths)
  {
    auto items = anm2.element_get(ElementType::SPRITESHEETS);
    if (!items) return;
    auto directory = directory_get();

    std::vector<std::pair<std::filesystem::path, resource::Image>> loaded{};
    for (auto& path : paths)
    {
      auto storagePath = path::backslash_handle(path);
      std::optional<WorkingDirectory> workingDirectory{};
      if (!storagePath.is_absolute()) workingDirectory.emplace(directory);
      auto texture = resource::Image(path::case_insensitive_find(storagePath));
      if (!texture.is_valid())
      {
        toast_log(Level::ERROR, TOAST_SPRITESHEET_INIT_FAILED, path::to_utf8(path));
        continue;
      }
      loaded.emplace_back(path::backslash_replace(path::make_relative(storagePath, directory)), std::move(texture));
    }
    if (loaded.empty()) return;

    edit_begin(EDIT_ADD_SPRITESHEET);

    std::set<int> added{};
    for (auto& [relativePath, texture] : loaded)
    {
      auto& element = items->children.emplace_back(element_make(ElementType::SPRITESHEET));
      element.id = element_child_next_id_get(*items, ElementType::SPRITESHEET);
      element.path = relativePath;
      texture_set(element.id, std::move(texture));
      texturePaths[element.id] = element.path;
      added.insert(element.id);
      spritesheet.reference = element.id;
      spritesheet_hash_set_saved(element.id);
      toast_log(Level::INFO, TOAST_SPRITESHEET_INITIALIZED, element.id, path::to_utf8(element.path));
    }
    spritesheet.selection = added;
    change();
  }

  void Document::sounds_add(const std::vector<std::filesystem::path>& paths)
  {
    auto items = anm2.element_get(ElementType::SOUNDS);
    if (!items || std::ranges::all_of(paths, [](const auto& path) { return path.empty(); })) return;

    edit_begin(EDIT_ADD_SOUND);

    std::set<int> added{};
    for (auto& path : paths)
    {
      if (path.empty()) continue;
      auto& element = items->children.emplace_back(element_make(ElementType::SOUND_ELEMENT));
      element.id = element_child_next_id_get(*items, ElementType::SOUND_ELEMENT);
      element.path = path::backslash_replace(path::make_relative(path::backslash_handle(path), directory_get()));
      WorkingDirectory workingDirectory(directory_get());
      sound_set(element.id, resource::AudioData(path::case_insensitive_find(element.path)));
      soundPaths[element.id] = element.path;
      added.insert(element.id);
      sound.reference = element.id;
      toast_log(Level::INFO, TOAST_SOUND_INITIALIZED, element.id, path::to_utf8(element.path));
    }
    sound.selection = added;
    change();
  }

  void Document::undo()
  {
    if (!snapshots.undo()) return;
    document::frame_time_sync(*this);
    toast_log(Level::INFO, TOAST_UNDO, message);
    change();
  }

  void Document::redo()
  {
    if (!snapshots.redo()) return;
    document::frame_time_sync(*this);
    toast_log(Level::INFO, TOAST_REDO, message);
    change();
  }

  bool Document::is_able_to_undo() { return !snapshots.undoStack.is_empty(); }
  bool Document::is_able_to_redo() { return !snapshots.redoStack.is_empty(); }
}
