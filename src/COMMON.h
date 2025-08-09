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
#include <cstring>
#include <cmath>                          
#include <filesystem>                  
#include <format>           
#include <functional>            
#include <fstream>
#include <iostream>
#include <map>                          
#include <print>                          
#include <optional>
#include <ranges>                      
#include <string>
#include <vector>                      
#include <unordered_set>                      

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

#define ROUND_NEAREST_MULTIPLE(value, multiple) (roundf((value) / (multiple)) * (multiple))
#define FLOAT_TO_U8(x) (static_cast<u8>((x) * 255.0f))
#define U8_TO_FLOAT(x) ((x) / 255.0f)
#define PERCENT_TO_UNIT(x) (x / 100.0f)
#define TICK_DELAY 33.3f
#define TICK_CATCH_UP_MAX (33.3f * 5)
#define SECOND 1000.0f
#define TICK_RATE (SECOND / TICK_DELAY)
#define ID_NONE -1
#define INDEX_NONE -1
#define TIME_NONE -1.0f

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

static inline void log_error(const std::string& string)
{
    std::println("[ERROR] {}", string);
}

static inline void log_info(const std::string& string)
{
    std::println("[INFO] {}", string);
}

static inline void log_warning(const std::string& string)
{
    std::println("[WARNING] {}", string);
}

static inline bool string_to_bool(const std::string& string) 
{
    if (string == "1") return true;

    std::string lower = string;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
 
    return lower == "true";
}

static inline void working_directory_from_file_set(const std::string& path)
{
    std::filesystem::path filePath = path;
    std::filesystem::path parentPath = filePath.parent_path();
	std::filesystem::current_path(parentPath);
};

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
    TYPE_VEC4
};

enum OriginType
{
    ORIGIN_TOP_LEFT,
    ORIGIN_CENTER
};