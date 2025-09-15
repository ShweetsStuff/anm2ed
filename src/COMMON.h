#pragma once

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <GL/gl.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <tinyxml2.h>

#include <algorithm>                   
#include <chrono>                      
#include <climits>
#include <cmath>                          
#include <cstring>
#include <filesystem>                  
#include <format>           
#include <fstream>
#include <functional>            
#include <iostream>
#include <map>                          
#include <optional>
#include <print>                          
#include <ranges>                      
#include <string>
#include <unordered_set>                      
#include <variant>                  
#include <vector>           

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

#define PI (GLM_PI)
#define TAU (PI * 2)

using namespace glm; 

#define PREFERENCES_DIRECTORY "anm2ed"

#define ROUND_NEAREST_MULTIPLE(value, multiple) (roundf((value) / (multiple)) * (multiple))
#define FLOAT_TO_U8(x) (static_cast<u8>((x) * 255.0f))
#define U8_TO_FLOAT(x) ((x) / 255.0f)
#define PERCENT_TO_UNIT(x) (x / 100.0f)
#define UNIT_TO_PERCENT(x) (x * 100.0f)
#define SECOND 1000.0f
#define TICK_DELAY (SECOND / 30.0)
#define UPDATE_DELAY (SECOND / 120.0)
#define ID_NONE -1
#define INDEX_NONE -1
#define VALUE_NONE -1
#define TIME_NONE -1.0f
#define GL_ID_NONE 0

#ifdef _WIN32
  #define POPEN  _popen
  #define PCLOSE _pclose
  #define PWRITE_MODE "wb"
  #define PREAD_MODE "r"
#else
  #define POPEN  popen
  #define PCLOSE pclose
  #define PWRITE_MODE "w"
  #define PREAD_MODE "r"
#endif

static const GLuint GL_TEXTURE_INDICES[] = {0, 1, 2, 2, 3, 0};
static const vec4 COLOR_RED = {1.0f, 0.0f, 0.0f, 1.0f};
static const vec4 COLOR_GREEN = {0.0f, 1.0f, 0.0f, 1.0f};
static const vec4 COLOR_BLUE = {0.0f, 0.0f, 1.0f, 1.0f};
static const vec4 COLOR_PINK = {1.0f, 0.0f, 1.0f, 1.0f};
static const vec4 COLOR_OPAQUE = {1.0f, 1.0f, 1.0f, 1.0f};
static const vec4 COLOR_TRANSPARENT = {0.0f, 0.0f, 0.0f, 0.0f};
static const vec3 COLOR_OFFSET_NONE = {0.0f, 0.0f, 0.0f};

static inline std::string preferences_path_get(void)
{
    char* preferencesPath = SDL_GetPrefPath("", PREFERENCES_DIRECTORY);
    std::string preferencesPathString = preferencesPath;
    SDL_free(preferencesPath);
    return preferencesPathString;
}

static inline bool string_to_bool(const std::string& string) 
{
    if (string == "1") return true;

    std::string lower = string;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
 
    return lower == "true";
}

static inline std::string string_quote(const std::string& string) 
{
    return "\"" + string + "\"";
}

static inline std::string string_to_lowercase(std::string string) {
    std::transform
    (
        string.begin(), string.end(), string.begin(),
        [](u8 character){ return std::tolower(character); }
    );
    return string;
}

#define FLOAT_FORMAT_MAX_DECIMALS 5
#define FLOAT_FORMAT_EPSILON 1e-5f
static constexpr f32 FLOAT_FORMAT_POW10[] = {
    1.f,
    10.f,
    100.f,
    1000.f,
    10000.f,
    100000.f
};

static inline s32 f32_decimals_needed(f32 value)
{
    for (s32 decimalCount = 0; decimalCount <= FLOAT_FORMAT_MAX_DECIMALS; ++decimalCount)
    {
        f32 scale = FLOAT_FORMAT_POW10[decimalCount];
        f32 rounded = roundf(value * scale) / scale;
        if (fabsf(value - rounded) < FLOAT_FORMAT_EPSILON)
            return decimalCount;
    }
    return FLOAT_FORMAT_MAX_DECIMALS;
}

