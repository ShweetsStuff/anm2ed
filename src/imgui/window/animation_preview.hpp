#pragma once

#include <cstdint>
#include <filesystem>

#include "audio_stream.hpp"
#include "canvas_view.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"

namespace anm2ed::imgui
{
  class AnimationPreview : public CanvasView
  {
    MIX_Mixer* mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    AudioStream audioStream = AudioStream(mixer);
    bool wasPlaybackPlaying{};
    bool isPreviewHovered{};
    bool isSizeTrySet{true};
    Settings savedSettings{};
    float savedZoom{};
    glm::vec2 savedPan{};
    int savedOverlayIndex{};
    uint64_t savedOverlayDocumentId{};
    bool isMoveDragging{};
    glm::vec2 moveOffset{};
    glm::vec2 nullRectScaleAnchor{};
    std::filesystem::path renderTempDirectory{};
    std::vector<std::filesystem::path> renderTempFrames{};
    std::vector<int> renderFrameSoundIDs{};
    int renderFrameIndex{};
    int renderFrameCount{};
    int renderFrameRate{1};
    int renderAnimationFps{1};
    int renderFrameSoundTimePrev{-1};

  public:
    void tick(Manager&, Settings&, float);
    void update(Manager&, Settings&, Resources&);
  };
}
