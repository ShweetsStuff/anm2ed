#include "common.hpp"

#include "asset_store.hpp"
#include "audio_data.hpp"
#include "image.hpp"

using namespace anm2ed;
using namespace anm2ed::resource;

TEST_CASE("Image png round trip and uid tracking")
{
  const std::vector<uint8_t> pixels{255, 0, 0, 255, 0, 255, 0, 128, 0, 0, 255, 0, 10, 20, 30, 40};
  Image image(pixels.data(), {2, 2});
  REQUIRE(image.is_valid());

  auto path = std::filesystem::temp_directory_path() / "anm2ed_test_image.png";
  REQUIRE(image.write_png(path));
  Image loaded(path);
  std::filesystem::remove(path);
  CHECK(loaded.size == image.size);
  CHECK(loaded.pixels == image.pixels);
  CHECK(loaded.uid != image.uid);

  auto copy = image;
  CHECK(copy.uid == image.uid);
  copy.pixel_set({1, 1}, {1.0f, 1.0f, 1.0f, 1.0f});
  CHECK(copy.uid != image.uid);
  CHECK(copy.pixel_read({1, 1}) == glm::vec4(1.0f));

  auto merged = Image::compose({{&image, {}, image.size, {}}, {&loaded, {}, loaded.size, {image.size.x, 0}}},
                               {image.size.x * 2, image.size.y}, false);
  CHECK(merged.size == glm::ivec2(4, 2));
  CHECK(merged.pixel_read({2, 0}) == image.pixel_read({0, 0}));
}

TEST_CASE("AudioData holds file bytes")
{
  const unsigned char bytes[] = {'R', 'I', 'F', 'F'};
  AudioData audio(bytes, sizeof(bytes));
  CHECK(audio.is_valid());
  CHECK(audio.uid != 0);
  CHECK_FALSE(AudioData(std::filesystem::path("missing.wav")).is_valid());
}

TEST_CASE("AssetStore keys assets by content")
{
  AssetStore store{};
  const std::vector<uint8_t> pixels{1, 2, 3, 4};
  auto key = store.image_add(Image(pixels.data(), {1, 1}));
  CHECK(key != 0);
  CHECK(store.image_add(Image(pixels.data(), {1, 1})) == key);
  CHECK(store.image_add(Image(pixels.data(), {1, 1}, true)) != key);
  REQUIRE(store.image_get(key));
  CHECK(store.image_get(key)->pixels == pixels);
  CHECK_FALSE(store.image_get(key + 1));

  const unsigned char bytes[] = {'O', 'g', 'g', 'S'};
  auto audioKey = store.audio_add(AudioData(bytes, sizeof(bytes)));
  CHECK(store.audio_add(AudioData(bytes, sizeof(bytes))) == audioKey);
  CHECK(store.audio_get(audioKey)->bytes.size() == sizeof(bytes));
}

TEST_CASE("an SVG spritesheet loads in SVG units and draws again at any scale")
{
  Image image(test::TEST_DIR / "render" / "vector.svg");
  REQUIRE(image.is_valid());
  CHECK(image.is_vector());
  CHECK(image.size == glm::ivec2(32, 16));
  auto red = image.pixel_read({8.0f, 8.0f});
  CHECK(red.r > 0.8f);
  CHECK(red.b < 0.2f);

  auto large = image.vector_render(4.0f);
  CHECK(large.size == glm::ivec2(128, 64));
  CHECK(large.unit_size_get() == image.unit_size_get());
  CHECK(large.uid != image.uid);
}

TEST_CASE("SVG spritesheets compose into SVGs that draw the same")
{
  Image vector(test::TEST_DIR / "render" / "vector.svg");
  REQUIRE(vector.is_vector());

  // The circle (left half) and the square (right half) swapped: each part is a clipped view of the drawing.
  auto swapped =
      Image::compose({{&vector, {16, 0}, {16, 16}, {0, 0}}, {&vector, {0, 0}, {16, 16}, {16, 0}}}, vector.size, true);
  REQUIRE(swapped.is_vector());
  CHECK(swapped.size == vector.size);
  CHECK(swapped.pixel_read({24.0f, 8.0f}) == vector.pixel_read({8.0f, 8.0f}));
  CHECK(swapped.pixel_read({1.0f, 1.0f}) == glm::vec4());

  // A PNG part is embedded as an image, pixel for pixel; composing the result again still works.
  const std::vector<uint8_t> pixels{255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255};
  Image raster(pixels.data(), {2, 2});
  auto mixed = Image::compose({{&swapped, {}, swapped.size, {}}, {&raster, {}, raster.size, {32, 0}}}, {34, 16}, true);
  REQUIRE(mixed.is_vector());
  CHECK(mixed.pixel_read({33.0f, 1.0f}) == raster.pixel_read({1.0f, 1.0f}));
  CHECK(Image::compose({{&mixed, {}, mixed.size, {}}}, mixed.size, true).pixel_read({24.0f, 8.0f}) ==
        vector.pixel_read({8.0f, 8.0f}));
}

TEST_CASE("an SVG's shapes meet without a seam")
{
  // Two rectangles of one color meeting at x 10.5: the pixel they share is solid, while the outer edge at 20.5 keeps
  // its anti-aliasing.
  auto text = std::make_shared<std::string>(R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="4">
    <rect x="0" y="0" width="10.5" height="4" fill="#804020"/>
    <rect x="10.5" y="0" width="10" height="4" fill="#804020"/></svg>)");
  auto path = std::filesystem::temp_directory_path() / "anm2ed_seam.svg";
  test::file_save(path, *text);
  Image image(path);
  std::filesystem::remove(path);
  REQUIRE(image.is_vector());
  auto seam = image.pixel_read({10.0f, 2.0f});
  CHECK(seam.a == 1.0f);
  CHECK(glm::distance(glm::vec3(seam), glm::vec3(image.pixel_read({5.0f, 2.0f}))) < 2.0f / 255.0f);
  CHECK(image.pixel_read({20.0f, 2.0f}).a < 1.0f);
}
