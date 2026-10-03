#include "animation_preview.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <glm/gtc/type_ptr.hpp>

#include "actions.hpp"
#include "log.hpp"
#include "math.hpp"
#include "model/draw.hpp"
#include "model/frames.hpp"
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
using namespace anm2ed::resource::image;
using namespace glm;

namespace anm2ed::imgui
{
  constexpr auto NULL_COLOR = vec4(0.0f, 0.0f, 1.0f, 0.90f);
  constexpr auto SELECTED_LAYER_BORDER_COLOR = vec4(1.0f, 0.1059f, 0.5882f, 1.0f);
  constexpr auto TARGET_SIZE = vec2(32, 32);
  constexpr auto POINT_SIZE = vec2(4, 4);
  constexpr auto TRIGGER_TEXT_COLOR_DARK = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
  constexpr auto TRIGGER_TEXT_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
  struct OverlayAnimationOption
  {
    uint64_t documentId{};
    int animationIndex{-1};
    std::string label{};
  };

  std::vector<OverlayAnimationOption> overlay_animation_options_get(Manager& manager)
  {
    std::vector<OverlayAnimationOption> options{{.label = std::string(localize.get(BASIC_NONE))}};

    for (auto& document : manager.documents)
    {
      auto documentLabel = path::to_utf8(document.filename_get());
      for (auto [animationIndex, animation] : document.model.animations_get())
      {
        auto label = std::format("{} ({})", animation->name, documentLabel);
        options.push_back({.documentId = document.tabId, .animationIndex = animationIndex, .label = label});
      }
    }

    return options;
  }

  std::vector<const char*> overlay_animation_option_labels_get(std::vector<OverlayAnimationOption>& options)
  {
    std::vector<const char*> labels{};
    labels.reserve(options.size());
    for (auto& option : options)
      labels.push_back(option.label.c_str());
    return labels;
  }

  int overlay_animation_option_index_get(const Document& document, const std::vector<OverlayAnimationOption>& options)
  {
    if (document.overlayIndex == -1) return 0;
    auto documentId = document.overlayDocumentId ? document.overlayDocumentId : document.tabId;
    for (int i = 1; i < (int)options.size(); ++i)
      if (options[i].documentId == documentId && options[i].animationIndex == document.overlayIndex) return i;
    return -1;
  }

  Document* overlay_animation_document_get(Manager& manager, const Document& document)
  {
    if (document.overlayIndex == -1) return nullptr;
    auto documentId = document.overlayDocumentId ? document.overlayDocumentId : document.tabId;
    for (auto& candidate : manager.documents)
      if (candidate.tabId == documentId) return &candidate;
    return nullptr;
  }

  std::optional<vec4> animation_rects_merge(std::optional<vec4> rect, vec4 next)
  {
    if (next == vec4(-1.0f)) return rect;
    if (!rect) return next;

    auto minPoint = glm::min(vec2(rect->x, rect->y), vec2(next.x, next.y));
    auto maxPoint = glm::max(vec2(rect->x + rect->z, rect->y + rect->w), vec2(next.x + next.z, next.y + next.w));
    return vec4(minPoint, maxPoint - minPoint);
  }

  std::optional<vec4> animation_render_rect_get(Manager& manager, Document& document, const model::Animation* animation,
                                                bool isRootTransform)
  {
    std::optional<vec4> rect{};

    if (animation)
      rect = animation_rects_merge(rect, model::animation_rect(document.model, *animation, isRootTransform));

    if (auto overlayDocument = overlay_animation_document_get(manager, document))
      if (auto overlayAnimation = overlayDocument->model.animation_get(document.overlayIndex))
        rect = animation_rects_merge(rect,
                                     model::animation_rect(overlayDocument->model, *overlayAnimation, isRootTransform));

    return rect;
  }

  std::filesystem::path render_destination_directory(const std::filesystem::path& path, int type)
  {
    if (type == render::PNGS) return path;
    auto directory = path.parent_path();
    if (directory.empty()) directory = std::filesystem::current_path();
    return directory;
  }

