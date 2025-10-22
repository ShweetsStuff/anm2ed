#include "math.h"

#include <glm/ext/matrix_transform.hpp>
#include <string>

using namespace glm;

namespace anm2ed::math
{
  constexpr auto FLOAT_FORMAT_MAX_DECIMALS = 7;
  constexpr auto FLOAT_FORMAT_EPSILON = 1e-7f;
  constexpr float FLOAT_FORMAT_POW10[] = {1.f, 10.f, 100.f, 1000.f, 10000.f, 100000.f, 1000000.f, 10000000.f};

  float round_nearest_multiple(float value, float multiple)
  {
    return (roundf((value) / (multiple)) * (multiple));
  }

  int float_decimals_needed(float value)
  {
    for (int decimalCount = 0; decimalCount <= FLOAT_FORMAT_MAX_DECIMALS; ++decimalCount)
    {
      auto scale = FLOAT_FORMAT_POW10[decimalCount];
      auto rounded = roundf(value * scale) / scale;
      if (fabsf(value - rounded) < FLOAT_FORMAT_EPSILON) return decimalCount;
    }
    return FLOAT_FORMAT_MAX_DECIMALS;
  }

  const char* float_format_get(float value)
  {
    static std::string formatString{};
    int decimalCount = float_decimals_needed(value);
    formatString = (decimalCount == 0) ? "%.0f" : ("%." + std::to_string(decimalCount) + "f");
    return formatString.c_str();
  }

  const char* vec2_format_get(vec2& value)
  {
    static std::string formatString{};
    int decimalCountX = float_decimals_needed(value.x);
    int decimalCountY = float_decimals_needed(value.y);
    int decimalCount = (decimalCountX > decimalCountY) ? decimalCountX : decimalCountY;
    formatString = (decimalCount == 0) ? "%.0f" : ("%." + std::to_string(decimalCount) + "f");
    return formatString.c_str();
  }

  mat4 quad_model_get(vec2 size, vec2 position, vec2 pivot, vec2 scale, float rotation)
  {
    vec2 scaleAbsolute = glm::abs(scale);
    vec2 scaleSign = glm::sign(scale);
    vec2 pivotScaled = pivot * scaleAbsolute;
    vec2 sizeScaled = size * scaleAbsolute;

    mat4 model(1.0f);
    model = glm::translate(model, vec3(position - pivotScaled, 0.0f));
    model = glm::translate(model, vec3(pivotScaled, 0.0f));
    model = glm::scale(model, vec3(scaleSign, 1.0f));
    model = glm::rotate(model, glm::radians(rotation), vec3(0, 0, 1));
    model = glm::translate(model, vec3(-pivotScaled, 0.0f));
    model = glm::scale(model, vec3(sizeScaled, 1.0f));
    return model;
  }

  mat4 quad_model_parent_get(vec2 position, vec2 pivot, vec2 scale, float rotation)
  {
    vec2 scaleSign = glm::sign(scale);
    vec2 scaleAbsolute = glm::abs(scale);
    float handedness = (scaleSign.x * scaleSign.y) < 0.0f ? -1.0f : 1.0f;

    mat4 local(1.0f);
    local = glm::translate(local, vec3(pivot, 0.0f));
    local = glm::scale(local, vec3(scaleSign, 1.0f));
    local = glm::rotate(local, glm::radians(rotation) * handedness, vec3(0, 0, 1));
    local = glm::translate(local, vec3(-pivot, 0.0f));
    local = glm::scale(local, vec3(scaleAbsolute, 1.0f));

    return glm::translate(mat4(1.0f), vec3(position, 0.0f)) * local;
  }

}
