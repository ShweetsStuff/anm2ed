#include "animation_preview.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <ranges>
#include <system_error>

#include <glm/gtc/type_ptr.hpp>

#include "imgui_.hpp"
#include "log.hpp"
#include "math_.hpp"
#include "path_.hpp"
#include "strings.hpp"
#include "toast.hpp"
#include "tool.hpp"
#include "types.hpp"

using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace anm2ed::resource;
using namespace anm2ed::resource::texture;
using namespace glm;

namespace anm2ed::imgui
{
  constexpr auto NULL_COLOR = vec4(0.0f, 0.0f, 1.0f, 0.90f);
  constexpr auto TARGET_SIZE = vec2(32, 32);
  constexpr auto POINT_SIZE = vec2(4, 4);
  constexpr auto TRIGGER_TEXT_COLOR_DARK = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
  constexpr auto TRIGGER_TEXT_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
  constexpr auto PLAYBACK_TICK_RATE = 30.0f;

  namespace
  {
    std::filesystem::path render_destination_directory(const std::filesystem::path& path, int type)
    {
      if (type == render::PNGS) return path;
      auto directory = path.parent_path();
      if (directory.empty()) directory = std::filesystem::current_path();
      return directory;
    }

    std::filesystem::path render_frame_filename(const std::filesystem::path& format, int index)
    {
      auto formatString = path::to_utf8(format);
      try
      {
        auto name = std::vformat(formatString, std::make_format_args(index));
        auto filename = path::from_utf8(name).filename();
        if (filename.empty()) return path::from_utf8(std::format("frame_{:06}.png", index));
        if (filename.extension().empty()) filename.replace_extension(render::EXTENSIONS[render::SPRITESHEET]);
        return filename;
      }
      catch (...)
      {
        return path::from_utf8(std::format("frame_{:06}.png", index));
      }
    }

    std::filesystem::path render_temp_directory_create(const std::filesystem::path& directory)
    {
      auto timestamp = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
      for (int suffix = 0; suffix < 1000; ++suffix)
      {
        auto tempDirectory = directory / path::from_utf8(std::format(".anm2ed_render_tmp_{}_{}", timestamp, suffix));
        std::error_code ec;
        if (std::filesystem::create_directories(tempDirectory, ec)) return tempDirectory;
      }
      return {};
    }

    void render_temp_cleanup(std::filesystem::path& directory, std::vector<std::filesystem::path>& frames)
    {
      std::error_code ec;
      if (!directory.empty()) std::filesystem::remove_all(directory, ec);
      directory.clear();
      frames.clear();
    }

    void pixels_unpremultiply_alpha(std::vector<uint8_t>& pixels)
    {
      for (size_t index = 0; index + 3 < pixels.size(); index += 4)
      {
        auto alpha = pixels[index + 3];
        if (alpha == 0)
        {
          pixels[index + 0] = 0;
          pixels[index + 1] = 0;
          pixels[index + 2] = 0;
          continue;
        }
        if (alpha == 255) continue;

        float alphaUnit = (float)alpha / 255.0f;
        pixels[index + 0] = (uint8_t)glm::clamp((float)std::round((float)pixels[index + 0] / alphaUnit), 0.0f, 255.0f);
        pixels[index + 1] = (uint8_t)glm::clamp((float)std::round((float)pixels[index + 1] / alphaUnit), 0.0f, 255.0f);
        pixels[index + 2] = (uint8_t)glm::clamp((float)std::round((float)pixels[index + 2] / alphaUnit), 0.0f, 255.0f);
      }
    }

    bool render_audio_stream_generate(AudioStream& audioStream, std::map<int, anm2::Sound>& sounds,
                                      const std::vector<int>& frameSoundIDs, int fps)
    {
      audioStream.stream.clear();
      if (frameSoundIDs.empty() || fps <= 0) return true;

      SDL_AudioSpec mixSpec = audioStream.spec;
      mixSpec.format = SDL_AUDIO_F32;
      auto* mixer = MIX_CreateMixer(&mixSpec);
      if (!mixer) return false;

      auto channels = std::max(mixSpec.channels, 1);
      auto sampleRate = std::max(mixSpec.freq, 1);
      auto framesPerStep = (double)sampleRate / (double)fps;
      auto sampleFrameAccumulator = 0.0;
      auto frameBuffer = std::vector<float>{};

      for (auto soundID : frameSoundIDs)
      {
        if (soundID != -1 && sounds.contains(soundID)) sounds.at(soundID).audio.play(false, mixer);

        sampleFrameAccumulator += framesPerStep;
        auto sampleFramesToGenerate = (int)std::floor(sampleFrameAccumulator);
        sampleFramesToGenerate = std::max(sampleFramesToGenerate, 1);
        sampleFrameAccumulator -= (double)sampleFramesToGenerate;

        frameBuffer.resize((std::size_t)sampleFramesToGenerate * (std::size_t)channels);
        if (!MIX_Generate(mixer, frameBuffer.data(), (int)(frameBuffer.size() * sizeof(float))))
        {
          for (auto& [_, sound] : sounds)
            sound.audio.track_detach(mixer);
          MIX_DestroyMixer(mixer);
          audioStream.stream.clear();
          return false;
        }

        audioStream.stream.insert(audioStream.stream.end(), frameBuffer.begin(), frameBuffer.end());
      }

      for (auto& [_, sound] : sounds)
        sound.audio.track_detach(mixer);
      MIX_DestroyMixer(mixer);
      return true;
    }
  }

  AnimationPreview::AnimationPreview() : Canvas(vec2()) {}

  void AnimationPreview::tick(Manager& manager, Settings& settings)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& playback = document.playback;
    auto& frameTime = document.frameTime;
    auto& end = manager.recordingEnd;
    auto& zoom = document.previewZoom;
    auto& overlayIndex = document.overlayIndex;
    auto& pan = document.previewPan;

