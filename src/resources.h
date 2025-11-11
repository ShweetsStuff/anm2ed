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
    resource::Texture icons[resource::icon::COUNT]{};
    resource::Shader shaders[resource::shader::COUNT]{};
    resource::Audio music{};

    Resources();
  };
}