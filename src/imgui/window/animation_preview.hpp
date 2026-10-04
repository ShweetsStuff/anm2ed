#pragma once

#include <tuple>
#include <vector>

#include "audio_stream.hpp"
#include "canvas_view.hpp"
#include "manager.hpp"
#include "model/draw.hpp"
#include "render.hpp"
#include "resources.hpp"
#include "settings.hpp"

namespace anm2ed::imgui
{
  class AnimationPreview : public CanvasView
  {
    MIX_Mixer* mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    AudioStream audioStream = AudioStream(mixer);
    bool wasPlaybackPlaying{};
    bool isMoveDragging{};
    glm::vec2 moveOffset{};
    glm::vec2 nullRectScaleAnchor{};
    render::Recorder recorder{};
    glm::vec2 recordSize{};
    glm::vec2 recordPan{};
    float recordZoom{};

    // Where each item's pivot is at every frame of the animation, kept until the document or view changes.
    struct PivotPath
    {
      model::Draw item{};
      std::vector<glm::vec2> points{};
    };
    std::vector<PivotPath> pivotPaths{};
    std::tuple<std::uint64_t, int, bool> pivotPathsKey{};

    void recording_start(Manager&, Settings&, Document&, const model::Animation*);
    void recording_stop(Manager&, Document&);

  public:
    void tick(Manager&, Settings&, float);
    void update(Manager&, Settings&, Resources&);
  };
}
