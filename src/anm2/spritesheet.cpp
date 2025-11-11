#include "spritesheet.h"

#include "filesystem_.h"
#include "xml_.h"

using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  Spritesheet::Spritesheet(XMLElement* element, int& id)
  {
    if (!element) return;
    element->QueryIntAttribute("Id", &id);
    xml::query_path_attribute(element, "Path", &path);
    // Spritesheet paths from Isaac Rebirth are made with the assumption that paths are case-insensitive
    // However when using the resource dumper, the spritesheet paths are all lowercase (on Linux anyway)
    // This will handle this case and make the paths OS-agnostic
    path = filesystem::path_lower_case_backslash_handle(path);
    texture = Texture(path);
  }

  Spritesheet::Spritesheet(const std::string& directory, const std::string& path)
  {
    filesystem::WorkingDirectory workingDirectory(directory);
    this->path = !path.empty() ? std::filesystem::relative(path) : this->path;
    this->path = filesystem::path_lower_case_backslash_handle(this->path);
    texture = Texture(this->path);
  }

  XMLElement* Spritesheet::to_element(XMLDocument& document, int id)
  {
    auto element = document.NewElement("Spritesheet");
    element->SetAttribute("Id", id);
    element->SetAttribute("Path", path.c_str());
    return element;
  }

  void Spritesheet::serialize(XMLDocument& document, XMLElement* parent, int id)
  {
    parent->InsertEndChild(to_element(document, id));
  }

  std::string Spritesheet::to_string(int id)
  {
    XMLDocument document{};
    document.InsertEndChild(to_element(document, id));
    return xml::document_to_string(document);
  }

  bool Spritesheet::save(const std::string& directory, const std::string& path)
  {
    filesystem::WorkingDirectory workingDirectory(directory);
    this->path = !path.empty() ? std::filesystem::relative(path).string() : this->path.string();
    return texture.write_png(this->path);
  }

  void Spritesheet::reload(const std::string& directory) { *this = Spritesheet(directory, this->path); }

  bool Spritesheet::is_valid() { return texture.is_valid(); }

}