#pragma once

#include "anm2.h"
#include "canvas.h"
#include "resources.h"
#include "settings.h"

#define GENERATE_PREVIEW_TIME_MIN 0.0f
#define GENERATE_PREVIEW_TIME_MAX 1.0f

const vec2 GENERATE_PREVIEW_SIZE = {325, 215};

struct GeneratePreview {
  Anm2* anm2 = nullptr;
  Anm2Reference* reference = nullptr;
  Resources* resources = nullptr;
  Settings* settings = nullptr;
  Canvas canvas;
  float time{};
};

void generate_preview_init(GeneratePreview* self, Anm2* anm2, Anm2Reference* reference, Resources* resources, Settings* settings);
void generate_preview_draw(GeneratePreview* self);
void generate_preview_free(GeneratePreview* self);