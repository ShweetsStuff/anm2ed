#include "sound.h"

#include "filesystem_.h"
#include "xml_.h"

using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  Sound::Sound(const Sound& other) : path(other.path) { audio = path.empty() ? Audio() : Audio(path); }

  Sound& Sound::operator=(const Sound& other)
  {
    if (this != &other)
    {
      path = other.path;
      audio = path.empty() ? Audio() : Audio(path);
    }
    return *this;
  }

  namespace
  {
    std::filesystem::path make_relative_or_keep(const std::filesystem::path& input)
    {
      if (input.empty()) return input;
      std::error_code ec{};
      auto relative = std::filesystem::relative(input, ec);
      if (!ec) return relative;
      return input;
    }
  }

  Sound::Sound(const std::string& directory, const std::string& path)
  {
    filesystem::WorkingDirectory workingDirectory(directory);
    this->path = !path.empty() ? make_relative_or_keep(path) : this->path;
    this->path = filesystem::path_lower_case_backslash_handle(this->path);
    audio = Audio(this->path);
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
    audio = Audio(path);
  }

  XMLElement* Sound::to_element(XMLDocument& document, int id)
  {
    auto element = document.NewElement("Sound");
    element->SetAttribute("Id", id);
    auto pathString = path.generic_string();
    element->SetAttribute("Path", pathString.c_str());
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

  void Sound::reload(const std::filesystem::path& directory) { *this = Sound(directory, this->path); }

  bool Sound::is_valid() { return audio.is_valid(); }

  void Sound::play() { audio.play(); }
}
