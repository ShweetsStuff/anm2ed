#include "common.hpp"

#include "model/draw.hpp"
#include "model/frames.hpp"

using namespace anm2ed;
using namespace anm2ed::test;

TEST_CASE("draw lists put the root first and each item's onion samples before it")
{
  auto anm2 = model_load(file_load(CORPUS_DIR / "02_items.anm2"));
  auto& animation = *anm2.animation_get(0);
  auto draws = model::animation_draws_get(anm2, animation, {.time = 1.0f, .samples = {{-1.0f}, {1.0f}}});
  REQUIRE(!draws.empty());
  CHECK(draws.front().type == model::DrawType::ROOT);

  for (int i = 0; i + 1 < (int)draws.size(); ++i)
    if (draws[i].sample != -1) CHECK((draws[i + 1].id == draws[i].id && draws[i + 1].type == draws[i].type));

  auto layerDraws = std::ranges::count_if(draws, [](const model::Draw& draw)
                                          { return draw.type == model::DrawType::LAYER && draw.sample == -1; });
  CHECK(layerDraws > 0);
  CHECK(model::animation_rect(anm2, animation, true) != glm::vec4(-1.0f));
}

TEST_CASE("root transforms fold into layer draws")
{
  auto anm2 = model_load(file_load(CORPUS_DIR / "04_group_root_transform.anm2"));
  auto& animation = *anm2.animation_get(0);
  auto plain = model::animation_draws_get(anm2, animation, {});
  auto rooted = model::animation_draws_get(anm2, animation, {.isRootTransform = true});
  REQUIRE(plain.size() == rooted.size());
  bool isAnyMoved{};
  for (int i = 0; i < (int)plain.size(); ++i)
    isAnyMoved |= plain[i].type == model::DrawType::LAYER && plain[i].parent != rooted[i].parent;
  CHECK(isAnyMoved);
}

TEST_CASE("a layer frame's depth moves it among the layers for that frame only")
{
  auto anm2 = model_load(file_load(TEST_DIR / "render" / "spin.anm2"));
  auto layer_ids_get = [&](float time)
  {
    std::vector<int> ids{};
    for (const auto& draw : model::animation_draws_get(anm2, *anm2.animation_get(0), {.time = time}))
      if (draw.type == model::DrawType::LAYER) ids.push_back(draw.id);
    return ids;
  };
  CHECK(layer_ids_get(0.0f) == std::vector{1, 0});

  anm2.frame_edit({0, LAYER, 0, 0})->depth = -1;
  CHECK(layer_ids_get(0.0f) == std::vector{0, 1});
  CHECK(layer_ids_get(5.0f) == std::vector{1, 0});

  auto text = model::model_to_string(anm2);
  CHECK(game_document_get(text).find("Depth") == std::string::npos);
  CHECK(model_load(text).frame_get({0, LAYER, 0, 0})->depth == -1);
  CHECK(model::isaac_issues_get(anm2).depthFrames == 1);
}

TEST_CASE("a layer frame's blur reaches its child's draws and stays out of the game's copy")
{
  auto anm2 = model_load(file_load(TEST_DIR / "render" / "nested.anm2"));
  anm2.frame_edit({0, LAYER, 1, 0})->blur = 6.0f;
  bool isAnyChild{};
  for (const auto& draw : model::animation_draws_get(anm2, *anm2.animation_get(0), {}))
    if (draw.type == model::DrawType::LAYER && draw.id == 1)
    {
      isAnyChild = true;
      CHECK(draw.frame.blur >= 6.0f);
    }
  CHECK(isAnyChild);

  auto text = model::model_to_string(anm2);
  CHECK(game_document_get(text).find("Blur") == std::string::npos);
  CHECK(model_load(text).frame_get({0, LAYER, 1, 0})->blur == 6.0f);
  CHECK(model::isaac_issues_get(anm2).blurFrames == 1);
}

TEST_CASE("an animation layer draws its child animation, from this file or a library")
{
  auto anm2 = model_load(file_load(TEST_DIR / "render" / "nested.anm2"));
  auto spin = model_load(file_load(TEST_DIR / "render" / "spin.anm2"));
  model::DrawOptions options{.library_get = [&](const model::Model&, int id) { return id == 0 ? &spin : nullptr; }};
  auto red_position_get = [&](int layerId)
  {
    std::optional<glm::vec2> position{};
    for (const auto& draw : model::animation_draws_get(anm2, *anm2.animation_get(0), options))
      if (draw.type == model::DrawType::LAYER && draw.id == layerId && draw.frame.crop == glm::vec2())
        position = glm::vec2(draw.parent * glm::vec4(draw.frame.position, 0.0f, 1.0f));
    return position;
  };

  // The library's red square, half size at (40, 0), starts on its fifth frame (at x 16).
  CHECK(red_position_get(0) == glm::vec2(48.0f, 0.0f));
  // This file's Inner holds its first frame however far the parent plays.
  options.time = 1.0f;
  CHECK(red_position_get(1) == glm::vec2(-40.0f, 0.0f));
  // An animation never draws inside itself.
  CHECK_FALSE(red_position_get(3));

  // Once stops on the child's last frame; Loop wraps around it.
  options.time = 6.0f;
  anm2.frame_edit({0, LAYER, 1, 0})->childMode = ChildMode::ONCE;
  CHECK(red_position_get(1) == glm::vec2(-40.0f, 10.0f));
  anm2.frame_edit({0, LAYER, 1, 0})->childMode = ChildMode::LOOP;
  CHECK(red_position_get(1) == glm::vec2(-40.0f, 0.0f));

  // Without the library loaded, its layer draws nothing.
  options.library_get = {};
  CHECK_FALSE(red_position_get(0));

  auto text = model::model_to_string(anm2, {.isExtendedFormat = true});
  CHECK(model::model_hash(model_load(text)) == model::model_hash(anm2));
  CHECK(model::isaac_issues_get(anm2).animationLayers == 3);
  CHECK(game_document_get(model::model_to_string(anm2)).find(" Animation=\"") == std::string::npos);
}

TEST_CASE("an alpha offset is kept by the editor's copy only")
{
  auto anm2 = model_load(file_load(TEST_DIR / "render" / "spin.anm2"));
  anm2.frame_edit({0, LAYER, 0, 0})->colorOffset.a = -0.5f;
  auto text = model::model_to_string(anm2);
  CHECK(game_document_get(text).find("AlphaOffset") == std::string::npos);
  CHECK(model_load(text).frame_get({0, LAYER, 0, 0})->colorOffset.a == doctest::Approx(-127.0f / 255.0f));
  CHECK(model::isaac_issues_get(anm2).alphaOffsetFrames == 1);
}

TEST_CASE("only SVG spritesheets that layers show count against Rebirth")
{
  auto anm2 = model_load(file_load(TEST_DIR / "render" / "vector.anm2"));
  CHECK(model::spritesheets_vector_used_get(anm2) == std::set{0});
  anm2.content.layers.clear();
  CHECK(model::spritesheets_vector_used_get(anm2).empty());
}
