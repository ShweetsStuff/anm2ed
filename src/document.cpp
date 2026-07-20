#include "document.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <set>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <format>

#include "file.hpp"
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
  uint64_t document_tab_id_next()
  {
    static uint64_t next{1};
    return next++;
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

  const Element* shader_element_get(const Element& spritesheet)
  {
    return element_child_first_get(spritesheet, ElementType::SHADER);
  }

  Element* shader_element_get(Element& spritesheet)
  {
    return element_child_first_get(spritesheet, ElementType::SHADER);
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
      auto line = source.substr(position, isEnd ? std::string_view::npos : end - position);
      output += std::format("{:4}: {}\n", lineNumber++, line);
      if (isEnd) break;
      position = end + 1;
    }
    return output;
  }

  bool shader_source_load(Document& document, const std::filesystem::path& shaderPath, std::string& source,
                          StringType label, std::string* status, const char* fallback = nullptr)
  {
    if (shaderPath.empty())
    {
      if (fallback)
      {
        source = fallback;
        return true;
      }

      if (status)
      {
        auto labelString = std::string(localize.get(label));
        *status += std::vformat(localize.get(LABEL_SHADER_PATH_EMPTY), std::make_format_args(labelString)) + "\n";
      }
      return false;
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

  void shader_uniform_configs_apply(const Element& shaderElement, resource::Shader& shader)
  {
    for (auto& uniform : shader.uniforms)
    {
      auto config = shader_uniform_get(shaderElement, uniform.name);
      if (!config) continue;

      if (!config->binding.empty())
      {
        auto binding = resource::shader::uniform_binding_get(config->binding);
        if (resource::shader::is_uniform_binding_valid(binding, uniform.valueType)) uniform.binding = binding;
      }
      if (!config->value.empty()) resource::shader::uniform_value_parse(uniform, config->value);
      for (int index = 0; index < (int)uniform.components.size(); ++index)
      {
        auto component = shader_uniform_component_get(*config, index);
        if (!component) continue;

        if (!component->binding.empty())
        {
          auto binding = resource::shader::uniform_binding_get(component->binding);
          if (binding == resource::shader::UNIFORM_BINDING_MANUAL ||
              binding == resource::shader::UNIFORM_BINDING_PLAYBACK_TIME)
            uniform.components[index].binding = binding;
        }
        if (!component->value.empty())
        {
          std::stringstream stream{component->value};
          stream >> uniform.components[index].value;
        }
      }
    }
  }

  bool shader_uniform_configs_trim(Element& shaderElement, const resource::Shader& shader)
  {
    auto component_count_get = [](resource::shader::UniformValueType type)
    {
      if (type == resource::shader::UNIFORM_VALUE_VEC2) return 2;
      if (type == resource::shader::UNIFORM_VALUE_VEC3) return 3;
      if (type == resource::shader::UNIFORM_VALUE_VEC4) return 4;
      return 0;
    };

    bool isChanged{};
    for (auto it = shaderElement.children.begin(); it != shaderElement.children.end();)
    {
      if (it->type != ElementType::UNIFORM)
      {
        ++it;
        continue;
      }

      auto uniform =
          std::find_if(shader.uniforms.begin(), shader.uniforms.end(),
                       [&](const resource::shader::Uniform& uniform) { return uniform.name == it->name; });
      if (uniform == shader.uniforms.end())
      {
        it = shaderElement.children.erase(it);
        isChanged = true;
        continue;
      }

      if (!it->binding.empty())
      {
        auto binding = resource::shader::uniform_binding_get(it->binding);
        if (!resource::shader::is_uniform_binding_valid(binding, uniform->valueType))
        {
          it->binding.clear();
          isChanged = true;
        }
      }

      auto componentCount = component_count_get(uniform->valueType);
      auto isComponents = resource::shader::uniform_binding_get(it->binding) == resource::shader::UNIFORM_BINDING_COMPONENTS;
      auto removed = std::erase_if(it->children,
                                   [&](const Element& component)
                                   {
                                     if (component.type != ElementType::COMPONENT) return false;
                                     if (!isComponents) return true;
                                     return component.index < 0 || component.index >= componentCount;
                                   });
      if (removed > 0) isChanged = true;

      for (auto& component : it->children)
      {
        if (component.type != ElementType::COMPONENT || component.binding.empty()) continue;
        auto binding = resource::shader::uniform_binding_get(component.binding);
        if (binding == resource::shader::UNIFORM_BINDING_MANUAL ||
            binding == resource::shader::UNIFORM_BINDING_PLAYBACK_TIME)
          continue;
        component.binding.clear();
        isChanged = true;
      }

      ++it;
    }

    return isChanged;
  }

  Element* frame_region_match_get(Element& spritesheet, const Element& frame)
  {
    auto frameCrop = glm::ivec2(frame.crop);
    auto frameSize = glm::ivec2(frame.size);
    auto framePivot = glm::ivec2(frame.pivot);
    for (auto& region : spritesheet.children)
      if (region.type == ElementType::REGION && glm::ivec2(region.crop) == frameCrop &&
          glm::ivec2(region.size) == frameSize && glm::ivec2(region.pivot) == framePivot)
        return &region;
    return nullptr;
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

  std::vector<std::string> overlay_labels_get(const Element* spritesheet)
  {
    std::vector<std::string> labels{};
    if (!spritesheet) return labels;
    for (auto& overlay : spritesheet->children)
      if (overlay.type == ElementType::OVERLAY)
      {
        auto pathString = path::to_utf8(overlay.path);
        labels.emplace_back(std::vformat(localize.get(FORMAT_OVERLAY), std::make_format_args(overlay.id, pathString)));
      }
    return labels;
  }

  std::vector<int> overlay_ids_get(const Element* spritesheet)
  {
    std::vector<int> ids{};
    if (!spritesheet) return ids;
    for (auto& overlay : spritesheet->children)
      if (overlay.type == ElementType::OVERLAY) ids.emplace_back(overlay.id);
    return ids;
  }

  int overlay_next_id_get(const Anm2& data)
  {
    int nextId{};
    if (auto spritesheets = data.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
      {
        if (spritesheet.type != ElementType::SPRITESHEET) continue;
        for (auto& overlay : spritesheet.children)
          if (overlay.type == ElementType::OVERLAY) nextId = std::max(nextId, overlay.id + 1);
      }
    return nextId;
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
    auto& groupReferences = document.groupReferences;

    auto animationCount = animation_count_get(document.anm2);
    if (animationCount <= 0)
    {
      reference = {};
      selection.clear();
      frameReferences.clear();
      itemReferences.clear();
      groupReferences.clear();
      document.frameTime = 0.0f;
      return;
    }

    if (reference.animationIndex < 0 || reference.animationIndex >= animationCount)
      reference.animationIndex = std::clamp(reference.animationIndex, 0, animationCount - 1);

    auto referenceItem = reference;
    referenceItem.frameIndex = -1;
    auto item = document.anm2.element_get(referenceItem);
    if (!item)
    {
      reference.itemType = (int)ItemType::ROOT;
      reference.itemID = -1;
      reference.groupType = (int)ItemType::NONE;
      reference.groupId = -1;
      referenceItem = reference;
      referenceItem.frameIndex = -1;
      item = document.anm2.element_get(referenceItem);
    }

    if (!item)
    {
      reference.frameIndex = -1;
      selection.clear();
      frameReferences.clear();
      groupReferences.clear();
      document.frameTime = 0.0f;
      return;
    }

    for (auto it = itemReferences.begin(); it != itemReferences.end();)
    {
      auto itemReference = *it;
      itemReference.frameIndex = -1;
      auto item = document.anm2.element_get(itemReference);
      if (!item)
        it = itemReferences.erase(it);
      else
        ++it;
    }

    for (auto it = groupReferences.begin(); it != groupReferences.end();)
    {
      auto animation = document.anm2.element_get(ElementType::ANIMATION, it->animationIndex);
      Element* container{};
      if (animation && it->itemType == LAYER)
        container = element_child_first_get(*animation, ElementType::LAYER_ANIMATIONS);
      else if (animation && it->itemType == NULL_)
        container = element_child_first_get(*animation, ElementType::NULL_ANIMATIONS);
      auto group = container ? element_child_id_get(*container, ElementType::GROUP, it->itemID) : nullptr;
      if (!group)
        it = groupReferences.erase(it);
      else
        ++it;
    }

    for (auto it = frameReferences.begin(); it != frameReferences.end();)
    {
      auto itemReference = *it;
      itemReference.frameIndex = -1;
      auto item = document.anm2.element_get(itemReference);
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
      {
        auto frameReference = reference;
        frameReference.frameIndex = frameIndex;
        frameReferences.insert(frameReference);
      }

    selection.clear();
    for (const auto& frameReference : frameReferences)
      if (frameReference.animationIndex == reference.animationIndex && frameReference.itemType == reference.itemType &&
          frameReference.itemID == reference.itemID && frameReference.groupType == reference.groupType &&
          frameReference.groupId == reference.groupId)
        selection.insert(frameReference.frameIndex);

    document.frameTime = frame_time_from_index_get(*item, reference.frameIndex);
  }
}

namespace anm2ed
{
  Document::Document(const std::filesystem::path& path, bool isNew, std::string* errorString)
  {
    tabId = document::document_tab_id_next();

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
      : path(std::move(other.path)), tabId(other.tabId), snapshots(std::move(other.snapshots)), current(snapshots.current),
        playback(current.playback), animation(current.animation), event(current.event), frames(current.frames),
        items(current.items), layer(current.layer), merge(current.merge), null(current.null), overlay(current.overlay),
        region(current.region), sound(current.sound), spritesheet(current.spritesheet), textures(current.textures),
        overlayTextures(current.overlayTextures), sounds(current.sounds), anm2(current.anm2),
        reference(current.reference), groupReferences(current.groupReferences),
        frameTime(current.frameTime), message(current.message), regionBySpritesheet(std::move(other.regionBySpritesheet)),
        changeAllFramePropertiesRegionId(other.changeAllFramePropertiesRegionId), previewZoom(other.previewZoom),
        previewPan(other.previewPan), editorPan(other.editorPan), editorZoom(other.editorZoom),
        overlayIndex(other.overlayIndex), hash(other.hash), saveHash(other.saveHash), autosaveHash(other.autosaveHash),
        lastAutosaveTime(other.lastAutosaveTime), isValid(other.isValid), isOpen(other.isOpen),
        isForceDirty(other.isForceDirty), spritesheetHashes(std::move(other.spritesheetHashes)),
        spritesheetSaveHashes(std::move(other.spritesheetSaveHashes)), overlayHashes(std::move(other.overlayHashes)),
        overlaySaveHashes(std::move(other.overlaySaveHashes)), texturePaths(std::move(other.texturePaths)),
        overlayTexturePaths(std::move(other.overlayTexturePaths)), soundPaths(std::move(other.soundPaths)),
        shaderVertexPaths(std::move(other.shaderVertexPaths)),
        shaderFragmentPaths(std::move(other.shaderFragmentPaths)), shaders(std::move(other.shaders)),
        isAnimationPreviewSet(other.isAnimationPreviewSet), isSpritesheetEditorSet(other.isSpritesheetEditorSet),
        editTarget(other.editTarget)
  {
  }

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

  bool Document::autosave(std::string* errorString, Options options)
  {
    auto autosavePath = autosave_path_get();
    auto autosavePathUtf8 = path::to_utf8(autosavePath);
    if (anm2.save(autosavePath, errorString, options))
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

  void Document::overlay_texture_change(int id)
  {
    auto texture = overlay_texture_get(id);
    if (!texture || !overlay_get(id)) return;
    change(SPRITESHEETS);
  }

  bool Document::texture_reload(int id)
  {
    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, id);
    if (!spritesheet) return false;

    util::WorkingDirectory workingDirectory(directory_get());
    textures[id] = resource::Texture(path::case_insensitive_find(spritesheet->path));
    texturePaths[id] = spritesheet->path;
    return true;
  }

  bool Document::overlay_texture_reload(int id)
  {
    auto overlay = overlay_get(id);
    if (!overlay) return false;

    util::WorkingDirectory workingDirectory(directory_get());
    overlayTextures[id] = resource::Texture(path::case_insensitive_find(overlay->path));
    overlayTexturePaths[id] = overlay->path;
    return true;
  }

  bool Document::sound_reload(int id)
  {
    auto sound = anm2.element_get(ElementType::SOUND_ELEMENT, id);
    if (!sound) return false;

    util::WorkingDirectory workingDirectory(directory_get());
    sounds[id] = resource::Audio(path::case_insensitive_find(sound->path));
    soundPaths[id] = sound->path;
    return true;
  }

  bool Document::shader_reload(int id, std::string* status)
  {
    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, id);
    auto shaderElement = spritesheet ? document::shader_element_get(*spritesheet) : nullptr;
    if (!shaderElement || !shaderElement->isEnabled)
    {
      if (status && !shaderElement)
      {
        auto labelString = std::string(localize.get(LABEL_FRAGMENT));
        *status += std::vformat(localize.get(LABEL_SHADER_PATH_EMPTY), std::make_format_args(labelString)) + "\n";
      }
      shaders.erase(id);
      shaderVertexPaths.erase(id);
      shaderFragmentPaths.erase(id);
      return false;
    }

    std::string vertexSource{};
    std::string fragmentSource{};
    auto isVertexLoaded = document::shader_source_load(*this, shaderElement->vertex, vertexSource, LABEL_VERTEX, status,
                                                       resource::shader::TEXTURE_COMPATIBILITY_VERTEX);
    auto isFragmentLoaded =
        document::shader_source_load(*this, shaderElement->fragment, fragmentSource, LABEL_FRAGMENT, status);
    if (!isVertexLoaded || !isFragmentLoaded)
    {
      shaders.erase(id);
      shaderVertexPaths.erase(id);
      shaderFragmentPaths.erase(id);
      return false;
    }

    auto vertex = resource::shader::gles_vertex_convert(vertexSource);
    auto fragment = resource::shader::gles_fragment_convert(fragmentSource);
    auto result = resource::shader_compile(vertex.c_str(), fragment.c_str());
    document::shader_status_append(result, status);
    if (status && (!result.isCompiled || !result.isLinked))
    {
      *status += document::shader_source_lines_get("Converted vertex shader", vertex);
      *status += document::shader_source_lines_get("Converted fragment shader", fragment);
    }
    if (!result.isLinked || !result.id)
    {
      shaders.erase(id);
      shaderVertexPaths.erase(id);
      shaderFragmentPaths.erase(id);
      return false;
    }

    resource::Shader runtime{};
    runtime.id = result.id;
    runtime.uniforms = std::move(result.uniforms);
    if (document::shader_uniform_configs_trim(*shaderElement, runtime)) hash_set();
    document::shader_uniform_configs_apply(*shaderElement, runtime);
    shaders[id] = std::move(runtime);
    shaderVertexPaths[id] = shaderElement->vertex;
    shaderFragmentPaths[id] = shaderElement->fragment;
    return true;
  }

  void Document::assets_sync(ChangeType type)
  {
    if (type == ALL || type == SPRITESHEETS || type == TEXTURES)
    {
      std::set<int> validIds{};
      std::set<int> validOverlayIds{};
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
            textures[spritesheet.id] = resource::Texture(path::case_insensitive_find(spritesheet.path));
            texturePaths[spritesheet.id] = spritesheet.path;
          }

          for (auto& overlay : spritesheet.children)
          {
            if (overlay.type != ElementType::OVERLAY) continue;
            validOverlayIds.insert(overlay.id);
            auto isOverlayReload = !overlayTextures.contains(overlay.id) || !overlayTexturePaths.contains(overlay.id) ||
                                   overlayTexturePaths.at(overlay.id) != overlay.path;
            if (isOverlayReload)
            {
              overlayTextures[overlay.id] = resource::Texture(path::case_insensitive_find(overlay.path));
              overlayTexturePaths[overlay.id] = overlay.path;
            }
          }

          auto shaderElement = document::shader_element_get(spritesheet);
          if (!shaderElement || !shaderElement->isEnabled || shaderElement->fragment.empty())
          {
            shaders.erase(spritesheet.id);
            shaderVertexPaths.erase(spritesheet.id);
            shaderFragmentPaths.erase(spritesheet.id);
            continue;
          }

          auto isShaderReload =
              !shaders.contains(spritesheet.id) || !shaderVertexPaths.contains(spritesheet.id) ||
              !shaderFragmentPaths.contains(spritesheet.id) ||
              shaderVertexPaths.at(spritesheet.id) != shaderElement->vertex ||
              shaderFragmentPaths.at(spritesheet.id) != shaderElement->fragment;
          if (isShaderReload)
            shader_reload(spritesheet.id);
          else if (auto shader = shader_get(spritesheet.id))
            document::shader_uniform_configs_apply(*shaderElement, *shader);
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

      for (auto it = overlayTextures.begin(); it != overlayTextures.end();)
      {
        if (!validOverlayIds.contains(it->first))
        {
          overlayTexturePaths.erase(it->first);
          it = overlayTextures.erase(it);
        }
        else
          ++it;
      }

      for (auto it = shaders.begin(); it != shaders.end();)
      {
        if (!validIds.contains(it->first))
        {
          shaderVertexPaths.erase(it->first);
          shaderFragmentPaths.erase(it->first);
          it = shaders.erase(it);
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
            sounds[sound.id] = resource::Audio(path::case_insensitive_find(sound.path));
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

  resource::Texture* Document::overlay_texture_get(int id)
  {
    auto it = overlayTextures.find(id);
    return it == overlayTextures.end() ? nullptr : &it->second;
  }

  const resource::Texture* Document::overlay_texture_get(int id) const
  {
    auto it = overlayTextures.find(id);
    return it == overlayTextures.end() ? nullptr : &it->second;
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

  resource::Shader* Document::shader_get(int id)
  {
    auto it = shaders.find(id);
    return it == shaders.end() || !it->second.is_valid() ? nullptr : &it->second;
  }

  const resource::Shader* Document::shader_get(int id) const
  {
    auto it = shaders.find(id);
    return it == shaders.end() || !it->second.is_valid() ? nullptr : &it->second;
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
        region->pivot = region->size * 0.5f;
      else
        region->pivot -= region->crop - previousCrop;
      isChanged = true;
    }

    return isChanged;
  }

  bool Document::regions_generate_from_animations(const std::set<int>& animationIndices, const std::string& format,
                                                  RegionFrameMapping mapping)
  {
    if (animationIndices.empty()) return false;

    std::unordered_map<int, int> regionNumbers{};
    bool isChanged{};
    for (auto animationIndex : animationIndices)
    {
      auto animation = anm2.element_get(ElementType::ANIMATION, animationIndex);
      if (!animation) continue;

      auto layerAnimations = element_child_first_get(*animation, ElementType::LAYER_ANIMATIONS);
      if (!layerAnimations) continue;

      auto layer_animation_regions_generate = [&](auto&& self, Element& layerAnimation) -> void
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
          if (frame.type != ElementType::FRAME) continue;

          auto region = document::frame_region_match_get(*spritesheet, frame);
          if (!region)
          {
            auto generated = element_make(ElementType::REGION);
            generated.id = element_child_next_id_get(*spritesheet, ElementType::REGION);
            auto [regionNumberIt, _] = regionNumbers.try_emplace(layer->spritesheetId, 1);
            auto& regionNumber = regionNumberIt->second;
            generated.name = document::region_name_get(format, regionNumber++);
            generated.crop = frame.crop;
            generated.size = frame.size;
            generated.pivot = frame.pivot;
            generated.origin = Origin::CUSTOM;
            spritesheet->children.push_back(generated);
            region = &spritesheet->children.back();
            isChanged = true;
          }
          if (mapping == RegionFrameMapping::SET && frame.regionId != region->id)
          {
            frame.regionId = region->id;
            isChanged = true;
          }
        }
      };

      for (auto& layerAnimation : layerAnimations->children)
        layer_animation_regions_generate(layer_animation_regions_generate, layerAnimation);
    }

    return isChanged;
  }

  bool Document::regions_generate_from_frames(const std::set<Reference>& frameReferences, const std::string& format,
                                              RegionFrameMapping mapping)
  {
    std::unordered_map<int, int> regionNumbers{};
    bool isChanged{};
    for (auto frameReference : frameReferences)
    {
      if (frameReference.itemType != LAYER || frameReference.frameIndex < 0) continue;

      auto frame = anm2.element_get(frameReference);
      if (!frame || frame->type != ElementType::FRAME || frame->regionId != -1) continue;

      auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, frameReference.itemID);
      auto spritesheet = layer ? anm2.element_get(ElementType::SPRITESHEET, layer->spritesheetId) : nullptr;
      if (!spritesheet) continue;

      auto region = document::frame_region_match_get(*spritesheet, *frame);
      if (!region)
      {
        auto generated = element_make(ElementType::REGION);
        generated.id = element_child_next_id_get(*spritesheet, ElementType::REGION);
        auto& regionNumber = regionNumbers[layer->spritesheetId];
        generated.name = document::region_name_get(format, regionNumber++);
        generated.crop = frame->crop;
        generated.size = frame->size;
        generated.pivot = frame->pivot;
        generated.origin = Origin::CUSTOM;
        spritesheet->children.push_back(generated);
        region = &spritesheet->children.back();
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

  bool Document::file_merge(const std::filesystem::path& path, FileMergePreset preset)
  {
    Anm2 source(path);
    if (!source.isValid) return false;

    bool isAppendAsNew = preset == FILE_MERGE_PRESET_APPEND_AS_NEW;
    bool isReplaceMatching = preset == FILE_MERGE_PRESET_REPLACE_MATCHING;

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

    auto name_unique_get = [&](Element& container, ElementType type, const std::string& name)
    {
      if (!find_by_name(container, type, name)) return name;
      for (int i = 2;; ++i)
      {
        auto candidate = std::format("{} {}", name, i);
        if (!find_by_name(container, type, candidate)) return candidate;
      }
    };

    auto named_element_merge = [&](Element& container, Element item, ElementType type)
    {
      if (isAppendAsNew)
      {
        item.id = element_child_next_id_get(container, type);
        item.name = name_unique_get(container, type, item.name);
        container.children.push_back(item);
        return item.id;
      }

      if (auto existing = find_by_name(container, type, item.name))
      {
        item.id = existing->id;
        *existing = item;
      }
      else
      {
        item.id = element_child_next_id_get(container, type);
        container.children.push_back(item);
      }
      return item.id;
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

    auto spritesheet_import = [&](int sourceId)
    {
      if (sourceId < 0) return -1;
      if (spritesheetRemap.contains(sourceId)) return spritesheetRemap[sourceId];

      auto sourceSpritesheets = source.element_get(ElementType::SPRITESHEETS);
      auto destinationSpritesheets = anm2.element_get(ElementType::SPRITESHEETS);
      auto spritesheet =
          sourceSpritesheets ? element_child_id_get(*sourceSpritesheets, ElementType::SPRITESHEET, sourceId) : nullptr;
      if (!spritesheet || !destinationSpritesheets) return -1;

      auto imported = *spritesheet;
      imported.id = element_child_next_id_get(*destinationSpritesheets, ElementType::SPRITESHEET);
      imported.path = remap_path(imported.path);
      destinationSpritesheets->children.push_back(imported);
      spritesheetRemap[sourceId] = imported.id;
      return imported.id;
    };

    if (auto sourceSounds = source.element_get(ElementType::SOUNDS))
      if (auto destinationSounds = anm2.element_get(ElementType::SOUNDS))
        for (auto sound : sourceSounds->children)
        {
          if (sound.type != ElementType::SOUND_ELEMENT) continue;
          auto sourceId = sound.id;
          sound.path = remap_path(sound.path);
          int destinationId{-1};
          if (!isAppendAsNew)
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
          auto sourceSpritesheetId = layer.spritesheetId;
          auto existing =
              isAppendAsNew ? nullptr : find_by_name(*destinationLayers, ElementType::LAYER_ELEMENT, layer.name);
          layer.spritesheetId = existing ? existing->spritesheetId : spritesheet_import(sourceSpritesheetId);
          layerRemap[sourceId] = named_element_merge(*destinationLayers, layer, ElementType::LAYER_ELEMENT);
        }

    if (auto sourceNulls = source.element_get(ElementType::NULLS))
      if (auto destinationNulls = anm2.element_get(ElementType::NULLS))
        for (auto null : sourceNulls->children)
        {
          if (null.type != ElementType::NULL_ELEMENT) continue;
          auto sourceId = null.id;
          nullRemap[sourceId] = named_element_merge(*destinationNulls, null, ElementType::NULL_ELEMENT);
        }

    if (auto sourceEvents = source.element_get(ElementType::EVENTS))
      if (auto destinationEvents = anm2.element_get(ElementType::EVENTS))
        for (auto event : sourceEvents->children)
        {
          if (event.type != ElementType::EVENT_ELEMENT) continue;
          auto sourceId = event.id;
          eventRemap[sourceId] = named_element_merge(*destinationEvents, event, ElementType::EVENT_ELEMENT);
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
        group.isVisible = item.isVisible;
        if (auto root = element_child_first_get(item, ElementType::ROOT_ANIMATION))
          group.children.push_back(*root);
        else
        {
          auto defaultRoot = element_make(ElementType::ROOT_ANIMATION);
          defaultRoot.children.push_back(element_make(ElementType::FRAME));
          group.children.push_back(defaultRoot);
        }
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
        std::erase_if(item.children, [](const Element& child)
                      { return child.type != ElementType::ROOT_ANIMATION; });
        if (!element_child_first_get(item, ElementType::ROOT_ANIMATION))
        {
          auto root = element_make(ElementType::ROOT_ANIMATION);
          root.children.push_back(element_make(ElementType::FRAME));
          item.children.push_back(root);
        }
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
        std::string defaultAnimationName{};
        for (const auto& incoming : sourceAnimations->children)
        {
          if (incoming.type != ElementType::ANIMATION) continue;
          auto processed = animation_build(incoming);
          auto destination = find_by_name(*destinationAnimations, ElementType::ANIMATION, processed.name);
          if (incoming.name == sourceAnimations->defaultAnimation) defaultAnimationName = processed.name;
          if (!destination || isAppendAsNew)
          {
            if (isAppendAsNew)
            {
              processed.name = name_unique_get(*destinationAnimations, ElementType::ANIMATION, processed.name);
              if (incoming.name == sourceAnimations->defaultAnimation) defaultAnimationName = processed.name;
            }
            destinationAnimations->children.push_back(processed);
            continue;
          }

          if (isReplaceMatching)
          {
            *destination = processed;
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
          destinationAnimations->defaultAnimation =
              defaultAnimationName.empty() ? sourceAnimations->defaultAnimation : defaultAnimationName;
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
    overlay_hashes_reset();
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

  void Document::overlay_hashes_reset()
  {
    overlayHashes.clear();
    overlaySaveHashes.clear();
    if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
      {
        if (spritesheet.type != ElementType::SPRITESHEET) continue;
        for (auto& overlay : spritesheet.children)
          if (overlay.type == ElementType::OVERLAY)
          {
            auto currentHash = document::spritesheet_hash_get(overlay, overlay_texture_get(overlay.id));
            overlayHashes[overlay.id] = currentHash;
            overlaySaveHashes[overlay.id] = currentHash;
          }
      }
  }

  void Document::overlay_hashes_sync()
  {
    std::set<int> validIds{};
    if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
      {
        if (spritesheet.type != ElementType::SPRITESHEET) continue;
        for (auto& overlay : spritesheet.children)
          if (overlay.type == ElementType::OVERLAY) validIds.insert(overlay.id);
      }

    for (auto it = overlayHashes.begin(); it != overlayHashes.end();)
    {
      if (!validIds.contains(it->first))
        it = overlayHashes.erase(it);
      else
        ++it;
    }

    for (auto it = overlaySaveHashes.begin(); it != overlaySaveHashes.end();)
    {
      if (!validIds.contains(it->first))
        it = overlaySaveHashes.erase(it);
      else
        ++it;
    }

    if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
      {
        if (spritesheet.type != ElementType::SPRITESHEET) continue;
        for (auto& overlay : spritesheet.children)
          if (overlay.type == ElementType::OVERLAY)
          {
            auto currentHash = document::spritesheet_hash_get(overlay, overlay_texture_get(overlay.id));
            overlayHashes[overlay.id] = currentHash;
            if (!overlaySaveHashes.contains(overlay.id)) overlaySaveHashes[overlay.id] = currentHash;
          }
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
      if (auto currentSpritesheet = anm2.element_get(ElementType::SPRITESHEET, spritesheet.reference))
        overlay.labels_set(document::overlay_labels_get(currentSpritesheet),
                           document::overlay_ids_get(currentSpritesheet));
      else
        overlay.labels_set({}, {});
      overlay_hashes_sync();
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

    snapshots.commit(current);
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

  void Document::overlay_hash_update(int id)
  {
    auto overlay = overlay_get(id);
    if (!overlay) return;
    assets_sync(TEXTURES);
    overlayHashes[id] = document::spritesheet_hash_get(*overlay, overlay_texture_get(id));
  }

  void Document::overlay_hash_set_saved(int id)
  {
    auto overlay = overlay_get(id);
    if (!overlay) return;
    assets_sync(TEXTURES);
    auto currentHash = document::spritesheet_hash_get(*overlay, overlay_texture_get(id));
    overlayHashes[id] = currentHash;
    overlaySaveHashes[id] = currentHash;
  }

  bool Document::overlay_is_dirty(int id)
  {
    if (!overlay_get(id)) return false;
    if (!overlayHashes.contains(id)) overlay_hash_update(id);
    auto saveIt = overlaySaveHashes.find(id);
    if (saveIt == overlaySaveHashes.end()) return false;
    return overlayHashes.at(id) != saveIt->second;
  }

  bool Document::overlay_any_dirty()
  {
    if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
      {
        if (spritesheet.type != ElementType::SPRITESHEET) continue;
        for (auto& overlay : spritesheet.children)
          if (overlay.type == ElementType::OVERLAY && overlay_is_dirty(overlay.id)) return true;
      }
    return false;
  }

  std::filesystem::path Document::directory_get() const { return path.parent_path(); }
  std::filesystem::path Document::filename_get() const { return path.filename(); }
  bool Document::is_valid() const { return isValid && !path.empty(); }

  void Document::command_run(Manager& manager, Command& command)
  {
    if (command.run) command.run(manager, *this);
  }

  bool Document::is_frame_reference_valid(Reference frameReference) const
  {
    if (frameReference.itemType == NONE || frameReference.frameIndex < 0) return false;
    auto itemReference = frameReference;
    itemReference.frameIndex = -1;
    auto item = anm2.element_get(itemReference);
    return item && track_frame_get(*item, frameReference.frameIndex);
  }

  std::set<Reference> Document::item_frame_references_get(Reference itemReference) const
  {
    std::set<Reference> result{};
    itemReference.frameIndex = -1;
    if (itemReference.itemType == NONE) return result;

    auto item = anm2.element_get(itemReference);
    if (!item) return result;

    auto frameType = itemReference.itemType == TRIGGER ? ElementType::TRIGGER : ElementType::FRAME;
    int frameIndex{};
    for (auto& child : item->children)
    {
      if (child.type != frameType) continue;
      auto frameReference = itemReference;
      frameReference.frameIndex = frameIndex;
      result.insert(frameReference);
      ++frameIndex;
    }
    return result;
  }

  std::set<Reference> Document::selected_item_frame_references_get() const
  {
    auto selectedItems = items.references;
    if (selectedItems.empty() && reference.itemType != NONE)
    {
      auto itemReference = reference;
      itemReference.frameIndex = -1;
      selectedItems.insert(itemReference);
    }

    std::set<Reference> result{};
    for (auto itemReference : selectedItems)
    {
      auto itemFrames = item_frame_references_get(itemReference);
      result.insert(itemFrames.begin(), itemFrames.end());
    }
    return result;
  }

  std::set<Reference> Document::frame_references_get(FrameReferenceFallback fallback) const
  {
    auto result = frames.references;
    for (auto frameIndex : frames.selection)
    {
      auto frameReference = reference;
      frameReference.frameIndex = frameIndex;
      result.insert(frameReference);
    }

    std::map<Reference, int> frameCounts{};
    auto is_frame_reference_valid_cached = [&](Reference frameReference)
    {
      if (frameReference.itemType == NONE || frameReference.frameIndex < 0) return false;
      auto itemReference = frameReference;
      itemReference.frameIndex = -1;
      if (!frameCounts.contains(itemReference))
      {
        auto item = anm2.element_get(itemReference);
        frameCounts[itemReference] = item ? document::frame_count_get(*item) : 0;
      }
      return frameReference.frameIndex < frameCounts[itemReference];
    };

    bool isMultiFrameSelection = frames.references.size() > 1 || frames.selection.size() > 1;
    std::erase_if(result,
                  [&](const Reference& frameReference) { return !is_frame_reference_valid_cached(frameReference); });

    if (isMultiFrameSelection && result.size() <= 1)
    {
      auto itemFrames = selected_item_frame_references_get();
      std::erase_if(itemFrames,
                    [&](const Reference& frameReference) { return !is_frame_reference_valid_cached(frameReference); });
      if (itemFrames.size() > result.size()) result = std::move(itemFrames);
    }

    if (result.empty() && fallback == FrameReferenceFallback::CURRENT && is_frame_reference_valid_cached(reference))
      result.insert(reference);

    return result;
  }

  void Document::frame_references_set(std::set<Reference> frameReferences)
  {
    std::map<Reference, int> frameCounts{};
    auto is_frame_reference_valid_cached = [&](Reference frameReference)
    {
      if (frameReference.itemType == NONE || frameReference.frameIndex < 0) return false;
      auto itemReference = frameReference;
      itemReference.frameIndex = -1;
      if (!frameCounts.contains(itemReference))
      {
        auto item = anm2.element_get(itemReference);
        frameCounts[itemReference] = item ? document::frame_count_get(*item) : 0;
      }
      return frameReference.frameIndex < frameCounts[itemReference];
    };

    std::erase_if(frameReferences,
                  [&](const Reference& frameReference) { return !is_frame_reference_valid_cached(frameReference); });

    frames.references = std::move(frameReferences);
    frames.selection.clear();
    items.references.clear();

    if (frames.references.empty()) return;

    if (!frames.references.contains(reference) || !is_frame_reference_valid_cached(reference))
      reference = *frames.references.begin();

    for (auto frameReference : frames.references)
    {
      frameReference.frameIndex = -1;
      items.references.insert(frameReference);
    }

    for (auto frameReference : frames.references)
      if (frameReference.animationIndex == reference.animationIndex && frameReference.itemType == reference.itemType &&
          frameReference.itemID == reference.itemID && frameReference.groupType == reference.groupType &&
          frameReference.groupId == reference.groupId && frameReference.frameIndex >= 0)
        frames.selection.insert(frameReference.frameIndex);
  }

  void Document::frame_references_clear()
  {
    frames.references.clear();
    frames.selection.clear();
  }

  std::vector<Reference> Document::layer_references_get()
  {
    std::set<Reference> selectedReferences = items.references;
    if (selectedReferences.empty())
      for (auto frameReference : frame_references_get(FrameReferenceFallback::NONE))
      {
        frameReference.frameIndex = -1;
        selectedReferences.insert(frameReference);
      }
    if (selectedReferences.empty() && reference.itemType != NONE)
    {
      auto itemReference = reference;
      itemReference.frameIndex = -1;
      selectedReferences.insert(itemReference);
    }

    std::vector<Reference> result{};
    for (auto itemReference : selectedReferences)
    {
      itemReference.frameIndex = -1;
      if (itemReference.itemType != LAYER) return {};
      if (!anm2.element_get(itemReference))
      {
        auto targetReference = itemReference;
        targetReference.animationIndex = reference.animationIndex;
        if (reference.animationIndex == itemReference.animationIndex || !anm2.element_get(targetReference))
          return {};
        itemReference = targetReference;
      }
      result.push_back(itemReference);
    }
    return result;
  }

  Element* Document::frame_get()
  {
    return anm2.element_get(reference);
  }

  Element* Document::item_get()
  {
    auto itemReference = reference;
    itemReference.frameIndex = -1;
    return anm2.element_get(itemReference);
  }
  Element* Document::animation_get() { return anm2.element_get(ElementType::ANIMATION, reference.animationIndex); }
  Element* Document::spritesheet_get() { return anm2.element_get(ElementType::SPRITESHEET, spritesheet.reference); }

  Element* Document::overlay_get(int id)
  {
    if (id == -1) id = overlay.reference;
    if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
      {
        if (spritesheet.type != ElementType::SPRITESHEET) continue;
        if (auto overlay = element_child_id_get(spritesheet, ElementType::OVERLAY, id)) return overlay;
      }
    return nullptr;
  }

  const Element* Document::overlay_get(int id) const
  {
    if (id == -1) id = overlay.reference;
    if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
      {
        if (spritesheet.type != ElementType::SPRITESHEET) continue;
        if (auto overlay = element_child_id_get(spritesheet, ElementType::OVERLAY, id)) return overlay;
      }
    return nullptr;
  }

  int Document::overlay_spritesheet_id_get(int id) const
  {
    if (auto spritesheets = anm2.element_get(ElementType::SPRITESHEETS))
      for (auto& spritesheet : spritesheets->children)
      {
        if (spritesheet.type != ElementType::SPRITESHEET) continue;
        if (element_child_id_get(spritesheet, ElementType::OVERLAY, id)) return spritesheet.id;
      }
    return -1;
  }

  void Document::spritesheet_add(const std::filesystem::path& path)
  {
    spritesheets_add({path});
  }

  void Document::spritesheets_add(const std::vector<std::filesystem::path>& paths)
  {
    struct LoadedSpritesheet
    {
      std::filesystem::path relativePath{};
      resource::Texture texture{};
    };

    auto items = anm2.element_get(ElementType::SPRITESHEETS);
    if (!items) return;
    auto directory = directory_get();

    std::vector<LoadedSpritesheet> loaded{};
    for (auto& path : paths)
    {
      auto pathCopy = path;
      auto storagePath = path::backslash_handle(pathCopy);
      std::optional<WorkingDirectory> workingDirectory{};
      if (!storagePath.is_absolute()) workingDirectory.emplace(directory);
      auto loadPath = path::case_insensitive_find(storagePath);
      auto texture = resource::Texture(loadPath);
      if (!texture.is_valid())
      {
        auto pathUtf8 = path::to_utf8(pathCopy);
        toasts.push(std::vformat(localize.get(TOAST_SPRITESHEET_INIT_FAILED), std::make_format_args(pathUtf8)));
        logger.error(std::vformat(localize.get(TOAST_SPRITESHEET_INIT_FAILED, anm2ed::ENGLISH),
                                  std::make_format_args(pathUtf8)));
        continue;
      }

      loaded.push_back({.relativePath = path::backslash_replace(path::make_relative(storagePath, directory)),
                        .texture = std::move(texture)});
    }
    if (loaded.empty()) return;

    anm2_snapshot(localize.get(EDIT_ADD_SPRITESHEET));

    std::set<int> added{};
    for (auto& loadedSpritesheet : loaded)
    {
      auto id = element_child_next_id_get(*items, ElementType::SPRITESHEET);
      auto spritesheet = element_make(ElementType::SPRITESHEET);
      spritesheet.id = id;
      spritesheet.path = loadedSpritesheet.relativePath;
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

  void Document::overlay_add(int spritesheetId, const std::filesystem::path& path)
  {
    overlays_add(spritesheetId, {path});
  }

  void Document::overlays_add(int spritesheetId, const std::vector<std::filesystem::path>& paths)
  {
    struct LoadedOverlay
    {
      std::filesystem::path relativePath{};
      resource::Texture texture{};
    };

    auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, spritesheetId);
    if (!spritesheet) return;
    auto directory = directory_get();

    std::vector<LoadedOverlay> loaded{};
    for (auto& path : paths)
    {
      auto pathCopy = path;
      auto storagePath = path::backslash_handle(pathCopy);
      std::optional<WorkingDirectory> workingDirectory{};
      if (!storagePath.is_absolute()) workingDirectory.emplace(directory);
      auto loadPath = path::case_insensitive_find(storagePath);
      auto texture = resource::Texture(loadPath);
      if (!texture.is_valid())
      {
        auto pathUtf8 = path::to_utf8(pathCopy);
        toasts.push(std::vformat(localize.get(TOAST_OVERLAY_INIT_FAILED), std::make_format_args(pathUtf8)));
        logger.error(std::vformat(localize.get(TOAST_OVERLAY_INIT_FAILED, anm2ed::ENGLISH),
                                  std::make_format_args(pathUtf8)));
        continue;
      }

      loaded.push_back({.relativePath = path::backslash_replace(path::make_relative(storagePath, directory)),
                        .texture = std::move(texture)});
    }
    if (loaded.empty()) return;

    anm2_snapshot(localize.get(EDIT_ADD_OVERLAY));

    std::set<int> added{};
    for (auto& loadedOverlay : loaded)
    {
      auto id = document::overlay_next_id_get(anm2);
      auto overlayElement = element_make(ElementType::OVERLAY);
      overlayElement.id = id;
      overlayElement.path = loadedOverlay.relativePath;
      auto pathString = path::to_utf8(overlayElement.path);
      spritesheet->children.push_back(overlayElement);
      overlayTextures[id] = std::move(loadedOverlay.texture);
      overlayTexturePaths[id] = overlayElement.path;
      added.insert(id);
      overlay.reference = id;
      overlay_hash_set_saved(id);
      toasts.push(std::vformat(localize.get(TOAST_OVERLAY_INITIALIZED), std::make_format_args(id, pathString)));
      logger.info(std::vformat(localize.get(TOAST_OVERLAY_INITIALIZED, anm2ed::ENGLISH),
                               std::make_format_args(id, pathString)));
    }
    overlay.selection = added;
    editTarget = Document::EditTarget::OVERLAY;
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
    auto directory = directory_get();

    std::vector<std::filesystem::path> validPaths{};
    for (auto& path : paths)
      if (!path.empty()) validPaths.push_back(path);
    if (validPaths.empty()) return;

    anm2_snapshot(localize.get(EDIT_ADD_SOUND));

    std::set<int> added{};
    for (auto& path : validPaths)
    {
      auto id = element_child_next_id_get(*items, ElementType::SOUND_ELEMENT);
      auto soundElement = element_make(ElementType::SOUND_ELEMENT);
      soundElement.id = id;
      auto loadPath = path::backslash_handle(path);
      auto relativePath = path::backslash_replace(path::make_relative(loadPath, directory));
      {
        WorkingDirectory workingDirectory(directory_get());
        soundElement.path = relativePath;
        sounds[id] = resource::Audio(path::case_insensitive_find(soundElement.path));
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

  void Document::anm2_snapshot(const std::string& message)
  {
    this->message = message;
    snapshots.anm2_push(current, message);
  }

  void Document::tracks_snapshot(const std::string& message, const std::set<Reference>& trackReferences)
  {
    this->message = message;
    snapshots.tracks_push(current, message, trackReferences);
  }

  void Document::frames_snapshot(const std::string& message, const std::set<Reference>& frameReferences)
  {
    this->message = message;
    snapshots.frames_push(current, message, frameReferences);
  }

  void Document::regions_snapshot(const std::string& message, int spritesheetId, const std::set<int>& regionIds)
  {
    this->message = message;
    snapshots.regions_push(current, message, spritesheetId, regionIds);
  }

  void Document::textures_snapshot(const std::string& message)
  {
    this->message = message;
    snapshots.textures_push(current, message);
  }

  void Document::anm2_textures_snapshot(const std::string& message)
  {
    this->message = message;
    snapshots.anm2_textures_push(current, message);
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
