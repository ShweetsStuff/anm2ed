#include "internal.hpp"

using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed
{
  constexpr std::uint64_t ANM2_HASH_OFFSET = 14695981039346656037ull;
  constexpr std::uint64_t ANM2_HASH_PRIME = 1099511628211ull;

  struct BakeAttributes
  {
    Interpolation interpolation{Interpolation::NONE};
    int delay{FRAME_DURATION_MIN};
  };

  void element_hash_append(std::uint64_t&, const Element&, ElementType, Flags,
                           std::optional<BakeAttributes> = std::nullopt);


  void hash_byte_append(std::uint64_t& hash, unsigned char value)
  {
    hash ^= value;
    hash *= ANM2_HASH_PRIME;
  }

  void hash_bytes_append(std::uint64_t& hash, const char* data, std::size_t size)
  {
    for (std::size_t i = 0; i < size; ++i)
      hash_byte_append(hash, (unsigned char)data[i]);
  }

  void hash_string_append(std::uint64_t& hash, std::string_view value)
  {
    auto size = (std::uint64_t)value.size();
    hash_bytes_append(hash, reinterpret_cast<const char*>(&size), sizeof(size));
    hash_bytes_append(hash, value.data(), value.size());
  }

  void hash_attr_append(std::uint64_t& hash, std::string_view name, std::string_view value)
  {
    hash_byte_append(hash, 'A');
    hash_string_append(hash, name);
    hash_string_append(hash, value);
  }

  void hash_attr_append(std::uint64_t& hash, std::string_view name, int value)
  {
    char buffer[32]{};
    XMLUtil::ToStr(value, buffer, sizeof(buffer));
    hash_attr_append(hash, name, std::string_view(buffer));
  }

  void hash_attr_append(std::uint64_t& hash, std::string_view name, bool isValue)
  {
    char buffer[8]{};
    XMLUtil::ToStr(isValue, buffer, sizeof(buffer));
    hash_attr_append(hash, name, std::string_view(buffer));
  }

  void hash_attr_append(std::uint64_t& hash, std::string_view name, float value)
  {
    char buffer[64]{};
    XMLUtil::ToStr(value, buffer, sizeof(buffer));
    hash_attr_append(hash, name, std::string_view(buffer));
  }

  void hash_path_attr_append(std::uint64_t& hash, std::string_view name, const std::filesystem::path& pathValue)
  {
    if (pathValue.empty()) return;
    auto value = path::to_utf8(pathValue);
    hash_attr_append(hash, name, value);
  }

  void hash_interpolation_attr_append(std::uint64_t& hash, Interpolation interpolation)
  {
    if (interpolation == Interpolation::NONE || interpolation == Interpolation::LINEAR)
    {
      hash_attr_append(hash, "Interpolated", interpolation == Interpolation::LINEAR);
      return;
    }

    auto value = INTERPOLATION_VALUES[(std::size_t)interpolation];
    if (!value.empty()) hash_attr_append(hash, "Interpolated", value);
  }

  void hash_origin_attr_append(std::uint64_t& hash, const Element& element)
  {
    auto origin = ORIGIN_VALUES[(std::size_t)element.origin];
    if (!origin.empty())
    {
      hash_attr_append(hash, "Origin", origin);
      return;
    }

    hash_attr_append(hash, "XPivot", element.pivot.x);
    hash_attr_append(hash, "YPivot", element.pivot.y);
  }

  void frame_attributes_hash_append(std::uint64_t& hash, const Element& element, ElementType parentType, Flags flags)
  {
    if (parentType == ElementType::LAYER_ANIMATION)
    {
      bool isHasValidRegion = has_flag(flags, SERIALIZE_REGIONS) && element.regionId != -1;
      bool isWriteRegionValues = has_flag(flags, SERIALIZE_REDUNDANT_FRAME_REGION_VALUES) || !isHasValidRegion;

      if (isHasValidRegion) hash_attr_append(hash, "RegionId", element.regionId);
      if (isWriteRegionValues)
      {
        hash_attr_append(hash, "XPivot", element.pivot.x);
        hash_attr_append(hash, "YPivot", element.pivot.y);
        hash_attr_append(hash, "XCrop", element.crop.x);
        hash_attr_append(hash, "YCrop", element.crop.y);
        hash_attr_append(hash, "Width", element.size.x);
        hash_attr_append(hash, "Height", element.size.y);
      }
    }

    hash_attr_append(hash, "XPosition", element.position.x);
    hash_attr_append(hash, "YPosition", element.position.y);
    hash_attr_append(hash, "Delay", element.duration);
    hash_attr_append(hash, "Visible", element.isVisible);
    hash_attr_append(hash, "XScale", element.scale.x);
    hash_attr_append(hash, "YScale", element.scale.y);
    if (has_flag(flags, SERIALIZE_EXTENSIONS))
    {
      hash_attr_append(hash, "ShearX", element.shear.x);
      hash_attr_append(hash, "ShearY", element.shear.y);
    }
    hash_attr_append(hash, "RedTint", color_write(element.tint.r));
    hash_attr_append(hash, "GreenTint", color_write(element.tint.g));
    hash_attr_append(hash, "BlueTint", color_write(element.tint.b));
    hash_attr_append(hash, "AlphaTint", color_write(element.tint.a));
    hash_attr_append(hash, "RedOffset", color_write(element.colorOffset.r));
    hash_attr_append(hash, "GreenOffset", color_write(element.colorOffset.g));
    hash_attr_append(hash, "BlueOffset", color_write(element.colorOffset.b));
    hash_attr_append(hash, "Rotation", element.rotation);
    hash_interpolation_attr_append(hash, element.interpolation);
    if (has_flag(flags, SERIALIZE_EXTENSIONS) && element.shaderId != -1)
      hash_attr_append(hash, "ShaderId", element.shaderId);
  }

  void bake_attributes_hash_append(std::uint64_t& hash, Interpolation interpolation, int bakeDelay)
  {
    auto value = INTERPOLATION_VALUES[(std::size_t)interpolation];
    if (!value.empty()) hash_attr_append(hash, "BakeInterpolation", value);
    hash_attr_append(hash, "BakeDelay", bakeDelay);
  }

  void element_attributes_hash_append(std::uint64_t& hash, const Element& element, ElementType parentType, Flags flags)
  {
    if (element.type == ElementType::INFO)
    {
      hash_attr_append(hash, "CreatedBy", element.createdBy);
      hash_attr_append(hash, "CreatedOn", element.createdOn);
      hash_attr_append(hash, "Fps", element.fps);
      hash_attr_append(hash, "Version", element.version);
    }
    else if (element.type == ElementType::ANIMATIONS)
      hash_attr_append(hash, "DefaultAnimation", element.defaultAnimation);
    else if (element.type == ElementType::SPRITESHEET)
    {
      hash_attr_append(hash, "Id", element.id);
      hash_path_attr_append(hash, "Path", element.path);
    }
    else if (element.type == ElementType::SHADER)
    {
      hash_attr_append(hash, "Id", element.id);
      hash_attr_append(hash, "Name", element.name);
      hash_path_attr_append(hash, "Vertex", element.vertex);
      hash_path_attr_append(hash, "Fragment", element.fragment);
    }
    else if (element.type == ElementType::UNIFORM)
    {
      hash_attr_append(hash, "Name", element.name);
      if (!element.binding.empty()) hash_attr_append(hash, "Binding", element.binding);
      if (!element.value.empty()) hash_attr_append(hash, "Value", element.value);
    }
    else if (element.type == ElementType::COMPONENT)
    {
      hash_attr_append(hash, "Index", element.index);
      if (!element.binding.empty()) hash_attr_append(hash, "Binding", element.binding);
      if (!element.value.empty()) hash_attr_append(hash, "Value", element.value);
    }
    else if (element.type == ElementType::REGION)
    {
      hash_attr_append(hash, "Id", element.id);
      hash_attr_append(hash, "Name", element.name);
      hash_attr_append(hash, "XCrop", element.crop.x);
      hash_attr_append(hash, "YCrop", element.crop.y);
      hash_attr_append(hash, "Width", element.size.x);
      hash_attr_append(hash, "Height", element.size.y);
      hash_origin_attr_append(hash, element);
    }
    else if (element.type == ElementType::LAYER_ELEMENT)
    {
      hash_attr_append(hash, "Id", element.id);
      hash_attr_append(hash, "Name", element.name);
      hash_attr_append(hash, "SpritesheetId", element.spritesheetId);
    }
    else if (element.type == ElementType::NULL_ELEMENT)
    {
      hash_attr_append(hash, "Id", element.id);
      hash_attr_append(hash, "Name", element.name);
      if (element.isShowRect) hash_attr_append(hash, "ShowRect", element.isShowRect);
    }
    else if (element.type == ElementType::GROUP)
    {
      if (!is_nested_group_parent(parentType, flags)) hash_attr_append(hash, "Id", element.id);
      hash_attr_append(hash, "Name", element.name);
      hash_attr_append(hash, "IsExpanded", element.isExpanded);
      hash_attr_append(hash, "Visible", element.isVisible);
      if ((parentType == ElementType::LAYER_ANIMATION_GROUPS || parentType == ElementType::NULL_ANIMATION_GROUPS) &&
          element.index != -1)
        hash_attr_append(hash, "Index", element.index);
    }
    else if (element.type == ElementType::EVENT_ELEMENT)
    {
      hash_attr_append(hash, "Id", element.id);
      hash_attr_append(hash, "Name", element.name);
    }
    else if (element.type == ElementType::SOUND_ELEMENT)
    {
      hash_attr_append(hash, "Id", element.id);
      if (parentType != ElementType::TRIGGER) hash_path_attr_append(hash, "Path", element.path);
    }
    else if (element.type == ElementType::ANIMATION)
    {
      hash_attr_append(hash, "Name", element.name);
      hash_attr_append(hash, "FrameNum", element.frameNum);
      hash_attr_append(hash, "Loop", element.isLoop);
      if (element.groupId != -1 && is_group_id_serialized(flags)) hash_attr_append(hash, "GroupId", element.groupId);
    }
    else if (element.type == ElementType::LAYER_ANIMATION)
    {
      hash_attr_append(hash, "LayerId", element.layerId);
      hash_attr_append(hash, "Visible", element.isVisible);
      if (element.groupId != -1 && is_group_id_serialized(flags)) hash_attr_append(hash, "GroupId", element.groupId);
    }
    else if (element.type == ElementType::NULL_ANIMATION)
    {
      hash_attr_append(hash, "NullId", element.nullId);
      hash_attr_append(hash, "Visible", element.isVisible);
      if (element.groupId != -1 && is_group_id_serialized(flags)) hash_attr_append(hash, "GroupId", element.groupId);
    }
    else if (element.type == ElementType::FRAME)
      frame_attributes_hash_append(hash, element, parentType, flags);
    else if (element.type == ElementType::TRIGGER)
    {
      if (element.eventId != -1) hash_attr_append(hash, "EventId", element.eventId);
      hash_attr_append(hash, "AtFrame", element.atFrame);
    }
  }

  void trigger_sounds_hash_append(std::uint64_t& hash, const Element& element, Flags flags)
  {
    if (element.type != ElementType::TRIGGER || !has_flag(flags, SERIALIZE_SOUNDS)) return;

    for (auto soundId : element.soundIds)
    {
      if (soundId == -1) continue;
      auto sound = element_make(ElementType::SOUND_ELEMENT);
      sound.id = soundId;
      element_hash_append(hash, sound, ElementType::TRIGGER, flags);
    }
  }

  void baked_frames_hash_append(std::uint64_t& hash, const Element& track, int index, Flags flags)
  {
    const auto& original = track.children[index];
    auto nextFrame = index + 1 < (int)track.children.size() && track.children[index + 1].type == ElementType::FRAME
                         ? track.children[index + 1]
                         : original;
    auto bakeDelay = std::max(original.duration, FRAME_DURATION_MIN);

    for (int bakeIndex = 0; bakeIndex < bakeDelay; ++bakeIndex)
    {
      auto baked = original;
      auto amount = interpolation_factor(original.interpolation, (float)bakeIndex / (float)bakeDelay);
      baked.duration = FRAME_DURATION_MIN;
      baked.interpolation = Interpolation::NONE;
      baked.rotation = glm::mix(original.rotation, nextFrame.rotation, amount);
      baked.position = glm::mix(original.position, nextFrame.position, amount);
      baked.scale = glm::mix(original.scale, nextFrame.scale, amount);
      baked.shear = glm::mix(original.shear, nextFrame.shear, amount);
      baked.colorOffset = glm::mix(original.colorOffset, nextFrame.colorOffset, amount);
      baked.tint = glm::mix(original.tint, nextFrame.tint, amount);
      element_hash_append(hash, baked, track.type, flags,
                          bakeIndex == 0 ? std::optional<BakeAttributes>({original.interpolation, bakeDelay})
                                         : std::nullopt);
    }
  }

  void element_hash_append(std::uint64_t& hash, const Element& element, ElementType parentType, Flags flags,
                           std::optional<BakeAttributes> bakeAttributes)
  {
    auto tag = element.type == ElementType::UNKNOWN ? std::string_view(element.tag) : element_tag_get(element.type);
    hash_byte_append(hash, 'E');
    hash_string_append(hash, tag.empty() ? std::string_view(element.tag) : tag);

    element_attributes_hash_append(hash, element, parentType, flags);
    if (bakeAttributes) bake_attributes_hash_append(hash, bakeAttributes->interpolation, bakeAttributes->delay);

    hash_byte_append(hash, '[');
    trigger_sounds_hash_append(hash, element, flags);
    for (int i = 0; i < (int)element.children.size(); ++i)
    {
      const auto& child = element.children[i];
      if (element_write_skip(child, element.type, flags)) continue;

      if (is_track(element) && is_frame_bake_serialized(child, flags))
        baked_frames_hash_append(hash, element, i, flags);
      else
        element_hash_append(hash, child, element.type, flags);
    }
    hash_byte_append(hash, ']');
  }

  std::uint64_t element_hash(const Element& element, Flags flags)
  {
    auto hash = ANM2_HASH_OFFSET;
    element_hash_append(hash, element, ElementType::UNKNOWN, flags);
    return hash;
  }

  std::uint64_t anm2_hash_get(const Element& root, Options options)
  {
    auto hash = ANM2_HASH_OFFSET;
    hash_byte_append(hash, options.isExtendedFormat ? 'E' : 'A');
    element_hash_append(hash, root, ElementType::UNKNOWN, SERIALIZE_ANM2ED_DEFAULT);
    return hash;
  }
}