  std::filesystem::path render_frame_filename(const std::filesystem::path& format, int index, int type)
  {
    if (type != render::PNGS) return path::from_utf8(std::format("frame_{:06}.png", index));

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

  int trigger_sound_id_get(Document& document, const model::Animation* animation, float time,
                           int deterministicIndex = -1)
  {
    if (!animation || !animation->triggers.isVisible) return -1;

    auto trigger = model::frame_generate(animation->triggers, time);
    if (!trigger.isVisible || trigger.soundIds.empty()) return -1;

    size_t soundIndex{};
    if (trigger.soundIds.size() > 1)
      soundIndex = deterministicIndex >= 0 ? (size_t)deterministicIndex % trigger.soundIds.size()
                                           : (size_t)math::random_in_range(0.0f, (float)trigger.soundIds.size());
    soundIndex = std::min(soundIndex, trigger.soundIds.size() - 1);

    auto soundID = trigger.soundIds[soundIndex];
    return document.sound_get(soundID) ? soundID : -1;
  }

  void sounds_detach(const Document& document, MIX_Mixer* mixer)
  {
    for (auto& [id, _] : document.sounds)
      if (auto sound = document.sound_get(id)) audio::track_detach(*sound, mixer);
  }

  bool render_audio_stream_generate(AudioStream& audioStream, const Document& document,
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
      if (auto sound = document.sound_get(soundID)) audio::play(*sound, false, mixer);

      sampleFrameAccumulator += framesPerStep;
      auto sampleFramesToGenerate = (int)std::floor(sampleFrameAccumulator);
      sampleFramesToGenerate = std::max(sampleFramesToGenerate, 1);
      sampleFrameAccumulator -= (double)sampleFramesToGenerate;

      frameBuffer.resize((std::size_t)sampleFramesToGenerate * (std::size_t)channels);
      if (!MIX_Generate(mixer, frameBuffer.data(), (int)(frameBuffer.size() * sizeof(float))))
      {
        sounds_detach(document, mixer);
        MIX_DestroyMixer(mixer);
        audioStream.stream.clear();
        return false;
      }

      audioStream.stream.insert(audioStream.stream.end(), frameBuffer.begin(), frameBuffer.end());
    }

    sounds_detach(document, mixer);
    MIX_DestroyMixer(mixer);
    return true;
  }

