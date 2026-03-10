#include "spritesheet.hpp"

#include <algorithm>
#include <functional>
#include <ranges>
#include <string_view>
#include <vector>

#include "map_.hpp"
#include "path_.hpp"
#include "working_directory.hpp"
#include "xml_.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::util;
using namespace anm2ed::types;
using namespace tinyxml2;

namespace anm2ed::anm2
{
  namespace
  {
    const char* origin_to_string(Spritesheet::Region::Origin origin)
    {
      switch (origin)
      {
        case Spritesheet::Region::TOP_LEFT:
          return "TopLeft";
        case Spritesheet::Region::ORIGIN_CENTER:
          return "Center";
        case Spritesheet::Region::CUSTOM:
        default:
          return nullptr;
      }
    }

    Spritesheet::Region::Origin origin_from_string(const char* originString)
    {
      if (!originString) return Spritesheet::Region::CUSTOM;
      if (std::string(originString) == "TopLeft") return Spritesheet::Region::TOP_LEFT;
      if (std::string(originString) == "Center") return Spritesheet::Region::ORIGIN_CENTER;
      return Spritesheet::Region::CUSTOM;
    }
  }

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

    regionOrder.clear();
    for (auto child = element->FirstChildElement("Region"); child; child = child->NextSiblingElement("Region"))
    {
      Region region{};
      int id{};
      child->QueryIntAttribute("Id", &id);
      xml::query_string_attribute(child, "Name", &region.name);
      child->QueryFloatAttribute("XCrop", &region.crop.x);
      child->QueryFloatAttribute("YCrop", &region.crop.y);
      child->QueryFloatAttribute("Width", &region.size.x);
      child->QueryFloatAttribute("Height", &region.size.y);
      region.origin = origin_from_string(child->Attribute("Origin"));
      if (region.origin == Spritesheet::Region::TOP_LEFT)
        region.pivot = {};
      else if (region.origin == Spritesheet::Region::ORIGIN_CENTER)
        region.pivot = {(int)(region.size.x / 2.0f), (int)(region.size.y / 2.0f)};
      else
      {
        child->QueryFloatAttribute("XPivot", &region.pivot.x);
        child->QueryFloatAttribute("YPivot", &region.pivot.y);
      }
      regions.emplace(id, std::move(region));
      regionOrder.push_back(id);
    }

    if (regionOrder.size() != regions.size())
    {
      regionOrder.clear();
      regionOrder.reserve(regions.size());
      for (auto id : regions | std::views::keys)
        regionOrder.push_back(id);
    }
  }

  Spritesheet::Spritesheet(const std::filesystem::path& directory, const std::filesystem::path& path)
  {
    WorkingDirectory workingDirectory(directory);
    this->path = !path.empty() ? path::make_relative(path) : this->path;
    this->path = path::lower_case_backslash_handle(this->path);
    texture = Texture(this->path);
  }

  XMLElement* Spritesheet::to_element(XMLDocument& document, int id, Flags flags)
  {
    auto element = document.NewElement("Spritesheet");
    element->SetAttribute("Id", id);
    auto pathString = path::to_utf8(path);
    element->SetAttribute("Path", pathString.c_str());

    if (!has_flag(flags, NO_REGIONS))
    {
      if (regionOrder.size() != regions.size())
      {
        regionOrder.clear();
        regionOrder.reserve(regions.size());
        for (auto id : regions | std::views::keys)
          regionOrder.push_back(id);
      }

      for (auto id : regionOrder)
      {
        if (!regions.contains(id)) continue;
        auto& region = regions.at(id);
        auto regionElement = element->InsertNewChildElement("Region");
        regionElement->SetAttribute("Id", id);
        regionElement->SetAttribute("Name", region.name.c_str());
        regionElement->SetAttribute("XCrop", region.crop.x);
        regionElement->SetAttribute("YCrop", region.crop.y);
        regionElement->SetAttribute("Width", region.size.x);
        regionElement->SetAttribute("Height", region.size.y);
        if (auto originString = origin_to_string(region.origin); originString)
          regionElement->SetAttribute("Origin", originString);
        else
        {
          regionElement->SetAttribute("XPivot", region.pivot.x);
          regionElement->SetAttribute("YPivot", region.pivot.y);
        }
      }
    }

    return element;
  }

  void Spritesheet::serialize(XMLDocument& document, XMLElement* parent, int id, Flags flags)
  {
    parent->InsertEndChild(to_element(document, id, flags));
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
    element->SetAttribute("XCrop", region.crop.x);
    element->SetAttribute("YCrop", region.crop.y);
    element->SetAttribute("Width", region.size.x);
    element->SetAttribute("Height", region.size.y);
    if (auto originString = origin_to_string(region.origin); originString)
      element->SetAttribute("Origin", originString);
    else
    {
      element->SetAttribute("XPivot", region.pivot.x);
      element->SetAttribute("YPivot", region.pivot.y);
    }
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

      for (auto element = document.FirstChildElement("Region"); element;
           element = element->NextSiblingElement("Region"))
      {
        Region region{};
        element->QueryIntAttribute("Id", &id);
        xml::query_string_attribute(element, "Name", &region.name);
        element->QueryFloatAttribute("XCrop", &region.crop.x);
        element->QueryFloatAttribute("YCrop", &region.crop.y);
        element->QueryFloatAttribute("Width", &region.size.x);
        element->QueryFloatAttribute("Height", &region.size.y);
        region.origin = origin_from_string(element->Attribute("Origin"));
        if (region.origin == Spritesheet::Region::TOP_LEFT)
          region.pivot = {};
        else if (region.origin == Spritesheet::Region::ORIGIN_CENTER)
          region.pivot = glm::ivec2(region.size / 2.0f);
        else
        {
          element->QueryFloatAttribute("XPivot", &region.pivot.x);
          element->QueryFloatAttribute("YPivot", &region.pivot.y);
        }

        if (type == merge::APPEND) id = map::next_id_get(regions);
        regions[id] = std::move(region);
        if (std::find(regionOrder.begin(), regionOrder.end(), id) == regionOrder.end()) regionOrder.push_back(id);
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
    if (this->path.empty()) return false;
    path::ensure_directory(this->path.parent_path());
    return texture.write_png(this->path);
  }

  void Spritesheet::reload(const std::filesystem::path& directory, const std::filesystem::path& path)
  {
    WorkingDirectory workingDirectory(directory);
    this->path = !path.empty() ? path::make_relative(path) : this->path;
    this->path = path::lower_case_backslash_handle(this->path);
    texture = Texture(this->path);
  }
  bool Spritesheet::is_valid() { return texture.is_valid(); }

  uint64_t Spritesheet::hash() const
  {
    auto hash_combine = [](std::size_t& seed, std::size_t value)
    {
      seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    };

    std::size_t seed{};
    hash_combine(seed, std::hash<int>{}(texture.size.x));
    hash_combine(seed, std::hash<int>{}(texture.size.y));
    hash_combine(seed, std::hash<int>{}(texture.channels));
    hash_combine(seed, std::hash<int>{}(texture.filter));
    hash_combine(seed, std::hash<std::string>{}(path::to_utf8(path)));

    if (!texture.pixels.empty())
    {
      std::string_view bytes(reinterpret_cast<const char*>(texture.pixels.data()), texture.pixels.size());
      hash_combine(seed, std::hash<std::string_view>{}(bytes));
    }
    else
    {
      hash_combine(seed, 0);
    }

    return static_cast<uint64_t>(seed);
  }

}
