#include "resources.h"

#include <ranges>

#include "file.h"
#include "log.h"

#include <map>

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

// A font the atlas reads from memory it doesn't own, number `number` in its file.
struct FontSource
{
    const void* data = nullptr;
    std::size_t size = 0;
    int number = 0;
};

// A system font with the CJK glyphs the embedded subsets lack, read once and kept, as the atlas reads from it.
// The embedded subset when the system one isn't there.
static FontSource fallback_get(const char* path, int number, const Embed& embedded)
{
    static std::map<std::string, std::string> files;
    auto [it, isNew] = files.try_emplace(path);
    if (isNew && !file_text_load(path, &it->second))
        logger.error(std::format("Failed to load CJK fallback font: {}", path));
    if (it->second.empty())
        return {embedded.data, embedded.size, 0};
    return {it->second.data(), it->second.size(), number};
}

// A new font, or more glyphs for `target`.
static ImFont* font_add(FontSource source, ImFont* target)
{
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.FontNo = source.number;
    config.MergeMode = target != nullptr;
    config.DstFont = target;
    return ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)source.data, (int)source.size, FONT_SIZE, &config);
}

Resources::Resources()
{
    auto korean = fallback_get(KOREAN_FALLBACK, KOREAN_FALLBACK_NUMBER, FONT_KOREAN_TTF);
    auto chinese = fallback_get(CHINESE_FALLBACK, CHINESE_FALLBACK_NUMBER, FONT_CHINESE_TTF);
    for (int i = 0; i < FONT_COUNT; ++i)
    {
        fonts[i] = font_add({FONTS[i]->data, FONTS[i]->size}, nullptr);
        font_add(korean, fonts[i]);
        font_add(chinese, fonts[i]);
    }

    for (auto [i, iconInfo] : std::views::enumerate(ICONS))
        icons[i] = texture_svg_load((const char*)iconInfo.svg->data, iconInfo.svg->size, iconInfo.size);

    for (auto [i, shaderInfo] : std::views::enumerate(SHADERS))
        shaders[i] = ShaderProgram(shaderInfo.vertex, shaderInfo.fragment);
}

GLuint Resources::icon_id_get(int icon)
{
    return texture_id_get(icons[icon]);
}

AudioData& Resources::music_track()
{
    if (!isMusicLoaded)
    {
        music = AudioData(MUSIC_ABOUT_OGG.data, MUSIC_ABOUT_OGG.size);
        isMusicLoaded = true;
    }
    return music;
}

AudioData* Resources::music_track_if_loaded()
{
    return isMusicLoaded ? &music : nullptr;
}