    if (manager.isRecording)
    {
      auto& ffmpegPath = settings.renderFFmpegPath;
      auto& path = settings.renderPath;
      auto pathString = path::to_utf8(path);
      auto& type = settings.renderType;

      if (playback.time > end || playback.isFinished)
      {
        if (settings.timelineIsSound) audioStream.capture_end(mixer);

        if (type == render::PNGS)
        {
          if (!renderTempFrames.empty())
          {
            toasts.push(std::vformat(localize.get(TOAST_EXPORT_RENDERED_FRAMES), std::make_format_args(pathString)));
            logger.info(std::vformat(localize.get(TOAST_EXPORT_RENDERED_FRAMES, anm2ed::ENGLISH),
                                     std::make_format_args(pathString)));
          }
          else
          {
            toasts.push(
                std::vformat(localize.get(TOAST_EXPORT_RENDERED_FRAMES_FAILED), std::make_format_args(pathString)));
            logger.error(std::vformat(localize.get(TOAST_EXPORT_RENDERED_FRAMES_FAILED, anm2ed::ENGLISH),
                                      std::make_format_args(pathString)));
          }
        }
        else if (type == render::SPRITESHEET)
        {
          auto& rows = settings.renderRows;
          auto& columns = settings.renderColumns;

          if (renderTempFrames.empty())
          {
            toasts.push(localize.get(TOAST_SPRITESHEET_NO_FRAMES));
            logger.warning(localize.get(TOAST_SPRITESHEET_NO_FRAMES, anm2ed::ENGLISH));
          }
          else
          {
            auto firstFrame = Texture(renderTempFrames.front());
            if (firstFrame.size.x <= 0 || firstFrame.size.y <= 0 || firstFrame.pixels.empty())
            {
              toasts.push(localize.get(TOAST_SPRITESHEET_EMPTY));
              logger.error(localize.get(TOAST_SPRITESHEET_EMPTY, anm2ed::ENGLISH));
            }
            else
            {
              auto frameWidth = firstFrame.size.x;
              auto frameHeight = firstFrame.size.y;
              ivec2 spritesheetSize = ivec2(frameWidth * columns, frameHeight * rows);

              std::vector<uint8_t> spritesheet((size_t)(spritesheetSize.x) * spritesheetSize.y * CHANNELS);

              for (std::size_t index = 0; index < renderTempFrames.size(); ++index)
              {
                auto frame = Texture(renderTempFrames[index]);
                auto row = (int)(index / columns);
                auto column = (int)(index % columns);
                if (row >= rows || column >= columns) break;
                if ((int)frame.pixels.size() < frameWidth * frameHeight * CHANNELS) continue;

                for (int y = 0; y < frameHeight; ++y)
                {
                  auto destY = (size_t)(row * frameHeight + y);
                  auto destX = (size_t)(column * frameWidth);
                  auto destOffset = (destY * spritesheetSize.x + destX) * CHANNELS;
                  auto srcOffset = (size_t)(y * frameWidth) * CHANNELS;
                  std::copy_n(frame.pixels.data() + srcOffset, frameWidth * CHANNELS, spritesheet.data() + destOffset);
                }
              }

              Texture spritesheetTexture(spritesheet.data(), spritesheetSize);
              if (spritesheetTexture.write_png(path))
              {
                toasts.push(std::vformat(localize.get(TOAST_EXPORT_SPRITESHEET), std::make_format_args(pathString)));
                logger.info(std::vformat(localize.get(TOAST_EXPORT_SPRITESHEET, anm2ed::ENGLISH),
                                         std::make_format_args(pathString)));
              }
              else
              {
                toasts.push(
                    std::vformat(localize.get(TOAST_EXPORT_SPRITESHEET_FAILED), std::make_format_args(pathString)));
                logger.error(std::vformat(localize.get(TOAST_EXPORT_SPRITESHEET_FAILED, anm2ed::ENGLISH),
                                          std::make_format_args(pathString)));
              }
            }
          }
        }
        else
        {
          if (settings.timelineIsSound && type != render::GIF)
          {
            if (!render_audio_stream_generate(audioStream, anm2.content.sounds, renderFrameSoundIDs, anm2.info.fps))
            {
              toasts.push(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED));
              logger.error("Failed to generate deterministic render audio stream; exporting without audio.");
              audioStream.stream.clear();
            }
          }
          else
            audioStream.stream.clear();

          if (animation_render(ffmpegPath, path, renderTempFrames, renderTempFrameDurations, audioStream,
                               (render::Type)type, anm2.info.fps))
          {
            toasts.push(std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION), std::make_format_args(pathString)));
            logger.info(std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION, anm2ed::ENGLISH),
                                     std::make_format_args(pathString)));
          }
          else
          {
            toasts.push(
                std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED), std::make_format_args(pathString)));
            logger.error(std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED, anm2ed::ENGLISH),
                                      std::make_format_args(pathString)));
          }
        }

        if (type == render::PNGS)
        {
          renderTempDirectory.clear();
          renderTempFrames.clear();
          renderTempFrameDurations.clear();
          renderFrameSoundIDs.clear();
        }
        else
        {
          render_temp_cleanup(renderTempDirectory, renderTempFrames);
          renderTempFrameDurations.clear();
          renderFrameSoundIDs.clear();
        }

        if (settings.renderIsRawAnimation)
        {
          settings = savedSettings;

          pan = savedPan;
          zoom = savedZoom;
          overlayIndex = savedOverlayIndex;
          isSizeTrySet = true;
          hasPendingZoomPanAdjust = false;
          isCheckerPanInitialized = false;
        }

        playback.isPlaying = false;
        playback.isFinished = false;
        manager.isRecording = false;
        manager.progressPopup.close();
      }
      else
      {
        if (settings.timelineIsSound && renderTempFrames.empty()) audioStream.capture_begin(mixer);
        auto frameSoundID = -1;
        if (settings.timelineIsSound && !anm2.content.sounds.empty())
        {
          if (auto animation = document.animation_get();
              animation && animation->triggers.isVisible && (!settings.timelineIsOnlyShowLayers || manager.isRecording))
          {
            if (auto trigger = animation->triggers.frame_generate(playback.time, anm2::TRIGGER); trigger.isVisible)
            {
              if (!trigger.soundIDs.empty())
              {
                auto soundIndex = trigger.soundIDs.size() > 1
                                      ? (size_t)math::random_in_range(0.0f, (float)trigger.soundIDs.size())
                                      : (size_t)0;
                soundIndex = std::min(soundIndex, trigger.soundIDs.size() - 1);
                auto soundID = trigger.soundIDs[soundIndex];
                if (anm2.content.sounds.contains(soundID)) frameSoundID = soundID;
              }
            }
          }
        }
        renderFrameSoundIDs.push_back(frameSoundID);

        bind();
        auto pixels = pixels_get();
        if (settings.renderIsRawAnimation) pixels_unpremultiply_alpha(pixels);
        auto frameIndex = (int)renderTempFrames.size();
        auto framePath = renderTempDirectory / render_frame_filename(settings.renderFormat, frameIndex);
        if (Texture::write_pixels_png(framePath, size, pixels.data()))
        {
          renderTempFrames.push_back(framePath);
          auto nowCounter = SDL_GetPerformanceCounter();
          auto counterFrequency = SDL_GetPerformanceFrequency();
          auto fallbackDuration = 1.0 / (double)std::max(anm2.info.fps, 1);

          if (renderTempFrames.size() == 1)
          {
            renderCaptureCounterPrev = nowCounter;
            renderTempFrameDurations.push_back(fallbackDuration);
          }
          else
          {
            auto elapsedCounter = nowCounter - renderCaptureCounterPrev;
            auto frameDuration = counterFrequency > 0 ? (double)elapsedCounter / (double)counterFrequency : 0.0;
            frameDuration = std::max(frameDuration, 1.0 / 1000.0);
            renderTempFrameDurations.back() = frameDuration;
            renderTempFrameDurations.push_back(frameDuration);
            renderCaptureCounterPrev = nowCounter;
          }
        }
        else
        {
          toasts.push(std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED), std::make_format_args(pathString)));
          logger.error(std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED, anm2ed::ENGLISH),
                                    std::make_format_args(pathString)));
          if (type != render::PNGS) render_temp_cleanup(renderTempDirectory, renderTempFrames);
          renderTempFrameDurations.clear();
          renderFrameSoundIDs.clear();
          playback.isPlaying = false;
          playback.isFinished = false;
          manager.isRecording = false;
          manager.progressPopup.close();
        }
      }
    }

    if (playback.isPlaying)
    {
      auto animation = document.animation_get();
      auto& isSound = settings.timelineIsSound;
      auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;

      if (!manager.isRecording && !anm2.content.sounds.empty() && isSound)
      {
        if (auto animation = document.animation_get();
            animation && animation->triggers.isVisible && (!isOnlyShowLayers || manager.isRecording))
        {
          if (auto trigger = animation->triggers.frame_generate(playback.time, anm2::TRIGGER); trigger.isVisible)
          {
            if (!trigger.soundIDs.empty())
            {
              auto soundIndex = trigger.soundIDs.size() > 1
                                    ? (size_t)math::random_in_range(0.0f, (float)trigger.soundIDs.size())
                                    : (size_t)0;
              soundIndex = std::min(soundIndex, trigger.soundIDs.size() - 1);
              auto soundID = trigger.soundIDs[soundIndex];

              if (anm2.content.sounds.contains(soundID)) anm2.content.sounds[soundID].audio.play(false, mixer);
            }
          }
        }
      }

      auto fps = std::max(anm2.info.fps, 1);
      auto deltaSeconds = manager.isRecording ? (1.0f / (float)fps) : (1.0f / PLAYBACK_TICK_RATE);
      playback.tick(fps, animation->frameNum, (animation->isLoop || settings.playbackIsLoop) && !manager.isRecording,
                    deltaSeconds);

      frameTime = playback.time;
    }
  }

  void AnimationPreview::update(Manager& manager, Settings& settings, Resources& resources)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& playback = document.playback;
    auto& reference = document.reference;
    auto animation = document.animation_get();
    auto& pan = document.previewPan;
    auto& zoom = document.previewZoom;
    auto& backgroundColor = settings.previewBackgroundColor;
    auto& axesColor = settings.previewAxesColor;
    auto& gridColor = settings.previewGridColor;
    auto& gridSize = settings.previewGridSize;
    auto& gridOffset = settings.previewGridOffset;
    auto& zoomStep = settings.inputZoomStep;
    auto& isGrid = settings.previewIsGrid;
    auto& overlayTransparency = settings.previewOverlayTransparency;
    auto& overlayIndex = document.overlayIndex;
    auto& isRootTransform = settings.previewIsRootTransform;
    auto& isPivots = settings.previewIsPivots;
    auto& isAxes = settings.previewIsAxes;
    auto& isAltIcons = settings.previewIsAltIcons;
    auto& isBorder = settings.previewIsBorder;
    auto& tool = settings.tool;
    auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;
    auto& shaderLine = resources.shaders[shader::LINE];
    bool isLightTheme = settings.theme == theme::LIGHT;
    auto& shaderAxes = resources.shaders[shader::AXIS];
    auto& shaderGrid = resources.shaders[shader::GRID];
    auto& shaderTexture = resources.shaders[shader::TEXTURE];
    auto& frames = document.frames.selection;

    auto reset_checker_pan = [&]()
    {
      checkerPan = pan;
      checkerSyncPan = pan;
      checkerSyncZoom = zoom;
      isCheckerPanInitialized = true;
      hasPendingZoomPanAdjust = false;
    };

    auto sync_checker_pan = [&]()
    {
      if (!isCheckerPanInitialized)
      {
        reset_checker_pan();
        return;
      }

      if (pan != checkerSyncPan || zoom != checkerSyncZoom)
      {
        bool ignorePanDelta = hasPendingZoomPanAdjust && zoom != checkerSyncZoom;
        if (!ignorePanDelta) checkerPan += pan - checkerSyncPan;
        checkerSyncPan = pan;
        checkerSyncZoom = zoom;
        if (ignorePanDelta) hasPendingZoomPanAdjust = false;
      }
    };

    auto center_view = [&]() { pan = vec2(); };

    auto fit_view = [&]()
    {
      if (animation) set_to_rect(zoom, pan, animation->rect(isRootTransform));
    };

    auto zoom_adjust = [&](float delta)
    {
      auto focus = position_translate(zoom, pan, size * 0.5f);
      auto previousZoom = zoom;
      zoom_set(zoom, pan, focus, delta);
      if (zoom != previousZoom) hasPendingZoomPanAdjust = true;
    };

    auto zoom_in = [&]() { zoom_adjust(zoomStep); };
    auto zoom_out = [&]() { zoom_adjust(-zoomStep); };

    manager.isAbleToRecord = false;

    if (ImGui::Begin(localize.get(LABEL_ANIMATION_PREVIEW_WINDOW), &settings.windowIsAnimationPreview))
    {
      manager.isAbleToRecord = true;

      auto childSize = ImVec2(row_widget_width_get(4),
                              (ImGui::GetTextLineHeightWithSpacing() * 4) + (ImGui::GetStyle().WindowPadding.y * 2));

      if (ImGui::BeginChild("##Grid Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::Checkbox(localize.get(BASIC_GRID), &isGrid);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_VISIBILITY));
        ImGui::SameLine();
        ImGui::ColorEdit4(localize.get(BASIC_COLOR), value_ptr(gridColor), ImGuiColorEditFlags_NoInputs);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_COLOR));

        input_int2_range(localize.get(BASIC_SIZE), gridSize, ivec2(GRID_SIZE_MIN), ivec2(GRID_SIZE_MAX));
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_SIZE));

        input_int2_range(localize.get(BASIC_OFFSET), gridOffset, ivec2(GRID_OFFSET_MIN), ivec2(GRID_OFFSET_MAX));
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_GRID_OFFSET));
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##View Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::InputFloat(localize.get(BASIC_ZOOM), &zoom, zoomStep, zoomStep, "%.0f%%");
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_PREVIEW_ZOOM));

        auto widgetSize = widget_size_with_row_get(2);

        shortcut(manager.chords[SHORTCUT_CENTER_VIEW]);
        if (ImGui::Button(localize.get(LABEL_CENTER_VIEW), widgetSize)) center_view();
        set_item_tooltip_shortcut(localize.get(TOOLTIP_CENTER_VIEW), settings.shortcutCenterView);

        ImGui::SameLine();

        shortcut(manager.chords[SHORTCUT_FIT]);
        if (ImGui::Button(localize.get(LABEL_FIT), widgetSize)) fit_view();
        set_item_tooltip_shortcut(localize.get(TOOLTIP_FIT), settings.shortcutFit);

        auto mousePosInt = ivec2(mousePos);
        ImGui::TextUnformatted(
            std::vformat(localize.get(FORMAT_POSITION_SPACED), std::make_format_args(mousePosInt.x, mousePosInt.y))
                .c_str());
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Background Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::ColorEdit3(localize.get(LABEL_BACKGROUND_COLOR), value_ptr(backgroundColor),
                          ImGuiColorEditFlags_NoInputs);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BACKGROUND_COLOR));
        ImGui::SameLine();
        ImGui::Checkbox(localize.get(LABEL_AXES), &isAxes);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_AXES));
        ImGui::SameLine();
        ImGui::ColorEdit4(localize.get(BASIC_COLOR), value_ptr(axesColor), ImGuiColorEditFlags_NoInputs);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_AXES_COLOR));

        combo_negative_one_indexed(localize.get(LABEL_OVERLAY), &overlayIndex, document.animation.labels);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OVERLAY));

        ImGui::DragFloat(localize.get(BASIC_ALPHA), &overlayTransparency, DRAG_SPEED, 0, 255, "%.0f");
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OVERLAY_ALPHA));
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Helpers Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        auto helpersChildSize = ImVec2(row_widget_width_get(2), ImGui::GetContentRegionAvail().y);

        if (ImGui::BeginChild("##Helpers Child 1", helpersChildSize))
        {
          ImGui::Checkbox(localize.get(LABEL_ROOT_TRANSFORM), &isRootTransform);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROOT_TRANSFORM));
          ImGui::Checkbox(localize.get(LABEL_PIVOTS), &isPivots);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_PIVOTS));
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("##Helpers Child 2", helpersChildSize))
        {
          ImGui::Checkbox(localize.get(LABEL_ALT_ICONS), &isAltIcons);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ALT_ICONS));
          ImGui::Checkbox(localize.get(LABEL_BORDER), &isBorder);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BORDER));
        }
        ImGui::EndChild();
      }
      ImGui::EndChild();

      auto cursorScreenPos = ImGui::GetCursorScreenPos();
      auto min = cursorScreenPos;
      auto max = to_imvec2(to_vec2(min) + size);

      if (manager.isRecordingStart)
      {
        savedSettings = settings;

        if (settings.renderIsRawAnimation)
        {
          settings.previewBackgroundColor = vec4();
          settings.previewIsGrid = false;
          settings.previewIsAxes = false;
          settings.previewIsPivots = false;
          settings.previewIsBorder = false;
          settings.timelineIsOnlyShowLayers = true;
          settings.onionskinIsEnabled = false;

          savedOverlayIndex = overlayIndex;
          savedZoom = zoom;
          savedPan = pan;

          if (auto rect = document.animation_get()->rect(isRootTransform); rect != vec4(-1.0f))
          {
            size_set(vec2(rect.z, rect.w) * settings.renderScale);
            set_to_rect(zoom, pan, rect);
          }

          isSizeTrySet = false;
        }

        manager.isRecordingStart = false;
        manager.isRecording = true;
        renderTempFrames.clear();
        renderTempFrameDurations.clear();
        renderFrameSoundIDs.clear();
        renderCaptureCounterPrev = 0;
        if (settings.renderType == render::PNGS)
        {
          renderTempDirectory = settings.renderPath;
          std::error_code ec;
          std::filesystem::create_directories(renderTempDirectory, ec);
        }
        else
        {
          auto destinationDirectory = render_destination_directory(settings.renderPath, settings.renderType);
          std::error_code ec;
          std::filesystem::create_directories(destinationDirectory, ec);
          renderTempDirectory = render_temp_directory_create(destinationDirectory);
        }
        if (renderTempDirectory.empty())
        {
          auto pathString = path::to_utf8(settings.renderPath);
          toasts.push(std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED), std::make_format_args(pathString)));
          logger.error(std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED, anm2ed::ENGLISH),
                                    std::make_format_args(pathString)));
          manager.isRecording = false;
          manager.progressPopup.close();
          playback.isPlaying = false;
          playback.isFinished = false;
          return;
        }
        playback.isPlaying = true;
        playback.timing_reset();
        playback.time = manager.recordingStart;
      }

      if (isSizeTrySet) size_set(to_vec2(ImGui::GetContentRegionAvail()));

      bind();
      viewport_set();
      clear(manager.isRecording && settings.renderIsRawAnimation ? vec4(0) : vec4(backgroundColor, 1.0f));

      if (isAxes) axes_render(shaderAxes, zoom, pan, axesColor);
      if (isGrid) grid_render(shaderGrid, zoom, pan, gridSize, gridOffset, gridColor);

      auto baseTransform = transform_get(zoom, pan);
      auto frameTime = document.frameTime > -1 && !playback.isPlaying ? document.frameTime : playback.time;

      struct OnionskinSample
      {
        float time{};
        int indexOffset{};
        vec3 colorOffset{};
        float alphaOffset{};
      };

      std::vector<OnionskinSample> onionskinSamples;

      if (animation && settings.onionskinIsEnabled)
      {
        auto add_samples = [&](int count, int direction, vec3 color)
        {
          for (int i = 1; i <= count; ++i)
          {
            float useTime = frameTime + (float)(direction * i);

            float alphaOffset = (1.0f / (count + 1)) * i;
            OnionskinSample sample{};
            sample.time = useTime;
            sample.colorOffset = color;
            sample.alphaOffset = alphaOffset;
            sample.indexOffset = direction * i;
            onionskinSamples.push_back(sample);
          }
        };

        add_samples(settings.onionskinBeforeCount, -1, settings.onionskinBeforeColor);
        add_samples(settings.onionskinAfterCount, 1, settings.onionskinAfterColor);
      }

      auto render = [&](anm2::Animation* animation, float time, vec3 colorOffset = {}, float alphaOffset = {},
                        const std::vector<OnionskinSample>* layeredOnions = nullptr, bool isIndexMode = false)
      {
        auto sample_time_for_item = [&](anm2::Item& item, const OnionskinSample& sample) -> std::optional<float>
        {
          if (!isIndexMode)
          {
            if (sample.time < 0.0f || sample.time > animation->frameNum) return std::nullopt;
            return sample.time;
          }
          if (item.frames.empty()) return std::nullopt;
          int baseIndex = item.frame_index_from_time_get(frameTime);
          if (baseIndex < 0) return std::nullopt;
          int sampleIndex = baseIndex + sample.indexOffset;
          if (sampleIndex < 0 || sampleIndex >= (int)item.frames.size()) return std::nullopt;
          return item.frame_time_from_index_get(sampleIndex);
        };

        auto transform_for_time = [&](anm2::Animation* anim, float t)
        {
          auto sampleTransform = baseTransform;
          if (isRootTransform)
          {
            auto rootFrame = anim->rootAnimation.frame_generate(t, anm2::ROOT);
            sampleTransform *= math::quad_model_parent_get(rootFrame.position, {},
                                                           math::percent_to_unit(rootFrame.scale), rootFrame.rotation);
          }
          return sampleTransform;
        };

        auto transform = transform_for_time(animation, time);

        auto draw_root =
            [&](float sampleTime, const glm::mat4& sampleTransform, vec3 sampleColor, float sampleAlpha, bool isOnion)
        {
          auto rootFrame = animation->rootAnimation.frame_generate(sampleTime, anm2::ROOT);
          if (isOnlyShowLayers || !rootFrame.isVisible || !animation->rootAnimation.isVisible) return;

          auto rootModel = isRootTransform
                               ? math::quad_model_get(TARGET_SIZE, {}, TARGET_SIZE * 0.5f)
                               : math::quad_model_get(TARGET_SIZE, rootFrame.position, TARGET_SIZE * 0.5f,
                                                      math::percent_to_unit(rootFrame.scale), rootFrame.rotation);
          auto rootTransform = sampleTransform * rootModel;

          vec4 color = isOnion ? vec4(sampleColor, sampleAlpha) : color::GREEN;

          auto icon = isAltIcons ? icon::TARGET_ALT : icon::TARGET;
          texture_render(shaderTexture, resources.icons[icon].id, rootTransform, color);
        };

        if (layeredOnions)
          for (auto& sample : *layeredOnions)
            if (auto sampleTime = sample_time_for_item(animation->rootAnimation, sample))
            {
              auto sampleTransform = transform_for_time(animation, *sampleTime);
              draw_root(*sampleTime, sampleTransform, sample.colorOffset, sample.alphaOffset, true);
            }

        draw_root(time, transform, {}, 0.0f, false);

        for (auto& id : animation->layerOrder)
        {
          auto& layerAnimation = animation->layerAnimations[id];
          if (!layerAnimation.isVisible) continue;

          auto& layer = anm2.content.layers.at(id);

          auto spritesheet = anm2.spritesheet_get(layer.spritesheetID);
          if (!spritesheet || !spritesheet->is_valid()) continue;

          auto draw_layer =
              [&](float sampleTime, const glm::mat4& sampleTransform, vec3 sampleColor, float sampleAlpha, bool isOnion)
          {
            if (auto frame = layerAnimation.frame_generate(sampleTime, anm2::LAYER); frame.isVisible)
            {
              auto& texture = spritesheet->texture;

              auto texSize = vec2(texture.size);
              if (texSize.x <= 0.0f || texSize.y <= 0.0f) return;

              auto crop = frame.crop;
              auto size = frame.size;
              auto pivot = frame.pivot;

              if (frame.regionID != -1)
              {
                auto regionIt = spritesheet->regions.find(frame.regionID);
                if (regionIt != spritesheet->regions.end())
                {
                  crop = regionIt->second.crop;
                  size = regionIt->second.size;
                  pivot = regionIt->second.pivot;
                }
              }

              auto layerModel =
                  math::quad_model_get(size, frame.position, pivot, math::percent_to_unit(frame.scale), frame.rotation);
              auto layerTransform = sampleTransform * layerModel;

              auto uvMin = crop / texSize;
              auto uvMax = (crop + size) / texSize;

              vec3 frameColorOffset = frame.colorOffset + colorOffset + sampleColor;
              vec4 frameTint = frame.tint;

              if (isRootTransform)
              {
                auto rootFrame = animation->rootAnimation.frame_generate(sampleTime, anm2::ROOT);
                frameColorOffset += rootFrame.colorOffset;
                frameTint *= rootFrame.tint;
              }

              frameTint.a = std::max(0.0f, frameTint.a - (alphaOffset + sampleAlpha));

              auto vertices = math::uv_vertices_get(uvMin, uvMax);

              texture_render(shaderTexture, texture.id, layerTransform, frameTint, frameColorOffset, vertices.data());

              auto color = isOnion ? vec4(sampleColor, 1.0f - sampleAlpha) : color::RED;

              if (isBorder) rect_render(shaderLine, layerTransform, layerModel, color);

              if (isPivots)
              {
                auto pivotModel = math::quad_model_get(PIVOT_SIZE, frame.position, PIVOT_SIZE * 0.5f,
                                                       math::percent_to_unit(frame.scale), frame.rotation);
                auto pivotTransform = sampleTransform * pivotModel;

                texture_render(shaderTexture, resources.icons[icon::PIVOT].id, pivotTransform, color);
              }
            }
          };

          if (layeredOnions)
            for (auto& sample : *layeredOnions)
              if (auto sampleTime = sample_time_for_item(layerAnimation, sample))
              {
                auto sampleTransform = transform_for_time(animation, *sampleTime);
                draw_layer(*sampleTime, sampleTransform, sample.colorOffset, sample.alphaOffset, true);
              }

          draw_layer(time, transform, {}, 0.0f, false);
        }

        for (auto& [id, nullAnimation] : animation->nullAnimations)
        {
          if (!nullAnimation.isVisible || isOnlyShowLayers) continue;

          auto& isShowRect = anm2.content.nulls[id].isShowRect;

          auto draw_null =
              [&](float sampleTime, const glm::mat4& sampleTransform, vec3 sampleColor, float sampleAlpha, bool isOnion)
          {
            if (auto frame = nullAnimation.frame_generate(sampleTime, anm2::NULL_); frame.isVisible)
            {
              auto icon = isShowRect ? icon::POINT : isAltIcons ? icon::TARGET_ALT : icon::TARGET;

              auto& size = isShowRect ? POINT_SIZE : TARGET_SIZE;
              auto color = isOnion ? vec4(sampleColor, 1.0f - sampleAlpha)
                           : id == reference.itemID && reference.itemType == anm2::NULL_ ? color::RED
                                                                                         : NULL_COLOR;

              auto nullModel = math::quad_model_get(size, frame.position, size * 0.5f,
                                                    math::percent_to_unit(frame.scale), frame.rotation);
              auto nullTransform = sampleTransform * nullModel;

              texture_render(shaderTexture, resources.icons[icon].id, nullTransform, color);

              if (isShowRect)
              {
                auto rectModel =
                    math::quad_model_get(frame.scale, frame.position, frame.scale * 0.5f, vec2(1.0f), frame.rotation);
                auto rectTransform = sampleTransform * rectModel;

                rect_render(shaderLine, rectTransform, rectModel, color);
              }
            }
          };

          if (layeredOnions)
            for (auto& sample : *layeredOnions)
              if (auto sampleTime = sample_time_for_item(nullAnimation, sample))
              {
                auto sampleTransform = transform_for_time(animation, *sampleTime);
                draw_null(*sampleTime, sampleTransform, sample.colorOffset, sample.alphaOffset, true);
              }

          draw_null(time, transform, {}, 0.0f, false);
        }
      };

      if (animation)
      {
        auto layeredOnions = settings.onionskinIsEnabled ? &onionskinSamples : nullptr;

        render(animation, frameTime, {}, 0.0f, layeredOnions, settings.onionskinMode == (int)OnionskinMode::INDEX);

        if (auto overlayAnimation = anm2.animation_get(overlayIndex))
          render(overlayAnimation, frameTime, {}, 1.0f - math::uint8_to_float(overlayTransparency), layeredOnions,
                 settings.onionskinMode == (int)OnionskinMode::INDEX);
      }

      unbind();

      if (manager.isRecording && settings.renderIsRawAnimation)
      {
        sync_checker_pan();
        render_checker_background(ImGui::GetWindowDrawList(), min, max, -size - checkerPan, CHECKER_SIZE);
      }
      ImGui::Image(texture, to_imvec2(size));

      isPreviewHovered = ImGui::IsItemHovered();

      if (animation && animation->triggers.isVisible && !isOnlyShowLayers && !manager.isRecording)
      {
        if (auto trigger = animation->triggers.frame_generate(frameTime, anm2::TRIGGER);
            trigger.isVisible && trigger.eventID > -1)
        {
          auto clipMin = ImGui::GetItemRectMin();
          auto clipMax = ImGui::GetItemRectMax();
          auto drawList = ImGui::GetWindowDrawList();
          auto textPos = to_imvec2(to_vec2(cursorScreenPos) + to_vec2(ImGui::GetStyle().WindowPadding));

          drawList->PushClipRect(clipMin, clipMax);
          ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE_LARGE);
          auto triggerTextColor = isLightTheme ? TRIGGER_TEXT_COLOR_LIGHT : TRIGGER_TEXT_COLOR_DARK;
          if (anm2.content.events.contains(trigger.eventID))
            drawList->AddText(textPos, ImGui::GetColorU32(triggerTextColor),
                              anm2.content.events.at(trigger.eventID).name.c_str());
          ImGui::PopFont();
          drawList->PopClipRect();
        }
      }

      if (isPreviewHovered)
      {
        auto isMouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        auto isMouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        auto isMouseLeftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        auto isMouseMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
        auto isMouseRightDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
        auto isMouseDown = isMouseLeftDown || isMouseMiddleDown || isMouseRightDown;
        auto mouseDelta = to_ivec2(ImGui::GetIO().MouseDelta);
        auto mouseWheel = ImGui::GetIO().MouseWheel;

        auto isLeftJustPressed = ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false);
        auto isRightJustPressed = ImGui::IsKeyPressed(ImGuiKey_RightArrow, false);
        auto isUpJustPressed = ImGui::IsKeyPressed(ImGuiKey_UpArrow, false);
        auto isDownJustPressed = ImGui::IsKeyPressed(ImGuiKey_DownArrow, false);
        auto isLeftPressed = ImGui::IsKeyPressed(ImGuiKey_LeftArrow);
        auto isRightPressed = ImGui::IsKeyPressed(ImGuiKey_RightArrow);
        auto isUpPressed = ImGui::IsKeyPressed(ImGuiKey_UpArrow);
        auto isDownPressed = ImGui::IsKeyPressed(ImGuiKey_DownArrow);
        auto isLeftDown = ImGui::IsKeyDown(ImGuiKey_LeftArrow);
        auto isRightDown = ImGui::IsKeyDown(ImGuiKey_RightArrow);
        auto isUpDown = ImGui::IsKeyDown(ImGuiKey_UpArrow);
        auto isDownDown = ImGui::IsKeyDown(ImGuiKey_DownArrow);
        auto isLeftReleased = ImGui::IsKeyReleased(ImGuiKey_LeftArrow);
        auto isRightReleased = ImGui::IsKeyReleased(ImGuiKey_RightArrow);
        auto isUpReleased = ImGui::IsKeyReleased(ImGuiKey_UpArrow);
        auto isDownReleased = ImGui::IsKeyReleased(ImGuiKey_DownArrow);
        auto isKeyJustPressed = isLeftJustPressed || isRightJustPressed || isUpJustPressed || isDownJustPressed;
        auto isKeyDown = isLeftDown || isRightDown || isUpDown || isDownDown;
        auto isKeyReleased = isLeftReleased || isRightReleased || isUpReleased || isDownReleased;

        auto isZoomIn = shortcut(manager.chords[SHORTCUT_ZOOM_IN], shortcut::GLOBAL);
        auto isZoomOut = shortcut(manager.chords[SHORTCUT_ZOOM_OUT], shortcut::GLOBAL);

        auto isBegin = isMouseClicked || isKeyJustPressed;
        auto isDuring = isMouseDown || isKeyDown;
        auto isEnd = isMouseReleased || isKeyReleased;

        auto isMod = ImGui::IsKeyDown(ImGuiMod_Shift);

        auto frame = document.frame_get();
        auto item = document.item_get();
        auto useTool = tool;
        auto step = isMod ? STEP_FAST : STEP;
        mousePos = position_translate(zoom, pan, to_vec2(ImGui::GetMousePos()) - to_vec2(cursorScreenPos));

        if (isMouseMiddleDown) useTool = tool::PAN;
        if (tool == tool::MOVE && isMouseRightDown) useTool = tool::SCALE;
        if (tool == tool::SCALE && isMouseRightDown) useTool = tool::MOVE;

        auto frame_change_apply = [&](anm2::FrameChange frameChange, anm2::ChangeType changeType = anm2::ADJUST)
        { item->frames_change(frameChange, reference.itemType, changeType, frames); };

        auto& toolInfo = tool::INFO[useTool];
        auto& areaType = toolInfo.areaType;
        bool isAreaAllowed = areaType == tool::ALL || areaType == tool::ANIMATION_PREVIEW;
        bool isFrameRequired =
            !(useTool == tool::PAN || useTool == tool::DRAW || useTool == tool::ERASE || useTool == tool::COLOR_PICKER);
        bool isFrameAvailable = !isFrameRequired || frame;
        auto cursor = (isAreaAllowed && isFrameAvailable) ? toolInfo.cursor : ImGuiMouseCursor_NotAllowed;
        ImGui::SetMouseCursor(cursor);
        ImGui::SetKeyboardFocusHere();
        if (useTool != tool::MOVE) isMoveDragging = false;
        switch (useTool)
        {
          case tool::PAN:
            if (isMouseDown || isMouseMiddleDown) pan += vec2(mouseDelta.x, mouseDelta.y);
            break;
          case tool::MOVE:
            if (!item || frames.empty()) break;
            if (isBegin)
            {
              document.snapshot(localize.get(EDIT_FRAME_POSITION));
              if (isMouseClicked)
              {
                moveOffset = settings.inputIsMoveToolSnapToMouse ? vec2() : mousePos - frame->position;
                isMoveDragging = true;
              }
            }
            if (isMouseDown && isMoveDragging)
              frame_change_apply(
                  {.positionX = (int)(mousePos.x - moveOffset.x), .positionY = (int)(mousePos.y - moveOffset.y)});

            if (isLeftPressed) frame_change_apply({.positionX = step}, anm2::SUBTRACT);
            if (isRightPressed) frame_change_apply({.positionX = step}, anm2::ADD);
            if (isUpPressed) frame_change_apply({.positionY = step}, anm2::SUBTRACT);
            if (isDownPressed) frame_change_apply({.positionY = step}, anm2::ADD);

            if (isMouseReleased) isMoveDragging = false;
            if (isEnd) document.change(Document::FRAMES);
            if (isDuring)
            {
              if (ImGui::BeginTooltip())
              {
                ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_POSITION),
                                                    std::make_format_args(frame->position.x, frame->position.y))
                                           .c_str());
                ImGui::EndTooltip();
              }
            }
            break;
          case tool::SCALE:
            if (!item || frames.empty()) break;
            if (isBegin) document.snapshot(localize.get(EDIT_FRAME_SCALE));
            if (isMouseDown)
            {
              frame->scale += vec2(mouseDelta.x, mouseDelta.y);
              if (isMod) frame->scale = {frame->scale.x, frame->scale.x};
              frame_change_apply({.scaleX = (int)frame->scale.x, .scaleY = (int)frame->scale.y});
            }

            if (isLeftPressed) frame_change_apply({.scaleX = step}, anm2::SUBTRACT);
            if (isRightPressed) frame_change_apply({.scaleX = step}, anm2::ADD);
            if (isUpPressed) frame_change_apply({.scaleY = step}, anm2::SUBTRACT);
            if (isDownPressed) frame_change_apply({.scaleY = step}, anm2::ADD);

            if (isDuring)
            {
              if (ImGui::BeginTooltip())
              {
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_SCALE), std::make_format_args(frame->scale.x, frame->scale.y))
                        .c_str());
                ImGui::EndTooltip();
              }
            }

            if (isEnd) document.change(Document::FRAMES);
            break;
          case tool::ROTATE:
            if (!item || frames.empty()) break;
            if (isBegin) document.snapshot(localize.get(EDIT_FRAME_ROTATION));
            if (isMouseDown) frame_change_apply({.rotation = (int)mouseDelta.x}, anm2::ADD);
            if (isLeftPressed || isDownPressed) frame_change_apply({.rotation = step}, anm2::SUBTRACT);
            if (isUpPressed || isRightPressed) frame_change_apply({.rotation = step}, anm2::ADD);

            if (isDuring)
            {
              if (ImGui::BeginTooltip())
              {
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_ROTATION), std::make_format_args(frame->rotation)).c_str());
                ImGui::EndTooltip();
              }
            }

            if (isEnd) document.change(Document::FRAMES);
            break;
          default:
            break;
        }

        if ((isMouseDown || isKeyDown) && useTool != tool::PAN)
        {
          if (!isAreaAllowed && areaType == tool::SPRITESHEET_EDITOR)
          {
            if (ImGui::BeginTooltip())
            {
              ImGui::TextUnformatted(localize.get(TEXT_TOOL_SPRITESHEET_EDITOR));
              ImGui::EndTooltip();
            }
          }
          else if (isFrameRequired && !isFrameAvailable)
          {
            if (ImGui::BeginTooltip())
            {
              ImGui::TextUnformatted(localize.get(TEXT_SELECT_FRAME));
              ImGui::EndTooltip();
            }
          }
        }

        if (mouseWheel != 0 || isZoomIn || isZoomOut)
        {
          auto previousZoom = zoom;
          zoom_set(zoom, pan, vec2(mousePos), (mouseWheel > 0 || isZoomIn) ? zoomStep : -zoomStep);
          if (zoom != previousZoom) hasPendingZoomPanAdjust = true;
        }
      }
    }

    if (tool == tool::PAN && ImGui::BeginPopupContextWindow("##Animation Preview Context Menu", ImGuiMouseButton_Right))
    {
      if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_UNDO), settings.shortcutUndo.c_str(), false,
                          document.is_able_to_undo()))
        document.undo();

      if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_REDO), settings.shortcutRedo.c_str(), false,
                          document.is_able_to_redo()))
        document.redo();

      ImGui::Separator();

      if (ImGui::MenuItem(localize.get(LABEL_CENTER_VIEW), settings.shortcutCenterView.c_str())) center_view();
      if (ImGui::MenuItem(localize.get(LABEL_FIT), settings.shortcutFit.c_str(), false, animation)) fit_view();

      ImGui::Separator();

      if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_ZOOM_IN), settings.shortcutZoomIn.c_str())) zoom_in();
      if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_ZOOM_OUT), settings.shortcutZoomOut.c_str())) zoom_out();

      ImGui::EndPopup();
    }

    manager.progressPopup.trigger();

    if (ImGui::BeginPopupModal(manager.progressPopup.label(), &manager.progressPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto& start = manager.recordingStart;
      auto& end = manager.recordingEnd;
      auto progress = (playback.time - start) / (end - start);

      ImGui::ProgressBar(progress);

      ImGui::TextUnformatted(localize.get(TEXT_RECORDING_PROGRESS));

      shortcut(manager.chords[SHORTCUT_CANCEL]);
      if (ImGui::Button(localize.get(BASIC_CANCEL), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
      {
        if (settings.renderType == render::PNGS)
        {
          renderTempDirectory.clear();
          renderTempFrames.clear();
          renderTempFrameDurations.clear();
        }
        else
        {
          render_temp_cleanup(renderTempDirectory, renderTempFrames);
          renderTempFrameDurations.clear();
        }

        pan = savedPan;
        zoom = savedZoom;
        settings = savedSettings;
        overlayIndex = savedOverlayIndex;
        isSizeTrySet = true;
        hasPendingZoomPanAdjust = false;
        isCheckerPanInitialized = false;

        if (settings.timelineIsSound) audioStream.capture_end(mixer);

        playback.isPlaying = false;
        playback.isFinished = false;
        manager.isRecording = false;
        manager.progressPopup.close();
      }

      ImGui::EndPopup();
    }

    if (!document.isAnimationPreviewSet)
    {
      center_view();
      zoom = settings.previewStartZoom;
      reset_checker_pan();
      document.isAnimationPreviewSet = true;
    }

    settings.previewStartZoom = zoom;
    ImGui::End();
  }
}
