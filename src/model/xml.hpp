#pragma once

#include <filesystem>
#include <set>
#include <string>

#include "model.hpp"

// The only place the model meets XML: reading and writing documents (with the SourceDocument copy) and clipboard text.
namespace anm2ed::model
{
  Model model_make();
  bool model_load(Model&, const std::filesystem::path&, std::string* = nullptr);
  bool model_load_string(Model&, std::string_view, std::string* = nullptr);
  bool model_save(const Model&, const std::filesystem::path&, std::string* = nullptr, Options = {});
  std::string model_to_string(const Model&, Options = {});
  std::string model_serialize(const Model&, Flags);
  std::uint64_t model_hash(const Model&, Options = {});
  bool is_special_interpolated_frames(const Model&);

  std::string frame_to_string(const Frame&, ItemType);
  std::string animation_to_string(const Animation&, int = -1);
  std::string animation_group_to_string(const AnimationGroup&);
  std::vector<Frame> frames_from_string(const std::string&, ItemType, std::string* = nullptr);
  std::vector<AnimationEntry> animations_from_string(const std::string&, std::string* = nullptr);

  template <class Item> std::string item_to_string(const Item&);
  template <class Item> std::vector<Item> items_from_string(const std::string&, std::string* = nullptr);
}
