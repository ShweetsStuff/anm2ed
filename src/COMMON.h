#pragma once

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits.h>
#include <math.h>
#include <pugixml.hpp>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <vector>

#include "STRINGS.h"
#include "RESOURCES.h"

using namespace glm;
using namespace pugi;
using namespace std;

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
#define ATAN(x1, x2, y1, y2) (fmod((atan2(y2 - y1, x2 - x1) + TAU), TAU)) /* [0, 2PI] */

struct State
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_GLContext glContext;
	vec3 clearColor = {0.69, 0.69, 0.69};
	u64 tick = 0;
	u64 lastTick = 0;
	bool isRunning = true;
}; 
