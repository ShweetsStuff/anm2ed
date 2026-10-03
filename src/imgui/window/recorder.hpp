#pragma once

#include <filesystem>
#include <vector>

#include "audio_stream.hpp"
#include "document.hpp"
#include "render.hpp"

namespace anm2ed::imgui
{
  struct RecorderOptions
  {
    render::Type type{};
    std::filesystem::path path{};
    std::filesystem::path format{};
    std::filesystem::path ffmpegPath{};
    int start{};
    int end{};
    int animationFps{1};
    int fps{1};
    int rows{};
    int columns{};
    bool isSound{};
    bool isIsolated{};
    bool isBounded{};
  };

  // A render in progress: which animation times to capture, the frames captured so far (written to disk) and the
  // trigger sound of each, and assembling the output when done.
  class Recorder
  {
  public:
    RecorderOptions options{};
    int index{};
    int count{};

    bool start(const RecorderOptions&);
    bool is_done() const;
    float time_get() const;
    bool is_sound_frame() const;
    bool frame_capture(std::vector<uint8_t>, glm::ivec2, int);
    void finish(const Document&, AudioStream&);
    void cancel();

  private:
    std::filesystem::path directory{};
    std::vector<std::filesystem::path> frames{};
    std::vector<int> soundIds{};
    int soundTime{-1};
  };
}
