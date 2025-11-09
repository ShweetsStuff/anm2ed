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
    glm::ivec2 mousePos{};
    std::vector<resource::Texture> renderFrames{};

  public:
    AnimationPreview();
    void tick(Manager&, Document&, Settings&);
    void update(Manager&, Settings&, Resources&);
  };
}
