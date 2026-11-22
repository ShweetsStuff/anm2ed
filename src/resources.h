#pragma once

#include <imgui/imgui.h>

#include "audio.h"
#include "font.h"
#include "icon.h"
#include "shader.h"
#include "texture.h"

namespace anm2ed
{
  class Resources
  {
  public:
    resource::Font fonts[resource::font::COUNT]{};
    resource::Image icons[resource::icon::COUNT]{};
    resource::Shader shaders[resource::shader::COUNT]{};
    resource::AudioData music{};

    Resources();
    GLuint icon_id_get(int);
    resource::AudioData& music_track();
    resource::AudioData* music_track_if_loaded();

  private:
    bool isMusicLoaded{false};
  };
}
