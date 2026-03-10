#pragma once

#include <imgui/imgui.h>

#include "audio.hpp"
#include "font.hpp"
#include "icon.hpp"
#include "shader.hpp"
#include "texture.hpp"

namespace anm2ed
{
  class Resources
  {
  public:
    resource::Font fonts[resource::font::COUNT]{};
    resource::Texture icons[resource::icon::COUNT]{};
    resource::Shader shaders[resource::shader::COUNT]{};
    resource::Audio music{};

    Resources();
    resource::Audio& music_track();
    resource::Audio* music_track_if_loaded();

  private:
    bool isMusicLoaded{false};
  };
}
