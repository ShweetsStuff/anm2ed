#include "anm2.h"

#include <ranges>

#include "filesystem_.h"
#include "map_.h"

using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  Spritesheet* Anm2::spritesheet_get(int id) { return map::find(content.spritesheets, id); }

  bool Anm2::spritesheet_add(const std::string& directory, const std::string& path, int& id)
  {
    Spritesheet spritesheet(directory, path);
    if (!spritesheet.is_valid()) return false;
    id = map::next_id_get(content.spritesheets);
    content.spritesheets[id] = std::move(spritesheet);
    return true;
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

  std::vector<std::string> Anm2::spritesheet_labels_get()
  {
    std::vector<std::string> labels{};
    labels.emplace_back(localize.get(BASIC_NONE));
    for (auto& [id, spritesheet] : content.spritesheets)
    {
      auto string = spritesheet.path.string();
      labels.emplace_back(std::vformat(localize.get(FORMAT_SPRITESHEET), std::make_format_args(id, string)));
    }
    return labels;
  }

  bool Anm2::spritesheets_deserialize(const std::string& string, const std::string& directory, merge::Type type,
                                      std::string* errorString)
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

      filesystem::WorkingDirectory workingDirectory(directory);

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
