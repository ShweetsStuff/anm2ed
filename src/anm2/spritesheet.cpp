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

  Spritesheet::Spritesheet(const std::filesystem::path& directory, const std::filesystem::path& path)
  {
    filesystem::WorkingDirectory workingDirectory(directory);
    this->path = !path.empty() ? make_relative_or_keep(path) : this->path;
    this->path = filesystem::path_lower_case_backslash_handle(this->path);
    texture = Texture(this->path);
  }

  XMLElement* Spritesheet::to_element(XMLDocument& document, int id)
  {
    auto element = document.NewElement("Spritesheet");
    element->SetAttribute("Id", id);
    auto pathString = path.generic_string();
    element->SetAttribute("Path", pathString.c_str());
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

  bool Spritesheet::save(const std::filesystem::path& directory, const std::filesystem::path& path)
  {
    filesystem::WorkingDirectory workingDirectory(directory);
    this->path = !path.empty() ? make_relative_or_keep(path) : this->path;
    return texture.write_png(this->path);
  }

  void Spritesheet::reload(const std::filesystem::path& directory) { *this = Spritesheet(directory, this->path); }

  bool Spritesheet::is_valid() { return texture.is_valid(); }

}
