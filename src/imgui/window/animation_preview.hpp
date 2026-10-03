#pragma once

#include <cstdint>
#include <filesystem>

#include "audio_stream.hpp"
#include "canvas_view.hpp"
#include "manager.hpp"
#include "recorder.hpp"
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
    bool isMoveDragging{};
    glm::vec2 moveOffset{};
    glm::vec2 nullRectScaleAnchor{};
    Recorder recorder{};
    glm::vec2 recordSize{};
    glm::vec2 recordPan{};
    float recordZoom{};

    void recording_start(Manager&, Settings&, Document&, const model::Animation*);
    void recording_stop(Manager&, Document&);

  public:
    void tick(Manager&, Settings&, float);
    void update(Manager&, Settings&, Resources&);
  };
}
