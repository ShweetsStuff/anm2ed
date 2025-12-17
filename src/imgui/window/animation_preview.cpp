#include "animation_preview.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <optional>
#include <ranges>

#include <glm/gtc/type_ptr.hpp>

#include "imgui_.h"
#include "log.h"
#include "math_.h"
#include "strings.h"
#include "toast.h"
#include "tool.h"
#include "types.h"

using namespace anm2ed::canvas;
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
      auto& type = settings.renderType;

      if (playback.time > end || playback.isFinished)
      {
        if (type == render::PNGS)
        {
          auto& format = settings.renderFormat;
          bool isSuccess{true};
          for (auto [i, frame] : std::views::enumerate(renderFrames))
          {
            std::filesystem::path outputPath =
                std::filesystem::path(path) / std::vformat(format, std::make_format_args(i));

            if (!frame.write_png(outputPath))
            {
              isSuccess = false;
              break;
            }
            logger.info(std::format("Saved frame to: {}", outputPath.string()));
          }

          if (isSuccess)
          {
            toasts.push(std::vformat(localize.get(TOAST_EXPORT_RENDERED_FRAMES), std::make_format_args(path)));
            logger.info(
                std::vformat(localize.get(TOAST_EXPORT_RENDERED_FRAMES, anm2ed::ENGLISH), std::make_format_args(path)));
          }
          else
          {
            toasts.push(std::vformat(localize.get(TOAST_EXPORT_RENDERED_FRAMES_FAILED), std::make_format_args(path)));
            logger.error(std::vformat(localize.get(TOAST_EXPORT_RENDERED_FRAMES_FAILED, anm2ed::ENGLISH),
                                      std::make_format_args(path)));
          }
        }
        else if (type == render::SPRITESHEET)
        {
          auto& rows = settings.renderRows;
          auto& columns = settings.renderColumns;

          if (renderFrames.empty())
          {
            toasts.push(localize.get(TOAST_SPRITESHEET_NO_FRAMES));
            logger.warning(localize.get(TOAST_SPRITESHEET_NO_FRAMES, anm2ed::ENGLISH));
          }
          else
          {
            auto& firstFrame = renderFrames.front();
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

              for (std::size_t index = 0; index < renderFrames.size(); ++index)
              {
                const auto& frame = renderFrames[index];
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
                toasts.push(std::vformat(localize.get(TOAST_EXPORT_SPRITESHEET), std::make_format_args(path)));
                logger.info(
                    std::vformat(localize.get(TOAST_EXPORT_SPRITESHEET, anm2ed::ENGLISH), std::make_format_args(path)));
              }
              else
              {
                toasts.push(std::vformat(localize.get(TOAST_EXPORT_SPRITESHEET_FAILED), std::make_format_args(path)));
                logger.error(std::vformat(localize.get(TOAST_EXPORT_SPRITESHEET_FAILED, anm2ed::ENGLISH),
                                          std::make_format_args(path)));
              }
            }
          }
        }
        else
        {
          if (animation_render(ffmpegPath, path, renderFrames, audioStream, (render::Type)type, size, anm2.info.fps))
          {
            toasts.push(std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION), std::make_format_args(path)));
            logger.info(std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION, anm2ed::ENGLISH),
                                     std::make_format_args(path)));
          }
          else
          {
            toasts.push(
                std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED), std::make_format_args(path)));
            logger.error(std::vformat(localize.get(TOAST_EXPORT_RENDERED_ANIMATION_FAILED, anm2ed::ENGLISH),
                                      std::make_format_args(path)));
          }
        }

        renderFrames.clear();

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

        if (settings.timelineIsSound) audioStream.capture_end(mixer);

        playback.isPlaying = false;
        playback.isFinished = false;
        manager.isRecording = false;
        manager.progressPopup.close();
      }
      else
      {

        bind();
        auto pixels = pixels_get();
        renderFrames.push_back(Texture(pixels.data(), size));
      }
    }

    if (playback.isPlaying)
    {
      auto animation = document.animation_get();
      auto& isSound = settings.timelineIsSound;
      auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;

      if (!anm2.content.sounds.empty() && isSound)
      {
        if (auto animation = document.animation_get();
            animation && animation->triggers.isVisible && (!isOnlyShowLayers || manager.isRecording))
        {
          if (auto trigger = animation->triggers.frame_generate(playback.time, anm2::TRIGGER); trigger.isVisible)
            if (anm2.content.sounds.contains(trigger.soundID))
              anm2.content.sounds[trigger.soundID].audio.play(false, mixer);
        }
      }

      playback.tick(anm2.info.fps, animation->frameNum,
                    (animation->isLoop || settings.playbackIsLoop) && !manager.isRecording);

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

    if (ImGui::Begin(localize.get(LABEL_ANIMATION_PREVIEW_WINDOW), &settings.windowIsAnimationPreview))
    {
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

        ImGui::InputFloat(localize.get(BASIC_ALPHA), &overlayTransparency, 0, 0, "%.0f");
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

        if (settings.timelineIsSound) audioStream.capture_begin(mixer);

        if (settings.renderIsRawAnimation)
        {
          settings.previewBackgroundColor = vec4();
          settings.previewIsGrid = false;
          settings.previewIsAxes = false;
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
        playback.isPlaying = true;
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

              auto layerModel = math::quad_model_get(frame.size, frame.position, frame.pivot,
                                                     math::percent_to_unit(frame.scale), frame.rotation);
              auto layerTransform = sampleTransform * layerModel;

              auto uvMin = frame.crop / texSize;
              auto uvMax = (frame.crop + frame.size) / texSize;
              auto rootFrame = animation->rootAnimation.frame_generate(sampleTime, anm2::ROOT);
              vec3 frameColorOffset = frame.colorOffset + rootFrame.colorOffset + colorOffset + sampleColor;
              vec4 frameTint = frame.tint * rootFrame.tint;
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
        auto useTool = tool;
        auto step = isMod ? canvas::STEP_FAST : canvas::STEP;
        mousePos = position_translate(zoom, pan, to_vec2(ImGui::GetMousePos()) - to_vec2(cursorScreenPos));

        if (isMouseMiddleDown) useTool = tool::PAN;
        if (tool == tool::MOVE && isMouseRightDown) useTool = tool::SCALE;
        if (tool == tool::SCALE && isMouseRightDown) useTool = tool::MOVE;

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
            if (!frame) break;
            if (isBegin)
            {
              document.snapshot(localize.get(EDIT_FRAME_POSITION));
              if (isMouseClicked)
              {
                moveOffset = settings.inputIsMoveToolSnapToMouse ? vec2() : mousePos - frame->position;
                isMoveDragging = true;
              }
            }
            if (isMouseDown && isMoveDragging) frame->position = ivec2(mousePos - moveOffset);
            if (isLeftPressed) frame->position.x -= step;
            if (isRightPressed) frame->position.x += step;
            if (isUpPressed) frame->position.y -= step;
            if (isDownPressed) frame->position.y += step;
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
            if (!frame) break;
            if (isBegin) document.snapshot(localize.get(EDIT_FRAME_SCALE));
            if (isMouseDown)
            {
              frame->scale += vec2(mouseDelta.x, mouseDelta.y);
              if (isMod) frame->scale = {frame->scale.x, frame->scale.x};
            }
            if (isLeftPressed) frame->scale.x -= step;
            if (isRightPressed) frame->scale.x += step;
            if (isUpPressed) frame->scale.y -= step;
            if (isDownPressed) frame->scale.y += step;

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
            if (!frame) break;
            if (isBegin) document.snapshot(localize.get(EDIT_FRAME_ROTATION));
            if (isMouseDown) frame->rotation += mouseDelta.y;
            if (isLeftPressed || isDownPressed) frame->rotation -= step;
            if (isUpPressed || isRightPressed) frame->rotation += step;

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
    ImGui::End();

    manager.progressPopup.trigger();

    if (ImGui::BeginPopupModal(manager.progressPopup.label(), &manager.progressPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      if (!animation) return;

      auto& start = manager.recordingStart;
      auto& end = manager.recordingEnd;
      auto progress = (playback.time - start) / (end - start);

      ImGui::ProgressBar(progress);

      ImGui::TextUnformatted(localize.get(TEXT_RECORDING_PROGRESS));

      if (ImGui::Button(localize.get(BASIC_CANCEL), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
      {
        renderFrames.clear();

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
  }
}
