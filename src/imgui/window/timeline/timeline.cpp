#include "context.hpp"

namespace anm2ed::imgui
{
  Timeline::Timeline() = default;
  Timeline::~Timeline() = default;

  TimelineContext::TimelineContext(TimelineState&& state, Manager& manager, Settings& settings, Resources& resources,
                                   Clipboard& clipboard)
      : TimelineState(std::move(state)), manager(manager), settings(settings), resources(resources),
        clipboard(clipboard), document(*manager.get()), model(document.model), playback(document.playback),
        reference(document.reference_get())
  {
  }

  void TimelineContext::update()
  {
    animation = model.animation_get(reference.animationIndex);
    rowFrameChildHeight = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 1.5f;

    style = ImGui::GetStyle();
    isLightTheme = settings.theme == theme::LIGHT;
    isTextPushed = false;
    if (isLightTheme)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, TIMELINE_TEXT_COLOR_LIGHT);
      isTextPushed = true;
    }

    frame_begin();
    draw();
  }

  void Timeline::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    if (!manager.get()) return;
    auto state = context ? std::move(static_cast<TimelineState&>(*context)) : TimelineState{};
    context = std::make_unique<TimelineContext>(std::move(state), manager, settings, resources, clipboard);
    context->update();
  }

  void TimelineContext::draw()
  {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
    if (ImGui::Begin(localize.get(LABEL_TIMELINE_WINDOW), &settings.windowIsTimeline))
    {
      isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows |
                                               ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
      frames_child();
      items_child();
    }
    ImGui::PopStyleVar();
    ImGui::End();

    if (itemProperties.update(manager, settings, document, reference)) group_selection_reset_for(document);
    group_properties_update();

    popups_update();
    shortcuts_update();

    if (isTextPushed) ImGui::PopStyleColor();
  }

  void TimelineContext::popups_update()
  {
    makeManyRegionsPopup.trigger();
    if (ImGui::BeginPopupModal(makeManyRegionsPopup.label(), &makeManyRegionsPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      input_text_string(localize.get(LABEL_FORMAT), &settings.generateRegionNameFormat);
      ImGui::Checkbox(localize.get(LABEL_MAP_FRAMES_TO_REGIONS), &isMakeManyRegionsMapFrames);

      auto result = popup_buttons_draw(manager, localize.get(LABEL_MAKE_MANY_REGIONS));
      if (result == PopupButton::CONFIRM)
        edit_push(EDIT_GENERATE_REGIONS_FROM_ANIMATIONS,
                  [targetFrames = makeManyRegionReferences, format = settings.generateRegionNameFormat,
                   mapping = isMakeManyRegionsMapFrames ? RegionFrameMapping::SET
                                                        : RegionFrameMapping::PRESERVE](model::Model& model)
                  { return edit::regions_generate(model, {}, targetFrames, format, mapping); });
      if (result != PopupButton::NONE) makeManyRegionsPopup.close();

      ImGui::EndPopup();
    }
    makeManyRegionsPopup.end();

    bakePopup.trigger();

    if (ImGui::BeginPopupModal(bakePopup.label(), &bakePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto& interval = settings.bakeInterval;
      auto& isRoundRotation = settings.bakeIsRoundRotation;
      auto& isRoundScale = settings.bakeIsRoundScale;

      auto frame = frame_get();

      input_int_range(localize.get(LABEL_INTERVAL), interval, FRAME_DURATION_MIN,
                      frame ? frame->duration : FRAME_DURATION_MIN);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_INTERVAL));

      ImGui::Checkbox(localize.get(LABEL_ROUND_ROTATION), &isRoundRotation);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROUND_ROTATION));

      ImGui::Checkbox(localize.get(LABEL_ROUND_SCALE), &isRoundScale);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROUND_SCALE));

      auto widgetSize = widget_size_with_row_get(2);

      shortcut(manager.chords[SHORTCUT_CONFIRM]);
      if (ImGui::Button(localize.get(LABEL_BAKE), widgetSize))
      {
        frames_bake();
        bakePopup.close();
      }
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BAKE_FRAMES_OPTIONS));

      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_CANCEL]);
      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) bakePopup.close();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CANCEL_BAKE_FRAMES));

      ImGui::EndPopup();
    }

    bakeIntoOtherFramesPopup.trigger();

    if (ImGui::BeginPopupModal(bakeIntoOtherFramesPopup.label(), &bakeIntoOtherFramesPopup.isOpen,
                               ImGuiWindowFlags_NoResize))
    {
      auto& isRoundRotation = settings.bakeIsRoundRotation;
      auto& isRoundScale = settings.bakeIsRoundScale;
      auto& isMatchRootInterpolation = settings.bakeIsMatchRootInterpolation;
      auto& isUseRootPivot = settings.bakeIsUseRootPivot;
      int target = (int)bakeIntoOtherFramesTarget;
      ImGui::RadioButton(localize.get(LABEL_CURRENT_SELECTION), &target,
                         (int)BakeIntoOtherFramesTarget::CURRENT_SELECTION);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BAKE_INTO_OTHER_FRAMES_CURRENT_SELECTION));
      ImGui::SameLine();
      ImGui::RadioButton(localize.get(LABEL_ALL), &target, (int)BakeIntoOtherFramesTarget::ALL);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BAKE_INTO_OTHER_FRAMES_ALL));
      bakeIntoOtherFramesTarget = (BakeIntoOtherFramesTarget)target;

      auto isCurrentSelection = bakeIntoOtherFramesTarget == BakeIntoOtherFramesTarget::CURRENT_SELECTION;
      ImGui::BeginDisabled(isCurrentSelection);
      ImGui::Checkbox(localize.get(LABEL_LAYERS), &isBakeIntoOtherFramesLayers);
      ImGui::SameLine();
      ImGui::Checkbox(localize.get(LABEL_NULLS), &isBakeIntoOtherFramesNulls);
      ImGui::EndDisabled();

      ImGui::Checkbox(localize.get(LABEL_ROUND_ROTATION), &isRoundRotation);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROUND_ROTATION));

      ImGui::Checkbox(localize.get(LABEL_ROUND_SCALE), &isRoundScale);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROUND_SCALE));

      ImGui::Checkbox(localize.get(LABEL_BAKE_MATCH_ROOT_INTERPOLATION), &isMatchRootInterpolation);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BAKE_MATCH_ROOT_INTERPOLATION));

      ImGui::Checkbox(localize.get(LABEL_BAKE_USE_ROOT_PIVOT), &isUseRootPivot);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BAKE_USE_ROOT_PIVOT));

      auto widgetSize = widget_size_with_row_get(2);

      shortcut(manager.chords[SHORTCUT_CONFIRM]);
      ImGui::BeginDisabled(!is_bake_into_other_frames_ready());
      if (ImGui::Button(localize.get(LABEL_BAKE), widgetSize))
      {
        bake_into_other_frames();
        bakeIntoOtherFramesPopup.close();
      }
      ImGui::EndDisabled();

      ImGui::SameLine();

      shortcut(manager.chords[SHORTCUT_CANCEL]);
      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) bakeIntoOtherFramesPopup.close();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CANCEL_BAKE_FRAMES));

      ImGui::EndPopup();
    }
  }

  // Global timeline shortcuts: playback, playhead, frame resizing and frame/item navigation.
  void TimelineContext::shortcuts_update()
  {
    if (!animation) return;

    if (shortcut(manager.chords[SHORTCUT_PLAY_PAUSE], shortcut::GLOBAL)) playback.toggle();
    if (shortcut(manager.chords[SHORTCUT_START_MARKER], shortcut::GLOBAL)) marker_set(true, (int)playback.time);
    if (shortcut(manager.chords[SHORTCUT_END_MARKER], shortcut::GLOBAL)) marker_set(false, (int)playback.time);

    if (shortcut(manager.chords[SHORTCUT_MOVE_PLAYHEAD_BACK], shortcut::GLOBAL))
    {
      playback_stop();
      playback.decrement(settings.playbackIsClamp ? animation->frameNum : FRAME_NUM_MAX);
      document.frameTime = playback.time;
    }

    if (shortcut(manager.chords[SHORTCUT_MOVE_PLAYHEAD_FORWARD], shortcut::GLOBAL))
    {
      playback_stop();
      playback.increment(settings.playbackIsClamp ? animation->frameNum : FRAME_NUM_MAX);
      document.frameTime = playback.time;
    }

    struct FrameResize
    {
      int shortcut;
      StringType edit;
      int delta;
    };
    constexpr FrameResize FRAME_RESIZES[] = {{SHORTCUT_SHORTEN_FRAME, EDIT_SHORTEN_FRAME, -1},
                                             {SHORTCUT_EXTEND_FRAME, EDIT_EXTEND_FRAME, 1}};
    for (int i = 0; i < (int)std::size(FRAME_RESIZES); ++i)
    {
      auto resize = FRAME_RESIZES[i];
      auto isPressed = shortcut(manager.chords[resize.shortcut], shortcut::GLOBAL);
      auto selectedFrames =
          isPressed ? document.frame_references_get(Document::FrameReferenceFallback::NONE) : std::set<Reference>{};
      std::erase_if(selectedFrames, is_trigger_reference);
      if (!selectedFrames.empty())
      {
        if (resizeChordTabIds[i] != document.tabId) edit_begin_push(resize.edit);
        command_push(
            [=, this](Manager&, Document& document)
            {
              for (auto frameReference : selectedFrames)
                if (auto frame = document.model.frame_edit(frameReference))
                  frame->duration = std::clamp(frame->duration + resize.delta, FRAME_DURATION_MIN, FRAME_DURATION_MAX);
              document.change();
            });
      }
      resizeChordTabIds[i] = isPressed ? document.tabId : 0;
    }

    auto isPreviousFrame = shortcut(manager.chords[SHORTCUT_PREVIOUS_FRAME], shortcut::GLOBAL);
    auto isNextFrame = shortcut(manager.chords[SHORTCUT_NEXT_FRAME], shortcut::GLOBAL);
    auto isPreviousItem = shortcut(manager.chords[SHORTCUT_PREVIOUS_ITEM], shortcut::GLOBAL);
    auto isNextItem = shortcut(manager.chords[SHORTCUT_NEXT_ITEM], shortcut::GLOBAL);

    if (isPreviousFrame || isNextFrame)
      if (auto item = selected_item_get(); item && !item->frames.empty())
      {
        auto frameReference = reference;
        frameReference.frameIndex =
            glm::clamp(reference.frameIndex + (int)isNextFrame - (int)isPreviousFrame, 0, (int)item->frames.size() - 1);
        reference_set(frameReference);
        document.frame_focus_select();
        document.frameTime = model::frame_time_from_index_get(*item, reference.frameIndex);
      }

    if (isPreviousItem) reference_set_adjacent_item(-1);
    if (isNextItem) reference_set_adjacent_item(1);
  }
}
