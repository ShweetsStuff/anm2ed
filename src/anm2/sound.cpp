#include "sound.h"

#include "filesystem_.h"
#include "xml_.h"

using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  Sound::Sound(const Sound& other) : path(other.path) { audio = path.empty() ? Audio() : Audio(path.c_str()); }

  Sound& Sound::operator=(const Sound& other)
  {
    if (this != &other)
    {
      path = other.path;
      audio = path.empty() ? Audio() : Audio(path.c_str());
    }
    return *this;
  }

  Sound::Sound(const std::string& directory, const std::string& path)
  {
    filesystem::WorkingDirectory workingDirectory(directory);
    this->path = !path.empty() ? std::filesystem::relative(path) : this->path;
    this->path = filesystem::path_lower_case_backslash_handle(this->path);
    audio = Audio(this->path.c_str());
  }

  Sound::Sound(const std::filesystem::path& directory, const std::filesystem::path& path)
      : Sound(directory.string(), path.string())
  {
  }

  Sound::Sound(XMLElement* element, int& id)
  {
    if (!element) return;
    element->QueryIntAttribute("Id", &id);
    xml::query_path_attribute(element, "Path", &path);
    path = filesystem::path_lower_case_backslash_handle(path);
    audio = Audio(path.c_str());
  }

  XMLElement* Sound::to_element(XMLDocument& document, int id)
  {
    auto element = document.NewElement("Sound");
    element->SetAttribute("Id", id);
    element->SetAttribute("Path", path.c_str());
    return element;
  }

  void Sound::serialize(XMLDocument& document, XMLElement* parent, int id)
  {
    parent->InsertEndChild(to_element(document, id));
  }

  std::string Sound::to_string(int id)
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document, id));
    return xml::document_to_string(document);
  }

  void Sound::reload(const std::string& directory) { *this = Sound(directory, this->path); }

  bool Sound::is_valid() { return audio.is_valid(); }

  void Sound::play() { audio.play(); }
}
