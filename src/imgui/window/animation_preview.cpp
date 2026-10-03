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
      auto is_layer_animation_selected = [&](Document& sampleDocument, int id)
      {
        if (&sampleDocument != &document) return false;
        if (reference.animationIndex != -1 && referenceItemType == ItemType::LAYER && reference.itemID == id)
          return true;
        for (auto itemReference : document.selected_get(SelectionKind::TRACKS))
          if (itemReference.animationIndex == reference.animationIndex && itemReference.itemType == LAYER &&
              itemReference.itemID == id)
            return true;
        for (auto frameReference : document.frame_references_get(Document::FrameReferenceFallback::NONE))
          if (frameReference.animationIndex == reference.animationIndex && frameReference.itemType == LAYER &&
              frameReference.itemID == id)
            return true;
        return false;
      };

      auto render = [&](Document& sampleDocument, const model::Animation* animation, float time, vec3 colorOffset = {},
                        float alphaOffset = {}, const std::vector<OnionskinSample>* layeredOnions = nullptr,
                        bool isIndexMode = false)
      {
        auto& sampleModel = sampleDocument.model;
        bool isActiveDocument = &sampleDocument == &document;

        auto sample_time_for_item = [&](const model::Track& item, const OnionskinSample& sample) -> std::optional<float>
        {
          if (!isIndexMode)
          {
            if (sample.time < 0.0f || sample.time > animation->frameNum) return std::nullopt;
            return sample.time;
          }
          if (item.frames.empty()) return std::nullopt;
          int baseIndex = model::frame_index_from_time_get(item, frameTime);
          if (baseIndex < 0) return std::nullopt;
          int sampleIndex = baseIndex + sample.indexOffset;
          if (sampleIndex < 0 || sampleIndex >= (int)item.frames.size()) return std::nullopt;
          return model::frame_time_from_index_get(item, sampleIndex);
        };

        auto root = &animation->root;

        auto transform_for_time = [&](float t)
        {
          auto sampleTransform = baseTransform;
          if (isRootTransform && root)
          {
            auto rootFrame = model::frame_generate(*root, t);
            sampleTransform *= model::frame_parent_model_get(rootFrame);
          }
          return sampleTransform;
        };

        auto transform = transform_for_time(time);

        auto group_root_frame_get = [](const model::TrackGroup* group, float t) -> std::optional<model::Frame>
        {
          if (!group) return std::nullopt;
          return model::frame_generate(group->root, t);
        };

        auto group_transform_for_time = [&](const model::TrackGroup* group, float t, const glm::mat4& sampleTransform)
        {
          auto itemTransform = sampleTransform;
          if (isRootTransform)
            if (auto groupRootFrame = group_root_frame_get(group, t))
              itemTransform *= model::frame_parent_model_get(*groupRootFrame);
          return itemTransform;
        };

        auto draw_root =
            [&](float sampleTime, const glm::mat4& sampleTransform, vec3 sampleColor, float sampleAlpha, bool isOnion)
        {
          if (!root) return;
          auto rootFrame = model::frame_generate(*root, sampleTime);
          if (isOnlyShowLayers || !rootFrame.isVisible || !root->isVisible) return;

          auto rootModel = isRootTransform
                               ? math::quad_model_get(TARGET_SIZE, {}, TARGET_SIZE * 0.5f)
                               : math::quad_model_get(TARGET_SIZE, rootFrame.position, TARGET_SIZE * 0.5f,
                                                      math::percent_to_unit(rootFrame.scale), rootFrame.rotation,
                                                      math::percent_to_unit(rootFrame.shear));
          auto rootTransform = sampleTransform * rootModel;

          vec4 color = isOnion ? vec4(sampleColor, sampleAlpha) : color::GREEN;

          auto icon = isAltIcons ? icon::TARGET_ALT : icon::TARGET;
          texture_render(shaderTexture, resources.icon_id_get(icon), rootTransform, color);
        };

        if (layeredOnions && root)
          for (auto& sample : *layeredOnions)
            if (auto sampleTime = sample_time_for_item(*root, sample))
            {
              auto sampleTransform = transform_for_time(*sampleTime);
              draw_root(*sampleTime, sampleTransform, sample.colorOffset, sample.alphaOffset, true);
            }

        draw_root(time, transform, {}, 0.0f, false);

        auto draw_group_root = [&](const model::TrackGroup& group, int groupType, float sampleTime,
                                   const glm::mat4& sampleTransform, vec3 sampleColor, float sampleAlpha, bool isOnion)
        {
          auto rootFrame = model::frame_generate(group.root, sampleTime);
          if (isOnlyShowLayers || !rootFrame.isVisible || !group.root.isVisible) return;

          auto itemTransform = sampleTransform;
          if (isRootTransform) itemTransform *= model::frame_parent_model_get(rootFrame);

          auto rootModel = isRootTransform
                               ? math::quad_model_get(TARGET_SIZE, {}, TARGET_SIZE * 0.5f)
                               : math::quad_model_get(TARGET_SIZE, rootFrame.position, TARGET_SIZE * 0.5f,
                                                      math::percent_to_unit(rootFrame.scale), rootFrame.rotation,
                                                      math::percent_to_unit(rootFrame.shear));
          auto rootTransform = itemTransform * rootModel;

          auto isSelected = isActiveDocument && referenceItemType == ItemType::ROOT &&
                            reference.groupType == groupType && reference.groupId == group.id;
          vec4 color = isOnion ? vec4(sampleColor, sampleAlpha) : isSelected ? color::RED : ROOT_COLOR;
          auto icon = isAltIcons ? icon::TARGET_ALT : icon::TARGET;
          texture_render(shaderTexture, resources.icon_id_get(icon), rootTransform, color);
        };

        // Draws loose tracks and visible groups (their root, then their tracks) in file order.
        auto entries_draw = [&](const std::vector<model::TrackEntry>& entries, int groupType, auto&& track_draw)
        {
          for (const auto& entry : entries)
          {
            if (auto track = std::get_if<model::Track>(&entry))
            {
              track_draw(*track, nullptr);
              continue;
            }
            auto& group = std::get<model::TrackGroup>(entry);
            if (!group.isVisible) continue;
            if (layeredOnions)
              for (auto& sample : *layeredOnions)
                if (auto sampleTime = sample_time_for_item(group.root, sample))
                  draw_group_root(group, groupType, *sampleTime, transform_for_time(*sampleTime), sample.colorOffset,
                                  sample.alphaOffset, true);
            draw_group_root(group, groupType, time, transform, {}, 0.0f, false);
            for (const auto& groupTrack : group.tracks)
              track_draw(groupTrack, &group);
          }
        };

        entries_draw(animation->layers, LAYER,
                     [&](const model::Track& layerAnimation, const model::TrackGroup* group)
                     {
                       if (!layerAnimation.isVisible) return;

                       auto id = layerAnimation.id;
                       auto layer = model::item_get(sampleModel.content.layers, id);
                       if (!layer) return;

                       auto textureInfo = sampleDocument.texture_get(layer->spritesheetId);
                       if (!textureInfo || !textureInfo->is_valid()) return;

                       auto draw_layer = [&](float sampleTime, const glm::mat4& sampleTransform, vec3 sampleColor,
                                             float sampleAlpha, bool isOnion)
                       {
                         auto frame = model::frame_generate(layerAnimation, sampleTime);
                         if (!frame.isVisible) return;

                         auto& texture = *textureInfo;

                         auto texSize = vec2(texture.size);
                         if (texSize.x <= 0.0f || texSize.y <= 0.0f) return;

                         frame = sampleModel.frame_effective(id, frame);
                         auto crop = frame.crop;
                         auto size = frame.size;
                         auto pivot = frame.pivot;

                         auto layerModel =
                             math::quad_model_get(size, frame.position, pivot, math::percent_to_unit(frame.scale),
                                                  frame.rotation, math::percent_to_unit(frame.shear));
                         auto itemTransform = group_transform_for_time(group, sampleTime, sampleTransform);
                         auto layerTransform = itemTransform * layerModel;

                         auto uvMin = crop / texSize;
                         auto uvMax = (crop + size) / texSize;

                         vec3 frameColorOffset = frame.colorOffset + colorOffset + sampleColor;
                         vec4 frameTint = frame.tint;

                         if (isRootTransform && root)
                         {
                           auto rootFrame = model::frame_generate(*root, sampleTime);
                           frameColorOffset += rootFrame.colorOffset;
                           frameTint *= rootFrame.tint;
                         }
                         if (isRootTransform)
                           if (auto groupRootFrame = group_root_frame_get(group, sampleTime))
                           {
                             frameColorOffset += groupRootFrame->colorOffset;
                             frameTint *= groupRootFrame->tint;
                           }

                         frameTint.a = std::max(0.0f, frameTint.a - (alphaOffset + sampleAlpha));

                         auto vertices = math::uv_vertices_get(uvMin, uvMax);
                         auto customShader = sampleDocument.shader_get(frame.shaderId);
                         auto& layerShader = customShader ? *customShader : shaderTexture;

                         texture_render(layerShader, resource::texture::id_get(texture), layerTransform, frameTint,
                                        frameColorOffset, vertices.data(), vec2(texture.size), sampleTime);

                         auto color = isOnion ? vec4(sampleColor, 1.0f - sampleAlpha)
                                      : is_layer_animation_selected(sampleDocument, id) ? SELECTED_LAYER_BORDER_COLOR
                                                                                        : color::RED;

                         if (isBorder) rect_render(shaderLine, layerTransform, layerModel, color);

                         if (isPivots)
                         {
                           auto pivotModel = math::quad_model_get(PIVOT_SIZE, frame.position, PIVOT_SIZE * 0.5f,
                                                                  math::percent_to_unit(frame.scale), frame.rotation,
                                                                  math::percent_to_unit(frame.shear));
                           auto pivotTransform = itemTransform * pivotModel;

                           texture_render(shaderTexture, resources.icon_id_get(icon::PIVOT), pivotTransform, color);
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
                     });

        entries_draw(animation->nulls, NULL_,
                     [&](const model::Track& nullAnimation, const model::TrackGroup* group)
                     {
                       if (!nullAnimation.isVisible || isOnlyShowLayers) return;

                       auto id = nullAnimation.id;
                       auto nullInfo = model::item_get(sampleModel.content.nulls, id);
                       if (!nullInfo) return;
                       auto isShowRect = nullInfo->isShowRect;

                       auto draw_null = [&](float sampleTime, const glm::mat4& sampleTransform, vec3 sampleColor,
                                            float sampleAlpha, bool isOnion)
                       {
                         auto frame = model::frame_generate(nullAnimation, sampleTime);
                         if (!frame.isVisible) return;

                         auto icon = isShowRect ? icon::POINT : isAltIcons ? icon::TARGET_ALT : icon::TARGET;

                         auto& size = isShowRect ? POINT_SIZE : TARGET_SIZE;
                         auto color =
                             isOnion ? vec4(sampleColor, 1.0f - sampleAlpha)
                             : isActiveDocument && id == reference.itemID && referenceItemType == ItemType::NULL_
                                 ? color::RED
                                 : NULL_COLOR;

                         auto nullModel =
                             math::quad_model_get(size, frame.position, size * 0.5f, math::percent_to_unit(frame.scale),
                                                  frame.rotation, math::percent_to_unit(frame.shear));
                         auto itemTransform = group_transform_for_time(group, sampleTime, sampleTransform);
                         auto nullTransform = itemTransform * nullModel;

                         texture_render(shaderTexture, resources.icon_id_get(icon), nullTransform, color);

                         if (isShowRect)
                         {
                           auto rectModel =
                               math::quad_model_get(frame.scale, frame.position, frame.scale * 0.5f, vec2(1.0f),
                                                    frame.rotation, math::percent_to_unit(frame.shear));
                           auto rectTransform = itemTransform * rectModel;

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
                     });
      };

      if (animation)
      {
        auto layeredOnions = settings.onionskinIsEnabled ? &onionskinSamples : nullptr;
        auto overlay_render = [&]()
        {
          if (auto overlayDocument = overlay_animation_document_get(manager, document))
            if (auto overlayAnimation = overlayDocument->model.animation_get(overlayIndex))
              render(*overlayDocument, overlayAnimation, frameTime, {},
                     1.0f - math::percent_to_unit(overlayTransparency), layeredOnions,
                     settings.onionskinMode == (int)OnionskinMode::INDEX);
        };

        if (overlayDrawOrder == overlay_draw_order::UNDER) overlay_render();
        render(document, animation, frameTime, {}, 0.0f, layeredOnions,
               settings.onionskinMode == (int)OnionskinMode::INDEX);
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
        auto selectedFrameReferences = document.frame_references_get();
        std::erase_if(selectedFrameReferences, [](const Reference& frameReference)
                      { return frameReference.itemType == NONE || frameReference.itemType == TRIGGER; });
        auto editReference = reference;
        auto editItemType = referenceItemType;
        if ((referenceItemType == ItemType::TRIGGER || !selectedFrameReferences.contains(reference)) &&
            !selectedFrameReferences.empty())
        {
          editReference = *selectedFrameReferences.begin();
          editItemType = static_cast<ItemType>(editReference.itemType);
        }

        auto frame = model.frame_get(editReference);
        auto item = model.track_get(editReference);
        auto selectedNull =
            editItemType == ItemType::NULL_ ? model::item_get(model.content.nulls, editReference.itemID) : nullptr;
        bool isSelectedNullRect = selectedNull && selectedNull->isShowRect;
        auto null_rect_top_left = [](const model::Frame& frame) { return frame.position - (frame.scale * 0.5f); };

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

        auto frame_snapshot = [&](StringType label)
        {
          manager.command_push(
              {manager.selected, [label](Manager&, Document& document) { document.edit_begin(label); }});
        };
        auto frame_change_apply = [&](FrameChange frameChange, ChangeType changeType = ChangeType::ADJUST)
        {
          auto queuedFrameReferences = selectedFrameReferences;
          manager.command_push({manager.selected, [=](Manager&, Document& document)
                                {
                                  std::map<Reference, std::set<int>> groupedFrames{};
                                  for (auto frameReference : queuedFrameReferences)
                                  {
                                    auto itemReference = frameReference;
                                    itemReference.frameIndex = -1;
                                    groupedFrames[itemReference].insert(frameReference.frameIndex);
                                  }

                                  for (auto& [itemReference, itemFrames] : groupedFrames)
                                  {
                                    auto itemType = static_cast<ItemType>(itemReference.itemType);
                                    if (auto item = document.model.track_edit(itemReference))
                                      model::frames_change(*item, frameChange, itemType, changeType, itemFrames);
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
        auto frame_shear_apply = [&](vec2 shear)
        {
          if (!frame) return;
          frame_change_apply({.shearX = shear.x - frame->shear.x, .shearY = shear.y - frame->shear.y}, ChangeType::ADD);
        };
        auto frames_changed = [&]()
        { manager.command_push({manager.selected, [](Manager&, Document& document) { document.change(); }}); };
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
          case tool::SHEAR:
            if (!item || !frame || selectedFrameReferences.empty()) break;
            if (isToolBegin) frame_snapshot(EDIT_FRAME_SHEAR);
            if (isToolMouseDown)
            {
              auto shear = frame->shear + vec2(mouseDelta.x, mouseDelta.y);
              if (isMod)
              {
                if (std::abs(mouseDelta.x) >= std::abs(mouseDelta.y))
                  shear.y = frame->shear.y;
                else
                  shear.x = frame->shear.x;
              }
              frame_shear_apply(shear);
            }
            if (isLeftPressed) frame_change_apply({.shearX = step}, ChangeType::SUBTRACT);
            if (isRightPressed) frame_change_apply({.shearX = step}, ChangeType::ADD);
            if (isUpPressed) frame_change_apply({.shearY = step}, ChangeType::SUBTRACT);
            if (isDownPressed) frame_change_apply({.shearY = step}, ChangeType::ADD);

            if (isToolDuring)
            {
              if (ImGui::BeginTooltip())
              {
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_SHEAR), std::make_format_args(frame->shear.x, frame->shear.y))
                        .c_str());
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
          zoom_step(zoom, pan, vec2(mousePos), (mouseWheel > 0 || isZoomIn) ? ZOOM_LEVEL_STEP : -ZOOM_LEVEL_STEP);
        }
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
