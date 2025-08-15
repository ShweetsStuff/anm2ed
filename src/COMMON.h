#pragma once

#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <tinyxml2.h>

#include <algorithm>                   
#include <chrono>                      
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
#define SECOND 1000.0f
#define TICK_DELAY (SECOND / 30.0)
#define UPDATE_DELAY (SECOND / 120.0)
#define ID_NONE -1
#define INDEX_NONE -1
#define TIME_NONE -1.0f

#if defined(_WIN32)
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

#define UV_VERTICES(uvMin, uvMax) \
{ \
  0, 0, uvMin.x, uvMin.y, \
  1, 0, uvMax.x, uvMin.y, \
  1, 1, uvMax.x, uvMax.y, \
  0, 1, uvMin.x, uvMax.y  \
}

static const f32 GL_VERTICES[] =
{
    0, 0,  
    1, 0,  
    1, 1,  
    0, 1  
};

constexpr f32 GL_UV_VERTICES[] = 
{
    0, 0, 0.0f, 0.0f,
    1, 0, 1.0f, 0.0f,
    1, 1, 1.0f, 1.0f,
    0, 1, 0.0f, 1.0f
};

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

#define FLOAT_FORMAT_MAX_DECIMALS 2
#define FLOAT_FORMAT_EPSILON 1e-6f
static constexpr f32 FLOAT_FORMAT_POW10[] = {1.f, 10.f, 100.f};

static inline s32 f32_decimals_needed(f32 value)
{
    f32 integerPart = 0.f;
    f32 fractionalPart = modff(value, &integerPart);
    fractionalPart = fabsf(fractionalPart);

    if (fractionalPart < FLOAT_FORMAT_EPSILON)
        return 0;

    for (s32 decimalCount = 1; decimalCount <= FLOAT_FORMAT_MAX_DECIMALS; ++decimalCount)
    {
        f32 scaledFraction = fractionalPart * FLOAT_FORMAT_POW10[decimalCount];
        if (fabsf(scaledFraction - roundf(scaledFraction)) < FLOAT_FORMAT_EPSILON * FLOAT_FORMAT_POW10[decimalCount])
            return decimalCount;
    }
    return FLOAT_FORMAT_MAX_DECIMALS;
}

static inline const char* f32_format_get(f32 value)
{
    static std::string formatString;
    const s32 decimalCount = f32_decimals_needed(value);
    formatString = (decimalCount == 0) ? "%.0f" : ("%." + std::to_string(decimalCount) + "f");
    return formatString.c_str();
}

static inline const char* vec2_format_get(const vec2& value)
{
    static std::string formatString;
    const s32 decimalCountX = f32_decimals_needed(value.x);
    const s32 decimalCountY = f32_decimals_needed(value.y);
    const s32 decimalCount = (decimalCountX > decimalCountY) ? decimalCountX : decimalCountY;
    formatString = (decimalCount == 0) ? "%.0f" : ("%." + std::to_string(decimalCount) + "f");
    return formatString.c_str();
}

