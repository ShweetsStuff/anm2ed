#pragma once

#include <array>
#include <glm/glm/mat4x4.hpp>

namespace anm2ed::util::math
{
  template <typename T> constexpr T percent_to_unit(T value)
  {
    return value / 100.0f;
  }

  template <typename T> constexpr T unit_to_percent(T value)
  {
    return value * 100.0f;
  }

  constexpr float uint8_to_float(int value)
  {
    return (float)(value / 255.0f);
  }

  constexpr int float_to_uint8(float value)
  {
    return (int)(value * 255);
  }

  constexpr std::array<float, 16> uv_vertices_get(glm::vec2 uvMin, glm::vec2 uvMax)
  {
    return {0.0f, 0.0f, uvMin.x, uvMin.y, 1.0f, 0.0f, uvMax.x, uvMin.y,
            1.0f, 1.0f, uvMax.x, uvMax.y, 0.0f, 1.0f, uvMin.x, uvMax.y};
  }

  float round_nearest_multiple(float, float);

  int float_decimals_needed(float);

  const char* float_format_get(float);

  const char* vec2_format_get(glm::vec2&);

  glm::mat4 quad_model_get(glm::vec2 = {}, glm::vec2 = {}, glm::vec2 = {}, glm::vec2 = glm::vec2(1.0f), float = {});
  glm::mat4 quad_model_parent_get(glm::vec2 = {}, glm::vec2 = {}, glm::vec2 = glm::vec2(1.0f), float = {});
}