#include "math_util.h"

#include <ctime>
#include "vec.h"
#include <string>

constexpr auto FLOAT_FORMAT_MAX_DECIMALS = 7;
constexpr auto FLOAT_FORMAT_EPSILON = 1e-7f;
constexpr float FLOAT_FORMAT_POW10[] = {1.f, 10.f, 100.f, 1000.f, 10000.f, 100000.f, 1000000.f, 10000000.f};

int float_decimals_needed(float value)
{
    for (int decimalCount = 0; decimalCount <= FLOAT_FORMAT_MAX_DECIMALS; ++decimalCount)
    {
        auto scale = FLOAT_FORMAT_POW10[decimalCount];
        auto rounded = roundf(value * scale) / scale;
        if (fabsf(value - rounded) < FLOAT_FORMAT_EPSILON)
            return decimalCount;
    }
    return FLOAT_FORMAT_MAX_DECIMALS;
}

const char* float_format_get(float value)
{
    static std::string formatString;
    int decimalCount = float_decimals_needed(value);
    formatString = (decimalCount == 0) ? "%.0f" : ("%." + std::to_string(decimalCount) + "f");
    return formatString.c_str();
}

const char* vec2_format_get(const Vec2& value)
{
    static std::string formatString;
    int decimalCountX = float_decimals_needed(value.x);
    int decimalCountY = float_decimals_needed(value.y);
    int decimalCount = (decimalCountX > decimalCountY) ? decimalCountX : decimalCountY;
    formatString = (decimalCount == 0) ? "%.0f" : ("%." + std::to_string(decimalCount) + "f");
    return formatString.c_str();
}

static Mat4 skew_get(Vec2 skew)
{
    Mat4 result(1.0f);
    result[1][0] = skew.x;
    result[0][1] = skew.y;
    return result;
}

Mat4 quad_model_get(Vec2 size, Vec2 position, Vec2 pivot, Vec2 scale, float rotation, Vec2 skew)
{
    Vec2 scaleAbsolute = vec_abs(scale);
    Vec2 scaleSign = vec_sign(scale);
    Vec2 pivotScaled = pivot * scaleAbsolute;
    Vec2 sizeScaled = size * scaleAbsolute;
    float handedness = (scaleSign.x * scaleSign.y) < 0.0f ? -1.0f : 1.0f;

    Mat4 model(1.0f);
    model = mat4_translate(model, Vec3(position - pivotScaled, 0.0f));
    model = mat4_translate(model, Vec3(pivotScaled, 0.0f));
    model = mat4_scale(model, Vec3(scaleSign, 1.0f));
    model = mat4_rotate(model, radians(rotation) * handedness);
    model *= skew_get(skew);
    model = mat4_translate(model, Vec3(-pivotScaled, 0.0f));
    model = mat4_scale(model, Vec3(sizeScaled, 1.0f));
    return model;
}

Mat4 quad_model_parent_get(Vec2 position, Vec2 pivot, Vec2 scale, float rotation, Vec2 skew)
{
    Vec2 scaleSign = vec_sign(scale);
    Vec2 scaleAbsolute = vec_abs(scale);
    float handedness = (scaleSign.x * scaleSign.y) < 0.0f ? -1.0f : 1.0f;

    Mat4 local(1.0f);
    local = mat4_translate(local, Vec3(pivot, 0.0f));
    local = mat4_scale(local, Vec3(scaleSign, 1.0f));
    local = mat4_rotate(local, radians(rotation) * handedness);
    local *= skew_get(skew);
    local = mat4_translate(local, Vec3(-pivot, 0.0f));
    local = mat4_scale(local, Vec3(scaleAbsolute, 1.0f));

    return mat4_translate(Mat4(1.0f), Vec3(position, 0.0f)) * local;
}

float random_unit_get()
{
    return (float)rand() / RAND_MAX;
}

float random_in_range(float min, float max)
{
    return min + random_unit_get() * (max - min);
}

void random_seed_set()
{
    srand(std::time(nullptr));
}
