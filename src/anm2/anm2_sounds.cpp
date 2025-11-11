#include "anm2.h"

#include <ranges>

#include "filesystem_.h"
#include "map_.h"

using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  bool Anm2::sound_add(const std::string& directory, const std::string& path, int& id)
  {
    id = map::next_id_get(content.sounds);
    content.sounds[id] = Sound(directory, path);
    return true;
  }

  std::vector<std::string> Anm2::sound_labels_get()
  {
    std::vector<std::string> labels{};
    labels.emplace_back("None");
    for (auto& [id, sound] : content.sounds)
      labels.emplace_back(sound.path.string());
    return labels;
  }

  std::set<int> Anm2::sounds_unused()
  {
    std::set<int> used;
    for (auto& animation : animations.items)
      for (auto& trigger : animation.triggers.frames)
        if (content.sounds.contains(trigger.soundID)) used.insert(trigger.soundID);

    std::set<int> unused;
    for (auto& id : content.sounds | std::views::keys)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  bool Anm2::sounds_deserialize(const std::string& string, const std::string& directory, merge::Type type,
                                std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      int id{};

      if (!document.FirstChildElement("Sound"))
      {
        if (errorString) *errorString = "No valid sound(s).";
        return false;
      }

      filesystem::WorkingDirectory workingDirectory(directory);

      for (auto element = document.FirstChildElement("Sound"); element; element = element->NextSiblingElement("Sound"))
      {
        auto sound = Sound(element, id);
        if (type == merge::APPEND) id = map::next_id_get(content.sounds);
        content.sounds[id] = std::move(sound);
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }
}
