#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "vec.h"

constexpr auto IMAGE_CHANNELS = 4;
constexpr auto SVG_EXTENSION = "svg";
constexpr auto PNG_EXTENSION = "png";

class Image;

// A block of an image placed at `destination`, for composing images.
struct ImagePart
{
    const Image* image = nullptr;
    IVec2 source{};
    IVec2 size{};
    IVec2 destination{};
};

// CPU-side RGBA pixels. uid changes whenever the pixels do, so GPU copies can be cached by it. A vector image keeps
// its SVG and is drawn at `scale` pixels per SVG unit. It can be drawn again at any scale, and isn't edited.
class Image
{
public:
    IVec2 size{};
    std::vector<uint8_t> pixels;
    bool isLinear = false;
    std::uint64_t uid = 0;
    std::shared_ptr<const std::string> svg;
    float scale = 1.0f;

    Image() = default;
    Image(const uint8_t*, IVec2, bool = false);
    Image(const unsigned char*, size_t);
    Image(const std::filesystem::path&);

    bool is_valid() const;
    bool is_vector() const;
    Vec2 unit_size_get() const;
    Image vector_render(float) const;
    size_t pixel_size_get() const;
    bool write_png(const std::filesystem::path&) const;
    static bool write_pixels_png(const std::filesystem::path&, IVec2, const uint8_t*);
    bool write(const std::filesystem::path&) const;
    static Image compose(const std::vector<ImagePart>&, IVec2, bool);
    void paste(const Image&, IVec2);
    Image region_get(IVec2, IVec2) const;
    Vec4 pixel_read(Vec2) const;
    void pixel_set(IVec2, Vec4);
    void pixel_line(IVec2, IVec2, Vec4);
};
