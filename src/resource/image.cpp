#include "image.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <memory>
#include <set>
#include <utility>

#if defined(__clang__) || defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
  #pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#if defined(__clang__) || defined(__GNUC__)
  #pragma GCC diagnostic pop
#endif

#include <lunasvg.h>
#include <tinyxml2/tinyxml2.h>

#include "file.hpp"
#include "math.hpp"
#include "path.hpp"
#include "uid.hpp"

using namespace anm2ed::resource::image;
using namespace anm2ed::util::math;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::resource
{
  constexpr std::size_t BASE64_GROUP_BYTES = 3;
  constexpr int BASE64_DIGIT_BITS = 6;
  constexpr std::uint32_t BASE64_DIGIT_MASK = 0x3F;
  constexpr int BYTE_BITS = 8;
  constexpr std::string_view BASE64_DIGITS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  constexpr char BASE64_PADDING = '=';
  constexpr auto SVG_NAMESPACE = "http://www.w3.org/2000/svg";
  constexpr auto XLINK_NAMESPACE = "http://www.w3.org/1999/xlink";
  constexpr auto SVG_ID_PREFIX = "anm2ed-source-";
  constexpr std::string_view SVG_URL_START = "url(#";
  // A drawing's root attributes that place it, rather than style it.
  constexpr std::string_view SVG_ROOT_ONLY_ATTRIBUTES[] = {"width", "height", "viewBox", "preserveAspectRatio",
                                                           "x",     "y",      "version", "id"};

  bool Image::is_valid() const { return !pixels.empty(); }

  bool Image::is_vector() const { return svg != nullptr; }

  glm::vec2 Image::unit_size_get() const { return glm::vec2(size) / scale; }

  // An SVG drawn at about `scale` pixels per unit (exactly, after rounding to whole pixels); nothing if it won't parse.
  Image svg_render(std::shared_ptr<const std::string> svg, float scale)
  {
    auto document = svg ? lunasvg::Document::loadFromData(*svg) : nullptr;
    if (!document || document->width() <= 0.0f) return {};
    auto size = glm::max(ivec2(glm::ceil(vec2(document->width(), document->height()) * scale)), ivec2(1));
    auto bitmap = document->renderToBitmap(size.x, size.y);
    if (bitmap.isNull()) return {};
    bitmap.convertToRGBA();
    Image image(bitmap.data(), size, true);
    image.svg = std::move(svg);
    image.scale = (float)size.x / document->width();
    return image;
  }

  Image Image::vector_render(float scale) const { return svg_render(svg, scale); }

  size_t Image::pixel_size_get() const { return size.x * size.y * CHANNELS; }

  Image::Image(const uint8_t* data, ivec2 size, bool isLinear) : isLinear(isLinear)
  {
    if (!data || size.x <= 0 || size.y <= 0) return;
    this->size = size;
    pixels.assign(data, data + pixel_size_get());
    uid = uid_next();
  }

  Image::Image(const unsigned char* pngData, size_t pngSize)
  {
    if (!pngData || pngSize == 0) return;

    auto sizeInt = static_cast<int>(pngSize);
    if (sizeInt <= 0) return;

    if (auto data = stbi_load_from_memory(pngData, sizeInt, &size.x, &size.y, nullptr, CHANNELS); data)
    {
      *this = Image(data, size);
      stbi_image_free((void*)data);
    }
  }

  // A PNG, or else an SVG (whatever the file is named, so autosaved copies load too).
  Image::Image(const std::filesystem::path& path)
  {
    auto text = std::make_shared<std::string>();
    if (!file::read_to_string(path, text.get())) return;
    *this = Image((const unsigned char*)text->data(), text->size());
    if (!is_valid()) *this = svg_render(std::move(text), 1.0f);
  }

  bool Image::write_png(const std::filesystem::path& path) const
  {
    return write_pixels_png(path, size, pixels.empty() ? nullptr : pixels.data());
  }

  bool Image::write_pixels_png(const std::filesystem::path& path, ivec2 size, const uint8_t* data)
  {
    if (!data || size.x <= 0 || size.y <= 0) return false;

    File file(path, "wb");
    if (auto handle = file.get())
    {
      auto write_func = [](void* context, void* bytes, int count)
      { fwrite(bytes, 1, count, static_cast<FILE*>(context)); };
      return stbi_write_png_to_func(write_func, handle, size.x, size.y, CHANNELS, data, size.x * CHANNELS) != 0;
    }
    return false;
  }

  // Copies a size-sized block between images; pixels outside either image are skipped.
  void pixels_copy(const Image& from, glm::ivec2 fromMin, Image& to, glm::ivec2 toMin, glm::ivec2 size)
  {
    for (int y = 0; y < size.y; ++y)
      for (int x = 0; x < size.x; ++x)
      {
        auto source = fromMin + glm::ivec2(x, y);
        auto target = toMin + glm::ivec2(x, y);
        if (glm::any(glm::lessThan(source, glm::ivec2(0))) || glm::any(glm::greaterThanEqual(source, from.size)) ||
            glm::any(glm::lessThan(target, glm::ivec2(0))) || glm::any(glm::greaterThanEqual(target, to.size)))
          continue;
        std::copy_n(from.pixels.data() + ((std::size_t)source.y * from.size.x + source.x) * image::CHANNELS,
                    image::CHANNELS,
                    to.pixels.data() + ((std::size_t)target.y * to.size.x + target.x) * image::CHANNELS);
      }
  }

  void Image::paste(const Image& source, glm::ivec2 position) { pixels_copy(source, {}, *this, position, source.size); }

  // A copy of a block of the image (transparent where it falls outside).
  Image Image::region_get(glm::ivec2 min, glm::ivec2 regionSize) const
  {
    Image region{};
    region.size = regionSize;
    region.pixels.assign((std::size_t)regionSize.x * regionSize.y * image::CHANNELS, 0);
    pixels_copy(*this, min, region, {}, regionSize);
    return region;
  }

  // Base64 of bytes, for an image embedded in an SVG.
  std::string base64_get(const std::vector<uint8_t>& bytes)
  {
    std::string text{};
    for (std::size_t i = 0; i < bytes.size(); i += BASE64_GROUP_BYTES)
    {
      auto count = std::min(BASE64_GROUP_BYTES, bytes.size() - i);
      std::uint32_t value{};
      for (std::size_t j = 0; j < BASE64_GROUP_BYTES; ++j)
        value = value << BYTE_BITS | (j < count ? bytes[i + j] : 0u);
      for (std::size_t j = 0; j <= BASE64_GROUP_BYTES; ++j)
        text += j <= count
                    ? BASE64_DIGITS[(value >> ((BASE64_GROUP_BYTES - j) * BASE64_DIGIT_BITS)) & BASE64_DIGIT_MASK]
                    : BASE64_PADDING;
    }
    return text;
  }

  // An image as one SVG definition: a vector one as a symbol of its drawing (its ids prefixed with `id`, so
  // several drawings can share a document), a raster one as an embedded PNG. Either draws at the image's size in
  // SVG units.
  void svg_definition_add(tinyxml2::XMLDocument& document, tinyxml2::XMLElement* root, tinyxml2::XMLElement* defs,
                          const Image& image, const std::string& id)
  {
    if (!image.is_vector())
    {
      std::vector<uint8_t> png{};
      stbi_write_png_to_func(
          [](void* context, void* data, int count)
          {
            static_cast<std::vector<uint8_t>*>(context)->insert(static_cast<std::vector<uint8_t>*>(context)->end(),
                                                                (uint8_t*)data, (uint8_t*)data + count);
          },
          &png, image.size.x, image.size.y, CHANNELS, image.pixels.data(), image.size.x * CHANNELS);
      auto element = defs->InsertNewChildElement("image");
      element->SetAttribute("id", id.c_str());
      element->SetAttribute("width", image.size.x);
      element->SetAttribute("height", image.size.y);
      element->SetAttribute("xlink:href", ("data:image/png;base64," + base64_get(png)).c_str());
      return;
    }

    tinyxml2::XMLDocument source{};
    source.Parse(image.svg->c_str());
    auto sourceRoot = source.RootElement();
    if (!sourceRoot) return;

    // Every id in the drawing gets the prefix, and so does every reference to one (href="#id", url(#id)).
    std::set<std::string> ids{};
    auto elements_each = [](tinyxml2::XMLElement* element, auto&& callback, auto&& self) -> void
    {
      callback(element);
      for (auto child = element->FirstChildElement(); child; child = child->NextSiblingElement())
        self(child, callback, self);
    };
    elements_each(
        sourceRoot,
        [&](tinyxml2::XMLElement* element)
        {
          if (auto elementId = element->Attribute("id")) ids.insert(elementId);
        },
        elements_each);
    auto renamed_get = [&](std::string_view sourceId) { return id + "-" + std::string(sourceId); };
    elements_each(
        sourceRoot,
        [&](tinyxml2::XMLElement* element)
        {
          std::vector<std::pair<std::string, std::string>> attributes{};
          for (auto attribute = element->FirstAttribute(); attribute; attribute = attribute->Next())
            attributes.emplace_back(attribute->Name(), attribute->Value());
          for (auto& [name, value] : attributes)
          {
            if (name == "id")
              value = renamed_get(value);
            else if ((name == "href" || name == "xlink:href") && value.starts_with('#') &&
                     ids.contains(value.substr(1)))
              value = "#" + renamed_get(value.substr(1));
            else
              for (auto at = value.find(SVG_URL_START); at != std::string::npos; at = value.find(SVG_URL_START, at + 1))
              {
                auto start = at + SVG_URL_START.size();
                auto end = value.find(')', start);
                if (end == std::string::npos) break;
                if (auto reference = value.substr(start, end - start); ids.contains(reference))
                  value.replace(start, end - start, renamed_get(reference));
              }
            element->SetAttribute(name.c_str(), value.c_str());
          }
        },
        elements_each);

    auto unitSize = image.unit_size_get();
    auto symbol = defs->InsertNewChildElement("symbol");
    symbol->SetAttribute("id", id.c_str());
    auto viewBox = sourceRoot->Attribute("viewBox");
    symbol->SetAttribute("viewBox", viewBox ? viewBox : std::format("0 0 {} {}", unitSize.x, unitSize.y).c_str());
    if (auto aspect = sourceRoot->Attribute("preserveAspectRatio")) symbol->SetAttribute("preserveAspectRatio", aspect);
    // The drawing's root styles carry over through a group; its namespaces go on the new root.
    auto group = symbol->InsertNewChildElement("g");
    for (auto attribute = sourceRoot->FirstAttribute(); attribute; attribute = attribute->Next())
    {
      std::string_view name = attribute->Name();
      if (name.starts_with("xmlns"))
        root->SetAttribute(attribute->Name(), attribute->Value());
      else if (!std::ranges::contains(SVG_ROOT_ONLY_ATTRIBUTES, name))
        group->SetAttribute(attribute->Name(), attribute->Value());
    }
    for (auto child = sourceRoot->FirstChild(); child; child = child->NextSibling())
      group->InsertEndChild(child->DeepClone(&document));
  }

  // Parts of images placed into one image of `size`: an SVG when `isVector` (each part a clipped view of its image's
  // drawing, or of an embedded PNG), otherwise pixels.
  Image Image::compose(const std::vector<ImagePart>& parts, glm::ivec2 size, bool isVector)
  {
    if (!isVector)
    {
      std::vector<uint8_t> pixels((std::size_t)size.x * size.y * CHANNELS, 0);
      Image composed(pixels.data(), size);
      for (const auto& part : parts)
        composed.paste(part.image->region_get(part.source, part.size), part.destination);
      return composed;
    }

    tinyxml2::XMLDocument document{};
    auto root = document.NewElement("svg");
    document.InsertEndChild(root);
    root->SetAttribute("xmlns", SVG_NAMESPACE);
    root->SetAttribute("xmlns:xlink", XLINK_NAMESPACE);
    root->SetAttribute("width", size.x);
    root->SetAttribute("height", size.y);
    root->SetAttribute("viewBox", std::format("0 0 {} {}", size.x, size.y).c_str());
    auto defs = root->InsertNewChildElement("defs");

    std::vector<const Image*> sources{};
    for (const auto& part : parts)
    {
      auto source = std::ranges::find(sources, part.image);
      if (source == sources.end())
      {
        source = sources.insert(sources.end(), part.image);
        svg_definition_add(document, root, defs, *part.image, std::format("{}{}", SVG_ID_PREFIX, sources.size() - 1));
      }
      auto view = root->InsertNewChildElement("svg");
      view->SetAttribute("x", part.destination.x);
      view->SetAttribute("y", part.destination.y);
      view->SetAttribute("width", part.size.x);
      view->SetAttribute("height", part.size.y);
      view->SetAttribute("viewBox",
                         std::format("{} {} {} {}", part.source.x, part.source.y, part.size.x, part.size.y).c_str());
      auto use = view->InsertNewChildElement("use");
      auto unitSize = part.image->unit_size_get();
      use->SetAttribute("xlink:href", std::format("#{}{}", SVG_ID_PREFIX, source - sources.begin()).c_str());
      use->SetAttribute("width", unitSize.x);
      use->SetAttribute("height", unitSize.y);
    }

    tinyxml2::XMLPrinter printer{};
    document.Print(&printer);
    return svg_render(std::make_shared<std::string>(printer.CStr()), 1.0f);
  }

  // Writes a vector image as its SVG (unless the path asks for a PNG), and any other as a PNG.
  bool Image::write(const std::filesystem::path& path) const
  {
    if (is_vector() && !path::is_extension(path, PNG_EXTENSION)) return file::write_string(path, *svg);
    return write_png(path);
  }

  vec4 Image::pixel_read(vec2 position) const
  {
    if (pixels.size() < CHANNELS || size.x <= 0 || size.y <= 0) return vec4(0.0f);

    int x = glm::clamp((int)(position.x), 0, size.x - 1);
    int y = glm::clamp((int)(position.y), 0, size.y - 1);

    auto index = ((size_t)(y) * (size_t)(size.x) + (size_t)(x)) * CHANNELS;
    if (index + CHANNELS > pixels.size()) return vec4(0.0f);

    vec4 color{uint8_to_float(pixels[index + 0]), uint8_to_float(pixels[index + 1]), uint8_to_float(pixels[index + 2]),
               uint8_to_float(pixels[index + 3])};

    if (color.a <= 0.0f) color = vec4(0.0f);
    return color;
  }

  void Image::pixel_set(ivec2 position, vec4 color)
  {
    if (position.x < 0 || position.y < 0 || position.x >= size.x || position.y >= size.y) return;

    uint8 rgba8[4] = {(uint8)float_to_uint8(color.r), (uint8)float_to_uint8(color.g), (uint8)float_to_uint8(color.b),
                      (uint8)float_to_uint8(color.a)};

    if (pixels.size() != pixel_size_get()) return;
    size_t idx = (position.y * size.x + position.x) * CHANNELS;
    memcpy(&pixels[idx], rgba8, 4);
    uid = uid_next();
  }

  void Image::pixel_line(ivec2 start, ivec2 end, vec4 color)
  {
    auto plot = [&](ivec2 pos) { pixel_set(pos, color); };

    int x0 = start.x;
    int y0 = start.y;
    int x1 = end.x;
    int y1 = end.y;

    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true)
    {
      plot({x0, y0});
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 >= dy)
      {
        err += dy;
        x0 += sx;
      }
      if (e2 <= dx)
      {
        err += dx;
        y0 += sy;
      }
    }
  }
}
