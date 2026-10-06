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