static inline const char* f32_format_get(f32 value)
{
    static std::string formatString;
    const s32 decimalCount = f32_decimals_needed(value);
    formatString = (decimalCount == 0)
        ? "%.0f"
        : ("%." + std::to_string(decimalCount) + "f");
    return formatString.c_str();
}

static inline const char* vec2_format_get(const vec2& value)
{
    static std::string formatString;
    const s32 decimalCountX = f32_decimals_needed(value.x);
    const s32 decimalCountY = f32_decimals_needed(value.y);
    const s32 decimalCount = (decimalCountX > decimalCountY) ? decimalCountX : decimalCountY;
    formatString = (decimalCount == 0)
        ? "%.0f"
        : ("%." + std::to_string(decimalCount) + "f");
    return formatString.c_str();
}

static inline std::string working_directory_from_file_set(const std::string& path)
{
    std::filesystem::path filePath = path;
    std::filesystem::path parentPath = filePath.parent_path();
	std::filesystem::current_path(parentPath);
    return parentPath.string();
};

static inline std::string path_extension_change(const std::string& path, const std::string& extension)
{
    std::filesystem::path filePath(path);
    filePath.replace_extension(extension);
    return filePath.string();
}

static inline bool path_is_extension(const std::string& path, const std::string& extension)
{
    auto e = std::filesystem::path(path).extension().string();
    std::transform(e.begin(), e.end(), e.begin(), ::tolower);
    return e == ("." + extension);
}

static inline bool path_exists(const std::filesystem::path& pathCheck)
{
    std::error_code errorCode;
    return std::filesystem::exists(pathCheck, errorCode) && ((void)std::filesystem::status(pathCheck, errorCode), !errorCode);
}

static inline bool path_is_valid(const std::filesystem::path& pathCheck)
{
    namespace fs = std::filesystem;
    std::error_code ec;

    if (fs::is_directory(pathCheck, ec)) return false;

    fs::path parentDir = pathCheck.has_parent_path() ? pathCheck.parent_path() : fs::path(".");
    if (!fs::is_directory(parentDir, ec)) return false;

    bool existedBefore = fs::exists(pathCheck, ec);
    std::ofstream testStream(pathCheck, std::ios::app | std::ios::binary);
    bool isValid = testStream.is_open();
    testStream.close();

    if (!existedBefore && isValid)
        fs::remove(pathCheck, ec);

    return isValid;
}

static inline s32 string_to_enum(const std::string& string, const char* const* array, s32 n) 
{
    for (s32 i = 0; i < n; i++) 
        if (string == array[i]) 
            return i;
    return -1;
};

template <typename T>
T& dummy_value()
{
    static T value{}; 
    return value;
}

template<typename T>
static inline s32 map_next_id_get(const std::map<s32, T>& map) 
{
    s32 id = 0; 
    
    for (const auto& [key, value] : map) 
    {
        if (key != id) 
            break; 
        ++id; 
    }
    
    return id;
}

template <typename Map>
static inline auto map_find(Map& map, typename Map::key_type id)
    -> typename Map::mapped_type* 
{
    if (auto it = map.find(id); it != map.end())
        return &it->second;
    return nullptr;
}

template<typename Map, typename Key>
static inline void map_swap(Map& map, const Key& key1, const Key& key2)
{
    if (key1 == key2)
        return;
    
    auto it1 = map.find(key1);
    auto it2 = map.find(key2);

    if (it1 != map.end() && it2 != map.end()) 
    {
        using std::swap;
        swap(it1->second, it2->second);
    } 
    else if (it1 != map.end()) 
    {
        map[key2] = std::move(it1->second);
        map.erase(it1);
    } 
    else if (it2 != map.end()) 
    {
        map[key1] = std::move(it2->second);
        map.erase(it2);
    }
};

