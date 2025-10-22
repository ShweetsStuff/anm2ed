#include "resources.h"
#include <ranges>

using namespace anm2ed::texture;
using namespace anm2ed::shader;
using namespace anm2ed::font;

namespace anm2ed::resources
{
  Resources::Resources()
  {
    for (auto [i, font] : std::views::enumerate(font::FONTS))
      fonts[i] = Font((void*)font.data, font.length, font::SIZE);

    for (auto [i, icon] : std::views::enumerate(icon::ICONS))
      icons[i] = Texture(icon.data, icon.length, icon.size);

    for (auto [i, shader] : std::views::enumerate(shader::SHADERS))
      shaders[i] = Shader(shader.vertex, shader.fragment);
  };
}