  void AnimationPreview::tick(Manager& manager, Settings& settings, float deltaSeconds)
  {
    auto& document = *manager.get();
    auto& model = document.model;
    auto& playback = document.playback;
    auto& frameTime = document.frameTime;
    auto& zoom = document.previewZoom;
    auto& overlayIndex = document.overlayIndex;
    auto& overlayDocumentId = document.overlayDocumentId;
    auto& pan = document.previewPan;

    auto stop_all_sounds = [&]()
    {
      for (auto& [id, _] : document.sounds)
        if (auto sound = document.sound_get(id)) audio::stop(*sound, mixer);
    };

    if (manager.isRecording)
    {
      auto& ffmpegPath = settings.renderFFmpegPath;
      auto& path = settings.renderPath;
      auto pathString = path::to_utf8(path);
      auto& type = settings.renderType;
      auto isRenderPreviewOverridden = settings.renderIsUseAnimationBounds || settings.renderIsUseIsolatedAnimation;
      auto render_preview_restore = [&]()
      {
        if (!isRenderPreviewOverridden) return;

        settings = savedSettings;
        pan = savedPan;
        zoom = savedZoom;
        overlayIndex = savedOverlayIndex;
        overlayDocumentId = savedOverlayDocumentId;
        isSizeTrySet = true;
        hasPendingZoomPanAdjust = false;
        isCheckerPanInitialized = false;
      };

      if (renderFrameIndex >= renderFrameCount)
      {
        if (type == render::PNGS)
        {
          if (!renderTempFrames.empty())
          {
            toast_log(Level::INFO, TOAST_EXPORT_RENDERED_FRAMES, pathString);
          }
          else
          {
            toast_log(Level::ERROR, TOAST_EXPORT_RENDERED_FRAMES_FAILED, pathString);
          }
        }
        else if (type == render::SPRITESHEET)
        {
          auto& rows = settings.renderRows;
          auto& columns = settings.renderColumns;
          auto layout = render::spritesheet_layout_get(rows, columns, (int)renderTempFrames.size());
          rows = layout.rows;
          columns = layout.columns;

          if (renderTempFrames.empty())
          {
            toast_log(Level::WARNING, TOAST_SPRITESHEET_NO_FRAMES);
          }
          else
          {
            auto firstFrame = Image(renderTempFrames.front());
            if (firstFrame.size.x <= 0 || firstFrame.size.y <= 0 || firstFrame.pixels.empty())
            {
              toast_log(Level::ERROR, TOAST_SPRITESHEET_EMPTY);
            }
            else
            {
              auto frameWidth = firstFrame.size.x;
              auto frameHeight = firstFrame.size.y;
              ivec2 spritesheetSize = ivec2(frameWidth * columns, frameHeight * rows);

              std::vector<uint8_t> spritesheet((size_t)(spritesheetSize.x) * spritesheetSize.y * CHANNELS);

              for (std::size_t index = 0; index < renderTempFrames.size(); ++index)
              {
                auto frame = Image(renderTempFrames[index]);
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

              Image spritesheetTexture(spritesheet.data(), spritesheetSize);
              if (spritesheetTexture.write_png(path))
              {
                toast_log(Level::INFO, TOAST_EXPORT_SPRITESHEET, pathString);
              }
              else
              {
                toast_log(Level::ERROR, TOAST_EXPORT_SPRITESHEET_FAILED, pathString);
              }
            }
          }
        }
        else
        {
          if (settings.timelineIsSound && type != render::GIF)
          {
            if (!render_audio_stream_generate(audioStream, document, renderFrameSoundIDs, renderFrameRate))
            {
              toasts.push(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED));
              logger.error("Failed to generate deterministic render audio stream; exporting without audio.");
              audioStream.stream.clear();
            }
          }
          else
            audioStream.stream.clear();

          if (animation_render(ffmpegPath, path, renderTempFrames, audioStream, (render::Type)type, renderFrameRate))
          {
            toast_log(Level::INFO, TOAST_EXPORT_RENDERED_ANIMATION, pathString);
          }
          else
          {
            toast_log(Level::ERROR, TOAST_EXPORT_RENDERED_ANIMATION_FAILED, pathString);
          }
        }

        if (type == render::PNGS)
        {
          renderTempDirectory.clear();
          renderTempFrames.clear();
          renderFrameSoundIDs.clear();
        }
        else
        {
          render_temp_cleanup(renderTempDirectory, renderTempFrames);
          renderFrameSoundIDs.clear();
        }

        render_preview_restore();

        playback.isPlaying = false;
        playback.isFinished = false;
        manager.isRecording = false;
        manager.isRecordingStart = false;
        renderFrameIndex = 0;
        renderFrameCount = 0;
        manager.progressPopup.close();
      }
      stop_all_sounds();
      wasPlaybackPlaying = false;
      return;
    }

    if (playback.isPlaying)
    {
      auto animation = model.animation_get(document.reference_get().animationIndex);
      auto& isSound = settings.timelineIsSound;

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
          if (auto soundID = trigger_sound_id_get(document, animation, playback.time); soundID != -1)
            if (auto sound = document.sound_get(soundID)) audio::play(*sound, false, mixer);
        }

        auto fps = std::max(model.info.fps, 1);
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
    auto& model = document.model;
    auto& playback = document.playback;
    auto reference = document.reference_get();
    auto animation = model.animation_get(reference.animationIndex);
    auto& pan = document.previewPan;
    auto& zoom = document.previewZoom;
    auto& backgroundColor = settings.previewBackgroundColor;
    auto& isTransparent = settings.animationPreviewTransparent;
    auto& axesColor = settings.previewAxesColor;
    auto& gridColor = settings.previewGridColor;
    auto& gridSize = settings.previewGridSize;
    auto& gridOffset = settings.previewGridOffset;
    auto& isGrid = settings.previewIsGrid;
    auto& overlayTransparency = settings.previewOverlayTransparency;
    auto& overlayDrawOrder = settings.previewOverlayDrawOrder;
    auto& overlayIndex = document.overlayIndex;
    auto& overlayDocumentId = document.overlayDocumentId;
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
    auto center_view = [&]() { pan = vec2(); };

    auto fit_view = [&]()
    {
      if (animation) set_to_rect(zoom, pan, model::animation_rect(model, *animation, isRootTransform));
    };

    manager.isAbleToRecord = false;

    if (ImGui::Begin(localize.get(LABEL_ANIMATION_PREVIEW_WINDOW), &settings.windowIsAnimationPreview))
    {
      isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
      manager.isAbleToRecord = true;

      auto childSize = ImVec2(row_widget_width_get(4),
                              (ImGui::GetTextLineHeightWithSpacing() * 4) + (ImGui::GetStyle().WindowPadding.y * 2));

      grid_child_draw(childSize, isGrid, gridColor, gridSize, gridOffset);

      ImGui::SameLine();

      if (ImGui::BeginChild("##View Child", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
      {
        ImGui::InputFloat(localize.get(BASIC_ZOOM), &zoom, 0.0f, 0.0f, "%.0f%%");
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_PREVIEW_ZOOM));

        view_buttons_draw(manager, settings, center_view, fit_view);

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

        auto overlayOptions = overlay_animation_options_get(manager);
        auto overlayLabels = overlay_animation_option_labels_get(overlayOptions);
        auto overlayOptionIndex = overlay_animation_option_index_get(document, overlayOptions);
        if (overlayOptionIndex == -1)
        {
          overlayIndex = -1;
          overlayDocumentId = 0;
          overlayOptionIndex = 0;
        }
        if (ImGui::Combo(localize.get(LABEL_OVERLAY), &overlayOptionIndex, overlayLabels.data(),
                         (int)overlayLabels.size()))
        {
          auto& option = overlayOptions[overlayOptionIndex];
          overlayIndex = option.animationIndex;
          overlayDocumentId = overlayIndex == -1 ? 0 : option.documentId;
        }
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OVERLAY));

        overlayDrawOrder = glm::clamp(overlayDrawOrder, (int)overlay_draw_order::OVER, (int)overlay_draw_order::UNDER);
        overlayTransparency = glm::clamp(overlayTransparency, 0.0f, 100.0f);
        auto orderIconSize = icon_size_get();
        auto orderButtonWidth = orderIconSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
        auto alphaWidth = std::max(ImGui::GetContentRegionAvail().x - (orderButtonWidth * 2.0f) -
                                       (ImGui::GetStyle().ItemSpacing.x * 2.0f),
                                   ImGui::GetFrameHeight());
        auto overlayIconTint = isLightTheme ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImGui::PushItemWidth(alphaWidth);
        ImGui::DragFloat("##Overlay Alpha", &overlayTransparency, DRAG_SPEED, 0, 100, "%.0f%%");
        ImGui::PopItemWidth();
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OVERLAY_ALPHA));
        overlayTransparency = glm::clamp(overlayTransparency, 0.0f, 100.0f);

        ImGui::SameLine();
        radio_button_icon("##Overlay Over", &overlayDrawOrder, overlay_draw_order::OVER,
                          resources.icon_id_get(icon::OVER), orderIconSize, overlayIconTint);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OVERLAY_OVER));

        ImGui::SameLine();
        radio_button_icon("##Overlay Under", &overlayDrawOrder, overlay_draw_order::UNDER,
                          resources.icon_id_get(icon::UNDER), orderIconSize, overlayIconTint);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_OVERLAY_UNDER));
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

        auto isRenderPreviewOverridden = settings.renderIsUseAnimationBounds || settings.renderIsUseIsolatedAnimation;
        if (isRenderPreviewOverridden)
        {
          savedOverlayIndex = overlayIndex;
          savedOverlayDocumentId = overlayDocumentId;
          savedZoom = zoom;
          savedPan = pan;
        }

        if (settings.renderIsUseIsolatedAnimation)
        {
          overlayIndex = -1;
          overlayDocumentId = 0;
          settings.animationPreviewTransparent = true;
          settings.timelineIsOnlyShowLayers = true;
          settings.previewIsAxes = false;
          settings.previewIsGrid = false;
          settings.previewIsPivots = false;
          settings.previewIsAltIcons = false;
          settings.previewIsBorder = false;
          settings.onionskinIsEnabled = false;
        }

        if (settings.renderIsUseAnimationBounds)
        {
          if (auto rect = animation_render_rect_get(manager, document, animation, isRootTransform))
          {
            size_set(vec2(rect->z, rect->w) * settings.renderScale);
            set_to_rect(zoom, pan, *rect);
          }

          isSizeTrySet = false;
        }

        manager.isRecordingStart = false;
        manager.isRecording = true;
        renderFrameIndex = 0;
        renderAnimationFps = std::max(model.info.fps, 1);
        renderFrameRate = render::fps_get(settings.renderFpsMode, renderAnimationFps, settings.playbackTickRate);
        renderFrameCount =
            render::frame_count_get(manager.recordingStart, manager.recordingEnd, renderAnimationFps, renderFrameRate);
        renderTempFrames.clear();
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
          if (isRenderPreviewOverridden)
          {
            settings = savedSettings;
            pan = savedPan;
            zoom = savedZoom;
            overlayIndex = savedOverlayIndex;
            overlayDocumentId = savedOverlayDocumentId;
            isSizeTrySet = true;
            hasPendingZoomPanAdjust = false;
            isCheckerPanInitialized = false;
          }

          auto pathString = path::to_utf8(settings.renderPath);
          toast_log(Level::ERROR, TOAST_EXPORT_RENDERED_ANIMATION_FAILED, pathString);
          manager.isRecording = false;
          manager.isRecordingStart = false;
          renderFrameIndex = 0;
          renderFrameCount = 0;
          manager.progressPopup.close();
          playback.isPlaying = false;
          playback.isFinished = false;
          return;
        }
        playback.isPlaying = true;
        playback.timing_reset();
        playback.time =
            render::frame_time_get(manager.recordingStart, renderFrameIndex, renderAnimationFps, renderFrameRate);
        document.frameTime = playback.time;
      }

      if (manager.isRecording && renderFrameIndex < renderFrameCount)
      {
        playback.isPlaying = true;
        playback.isFinished = false;
        playback.time =
            render::frame_time_get(manager.recordingStart, renderFrameIndex, renderAnimationFps, renderFrameRate);
        document.frameTime = playback.time;
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

      model::DrawOptions drawOptions{.time = frameTime,
                                     .isRootTransform = isRootTransform,
                                     .isIndexSampled = settings.onionskinMode == (int)OnionskinMode::INDEX};
      std::vector<vec3> sampleColors{};
      std::vector<float> sampleAlphas{};
      auto samples_add = [&](int count, int direction, vec3 color)
      {
        for (int i = 1; i <= count; ++i)
        {
          drawOptions.samples.push_back({.timeOffset = (float)(direction * i), .indexOffset = direction * i});
          sampleColors.push_back(color);
          sampleAlphas.push_back((1.0f / (count + 1)) * i);
        }
      };
      if (settings.onionskinIsEnabled)
      {
        samples_add(settings.onionskinBeforeCount, -1, settings.onionskinBeforeColor);
        samples_add(settings.onionskinAfterCount, 1, settings.onionskinAfterColor);
      }

      auto referenceItemType = static_cast<ItemType>(reference.itemType);
      auto is_layer_selected = [&](int id)
      {
        auto is_layer = [&](const Reference& itemReference)
        {
          return itemReference.animationIndex == reference.animationIndex && itemReference.itemType == LAYER &&
                 itemReference.itemID == id;
        };
        return (reference.animationIndex != -1 && is_layer(reference)) ||
               std::ranges::any_of(document.selected_get(SelectionKind::TRACKS), is_layer) ||
               std::ranges::any_of(document.frame_references_get(Document::FrameReferenceFallback::NONE), is_layer);
      };

      // Draws an animation's draw list; onion-skin samples are tinted and faded, `alphaOffset` fades the overlay.
      auto render = [&](Document& sampleDocument, const model::Animation& sampleAnimation, float alphaOffset)
      {
        auto isActiveDocument = &sampleDocument == &document;
        auto& sampleModel = sampleDocument.model;
        auto targetIcon = resources.icon_id_get(isAltIcons ? icon::TARGET_ALT : icon::TARGET);
        for (const auto& draw : model::animation_draws_get(sampleModel, sampleAnimation, drawOptions))
        {
          auto isOnion = draw.sample != -1;
          auto sampleColor = isOnion ? sampleColors[draw.sample] : vec3();
          auto sampleAlpha = isOnion ? sampleAlphas[draw.sample] : 0.0f;
          auto onionColor = vec4(sampleColor, 1.0f - sampleAlpha);
          auto& frame = draw.frame;
          auto transform = baseTransform * draw.parent;
          auto marker_model_get = [&](vec2 markerSize)
          {
            return math::quad_model_get(markerSize, frame.position, markerSize * 0.5f,
                                        math::percent_to_unit(frame.scale), frame.rotation,
                                        math::percent_to_unit(frame.shear));
          };

          if (draw.type == model::DrawType::ROOT || draw.type == model::DrawType::GROUP_ROOT)
          {
            if (isOnlyShowLayers) continue;
            auto isSelected = isActiveDocument && draw.type == model::DrawType::GROUP_ROOT &&
                              referenceItemType == ItemType::ROOT && reference.groupType == draw.groupType &&
                              reference.groupId == draw.id;
            auto color = isOnion                              ? vec4(sampleColor, sampleAlpha)
                         : draw.type == model::DrawType::ROOT ? color::GREEN
                         : isSelected                         ? color::RED
                                                              : ROOT_COLOR;
            auto markerModel = isRootTransform ? math::quad_model_get(TARGET_SIZE, {}, TARGET_SIZE * 0.5f)
                                               : marker_model_get(TARGET_SIZE);
            texture_render(shaderTexture, targetIcon, transform * markerModel, color);
          }
          else if (draw.type == model::DrawType::LAYER)
          {
            auto layer = model::item_get(sampleModel.content.layers, draw.id);
            auto layerTexture = sampleDocument.texture_get(layer->spritesheetId);
            if (!layerTexture || !layerTexture->is_valid() || layerTexture->size.x <= 0 || layerTexture->size.y <= 0)
              continue;
            auto textureSize = vec2(layerTexture->size);
            auto layerModel = model::draw_quad_model_get(draw);
            auto layerTransform = transform * layerModel;
            auto vertices = math::uv_vertices_get(frame.crop / textureSize, (frame.crop + frame.size) / textureSize);
            auto tint = frame.tint;
            tint.a = std::max(0.0f, tint.a - (alphaOffset + sampleAlpha));
            auto customShader = sampleDocument.shader_get(frame.shaderId);
            texture_render(customShader ? *customShader : shaderTexture, resource::texture::id_get(*layerTexture),
                           layerTransform, tint, frame.colorOffset + sampleColor, vertices.data(), textureSize,
                           draw.time);

            auto color = isOnion                                          ? onionColor
                         : isActiveDocument && is_layer_selected(draw.id) ? SELECTED_LAYER_BORDER_COLOR
                                                                          : color::RED;
            if (isBorder) rect_render(shaderLine, layerTransform, layerModel, color);
            if (isPivots)
              texture_render(shaderTexture, resources.icon_id_get(icon::PIVOT),
                             transform * marker_model_get(PIVOT_SIZE), color);
          }
          else if (!isOnlyShowLayers)
          {
            auto isShowRect = model::item_get(sampleModel.content.nulls, draw.id)->isShowRect;
            auto isSelected = isActiveDocument && draw.id == reference.itemID && referenceItemType == ItemType::NULL_;
            auto color = isOnion ? onionColor : isSelected ? color::RED : NULL_COLOR;
            auto markerSize = isShowRect ? POINT_SIZE : TARGET_SIZE;
            texture_render(shaderTexture, isShowRect ? resources.icon_id_get(icon::POINT) : targetIcon,
                           transform * marker_model_get(markerSize), color);
            if (!isShowRect) continue;
            auto rectModel = math::quad_model_get(frame.scale, frame.position, frame.scale * 0.5f, vec2(1.0f),
                                                  frame.rotation, math::percent_to_unit(frame.shear));
            rect_render(shaderLine, transform * rectModel, rectModel, color);
          }
        }
      };

      if (animation)
      {
        auto overlay_render = [&]()
        {
          if (auto overlayDocument = overlay_animation_document_get(manager, document))
            if (auto overlayAnimation = overlayDocument->model.animation_get(overlayIndex))
              render(*overlayDocument, *overlayAnimation, 1.0f - math::percent_to_unit(overlayTransparency));
        };

        if (overlayDrawOrder == overlay_draw_order::UNDER) overlay_render();
        render(document, *animation, 0.0f);
        if (overlayDrawOrder == overlay_draw_order::OVER) overlay_render();
      }

      if (manager.isRecording && renderFrameIndex < renderFrameCount)
      {
        auto renderType = settings.renderType;
        auto isRenderPreviewOverridden = settings.renderIsUseAnimationBounds || settings.renderIsUseIsolatedAnimation;
        auto render_capture_fail = [&]()
        {
          auto pathString = path::to_utf8(settings.renderPath);
          toast_log(Level::ERROR, TOAST_EXPORT_RENDERED_ANIMATION_FAILED, pathString);
          if (renderType != render::PNGS) render_temp_cleanup(renderTempDirectory, renderTempFrames);
          renderFrameSoundIDs.clear();
          if (isRenderPreviewOverridden)
          {
            settings = savedSettings;
            pan = savedPan;
            zoom = savedZoom;
            overlayIndex = savedOverlayIndex;
            overlayDocumentId = savedOverlayDocumentId;
            isSizeTrySet = true;
            hasPendingZoomPanAdjust = false;
            isCheckerPanInitialized = false;
          }
          playback.isPlaying = false;
          playback.isFinished = false;
          manager.isRecording = false;
          manager.isRecordingStart = false;
          renderFrameIndex = 0;
          renderFrameCount = 0;
          manager.progressPopup.close();
        };

        auto frameSoundID = -1;
        if (settings.timelineIsSound && !document.sounds.empty())
        {
          auto soundTime = (int)std::floor(frameTime);
          if (soundTime != renderFrameSoundTimePrev && animation)
            frameSoundID = trigger_sound_id_get(document, animation, frameTime, renderFrameIndex);
          renderFrameSoundTimePrev = soundTime;
        }
        renderFrameSoundIDs.push_back(frameSoundID);

        auto pixels = pixels_get();
        if (isRenderPreviewOverridden) pixels_unpremultiply_alpha(pixels);
        auto framePath =
            renderTempDirectory / render_frame_filename(settings.renderFormat, renderFrameIndex, settings.renderType);
        if (Image::write_pixels_png(framePath, size, pixels.data()))
        {
          renderTempFrames.push_back(framePath);
          ++renderFrameIndex;
        }
        else
          render_capture_fail();
      }

      unbind();

      if (isTransparent)
      {
        checker_pan_sync(zoom, pan);
        render_checker_background(ImGui::GetWindowDrawList(), min, max, -size - checkerPan, CHECKER_SIZE);
      }
      image_premultiplied_draw(texture, to_imvec2(size));

      isPreviewHovered = ImGui::IsItemHovered();

      if (animation && animation->triggers.isVisible && !isOnlyShowLayers && !manager.isRecording)
      {
        if (auto trigger = model::frame_generate(animation->triggers, frameTime);
            trigger.isVisible && trigger.eventId > -1)
        {
          auto clipMin = ImGui::GetItemRectMin();
          auto clipMax = ImGui::GetItemRectMax();
          auto drawList = ImGui::GetWindowDrawList();
          auto textPos = to_imvec2(to_vec2(cursorScreenPos) + to_vec2(ImGui::GetStyle().WindowPadding));

          drawList->PushClipRect(clipMin, clipMax);
          ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE_LARGE);
          auto triggerTextColor = isLightTheme ? TRIGGER_TEXT_COLOR_LIGHT : TRIGGER_TEXT_COLOR_DARK;
          if (auto event = model::item_get(model.content.events, trigger.eventId))
            drawList->AddText(textPos, ImGui::GetColorU32(triggerTextColor), event->name.c_str());
          ImGui::PopFont();
          drawList->PopClipRect();
        }
      }

      if (isPreviewHovered)
      {
        auto input = canvas_input_get(manager, isFocused);
        mousePos = position_translate(zoom, pan, to_vec2(ImGui::GetMousePos()) - to_vec2(cursorScreenPos));
        auto selectedFrameReferences = document.frame_references_get();
        std::erase_if(selectedFrameReferences, [](const Reference& frameReference)
                      { return frameReference.itemType == NONE || frameReference.itemType == TRIGGER; });
        auto editReference = reference;
        if ((referenceItemType == ItemType::TRIGGER || !selectedFrameReferences.contains(reference)) &&
            !selectedFrameReferences.empty())
          editReference = *selectedFrameReferences.begin();

        auto frame = selectedFrameReferences.empty() ? nullptr : model.frame_get(editReference);
        auto selectedNull =
            editReference.itemType == NULL_ ? model::item_get(model.content.nulls, editReference.itemID) : nullptr;
        bool isSelectedNullRect = selectedNull && selectedNull->isShowRect;
        auto null_rect_top_left = [](const model::Frame& frame) { return frame.position - (frame.scale * 0.5f); };

        auto useTool = (tool::Type)tool;
        if (input.isMiddleDown) useTool = tool::PAN;
        if (tool == tool::MOVE && input.isRightDown) useTool = tool::SCALE;
        if (tool == tool::SCALE && input.isRightDown) useTool = tool::MOVE;

        // A tool swapped in by the right button is driven by the right button.
        auto isSwapped = useTool != tool && useTool != tool::PAN;
        auto isToolClicked = isSwapped ? input.isRightClicked : input.isLeftClicked;
        auto isToolDown = isSwapped ? input.isRightDown : input.isLeftDown;
        auto isToolReleased = isSwapped ? input.isRightReleased : input.isLeftReleased;
        auto isBegin = isToolClicked || input.isArrowBegin;
        auto isDuring = isToolDown || input.isArrowDown;
        auto isEnd = isToolReleased || input.isArrowEnd;
        auto arrow = input.arrow * input.step;
        auto isArrow = arrow != vec2();

        // Changes apply to every selected frame; absolute values are applied as the difference from the edited frame.
        auto frames_change = [&](FrameChange change, ChangeType type = ChangeType::ADD)
        { frames_change_push(manager, selectedFrameReferences, change, type); };
        auto null_rect_set = [&](vec2 minPoint, vec2 maxPoint)
        {
          auto rectSize = maxPoint - minPoint;
          frames_change({.positionX = minPoint.x + rectSize.x * 0.5f - frame->position.x,
                         .positionY = minPoint.y + rectSize.y * 0.5f - frame->position.y,
                         .scaleX = rectSize.x - frame->scale.x,
                         .scaleY = rectSize.y - frame->scale.y});
        };

        tool_cursor_update(useTool, tool::ANIMATION_PREVIEW, frame != nullptr, true,
                           input.isLeftDown || input.isRightDown || input.isMiddleDown || input.isArrowDown,
                           TEXT_SELECT_FRAME);
        if (useTool != tool::MOVE) isMoveDragging = false;
        if (useTool == tool::PAN && (input.isLeftDown || input.isRightDown || input.isMiddleDown))
          pan += input.mouseDelta;

        if (frame && useTool == tool::MOVE)
        {
          if (isBegin) edit_begin_push(manager, EDIT_FRAME_POSITION);
          if (isToolClicked)
          {
            auto origin = isSelectedNullRect ? null_rect_top_left(*frame) : frame->position;
            moveOffset = settings.inputIsMoveToolSnapToMouse ? vec2() : mousePos - origin;
            isMoveDragging = true;
          }
          if (isToolDown && isMoveDragging)
          {
            auto position = vec2(ivec2(mousePos - moveOffset + (isSelectedNullRect ? frame->scale * 0.5f : vec2())));
            frames_change({.positionX = position.x - frame->position.x, .positionY = position.y - frame->position.y});
          }
          if (isArrow) frames_change({.positionX = arrow.x, .positionY = arrow.y});
          if (isToolReleased) isMoveDragging = false;
          if (isDuring) tooltip_lines_draw({localize_format(FORMAT_POSITION, frame->position.x, frame->position.y)});
          if (isEnd) document_change_push(manager);
        }

        if (frame && useTool == tool::SCALE)
        {
          if (isBegin) edit_begin_push(manager, EDIT_FRAME_SCALE);
          if (isToolClicked && isSelectedNullRect) nullRectScaleAnchor = null_rect_top_left(*frame);
          if (isToolDown && isSelectedNullRect)
          {
            auto rectSize = mousePos - nullRectScaleAnchor;
            if (input.isMod)
            {
              auto squareSize = std::max(std::abs(rectSize.x), std::abs(rectSize.y));
              rectSize = {std::copysign(squareSize, rectSize.x), std::copysign(squareSize, rectSize.y)};
            }
            auto [minPoint, maxPoint] = rect_snap(nullRectScaleAnchor, nullRectScaleAnchor + rectSize, false, {}, {});
            null_rect_set(minPoint, maxPoint);
          }
          else if (isToolDown)
            frames_change(
                {.scaleX = input.mouseDelta.x,
                 .scaleY = input.isMod ? frame->scale.x + input.mouseDelta.x - frame->scale.y : input.mouseDelta.y});
          if (isArrow)
            frames_change({.positionX = isSelectedNullRect ? arrow.x * 0.5f : 0.0f,
                           .positionY = isSelectedNullRect ? arrow.y * 0.5f : 0.0f,
                           .scaleX = arrow.x,
                           .scaleY = arrow.y});
          if (isDuring) tooltip_lines_draw({localize_format(FORMAT_SCALE, frame->scale.x, frame->scale.y)});
          if (isEnd && isSelectedNullRect)
          {
            auto topLeft = null_rect_top_left(*frame);
            auto [minPoint, maxPoint] = rect_snap(topLeft, topLeft + frame->scale, false, {}, {});
            null_rect_set(minPoint, maxPoint);
          }
          if (isEnd) document_change_push(manager);
        }

        if (frame && useTool == tool::ROTATE)
        {
          if (isBegin) edit_begin_push(manager, EDIT_FRAME_ROTATION);
          if (isToolDown) frames_change({.rotation = input.mouseDelta.x});
          if (auto turn = glm::sign(input.arrow.x - input.arrow.y); turn != 0.0f)
            frames_change({.rotation = turn * input.step});
          if (isDuring) tooltip_lines_draw({localize_format(FORMAT_ROTATION, frame->rotation)});
          if (isEnd) document_change_push(manager);
        }

        if (frame && useTool == tool::SHEAR)
        {
          if (isBegin) edit_begin_push(manager, EDIT_FRAME_SHEAR);
          if (isToolDown)
          {
            auto delta = input.mouseDelta;
            if (input.isMod) (std::abs(delta.x) >= std::abs(delta.y) ? delta.y : delta.x) = 0.0f;
            frames_change({.shearX = delta.x, .shearY = delta.y});
          }
          if (isArrow) frames_change({.shearX = arrow.x, .shearY = arrow.y});
          if (isDuring) tooltip_lines_draw({localize_format(FORMAT_SHEAR, frame->shear.x, frame->shear.y)});
          if (isEnd) document_change_push(manager);
        }

        if (input.wheel != 0 || input.isZoomIn || input.isZoomOut)
          zoom_step(zoom, pan, vec2(mousePos),
                    (input.wheel > 0 || input.isZoomIn) ? ZOOM_LEVEL_STEP : -ZOOM_LEVEL_STEP);
      }
    }

    if (tool == tool::PAN)
      view_context_menu_draw("##Animation Preview Context Menu", manager, settings, document, zoom, pan, center_view,
                             fit_view, animation != nullptr);

    manager.progressPopup.trigger();

    if (ImGui::BeginPopupModal(manager.progressPopup.label(), &manager.progressPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto progress = renderFrameCount > 0 ? (float)renderFrameIndex / (float)renderFrameCount : 0.0f;

      ImGui::ProgressBar(progress);

      ImGui::TextUnformatted(localize.get(TEXT_RECORDING_PROGRESS));

      shortcut(manager.chords[SHORTCUT_CANCEL]);
      if (ImGui::Button(localize.get(BASIC_CANCEL), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
      {
        auto renderType = settings.renderType;
        auto isRenderPreviewOverridden = settings.renderIsUseAnimationBounds || settings.renderIsUseIsolatedAnimation;
        if (renderType == render::PNGS)
        {
          renderTempDirectory.clear();
          renderTempFrames.clear();
          renderFrameSoundIDs.clear();
        }
        else
        {
          render_temp_cleanup(renderTempDirectory, renderTempFrames);
          renderFrameSoundIDs.clear();
        }

        if (isRenderPreviewOverridden)
        {
          pan = savedPan;
          zoom = savedZoom;
          settings = savedSettings;
          overlayIndex = savedOverlayIndex;
          overlayDocumentId = savedOverlayDocumentId;
          isSizeTrySet = true;
          hasPendingZoomPanAdjust = false;
          isCheckerPanInitialized = false;
        }

        playback.isPlaying = false;
        playback.isFinished = false;
        manager.isRecording = false;
        manager.isRecordingStart = false;
        renderFrameIndex = 0;
        renderFrameCount = 0;
        manager.progressPopup.close();
      }

      ImGui::EndPopup();
    }

    if (!document.isAnimationPreviewSet)
    {
      center_view();
      zoom = settings.previewStartZoom;
      checker_pan_reset(zoom, pan);
      document.isAnimationPreviewSet = true;
    }

    settings.previewStartZoom = zoom;
    ImGui::End();
  }
}
