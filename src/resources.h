#pragma once

#include <imgui/imgui.h>

#include "audio.h"
#include "font.h"
#include "icon.h"
#include "shader.h"
#include "texture.h"

class Resources
{
public:
    ImFont* fonts[FONT_COUNT] = {};
    Image icons[ICON_COUNT]{};
    ShaderProgram shaders[SHADER_COUNT]{};
    AudioData music = {};

    Resources();
    GLuint icon_id_get(int);
    AudioData& music_track();
    AudioData* music_track_if_loaded();

private:
    bool isMusicLoaded = false;
};
