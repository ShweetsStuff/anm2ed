#pragma once

#include <filesystem>

#include "audio_stream.hpp"
#include "canvas.hpp"
#include "manager.hpp"
#include "resources.hpp"
#include "settings.hpp"

namespace anm2ed::imgui
{
  class AnimationPreview : public Canvas
  {
    MIX_Mixer* mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    AudioStream audioStream = AudioStream(mixer);
    bool isPreviewHovered{};
    bool isSizeTrySet{true};
    Settings savedSettings{};
    float savedZoom{};
    glm::vec2 savedPan{};
    int savedOverlayIndex{};
    glm::vec2 mousePos{};
    glm::vec2 checkerPan{};
    glm::vec2 checkerSyncPan{};
    float checkerSyncZoom{};
    bool isCheckerPanInitialized{};
    bool hasPendingZoomPanAdjust{};
    bool isMoveDragging{};
    glm::vec2 moveOffset{};
    std::filesystem::path renderTempDirectory{};
    std::vector<std::filesystem::path> renderTempFrames{};

  public:
    AnimationPreview();
    void tick(Manager&, Settings&);
    void update(Manager&, Settings&, Resources&);
  };
}
