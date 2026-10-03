#include "internal.hpp"

namespace anm2ed
{
  constexpr std::string_view SHADER_NAME_DEFAULT = "New Shader";

  void frame_ids_remap(Element& root, int Element::* member, const std::unordered_map<int, int>& remap)
  {
    element_each(root,
                 [&](Element& element)
                 {
                   if (element.type != ElementType::FRAME || element.*member == -1) return;
                   auto it = remap.find(element.*member);
                   element.*member = it == remap.end() ? -1 : it->second;
                 });
  }

  template <class Callback> void layer_frames_each(Element& root, Callback&& callback)
  {
    auto layers = content_container_get(root, ElementType::LAYERS);
    auto spritesheets = content_container_get(root, ElementType::SPRITESHEETS);
    if (!layers || !spritesheets) return;

    animations_tracks_each(root, ElementType::LAYER_ANIMATION,
                           [&](Element& track)
                           {
                             auto layer = child_id_get(*layers, ElementType::LAYER_ELEMENT, track.layerId);
                             auto spritesheet = layer ? child_id_get(*spritesheets, ElementType::SPRITESHEET,
                                                                     layer->spritesheetId)
                                                      : nullptr;
                             if (!spritesheet) return;
                             for (auto& frame : track.children)
                               if (frame.type == ElementType::FRAME) callback(frame, *spritesheet);
                           });
  }

  void shader_ids_repair(Element& root)
  {
    auto shaders = content_container_get(root, ElementType::SHADERS);
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
      if (shader.name.empty()) shader.name = SHADER_NAME_DEFAULT;
    }
  }

  void shader_frame_ids_repair(Element& root)
  {
    auto shaders = content_container_get(root, ElementType::SHADERS);
    auto animations = element_first_get(root, ElementType::ANIMATIONS);
    if (!animations) return;

    std::unordered_map<int, int> identity{};
    if (shaders)
      for (const auto& shader : shaders->children)
        if (shader.type == ElementType::SHADER) identity[shader.id] = shader.id;
    frame_ids_remap(*animations, &Element::shaderId, identity);
  }

  void region_frame_ids_repair(Element& root)
  {
    layer_frames_each(root,
                      [](Element& frame, const Element& spritesheet)
                      {
                        auto is_region_match = [&](const Element& region)
                        {
                          return region.type == ElementType::REGION &&
                                 glm::ivec2(region.crop) == glm::ivec2(frame.crop) &&
                                 glm::ivec2(region.size) == glm::ivec2(frame.size) &&
                                 glm::ivec2(region.pivot) == glm::ivec2(frame.pivot);
                        };

                        auto region = frame.regionId == -1
                                          ? nullptr
                                          : child_id_get(spritesheet, ElementType::REGION, frame.regionId);
                        if (region && is_region_match(*region)) return;
                        if (auto candidate = child_find(spritesheet, is_region_match)) frame.regionId = candidate->id;
                      });
  }

  void region_ids_remap(Element& root)
  {
    auto spritesheets = content_container_get(root, ElementType::SPRITESHEETS);
    if (!spritesheets) return;

    std::unordered_map<int, std::unordered_map<int, int>> remaps{};
    for (auto& spritesheet : spritesheets->children)
      if (spritesheet.type == ElementType::SPRITESHEET)
        if (auto remap = child_ids_compact(spritesheet, ElementType::REGION); !remap.empty())
          remaps[spritesheet.id] = std::move(remap);

    if (remaps.empty()) return;

    layer_frames_each(root,
                      [&](Element& frame, const Element& spritesheet)
                      {
                        auto remap = remaps.find(spritesheet.id);
                        if (remap == remaps.end() || frame.regionId == -1) return;
                        auto it = remap->second.find(frame.regionId);
                        frame.regionId = it == remap->second.end() ? -1 : it->second;
                      });
  }

  void shader_ids_remap(Element& root)
  {
    auto shaders = content_container_get(root, ElementType::SHADERS);
    auto remap = shaders ? child_ids_compact(*shaders, ElementType::SHADER) : std::unordered_map<int, int>{};
    frame_ids_remap(root, &Element::shaderId, remap);
  }

  void Anm2::region_frames_sync(bool isClearInvalid)
  {
    layer_frames_each(root,
                      [&](Element& frame, const Element& spritesheet)
                      {
                        if (frame.regionId == -1) return;
                        auto region = child_id_get(spritesheet, ElementType::REGION, frame.regionId);
                        if (!region)
                        {
                          if (isClearInvalid) frame.regionId = -1;
                          return;
                        }

                        frame.crop = region->crop;
                        frame.size = region->size;
                        frame.pivot = region->pivot;
                      });
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

    if (auto content = child_first_get(normalized.root, ElementType::CONTENT))
      std::erase_if(content->children, [](const Element& element) { return element.tag == "Groups"; });
    if (auto layers = content_container_get(normalized.root, ElementType::LAYERS))
    {
      auto remap = child_ids_compact(*layers, ElementType::LAYER_ELEMENT);
      element_each(normalized.root,
                   [&](Element& element)
                   {
                     if (element.type != ElementType::LAYER_ANIMATION) return;
                     if (auto it = remap.find(element.layerId); it != remap.end()) element.layerId = it->second;
                   });
    }

    return normalized;
  }
}
