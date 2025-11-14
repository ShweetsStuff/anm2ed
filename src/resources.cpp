#include "resources.h"

#include <cstddef>

#include "music.h"

using namespace anm2ed::resource;

namespace anm2ed
{
  Resources::Resources()
  {
    for (std::size_t i = 0; i < font::COUNT; ++i)
    {
      const auto& fontInfo = font::FONTS[i];
      fonts[i] = Font((void*)fontInfo.data, fontInfo.length, font::SIZE);
    }

    for (std::size_t i = 0; i < icon::COUNT; ++i)
    {
      const auto& iconInfo = icon::ICONS[i];
      icons[i] = Texture(iconInfo.data, iconInfo.length, iconInfo.size);
    }

    for (std::size_t i = 0; i < shader::COUNT; ++i)
    {
      const auto& shaderInfo = shader::SHADERS[i];
      shaders[i] = Shader(shaderInfo.vertex, shaderInfo.fragment);
    }
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
