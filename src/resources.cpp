#include "resources.h"

#include <ranges>

#include "log.h"

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

  Resources::Resources()
  {
    auto isKoreanFallback = std::filesystem::exists(KOREAN_FALLBACK);
    auto isChineseFallback = std::filesystem::exists(CHINESE_FALLBACK);

    if (!isKoreanFallback) logger.error(std::format("Failed to load Korean fallback font: {}", KOREAN_FALLBACK));
    if (!isChineseFallback) logger.error(std::format("Failed to load Chinese fallback font: {}", CHINESE_FALLBACK));

    for (auto [i, fontInfo] : std::views::enumerate(FONTS))
    {
      fonts[i] = Font((void*)fontInfo->data, fontInfo->size, FONT_SIZE);
      if (isKoreanFallback)
        fonts[i].append(KOREAN_FALLBACK, FONT_SIZE, KOREAN_FALLBACK_NUMBER);
      else
        fonts[i].append((void*)FONT_KOREAN_TTF.data, FONT_KOREAN_TTF.size, FONT_SIZE);
      if (isChineseFallback)
        fonts[i].append(CHINESE_FALLBACK, FONT_SIZE, CHINESE_FALLBACK_NUMBER);
      else
        fonts[i].append((void*)FONT_CHINESE_TTF.data, FONT_CHINESE_TTF.size, FONT_SIZE);
    }

    for (auto [i, iconInfo] : std::views::enumerate(ICONS))
      icons[i] = texture_svg_load((const char*)iconInfo.svg->data, iconInfo.svg->size, iconInfo.size);

    for (auto [i, shaderInfo] : std::views::enumerate(SHADERS))
      shaders[i] = ShaderProgram(shaderInfo.vertex, shaderInfo.fragment);
  };

  GLuint Resources::icon_id_get(int icon) { return texture_id_get(icons[icon]); }

  AudioData& Resources::music_track()
  {
    if (!isMusicLoaded)
    {
      music = AudioData(MUSIC_ABOUT_OPUS.data, MUSIC_ABOUT_OPUS.size);
      isMusicLoaded = true;
    }
    return music;
  }

  AudioData* Resources::music_track_if_loaded() { return isMusicLoaded ? &music : nullptr; }
