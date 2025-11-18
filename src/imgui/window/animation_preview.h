#pragma once

#include "audio_stream.h"
#include "canvas.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

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
    glm::ivec2 mousePos{};
    glm::vec2 checkerPan{};
    glm::vec2 checkerSyncPan{};
    float checkerSyncZoom{};
    bool isCheckerPanInitialized{};
    bool hasPendingZoomPanAdjust{};
    std::vector<resource::Texture> renderFrames{};

  public:
    AnimationPreview();
    void tick(Manager&, Settings&);
    void update(Manager&, Settings&, Resources&);
  };
}