static inline std::string path_canonical_resolve
(
    const std::string& inputPath,
    const std::string& basePath = std::filesystem::current_path().string()
)
{
    auto strings_equal_ignore_case = [](std::string a, std::string b) {
        auto to_lower = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };
        std::transform(a.begin(), a.end(), a.begin(), to_lower);
        std::transform(b.begin(), b.end(), b.begin(), to_lower);
        return a == b;
    };

    std::string sanitized = inputPath;
    std::replace(sanitized.begin(), sanitized.end(), '\\', '/');

    std::filesystem::path normalizedPath = sanitized;
    std::filesystem::path absolutePath = normalizedPath.is_absolute()
        ? normalizedPath
        : (std::filesystem::path(basePath) / normalizedPath);

    std::error_code error;
    if (std::filesystem::exists(absolutePath, error)) {
        std::error_code canonicalError;
        std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(absolutePath, canonicalError);
        return (canonicalError ? absolutePath : canonicalPath).generic_string();
    }

    std::filesystem::path resolvedPath = absolutePath.root_path();
    std::filesystem::path remainingPath = absolutePath.relative_path();

    for (const std::filesystem::path& segment : remainingPath) {
        std::filesystem::path candidatePath = resolvedPath / segment;
        if (std::filesystem::exists(candidatePath, error)) {
            resolvedPath = candidatePath;
            continue;
        }

        bool matched = false;
        if (std::filesystem::exists(resolvedPath, error) && std::filesystem::is_directory(resolvedPath, error)) {
            for (const auto& directoryEntry : std::filesystem::directory_iterator(resolvedPath, error)) {
                if (strings_equal_ignore_case(directoryEntry.path().filename().string(), segment.string())) {
                    resolvedPath = directoryEntry.path();
                    matched = true;
                    break;
                }
            }
        }
        if (!matched) return sanitized; 
    }

    if (!std::filesystem::exists(resolvedPath, error))
        return sanitized;

    std::error_code canonicalError;
    std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(resolvedPath, canonicalError);
    return (canonicalError ? resolvedPath : canonicalPath).generic_string();
};

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
    if (fs::exists(pathCheck, ec) && !fs::is_regular_file(pathCheck, ec)) return false;

    fs::path parentDir = pathCheck.has_parent_path() ? pathCheck.parent_path() : fs::path(".");
    if (!fs::is_directory(parentDir, ec)) return false;

    bool existedBefore = fs::exists(pathCheck, ec);
    std::ofstream testStream(pathCheck, std::ios::app | std::ios::binary);
    bool isValid = testStream.is_open();
    testStream.close();

    if (!existedBefore && isValid)
        fs::remove(pathCheck, ec); // cleanup if we created it

    return isValid;
}

static inline const char* enum_to_string(const char* array[], s32 count, s32 index) 
{ 
    return (index >= 0 && index < count) ? array[index] : ""; 
};

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
    
    for (const auto& [key, _] : map) 
        if (key != id) 
            break; 
        else 
            ++id; 
    
    return id;
}

template<typename T>
static inline T* map_find(std::map<s32, T>& map, s32 id) 
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

static inline mat4 quad_model_get(vec2 size, vec2 position, vec2 pivot, f32 rotation, vec2 scale)
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

static inline mat4 quad_parent_model_get(vec2 position, vec2 pivot, f32 rotation, vec2 scale)
{
    vec2 scaleSign = glm::sign(scale);
    vec2 scaleAbsolute  = glm::abs(scale);
    f32 handedness = (scaleSign.x * scaleSign.y) < 0.0f ? -1.0f : 1.0f;

    mat4 local(1.0f);
    local = glm::translate(local, vec3(pivot, 0.0f));
    local = glm::scale(local, vec3(scaleSign, 1.0f)); // mirror if needed
    local = glm::rotate(local, glm::radians(rotation) * handedness, vec3(0, 0, 1));
    local = glm::translate(local, vec3(-pivot, 0.0f));
    local = glm::scale(local, vec3(scaleAbsolute, 1.0f));

    return glm::translate(mat4(1.0f), vec3(position, 0.0f)) * local;
}

#define DEFINE_ENUM_TO_STRING_FUNCTION(function, array, count) \
    static inline std::string function(s32 index)              \
    {                                                          \
        return enum_to_string(array, count, index);            \
    };

#define DEFINE_STRING_TO_ENUM_FUNCTION(function, enumType, stringArray, count)    \
    static inline enumType function(const std::string& string)                    \
    {                                                                             \
        return static_cast<enumType>(string_to_enum(string, stringArray, count)); \
    };


enum DataType
{
    TYPE_INT,
    TYPE_BOOL,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_IVEC2,
    TYPE_VEC2,
    TYPE_VEC3,
    TYPE_VEC4
};

enum OriginType
{
    ORIGIN_TOP_LEFT,
    ORIGIN_CENTER
};