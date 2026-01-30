#include "spritesheet.h"

#include <ranges>

#include "path_.h"
#include "working_directory.h"
#include "xml_.h"
#include "map_.h"

using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace anm2ed::types;
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
    path = path::lower_case_backslash_handle(path);
    texture = Texture(path);

    for (auto child = element->FirstChildElement("Region"); child; child = child->NextSiblingElement("Region"))
    {
      Region region{};
      int id{};
      child->QueryIntAttribute("Id", &id);
      xml::query_string_attribute(child, "Name", &region.name);
      child->QueryFloatAttribute("CropX", &region.crop.x);
      child->QueryFloatAttribute("CropY", &region.crop.y);
      child->QueryFloatAttribute("PivotX", &region.pivot.x);
      child->QueryFloatAttribute("PivotY", &region.pivot.y);
      child->QueryFloatAttribute("Width", &region.size.x);
      child->QueryFloatAttribute("Height", &region.size.y);
      regions.emplace(id, std::move(region));
    }
  }

  Spritesheet::Spritesheet(const std::filesystem::path& directory, const std::filesystem::path& path)
  {
    WorkingDirectory workingDirectory(directory);
    this->path = !path.empty() ? path::make_relative(path) : this->path;
    this->path = path::lower_case_backslash_handle(this->path);
    texture = Texture(this->path);
  }

  XMLElement* Spritesheet::to_element(XMLDocument& document, int id)
  {
    auto element = document.NewElement("Spritesheet");
    element->SetAttribute("Id", id);
    auto pathString = path::to_utf8(path);
    element->SetAttribute("Path", pathString.c_str());

    for (auto [i, region] : regions)
    {
      auto regionElement = element->InsertNewChildElement("Region");
      regionElement->SetAttribute("Id", i);
      regionElement->SetAttribute("Name", region.name.c_str());
      regionElement->SetAttribute("CropX", region.crop.x);
      regionElement->SetAttribute("CropY", region.crop.y);
      regionElement->SetAttribute("PivotX", region.pivot.x);
      regionElement->SetAttribute("PivotY", region.pivot.y);
      regionElement->SetAttribute("Width", region.size.x);
      regionElement->SetAttribute("Height", region.size.y);
    }

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

  std::string Spritesheet::region_to_string(int id)
  {
    if (!regions.contains(id)) return {};

    XMLDocument document{};
    auto element = document.NewElement("Region");
    auto& region = regions.at(id);
    element->SetAttribute("Id", id);
    element->SetAttribute("Name", region.name.c_str());
    element->SetAttribute("CropX", region.crop.x);
    element->SetAttribute("CropY", region.crop.y);
    element->SetAttribute("PivotX", region.pivot.x);
    element->SetAttribute("PivotY", region.pivot.y);
    element->SetAttribute("Width", region.size.x);
    element->SetAttribute("Height", region.size.y);
    document.InsertEndChild(element);

    return xml::document_to_string(document);
  }

  bool Spritesheet::regions_deserialize(const std::string& string, merge::Type type, std::string* errorString)
  {
    XMLDocument document{};

    if (document.Parse(string.c_str()) == XML_SUCCESS)
    {
      int id{};

      if (!document.FirstChildElement("Region"))
      {
        if (errorString) *errorString = "No valid region(s).";
        return false;
      }

      for (auto element = document.FirstChildElement("Region"); element; element = element->NextSiblingElement("Region"))
      {
        Region region{};
        element->QueryIntAttribute("Id", &id);
        xml::query_string_attribute(element, "Name", &region.name);
        element->QueryFloatAttribute("CropX", &region.crop.x);
        element->QueryFloatAttribute("CropY", &region.crop.y);
        element->QueryFloatAttribute("PivotX", &region.pivot.x);
        element->QueryFloatAttribute("PivotY", &region.pivot.y);
        element->QueryFloatAttribute("Width", &region.size.x);
        element->QueryFloatAttribute("Height", &region.size.y);

        if (type == merge::APPEND) id = map::next_id_get(regions);
        regions[id] = std::move(region);
      }

      return true;
    }
    else if (errorString)
      *errorString = document.ErrorStr();

    return false;
  }

  bool Spritesheet::save(const std::filesystem::path& directory, const std::filesystem::path& path)
  {
    WorkingDirectory workingDirectory(directory);
    this->path = !path.empty() ? path::make_relative(path) : this->path;
    return texture.write_png(this->path);
  }

  void Spritesheet::reload(const std::filesystem::path& directory) { *this = Spritesheet(directory, this->path); }
  bool Spritesheet::is_valid() { return texture.is_valid(); }

}
