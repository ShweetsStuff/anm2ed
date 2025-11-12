#include "resources.h"

#include <ranges>

#include "music.h"

using namespace anm2ed::resource;

namespace anm2ed
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

  resource::Audio& Resources::music_track()
  {
    if (!isMusicLoaded)
    {
      music = Audio(music::ABOUT, std::size(music::ABOUT));
      isMusicLoaded = true;
    }
    return music;
  }

  resource::Audio* Resources::music_track_if_loaded() { return isMusicLoaded ? &music : nullptr; }
}
