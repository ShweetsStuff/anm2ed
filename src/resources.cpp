#include "resources.h"

#include <ranges>

#include "log.h"

using namespace anm2ed::resource;

#ifdef _WIN32
static constexpr const char* CHINESE_FALLBACK = "C:\\Windows\\Fonts\\msyh.ttc";
static constexpr const char* KOREAN_FALLBACK = "C:\\Windows\\Fonts\\malgun.ttf";
static constexpr int CHINESE_FALLBACK_NUMBER = 0;
static constexpr int KOREAN_FALLBACK_NUMBER = 0;
#else
static constexpr const char* CHINESE_FALLBACK = "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc";
static constexpr const char* KOREAN_FALLBACK = "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc";
static constexpr int CHINESE_FALLBACK_NUMBER = 2;
static constexpr int KOREAN_FALLBACK_NUMBER = 1;
#endif

namespace anm2ed
{
  Resources::Resources()
  {
    auto isKoreanFallback = std::filesystem::exists(KOREAN_FALLBACK);
    auto isChineseFallback = std::filesystem::exists(CHINESE_FALLBACK);

    if (!isKoreanFallback) logger.error(std::format("Failed to load Korean fallback font: {}", KOREAN_FALLBACK));
    if (!isChineseFallback) logger.error(std::format("Failed to load Chinese fallback font: {}", CHINESE_FALLBACK));

    for (auto [i, fontInfo] : std::views::enumerate(font::FONTS))
    {
      fonts[i] = Font((void*)fontInfo->data, fontInfo->size, font::SIZE);
      if (isKoreanFallback)
        fonts[i].append(KOREAN_FALLBACK, font::SIZE, KOREAN_FALLBACK_NUMBER);
      else
        fonts[i].append((void*)FONT_KOREAN_TTF.data, FONT_KOREAN_TTF.size, font::SIZE);
      if (isChineseFallback)
        fonts[i].append(CHINESE_FALLBACK, font::SIZE, CHINESE_FALLBACK_NUMBER);
      else
        fonts[i].append((void*)FONT_CHINESE_TTF.data, FONT_CHINESE_TTF.size, font::SIZE);
    }

    for (auto [i, iconInfo] : std::views::enumerate(icon::ICONS))
      icons[i] = texture::svg_load((const char*)iconInfo.svg->data, iconInfo.svg->size, iconInfo.size);

    for (auto [i, shaderInfo] : std::views::enumerate(shader::SHADERS))
      shaders[i] = Shader(shaderInfo.vertex, shaderInfo.fragment);
  };

  GLuint Resources::icon_id_get(int icon) { return texture::id_get(icons[icon]); }

  resource::AudioData& Resources::music_track()
  {
    if (!isMusicLoaded)
    {
      music = AudioData(MUSIC_ABOUT_OPUS.data, MUSIC_ABOUT_OPUS.size);
      isMusicLoaded = true;
    }
    return music;
  }

  resource::AudioData* Resources::music_track_if_loaded() { return isMusicLoaded ? &music : nullptr; }
}
