#include "sound.h"

#include "filesystem_.h"
#include "string_.h"
#include "xml_.h"

using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  Sound::Sound(const Sound& other) : path(other.path)
  {
    audio = path.empty() ? Audio() : Audio(path.c_str());
  }

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
    this->path = !path.empty() ? std::filesystem::relative(path).string() : this->path.string();
    this->path = string::backslash_replace_to_lower(this->path);
    audio = Audio(this->path.c_str());
  }

  Sound::Sound(XMLElement* element, int& id)
  {
    if (!element) return;
    element->QueryIntAttribute("Id", &id);
    xml::query_path_attribute(element, "Path", &path);
    string::backslash_replace_to_lower(this->path);
    audio = Audio(this->path.c_str());
  }

  XMLElement* Sound::to_element(XMLDocument& document, int id)
  {
    auto element = document.NewElement("Sound");
    element->SetAttribute("Id", id);
    element->SetAttribute("Path", path.c_str());
    return element;
  }

  std::string Sound::to_string(int id)
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document, id));
    return xml::document_to_string(document);
  }
}