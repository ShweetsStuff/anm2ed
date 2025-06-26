#pragma once

#include <SDL3/SDL.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <tinyxml2.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <ranges>
#include <vector>

#include "STRINGS.h"

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

using namespace glm; // fuck you

#define MIN(x, min) (x < min ? min : x)
#define MAX(x, max) (x > max ? max : x)
#define CLAMP(x, min, max) (MIN(MAX(x, max), min))

#define COLOR_FLOAT_TO_INT(x) (static_cast<int>((x) * 255.0f))
#define COLOR_INT_TO_FLOAT(x) ((x) / 255.0f)

#define TICK_DELAY 33.3f
#define SECOND 1000.0f
#define TICK_RATE (SECOND / TICK_DELAY)

static inline const char* enum_to_string(const char* arr[], s32 count, s32 index) { return (index >= 0 && index < count) ? arr[index] : "undefined"; }
static inline s32 string_to_enum(const char* str, const char* const* arr, s32 n) { for (s32 i=0; i<n; ++i) if (!strcmp(str, arr[i])) return i; return -1; }
static inline bool string_to_bool(const char* str) { if (strcmp(str, "true") == 0) return true; return false; }

static inline void working_directory_from_path_set(const char* path)
{
    std::filesystem::path filePath = path;
	std::filesystem::path dir = filePath.parent_path();
    if (!dir.empty())
        std::filesystem::current_path(dir);
};

template<typename T>
static inline s32 map_next_id_get(const std::map<s32, T>& map) {
    s32 id = 0; for (const auto& [key, _] : map) if (key != id) break; else ++id; return id;
}

template<typename Map, typename Key>
static inline void map_swap(Map& map, const Key& key1, const Key& key2)
{
    auto it1 = map.find(key1);
    auto it2 = map.find(key2);
    if (it1 == map.end() || it2 == map.end())
        return;

    using std::swap;
    swap(it1->second, it2->second);
}

#define DEFINE_ENUM_TO_STRING_FN(fn_name, arr, count) \
    static inline const char* fn_name(s32 index) {   \
        return enum_to_string(arr, count, index);    \
    }

#define DEFINE_STRING_TO_ENUM_FN(fn_name, enum_type, str_array, count) \
    static inline enum_type fn_name(const char* str) {                 \
        return (enum_type)string_to_enum(str, str_array, count);       \
    }
