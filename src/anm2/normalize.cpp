#include "internal.hpp"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed
{
  void shader_ids_repair(Element& root)
  {
    auto content = child_first_get(root, ElementType::CONTENT);
    auto shaders = content ? child_first_get(*content, ElementType::SHADERS) : nullptr;
    if (!shaders) return;

    std::set<int> usedIds{};
    int nextId{};
    for (auto& shader : shaders->children)
    {
      if (shader.type != ElementType::SHADER) continue;

      while (usedIds.contains(nextId))
        ++nextId;
      if (shader.id < 0 || usedIds.contains(shader.id)) shader.id = nextId++;
      usedIds.insert(shader.id);
      if (shader.name.empty()) shader.name = "New Shader";
    }
  }

  void shader_frame_ids_repair(Element& root)
  {
    auto content = child_first_get(root, ElementType::CONTENT);
    auto shaders = content ? child_first_get(*content, ElementType::SHADERS) : nullptr;
    auto animations = element_first_get(root, ElementType::ANIMATIONS);
    if (!animations) return;

    std::set<int> shaderIds{};
    if (shaders)
      for (const auto& shader : shaders->children)
        if (shader.type == ElementType::SHADER) shaderIds.insert(shader.id);

    auto frame_repair = [&](auto&& self, Element& element) -> void
    {
      if (element.type == ElementType::FRAME && element.shaderId != -1 && !shaderIds.contains(element.shaderId))
        element.shaderId = -1;
      for (auto& child : element.children)
        self(self, child);
    };
    frame_repair(frame_repair, *animations);
  }

  void region_frame_ids_repair(Element& root)
  {
    auto content = child_first_get(root, ElementType::CONTENT);
    auto layers = content ? child_first_get(*content, ElementType::LAYERS) : nullptr;
    auto spritesheets = content ? child_first_get(*content, ElementType::SPRITESHEETS) : nullptr;
    auto animations = element_first_get(root, ElementType::ANIMATIONS);
    if (!layers || !spritesheets || !animations) return;

    for (auto& animation : animations->children)
    {
      if (animation.type != ElementType::ANIMATION) continue;
      auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS);
      if (!layerAnimations) continue;

      tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                  [&](Element& layerAnimation)
                  {
                    auto layer = child_id_get(*layers, ElementType::LAYER_ELEMENT, layerAnimation.layerId);
                    auto spritesheet =
                        layer ? child_id_get(*spritesheets, ElementType::SPRITESHEET, layer->spritesheetId) : nullptr;
                    if (!spritesheet) return;

                    for (auto& frame : layerAnimation.children)
                    {
                      if (frame.type != ElementType::FRAME) continue;

                      auto frameCrop = glm::ivec2(frame.crop);
                      auto frameSize = glm::ivec2(frame.size);
                      auto framePivot = glm::ivec2(frame.pivot);
                      auto is_region_match = [&](const Element& region)
                      {
                        return region.type == ElementType::REGION && glm::ivec2(region.crop) == frameCrop &&
                               glm::ivec2(region.size) == frameSize && glm::ivec2(region.pivot) == framePivot;
                      };

                      auto region = frame.regionId == -1
                                        ? nullptr
                                        : child_id_get(*spritesheet, ElementType::REGION, frame.regionId);
                      if (region && is_region_match(*region)) continue;

                      for (const auto& candidate : spritesheet->children)
                        if (is_region_match(candidate))
                        {
                          frame.regionId = candidate.id;
                          break;
                        }
                    }
                  });
    }
  }

  void region_ids_remap(Element& root)
  {
    auto content = child_first_get(root, ElementType::CONTENT);
    auto layers = content ? child_first_get(*content, ElementType::LAYERS) : nullptr;
    auto spritesheets = content ? child_first_get(*content, ElementType::SPRITESHEETS) : nullptr;
    auto animations = element_first_get(root, ElementType::ANIMATIONS);
    if (!spritesheets) return;

    std::unordered_map<int, std::unordered_map<int, int>> remaps{};
    for (auto& spritesheet : spritesheets->children)
    {
      if (spritesheet.type != ElementType::SPRITESHEET) continue;

      std::unordered_map<int, int> remap{};
      int nextId{};
      for (auto& region : spritesheet.children)
      {
        if (region.type != ElementType::REGION) continue;
        remap[region.id] = nextId;
        region.id = nextId++;
      }
      if (!remap.empty()) remaps[spritesheet.id] = std::move(remap);
    }

    if (!layers || !animations || remaps.empty()) return;

    std::unordered_map<int, int> layerSpritesheets{};
    for (const auto& layer : layers->children)
      if (layer.type == ElementType::LAYER_ELEMENT) layerSpritesheets[layer.id] = layer.spritesheetId;

    for (auto& animation : animations->children)
    {
      if (animation.type != ElementType::ANIMATION) continue;
      auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS);
      if (!layerAnimations) continue;

      tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                  [&](Element& layerAnimation)
                  {
                    auto layer = layerSpritesheets.find(layerAnimation.layerId);
                    if (layer == layerSpritesheets.end()) return;
                    auto remap = remaps.find(layer->second);
                    if (remap == remaps.end()) return;

                    for (auto& frame : layerAnimation.children)
                    {
                      if (frame.type != ElementType::FRAME || frame.regionId == -1) continue;
                      if (auto it = remap->second.find(frame.regionId); it != remap->second.end())
                        frame.regionId = it->second;
                      else
                        frame.regionId = -1;
                    }
                  });
    }
  }

  void shader_ids_remap(Element& root)
  {
    auto content = child_first_get(root, ElementType::CONTENT);
    auto shaders = content ? child_first_get(*content, ElementType::SHADERS) : nullptr;

    std::unordered_map<int, int> remap{};
    int nextId{};
    if (shaders)
      for (auto& shader : shaders->children)
      {
        if (shader.type != ElementType::SHADER) continue;
        remap[shader.id] = nextId;
        shader.id = nextId++;
      }

    auto frame_remap = [&](auto&& self, Element& element) -> void
    {
      if (element.type == ElementType::FRAME && element.shaderId != -1)
      {
        if (auto it = remap.find(element.shaderId); it != remap.end())
          element.shaderId = it->second;
        else
          element.shaderId = -1;
      }
      for (auto& child : element.children)
        self(self, child);
    };
    frame_remap(frame_remap, root);
  }

  void Anm2::region_frames_sync(bool isClearInvalid)
  {
    auto content = child_first_get(root, ElementType::CONTENT);
    auto layers = content ? child_first_get(*content, ElementType::LAYERS) : nullptr;
    auto spritesheets = content ? child_first_get(*content, ElementType::SPRITESHEETS) : nullptr;
    auto animations = element_first_get(root, ElementType::ANIMATIONS);
    if (!layers || !spritesheets || !animations) return;

    for (auto& animation : animations->children)
    {
      if (animation.type != ElementType::ANIMATION) continue;
      auto layerAnimations = child_first_get(animation, ElementType::LAYER_ANIMATIONS);
      if (!layerAnimations) continue;

      tracks_each(*layerAnimations, ElementType::LAYER_ANIMATION,
                  [&](Element& layerAnimation)
                  {
                    auto layer = child_id_get(*layers, ElementType::LAYER_ELEMENT, layerAnimation.layerId);
                    auto spritesheet =
                        layer ? child_id_get(*spritesheets, ElementType::SPRITESHEET, layer->spritesheetId) : nullptr;
                    if (!spritesheet) return;

                    for (auto& frame : layerAnimation.children)
                    {
                      if (frame.type != ElementType::FRAME || frame.regionId == -1) continue;
                      auto region = child_id_get(*spritesheet, ElementType::REGION, frame.regionId);
                      if (!region)
                      {
                        if (isClearInvalid) frame.regionId = -1;
                        continue;
                      }

                      frame.crop = region->crop;
                      frame.size = region->size;
                      frame.pivot = region->pivot;
                    }
                  });
    }
  }

  Anm2 Anm2::normalized_for_serialize(Flags flags) const
  {
    auto normalized = *this;
    source_document_erase(normalized.root);
    group_frames_restore(normalized.root);
    groups_flatten(normalized.root);
    if (has_flag(flags, SERIALIZE_BAKE_GROUP_FRAMES)) group_frames_bake(normalized.root);
    if (has_flag(flags, SERIALIZE_FLATTEN_SPECIAL_INTERPOLATED_FRAMES))
      all_interpolated_frames_bake(normalized.root, FRAME_DURATION_MIN, false, false);
    region_ids_remap(normalized.root);
    shader_ids_remap(normalized.root);
    if (has_flag(flags, SERIALIZE_GROUPS) && has_flag(flags, SERIALIZE_NESTED_GROUPS))
      groups_nest(normalized.root);
    else
      group_metadata_extract(normalized.root, flags);

    auto content = child_first_get(normalized.root, ElementType::CONTENT);
    if (content) std::erase_if(content->children, [](const Element& element) { return element.tag == "Groups"; });
    if (auto layers = content ? child_first_get(*content, ElementType::LAYERS) : nullptr)
    {
      std::unordered_map<int, int> remap{};
      int nextId{};
      for (auto& layer : layers->children)
      {
        if (layer.type != ElementType::LAYER_ELEMENT) continue;
        remap[layer.id] = nextId;
        layer.id = nextId++;
      }
      layer_animation_ids_remap(normalized.root, remap);
    }

    return normalized;
  }
}
