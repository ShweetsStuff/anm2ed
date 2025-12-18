#include "anm2.h"

#include "map_.h"
#include "path_.h"
#include "working_directory.h"

using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  bool Anm2::sound_add(const std::filesystem::path& directory, const std::filesystem::path& path, int& id)
  {
    id = map::next_id_get(content.sounds);
    content.sounds[id] = Sound(directory, path);
    return true;
  }

  std::vector<std::string> Anm2::sound_labels_get()
  {
    std::vector<std::string> labels{};
    labels.emplace_back(localize.get(BASIC_NONE));
    for (auto& [id, sound] : content.sounds)
    {
      auto pathString = path::to_utf8(sound.path);
      labels.emplace_back(std::vformat(localize.get(FORMAT_SOUND), std::make_format_args(id, pathString)));
    }
    return labels;
  }

  std::set<int> Anm2::sounds_unused()
  {
    std::set<int> used;
    for (auto& animation : animations.items)
      for (auto& trigger : animation.triggers.frames)
        if (content.sounds.contains(trigger.soundID)) used.insert(trigger.soundID);

    std::set<int> unused;
    for (auto& [id, sound] : content.sounds)
      if (!used.contains(id)) unused.insert(id);

    return unused;
  }

  bool Anm2::sounds_deserialize(const std::string& string, const std::filesystem::path& directory, merge::Type type,
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

      WorkingDirectory workingDirectory(directory);

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
