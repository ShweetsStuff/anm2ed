#include "common.hpp"

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

  auto merged = Image::merge_append(image, loaded, true);
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
