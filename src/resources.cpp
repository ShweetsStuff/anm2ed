#include "resources.h"

#include <ranges>

#include "music.h"

using namespace anm2ed::resource;

namespace anm2ed
{
  Resources::Resources()
  {
    for (auto [i, fontInfo] : std::views::enumerate(font::FONTS))
      fonts[i] = Font((void*)fontInfo.data, fontInfo.length, font::SIZE);

    for (auto [i, iconInfo] : std::views::enumerate(icon::ICONS))
      icons[i] = Texture(iconInfo.data, iconInfo.length, iconInfo.size);

    for (auto [i, shaderInfo] : std::views::enumerate(shader::SHADERS))
      shaders[i] = Shader(shaderInfo.vertex, shaderInfo.fragment);
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
