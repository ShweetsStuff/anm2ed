#pragma once

#include <imgui/imgui.h>

#include "font.h"
#include "icon.h"
#include "shader.h"
#include "texture.h"

namespace anm2ed::resources
{
  class Resources
  {
  public:
    font::Font fonts[font::COUNT]{};
    texture::Texture icons[icon::COUNT]{};
    shader::Shader shaders[shader::COUNT]{};

    Resources();
  };
}