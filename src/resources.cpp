#include "resources.h"

#include <ranges>

#include "log.h"
#include "music.h"

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
      fonts[i] = Font((void*)fontInfo.data, fontInfo.length, font::SIZE);
      if (isKoreanFallback)
        fonts[i].append(KOREAN_FALLBACK, font::SIZE, KOREAN_FALLBACK_NUMBER);
      else
        fonts[i].append((void*)font::KOREAN_INFO.data, font::KOREAN_INFO.length, font::SIZE);
      if (isChineseFallback)
        fonts[i].append(CHINESE_FALLBACK, font::SIZE, CHINESE_FALLBACK_NUMBER);
      else
        fonts[i].append((void*)font::CHINESE_INFO.data, font::CHINESE_INFO.length, font::SIZE);
    }

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
