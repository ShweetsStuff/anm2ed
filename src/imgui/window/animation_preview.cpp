#include "animation_preview.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <system_error>

#include <glm/gtc/type_ptr.hpp>

#include "actions.hpp"
#include "log.hpp"
#include "math.hpp"
#include "path.hpp"
#include "strings.hpp"
#include "toast.hpp"
#include "tool.hpp"
#include "types.hpp"
#include "util/imgui/draw.hpp"
#include "util/imgui/input.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"
#include "util/imgui/tooltip.hpp"

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
    bool render_audio_stream_generate(AudioStream& audioStream, std::map<int, Audio>& sounds,
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
        if (soundID != -1 && sounds.contains(soundID)) sounds.at(soundID).play(false, mixer);

        sampleFrameAccumulator += framesPerStep;
        auto sampleFramesToGenerate = (int)std::floor(sampleFrameAccumulator);
        sampleFramesToGenerate = std::max(sampleFramesToGenerate, 1);
        sampleFrameAccumulator -= (double)sampleFramesToGenerate;

        frameBuffer.resize((std::size_t)sampleFramesToGenerate * (std::size_t)channels);
        if (!MIX_Generate(mixer, frameBuffer.data(), (int)(frameBuffer.size() * sizeof(float))))
        {
          for (auto& [_, sound] : sounds)
            sound.track_detach(mixer);
          MIX_DestroyMixer(mixer);
          audioStream.stream.clear();
          return false;
        }

        audioStream.stream.insert(audioStream.stream.end(), frameBuffer.begin(), frameBuffer.end());
      }

      for (auto& [_, sound] : sounds)
        sound.track_detach(mixer);
      MIX_DestroyMixer(mixer);
      return true;
    }
  }

  AnimationPreview::AnimationPreview() : Canvas(vec2()) {}

  bool AnimationPreview::is_focused_get() const { return isFocused; }

  void AnimationPreview::tick(Manager& manager, Settings& settings, float deltaSeconds)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& playback = document.playback;
    auto& frameTime = document.frameTime;
    auto& end = manager.recordingEnd;
    auto& zoom = document.previewZoom;
    auto& overlayIndex = document.overlayIndex;
    auto& pan = document.previewPan;

    auto stop_all_sounds = [&]()
    {
      for (auto& [_, sound] : document.sounds)
        sound.stop(mixer);
    };

    auto trigger_sound_id_get = [&](Element* animation, float time)
    {
      if (!animation) return -1;
      auto triggers = animation_item_get(*animation, ItemType::TRIGGER);
      if (!triggers || !triggers->isVisible) return -1;

      auto trigger = frame_generate(*triggers, time);
      if (!trigger.isVisible || trigger.soundIds.empty()) return -1;

      auto soundIndex =
          trigger.soundIds.size() > 1 ? (size_t)math::random_in_range(0.0f, (float)trigger.soundIds.size()) : (size_t)0;
      soundIndex = std::min(soundIndex, trigger.soundIds.size() - 1);
      auto soundID = trigger.soundIds[soundIndex];
      return document.sound_get(soundID) ? soundID : -1;
    };

    if (manager.isRecording)
    {
      auto& ffmpegPath = settings.renderFFmpegPath;
      auto& path = settings.renderPath;
      auto pathString = path::to_utf8(path);
      auto& type = settings.renderType;
      auto renderFrameRate = std::max(settings.playbackTickRate, 1);

      if (playback.time >= (float)end + 1.0f || playback.isFinished)
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
            if (!render_audio_stream_generate(audioStream, document.sounds, renderFrameSoundIDs, renderFrameRate))
            {
              toasts.push(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED));
              logger.error("Failed to generate deterministic render audio stream; exporting without audio.");
              audioStream.stream.clear();
            }
          }
          else
            audioStream.stream.clear();

          if (animation_render(ffmpegPath, path, renderTempFrames, renderTempFrameDurations, audioStream,
                               (render::Type)type, renderFrameRate))
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
        if (settings.timelineIsSound && !document.sounds.empty())
        {
          auto soundTime = (int)std::floor(playback.time);
          auto animation = anm2.element_get(ElementType::ANIMATION, document.reference.animationIndex);
          if (soundTime != renderFrameSoundTimePrev && animation &&
              (!settings.timelineIsOnlyShowLayers || manager.isRecording))
            frameSoundID = trigger_sound_id_get(animation, playback.time);
          renderFrameSoundTimePrev = soundTime;
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
          auto fallbackDuration = 1.0 / (double)renderFrameRate;
          renderTempFrameDurations.push_back(fallbackDuration);
        }
        else
        {
          toasts.push(
              std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED), std::make_format_args(pathString)));
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
      auto animation = anm2.element_get(ElementType::ANIMATION, document.reference.animationIndex);
      auto& isSound = settings.timelineIsSound;
      auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;

      if (!animation)
      {
        playback.isPlaying = false;
        playback.isFinished = false;
        playback.timing_reset();
      }
      else
      {
        if (!manager.isRecording && !document.sounds.empty() && isSound)
        {
          if (!isOnlyShowLayers || manager.isRecording)
            if (auto soundID = trigger_sound_id_get(animation, playback.time); soundID != -1)
              if (auto sound = document.sound_get(soundID)) sound->play(false, mixer);
        }

        auto info = element_first_get(anm2.root, ElementType::INFO);
        auto fps = std::max(info ? info->fps : 30, 1);
        playback.tick(fps, animation->frameNum, (animation->isLoop || settings.playbackIsLoop) && !manager.isRecording,
                      deltaSeconds);

        frameTime = playback.time;
      }
    }

    if (wasPlaybackPlaying && !playback.isPlaying) stop_all_sounds();
    wasPlaybackPlaying = playback.isPlaying;
  }

  void AnimationPreview::update(Manager& manager, Settings& settings, Resources& resources)
  {
    isFocused = false;

    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& playback = document.playback;
    auto& reference = document.reference;
    auto animation = anm2.element_get(ElementType::ANIMATION, reference.animationIndex);
    auto& pan = document.previewPan;
    auto& zoom = document.previewZoom;
    auto& backgroundColor = settings.previewBackgroundColor;
    auto& isTransparent = settings.animationPreviewTransparent;
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
      if (animation) set_to_rect(zoom, pan, anm2.animation_rect(*animation, isRootTransform));
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
      isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
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

        auto readoutSize = ImVec2(row_widget_width_get(2), ImGui::GetTextLineHeightWithSpacing());
        auto mousePosInt = ivec2(mousePos);
        auto positionText =
            std::vformat(localize.get(FORMAT_POSITION_SPACED), std::make_format_args(mousePosInt.x, mousePosInt.y));
        if (ImGui::BeginChild("##Position Readout", readoutSize, false, ImGuiWindowFlags_NoScrollbar))
        {
          ImGui::TextUnformatted(positionText.c_str());
          if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", positionText.c_str());
        }
        ImGui::EndChild();

        ImGui::SameLine();

        auto sizeInt = ivec2(size);
        auto sizeText = std::vformat(localize.get(FORMAT_SIZE_SPACED), std::make_format_args(sizeInt.x, sizeInt.y));
        if (ImGui::BeginChild("##Size Readout", readoutSize, false, ImGuiWindowFlags_NoScrollbar))
        {
          ImGui::TextUnformatted(sizeText.c_str());
          if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", sizeText.c_str());
        }
        ImGui::EndChild();
      }
      ImGui::EndChild();

      ImGui::SameLine();

      if (ImGui::BeginChild("##Background Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::BeginDisabled(isTransparent);
        {
          ImGui::ColorEdit3(localize.get(LABEL_BACKGROUND_COLOR), value_ptr(backgroundColor),
                            ImGuiColorEditFlags_NoInputs);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BACKGROUND_COLOR));
        }
        ImGui::EndDisabled();
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
          ImGui::Checkbox(localize.get(LABEL_TRANSPARENT), &isTransparent);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_PREVIEW_TRANSPARENT));
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
          savedOverlayIndex = overlayIndex;
          savedZoom = zoom;
          savedPan = pan;

          if (animation)
            if (auto rect = anm2.animation_rect(*animation, isRootTransform); rect != vec4(-1.0f))
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
        renderFrameSoundTimePrev = -1;
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
          toasts.push(
              std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED), std::make_format_args(pathString)));
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

      if (isSizeTrySet)
      {
        auto nextSize = to_vec2(ImGui::GetContentRegionAvail());
        bool isCanvasResized = ivec2(nextSize) != ivec2(size);
        size_set(nextSize);
        if (isCanvasResized && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
          auto resizeSizeInt = ivec2(size);
          auto resizeSizeText =
              std::vformat(localize.get(FORMAT_SIZE_SPACED), std::make_format_args(resizeSizeInt.x, resizeSizeInt.y));
          ImGui::SetTooltip("%s", resizeSizeText.c_str());
        }
      }

      bind();
      viewport_set();
      clear(isTransparent ? vec4(0) : vec4(backgroundColor, 1.0f));

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

      auto referenceItemType = static_cast<ItemType>(reference.itemType);

      auto render = [&](Element* animation, float time, vec3 colorOffset = {}, float alphaOffset = {},
                        const std::vector<OnionskinSample>* layeredOnions = nullptr, bool isIndexMode = false)
      {
        auto sample_time_for_item = [&](Element& item, const OnionskinSample& sample) -> std::optional<float>
        {
          if (!isIndexMode)
          {
            if (sample.time < 0.0f || sample.time > animation->frameNum) return std::nullopt;
            return sample.time;
          }
          if (item.children.empty()) return std::nullopt;
          int baseIndex = frame_index_from_time_get(item, frameTime);
          if (baseIndex < 0) return std::nullopt;
          int sampleIndex = baseIndex + sample.indexOffset;
          if (!track_frame_get(item, sampleIndex)) return std::nullopt;
          return frame_time_from_index_get(item, sampleIndex);
        };

        auto root = animation_item_get(*animation, ItemType::ROOT);

        auto transform_for_time = [&](float t)
        {
          auto sampleTransform = baseTransform;
          if (isRootTransform && root)
          {
            auto rootFrame = frame_generate(*root, t);
            sampleTransform *= math::quad_model_parent_get(rootFrame.position, {},
                                                           math::percent_to_unit(rootFrame.scale), rootFrame.rotation);
          }
          return sampleTransform;
        };

        auto transform = transform_for_time(time);

        auto is_track_group_visible = [](const Element& container, const Element& track)
        {
          if (track.groupId == -1) return true;
          for (const auto& child : container.children)
            if (child.type == ElementType::GROUP && child.id == track.groupId) return child.isVisible;
          return true;
        };

        auto draw_root =
            [&](float sampleTime, const glm::mat4& sampleTransform, vec3 sampleColor, float sampleAlpha, bool isOnion)
        {
          if (!root) return;
          auto rootFrame = frame_generate(*root, sampleTime);
          if (isOnlyShowLayers || !rootFrame.isVisible || !root->isVisible) return;

          auto rootModel = isRootTransform
                               ? math::quad_model_get(TARGET_SIZE, {}, TARGET_SIZE * 0.5f)
                               : math::quad_model_get(TARGET_SIZE, rootFrame.position, TARGET_SIZE * 0.5f,
                                                      math::percent_to_unit(rootFrame.scale), rootFrame.rotation);
          auto rootTransform = sampleTransform * rootModel;

          vec4 color = isOnion ? vec4(sampleColor, sampleAlpha) : color::GREEN;

          auto icon = isAltIcons ? icon::TARGET_ALT : icon::TARGET;
          texture_render(shaderTexture, resources.icons[icon].id, rootTransform, color);
        };

        if (layeredOnions && root)
          for (auto& sample : *layeredOnions)
            if (auto sampleTime = sample_time_for_item(*root, sample))
            {
              auto sampleTransform = transform_for_time(*sampleTime);
              draw_root(*sampleTime, sampleTransform, sample.colorOffset, sample.alphaOffset, true);
            }

        draw_root(time, transform, {}, 0.0f, false);

        if (auto layerAnimations = element_child_first_get(*animation, ElementType::LAYER_ANIMATIONS))
        {
          auto layer_animation_draw = [&](auto&& self, Element& layerAnimation, bool isParentVisible = true) -> void
          {
            if (layerAnimation.type == ElementType::GROUP)
            {
              for (auto& child : layerAnimation.children)
                self(self, child, isParentVisible && layerAnimation.isVisible);
              return;
            }
            if (layerAnimation.type != ElementType::LAYER_ANIMATION || !isParentVisible ||
                !layerAnimation.isVisible || !is_track_group_visible(*layerAnimations, layerAnimation))
              return;

            auto id = layerAnimation.layerId;
            auto layer = anm2.element_get(ElementType::LAYER_ELEMENT, id);
            if (!layer) return;

            auto spritesheet = anm2.element_get(ElementType::SPRITESHEET, layer->spritesheetId);
            auto textureInfo = document.texture_get(layer->spritesheetId);
            if (!spritesheet || !textureInfo || !textureInfo->is_valid()) return;

            auto draw_layer = [&](float sampleTime, const glm::mat4& sampleTransform, vec3 sampleColor,
                                  float sampleAlpha, bool isOnion)
            {
              auto frame = frame_generate(layerAnimation, sampleTime);
              if (!frame.isVisible) return;

              auto& texture = *textureInfo;

              auto texSize = vec2(texture.size);
              if (texSize.x <= 0.0f || texSize.y <= 0.0f) return;

              frame = anm2.frame_effective(id, frame);
              auto crop = frame.crop;
              auto size = frame.size;
              auto pivot = frame.pivot;

              auto layerModel =
                  math::quad_model_get(size, frame.position, pivot, math::percent_to_unit(frame.scale), frame.rotation);
              auto layerTransform = sampleTransform * layerModel;

              auto uvMin = crop / texSize;
              auto uvMax = (crop + size) / texSize;

              vec3 frameColorOffset = frame.colorOffset + colorOffset + sampleColor;
              vec4 frameTint = frame.tint;

              if (isRootTransform && root)
              {
                auto rootFrame = frame_generate(*root, sampleTime);
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
            };

            if (layeredOnions)
              for (auto& sample : *layeredOnions)
                if (auto sampleTime = sample_time_for_item(layerAnimation, sample))
                {
                  auto sampleTransform = transform_for_time(*sampleTime);
                  draw_layer(*sampleTime, sampleTransform, sample.colorOffset, sample.alphaOffset, true);
                }

            draw_layer(time, transform, {}, 0.0f, false);
          };
          for (auto& layerAnimation : layerAnimations->children)
            layer_animation_draw(layer_animation_draw, layerAnimation);
        }

        if (auto nullAnimations = element_child_first_get(*animation, ElementType::NULL_ANIMATIONS))
        {
          auto null_animation_draw = [&](auto&& self, Element& nullAnimation, bool isParentVisible = true) -> void
          {
            if (nullAnimation.type == ElementType::GROUP)
            {
              for (auto& child : nullAnimation.children)
                self(self, child, isParentVisible && nullAnimation.isVisible);
              return;
            }
            if (nullAnimation.type != ElementType::NULL_ANIMATION || !isParentVisible || !nullAnimation.isVisible ||
                !is_track_group_visible(*nullAnimations, nullAnimation) || isOnlyShowLayers)
              return;

            auto id = nullAnimation.nullId;
            auto nulls = anm2.element_get(ElementType::NULLS);
            auto nullInfo = nulls ? element_child_id_get(*nulls, ElementType::NULL_ELEMENT, id) : nullptr;
            if (!nullInfo) return;
            auto isShowRect = nullInfo->isShowRect;

            auto draw_null = [&](float sampleTime, const glm::mat4& sampleTransform, vec3 sampleColor,
                                 float sampleAlpha, bool isOnion)
            {
              auto frame = frame_generate(nullAnimation, sampleTime);
              if (!frame.isVisible) return;

              auto icon = isShowRect ? icon::POINT : isAltIcons ? icon::TARGET_ALT : icon::TARGET;

              auto& size = isShowRect ? POINT_SIZE : TARGET_SIZE;
              auto color = isOnion ? vec4(sampleColor, 1.0f - sampleAlpha)
                           : id == reference.itemID && referenceItemType == ItemType::NULL_ ? color::RED
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
            };

            if (layeredOnions)
              for (auto& sample : *layeredOnions)
                if (auto sampleTime = sample_time_for_item(nullAnimation, sample))
                {
                  auto sampleTransform = transform_for_time(*sampleTime);
                  draw_null(*sampleTime, sampleTransform, sample.colorOffset, sample.alphaOffset, true);
                }

            draw_null(time, transform, {}, 0.0f, false);
          };
          for (auto& nullAnimation : nullAnimations->children)
            null_animation_draw(null_animation_draw, nullAnimation);
        }
      };

      if (animation)
      {
        auto layeredOnions = settings.onionskinIsEnabled ? &onionskinSamples : nullptr;

        render(animation, frameTime, {}, 0.0f, layeredOnions, settings.onionskinMode == (int)OnionskinMode::INDEX);

        if (auto overlayAnimation = anm2.element_get(ElementType::ANIMATION, overlayIndex))
          render(overlayAnimation, frameTime, {}, 1.0f - math::uint8_to_float(overlayTransparency), layeredOnions,
                 settings.onionskinMode == (int)OnionskinMode::INDEX);
      }

      unbind();

      if (isTransparent)
      {
        sync_checker_pan();
        render_checker_background(ImGui::GetWindowDrawList(), min, max, -size - checkerPan, CHECKER_SIZE);
      }
      ImGui::Image(texture, to_imvec2(size));

      isPreviewHovered = ImGui::IsItemHovered();

      auto triggers = animation ? animation_item_get(*animation, ItemType::TRIGGER) : nullptr;
      if (animation && triggers && triggers->isVisible && !isOnlyShowLayers && !manager.isRecording)
      {
        if (auto trigger = frame_generate(*triggers, frameTime); trigger.isVisible && trigger.eventId > -1)
        {
          auto clipMin = ImGui::GetItemRectMin();
          auto clipMax = ImGui::GetItemRectMax();
          auto drawList = ImGui::GetWindowDrawList();
          auto textPos = to_imvec2(to_vec2(cursorScreenPos) + to_vec2(ImGui::GetStyle().WindowPadding));

          drawList->PushClipRect(clipMin, clipMax);
          ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE_LARGE);
          auto triggerTextColor = isLightTheme ? TRIGGER_TEXT_COLOR_LIGHT : TRIGGER_TEXT_COLOR_DARK;
          auto events = anm2.element_get(ElementType::EVENTS);
          auto event = events ? element_child_id_get(*events, ElementType::EVENT_ELEMENT, trigger.eventId) : nullptr;
          if (event) drawList->AddText(textPos, ImGui::GetColorU32(triggerTextColor), event->name.c_str());
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
        auto isMouseRightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
        auto isMouseRightReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Right);
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

        auto isZoomIn = isFocused && shortcut(manager.chords[SHORTCUT_ZOOM_IN], shortcut::GLOBAL);
        auto isZoomOut = isFocused && shortcut(manager.chords[SHORTCUT_ZOOM_OUT], shortcut::GLOBAL);

        auto isMod = ImGui::IsKeyDown(ImGuiMod_Shift);
        auto useTool = tool;
        auto step = (float)(isMod ? STEP_FAST : STEP);
        mousePos = position_translate(zoom, pan, to_vec2(ImGui::GetMousePos()) - to_vec2(cursorScreenPos));
        auto selected_frame_references_get = [&]()
        {
          std::set<Reference> result = document.frames.references;
          if (result.empty())
            for (auto frameIndex : frames)
              result.insert({reference.animationIndex, reference.itemType, reference.itemID, frameIndex});
          if (result.empty() && reference.itemType != NONE && reference.frameIndex != -1) result.insert(reference);
          std::erase_if(result,
                        [](const Reference& frameReference)
                        { return frameReference.itemType == NONE || frameReference.itemType == TRIGGER; });
          return result;
        };
        auto selectedFrameReferences = selected_frame_references_get();
        auto editReference = reference;
        auto editItemType = referenceItemType;
        if ((referenceItemType == ItemType::TRIGGER || !selectedFrameReferences.contains(reference)) &&
            !selectedFrameReferences.empty())
        {
          editReference = *selectedFrameReferences.begin();
          editItemType = static_cast<ItemType>(editReference.itemType);
        }

        auto frame =
            anm2.element_get(editReference.animationIndex, editItemType, editReference.frameIndex, editReference.itemID);
        auto item = anm2.element_get(editReference.animationIndex, editItemType, editReference.itemID);
        auto nulls = anm2.element_get(ElementType::NULLS);
        auto selectedNull = editItemType == ItemType::NULL_ && nulls
                                ? element_child_id_get(*nulls, ElementType::NULL_ELEMENT, editReference.itemID)
                                : nullptr;
        bool isSelectedNullRect = selectedNull && selectedNull->isShowRect;
        auto null_rect_top_left = [](const Element& frame) { return frame.position - (frame.scale * 0.5f); };

        if (isMouseMiddleDown) useTool = tool::PAN;
        if (tool == tool::MOVE && isMouseRightDown) useTool = tool::SCALE;
        if (tool == tool::SCALE && isMouseRightDown) useTool = tool::MOVE;

        bool isToolMouseClicked = isMouseClicked;
        bool isToolMouseReleased = isMouseReleased;
        bool isToolMouseDown = isMouseLeftDown;

        if ((tool == tool::MOVE && useTool == tool::SCALE) || (tool == tool::SCALE && useTool == tool::MOVE))
        {
          isToolMouseClicked = isMouseRightClicked;
          isToolMouseReleased = isMouseRightReleased;
          isToolMouseDown = isMouseRightDown;
        }

        auto isToolBegin = isToolMouseClicked || isKeyJustPressed;
        auto isToolDuring = isToolMouseDown || isKeyDown;
        auto isToolEnd = isToolMouseReleased || isKeyReleased;

        auto frame_snapshot = [&](auto message)
        {
          manager.command_push({manager.selected,
                                [message](Manager&, Document& document) { document.snapshot(localize.get(message)); }});
        };
        auto frame_change_apply = [&](FrameChange frameChange, ChangeType changeType = ChangeType::ADJUST)
        {
          auto queuedFrameReferences = selectedFrameReferences;
          manager.command_push({manager.selected,
                                [=](Manager&, Document& document)
                                {
                                  std::map<Reference, std::set<int>> groupedFrames{};
                                  for (auto frameReference : queuedFrameReferences)
                                    groupedFrames[{frameReference.animationIndex, frameReference.itemType,
                                                   frameReference.itemID, -1}]
                                        .insert(frameReference.frameIndex);

                                  for (auto& [itemReference, itemFrames] : groupedFrames)
                                  {
                                    auto itemType = static_cast<ItemType>(itemReference.itemType);
                                    auto item = document.anm2.element_get(itemReference.animationIndex, itemType,
                                                                          itemReference.itemID);
                                    if (!item) continue;
                                    frames_change(*item, frameChange, itemType, changeType, itemFrames);
                                  }
                                }});
        };
        auto frame_position_apply = [&](vec2 position)
        {
          if (!frame) return;
          frame_change_apply({.positionX = position.x - frame->position.x, .positionY = position.y - frame->position.y},
                             ChangeType::ADD);
        };
        auto frame_scale_apply = [&](vec2 scale)
        {
          if (!frame) return;
          frame_change_apply({.scaleX = scale.x - frame->scale.x, .scaleY = scale.y - frame->scale.y}, ChangeType::ADD);
        };
        auto frames_changed = [&]()
        {
          manager.command_push(
              {manager.selected, [](Manager&, Document& document) { document.anm2_change(Document::FRAMES); }});
        };
        auto null_rect_change = [&](vec2 topLeft, vec2 rectSize)
        {
          topLeft = vec2(ivec2(topLeft));
          rectSize = vec2(ivec2(rectSize));
          frame_change_apply({.positionX = topLeft.x + rectSize.x * 0.5f - frame->position.x,
                              .positionY = topLeft.y + rectSize.y * 0.5f - frame->position.y,
                              .scaleX = rectSize.x - frame->scale.x,
                              .scaleY = rectSize.y - frame->scale.y},
                             ChangeType::ADD);
        };

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
            if (!item || !frame || selectedFrameReferences.empty()) break;
            if (isToolBegin)
            {
              frame_snapshot(EDIT_FRAME_POSITION);
              if (isToolMouseClicked)
              {
                auto origin = isSelectedNullRect ? null_rect_top_left(*frame) : frame->position;
                moveOffset = settings.inputIsMoveToolSnapToMouse ? vec2() : mousePos - origin;
                isMoveDragging = true;
              }
            }
            if (isToolMouseDown && isMoveDragging)
            {
              auto position = mousePos - moveOffset;
              if (isSelectedNullRect)
                frame_position_apply(vec2((float)(int)(position.x + frame->scale.x * 0.5f),
                                          (float)(int)(position.y + frame->scale.y * 0.5f)));
              else
                frame_position_apply(vec2((float)(int)position.x, (float)(int)position.y));
            }

            if (isLeftPressed) frame_change_apply({.positionX = step}, ChangeType::SUBTRACT);
            if (isRightPressed) frame_change_apply({.positionX = step}, ChangeType::ADD);
            if (isUpPressed) frame_change_apply({.positionY = step}, ChangeType::SUBTRACT);
            if (isDownPressed) frame_change_apply({.positionY = step}, ChangeType::ADD);

            if (isToolMouseReleased) isMoveDragging = false;
            if (isToolEnd) frames_changed();
            if (isToolDuring)
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
            if (!item || !frame || selectedFrameReferences.empty()) break;
            if (isToolBegin)
            {
              frame_snapshot(EDIT_FRAME_SCALE);
              if (isToolMouseClicked && isSelectedNullRect) nullRectScaleAnchor = null_rect_top_left(*frame);
            }
            if (isToolMouseDown)
            {
              if (isSelectedNullRect)
              {
                auto size = mousePos - nullRectScaleAnchor;
                if (isMod)
                {
                  auto squareSize = std::max(std::abs(size.x), std::abs(size.y));
                  size = {std::copysign(squareSize, size.x), std::copysign(squareSize, size.y)};
                }

                auto minPoint = glm::min(nullRectScaleAnchor, nullRectScaleAnchor + size);
                auto maxPoint = glm::max(nullRectScaleAnchor, nullRectScaleAnchor + size);
                minPoint = vec2(ivec2(minPoint));
                maxPoint = vec2(ivec2(maxPoint));
                null_rect_change(minPoint, maxPoint - minPoint);
              }
              else
              {
                auto scale = frame->scale + vec2(mouseDelta.x, mouseDelta.y);
                if (isMod) scale = {scale.x, scale.x};
                frame_scale_apply(scale);
              }
            }

            if (isSelectedNullRect)
            {
              if (isLeftPressed) frame_change_apply({.positionX = step * 0.5f, .scaleX = step}, ChangeType::SUBTRACT);
              if (isRightPressed) frame_change_apply({.positionX = step * 0.5f, .scaleX = step}, ChangeType::ADD);
              if (isUpPressed) frame_change_apply({.positionY = step * 0.5f, .scaleY = step}, ChangeType::SUBTRACT);
              if (isDownPressed) frame_change_apply({.positionY = step * 0.5f, .scaleY = step}, ChangeType::ADD);
            }
            else
            {
              if (isLeftPressed) frame_change_apply({.scaleX = step}, ChangeType::SUBTRACT);
              if (isRightPressed) frame_change_apply({.scaleX = step}, ChangeType::ADD);
              if (isUpPressed) frame_change_apply({.scaleY = step}, ChangeType::SUBTRACT);
              if (isDownPressed) frame_change_apply({.scaleY = step}, ChangeType::ADD);
            }

            if (isToolDuring)
            {
              if (ImGui::BeginTooltip())
              {
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_SCALE), std::make_format_args(frame->scale.x, frame->scale.y))
                        .c_str());
                ImGui::EndTooltip();
              }
            }

            if (isToolEnd)
            {
              if (isSelectedNullRect)
              {
                auto topLeft = null_rect_top_left(*frame);
                auto minPoint = glm::min(topLeft, topLeft + frame->scale);
                auto maxPoint = glm::max(topLeft, topLeft + frame->scale);
                minPoint = vec2(ivec2(minPoint));
                maxPoint = vec2(ivec2(maxPoint));
                null_rect_change(minPoint, maxPoint - minPoint);
              }
              frames_changed();
            }
            break;
          case tool::ROTATE:
            if (!item || !frame || selectedFrameReferences.empty()) break;
            if (isToolBegin) frame_snapshot(EDIT_FRAME_ROTATION);
            if (isToolMouseDown) frame_change_apply({.rotation = (float)(int)mouseDelta.x}, ChangeType::ADD);
            if (isLeftPressed || isDownPressed) frame_change_apply({.rotation = step}, ChangeType::SUBTRACT);
            if (isUpPressed || isRightPressed) frame_change_apply({.rotation = step}, ChangeType::ADD);

            if (isToolDuring)
            {
              if (ImGui::BeginTooltip())
              {
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_ROTATION), std::make_format_args(frame->rotation)).c_str());
                ImGui::EndTooltip();
              }
            }

            if (isToolEnd) frames_changed();
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

    if (tool == tool::PAN)
    {
      Actions actions{};
      actions_undo_redo_add(actions, manager, document);
      actions.separator();
      actions.add(ACTION_CENTER_VIEW, []() { return true; }, center_view);
      actions.add(ACTION_FIT_VIEW, [&]() { return animation; }, fit_view);
      actions.separator();
      actions.add(ACTION_ZOOM_IN, []() { return true; }, zoom_in);
      actions.add(ACTION_ZOOM_OUT, []() { return true; }, zoom_out);
      actions_context_window_draw("##Animation Preview Context Menu", actions, settings);
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