template <typename T>
static inline void map_insert_shift(std::map<int, T>& map, s32 index, const T& value)
{
    const s32 insertIndex = index + 1;

    std::vector<std::pair<int, T>> toShift;
    for (auto it = map.rbegin(); it != map.rend(); ++it)
    {
        if (it->first < insertIndex)
            break;
        toShift.emplace_back(it->first + 1, std::move(it->second));
    }

    for (const auto& [newKey, _] : toShift)
        map.erase(newKey - 1);

    for (auto& [newKey, val] : toShift)
        map[newKey] = std::move(val);

    map[insertIndex] = value;
}

template <typename T>
void vector_value_erase(std::vector<T>& v, const T& value) 
{
    v.erase(std::remove(v.begin(), v.end(), value), v.end());
}

template <typename T>
void vector_value_swap(std::vector<T>& v, const T& a, const T& b) 
{
    for (auto& element : v) 
    {
        if (element == a) element = b;
        else if (element == b) element = a;
    }
}

static inline mat4 quad_model_get(vec2 size = {}, vec2 position = {}, vec2 pivot = {}, vec2 scale = vec2(1.0f), f32 rotation = {})
{
    vec2 scaleAbsolute  = glm::abs(scale);
    vec2 scaleSign = glm::sign(scale);
    vec2 pivotScaled = pivot * scaleAbsolute;
    vec2 sizeScaled  = size  * scaleAbsolute;

    mat4 model(1.0f);
    model = glm::translate(model, vec3(position - pivotScaled, 0.0f));
    model = glm::translate(model, vec3(pivotScaled, 0.0f));
    model = glm::scale(model, vec3(scaleSign, 1.0f));
    model = glm::rotate(model, glm::radians(rotation), vec3(0, 0, 1));
    model = glm::translate(model, vec3(-pivotScaled, 0.0f));
    model = glm::scale(model, vec3(sizeScaled, 1.0f));
    return model;
}

static inline mat4 quad_model_parent_get(vec2 position = {}, vec2 pivot = {}, vec2 scale = vec2(1.0f), f32 rotation = {})
{
    vec2 scaleSign = glm::sign(scale);
    vec2 scaleAbsolute  = glm::abs(scale);
    f32 handedness = (scaleSign.x * scaleSign.y) < 0.0f ? -1.0f : 1.0f;

    mat4 local(1.0f);
    local = glm::translate(local, vec3(pivot, 0.0f));
    local = glm::scale(local, vec3(scaleSign, 1.0f));
    local = glm::rotate(local, glm::radians(rotation) * handedness, vec3(0, 0, 1));
    local = glm::translate(local, vec3(-pivot, 0.0f));
    local = glm::scale(local, vec3(scaleAbsolute, 1.0f));

    return glm::translate(mat4(1.0f), vec3(position, 0.0f)) * local;
}

#define DEFINE_STRING_TO_ENUM_FUNCTION(function, enumType, stringArray, count)    \
    static inline enumType function(const std::string& string)                    \
    {                                                                             \
        return static_cast<enumType>(string_to_enum(string, stringArray, count)); \
    };

#define DATATYPE_LIST \
    X(TYPE_INT,      s32) \
    X(TYPE_BOOL,     bool) \
    X(TYPE_FLOAT,    f32) \
    X(TYPE_STRING,   std::string) \
    X(TYPE_IVEC2,    ivec2) \
    X(TYPE_IVEC2_WH, ivec2) \
    X(TYPE_VEC2,     vec2) \
    X(TYPE_VEC2_WH,  vec2) \
    X(TYPE_VEC3,     vec3) \
    X(TYPE_VEC4,     vec4)

enum DataType 
{
    #define X(symbol, ctype) symbol,
    DATATYPE_LIST
    #undef X
};

#define DATATYPE_TO_CTYPE(dt) DATATYPE_CTYPE_##dt
#define X(symbol, ctype) typedef ctype DATATYPE_CTYPE_##symbol;
DATATYPE_LIST
#undef X

enum OriginType
{
    ORIGIN_TOP_LEFT,
    ORIGIN_CENTER
};