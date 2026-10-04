#include "context.hpp"

namespace anm2ed::imgui
{
  constexpr StringType FRAME_PROPERTY_LABELS[] = {
      BASIC_POSITION, BASIC_SCALE,  BASIC_ROTATION, BASIC_SHEAR,        BASIC_PIVOT,   BASIC_CROP,
      BASIC_SIZE,     BASIC_REGION, BASIC_TINT,     BASIC_COLOR_OFFSET, BASIC_VISIBLE, BASIC_INTERPOLATED,
      BASIC_DURATION, BASIC_SHADER, BASIC_EVENT,    LABEL_SOUNDS};
  static_assert(std::size(FRAME_PROPERTY_LABELS) == (std::size_t)edit::FrameProperty::COUNT);

  void TimelineContext::frame_begin()
  {
    iconTintDefault = isLightTheme ? ICON_TINT_DEFAULT_LIGHT : ICON_TINT_DEFAULT_DARK;
    itemIconTint = isLightTheme ? ICON_TINT_DEFAULT_LIGHT : iconTintDefault;
    frameBorderColor = isLightTheme ? FRAME_BORDER_COLOR_LIGHT : FRAME_BORDER_COLOR_DARK;
    frameBorderColorReferenced =
        isLightTheme ? FRAME_BORDER_COLOR_REFERENCED_LIGHT : FRAME_BORDER_COLOR_REFERENCED_DARK;
    frameMultipleOverlayColor = isLightTheme ? FRAME_MULTIPLE_OVERLAY_COLOR_LIGHT : FRAME_MULTIPLE_OVERLAY_COLOR_DARK;
    textMultipleColor = isLightTheme ? TEXT_MULTIPLE_COLOR_LIGHT : TEXT_MULTIPLE_COLOR_DARK;
    playheadLineColor = isLightTheme ? PLAYHEAD_LINE_COLOR_LIGHT : PLAYHEAD_LINE_COLOR_DARK;
    playheadIconTint = isLightTheme ? PLAYHEAD_ICON_TINT_LIGHT : iconTintDefault;
    timelineBackgroundColor =
        isLightTheme ? TIMELINE_BACKGROUND_COLOR_LIGHT : ImGui::GetStyleColorVec4(ImGuiCol_Header);
    timelinePlayheadRectColor = isLightTheme ? TIMELINE_PLAYHEAD_RECT_COLOR_LIGHT : TIMELINE_PLAYHEAD_RECT_COLOR_DARK;
    timelineTickColor = isLightTheme ? TIMELINE_TICK_COLOR_LIGHT : frameBorderColor;
    itemTextColor = isLightTheme ? ITEM_TEXT_COLOR_LIGHT : ITEM_TEXT_COLOR_DARK;
  }

  void TimelineContext::overlay_icon(GLuint textureId, ImVec4 tint, bool isForced)
  {
    if (!isForced && !isLightTheme) return;
    auto min = ImGui::GetItemRectMin();
    auto max = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)textureId, min, max, ImVec2(0, 0), ImVec2(1, 1),
                                         ImGui::GetColorU32(tint));
  }

  void TimelineContext::playback_stop()
  {
    playback.isPlaying = false;
    playback.isFinished = false;
    playback.timing_reset();
  }

  void TimelineContext::context_menu()
  {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

    if (shortcut(manager.chords[SHORTCUT_CUT], shortcut::FOCUSED)) cut();
    if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
    if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste();
    if (shortcut(manager.chords[SHORTCUT_SPLIT], shortcut::FOCUSED)) frame_split();
    if (shortcut(manager.chords[SHORTCUT_BAKE], shortcut::FOCUSED)) frames_bake();
    if (shortcut(manager.chords[SHORTCUT_FIT], shortcut::FOCUSED)) fit_animation_length();

    auto make_region = [&]()
    {
      auto targetReference = reference;
      auto frame = frame_get();
      if (!frame || targetReference.itemType != LAYER || targetReference.itemID == -1) return;
      if (frame->regionId != -1) return;
      auto layer = model::item_get(model.content.layers, targetReference.itemID);
      if (!layer) return;

      auto spritesheetID = layer->spritesheetId;
      if (!model::item_get(model.content.spritesheets, spritesheetID)) return;

      auto settingsPtr = &settings;
      command_push(
          [=, this](Manager& manager, Document& document)
          {
            auto frame = document.model.frame_get(targetReference);
            if (!frame || frame->regionId != -1) return;
            auto layer = model::item_get(document.model.content.layers, targetReference.itemID);
            if (!layer) return;

            auto spritesheetID = layer->spritesheetId;
            if (!model::item_get(document.model.content.spritesheets, spritesheetID)) return;

            model::Region region{.crop = frame->crop, .size = frame->size, .pivot = frame->pivot};

            document.focused_id_set(SelectionKind::SPRITESHEETS, spritesheetID);
            settingsPtr->windowIsRegions = true;
            manager.makeRegionSpritesheetId = spritesheetID;
            manager.makeRegion = region;
            manager.isMakeRegionRequested = true;
          });
    };

    auto item = selected_item_get();
    auto selectedFrames = document.frame_references_get(Document::FrameReferenceFallback::NONE);
    auto copyFrames = copy_frame_references_get();
    auto selectedBakeFrames = selectedFrames;
    std::erase_if(selectedBakeFrames, is_trigger_reference);
    auto isReverseFrames = is_frames_reverse_available(selectedFrames);
    auto is_region_makeable = [&](const Reference& frameReference)
    {
      if (frameReference.itemType != LAYER || frameReference.frameIndex < 0) return false;
      auto frame = document.model.frame_get(frameReference);
      auto layer = model::item_get(model.content.layers, frameReference.itemID);
      return frame && frame->regionId == -1 && layer &&
             model::item_get(model.content.spritesheets, layer->spritesheetId);
    };
    auto selectedRegionFrames = selectedFrames;
    std::erase_if(selectedRegionFrames,
                  [&](const Reference& frameReference) { return !is_region_makeable(frameReference); });
    bool isMakeManyRegions = selectedRegionFrames.size() > 1;
    auto selectedRootFrames = selected_root_frame_references_get();
    bool isMakeRegion = is_region_makeable(reference);
    Actions actions{};
    actions_undo_redo_add(actions, manager, document);
    actions.separator();
    actions.add({.label = playback.isPlaying ? LABEL_PAUSE : LABEL_PLAY,
                 .shortcut = SHORTCUT_PLAY_PAUSE,
                 .isEnabled = []() { return true; },
                 .run = [&]() { playback.toggle(); }});
    actions.add({.label = LABEL_INSERT,
                 .shortcut = SHORTCUT_INSERT_FRAME,
                 .isEnabled = [&]() { return item; },
                 .run = [&]() { frame_insert(); }});
    actions.add(ACTION_DUPLICATE, [=, this]() { return !selectedBakeFrames.empty(); }, [&]() { frames_duplicate(); });
    actions.add({.label = LABEL_SPLIT,
                 .shortcut = SHORTCUT_SPLIT,
                 .isEnabled = [=, this]() { return selectedBakeFrames.size() == 1; },
                 .run = [&]() { frame_split(); }});
    actions.add(ACTION_REVERSE, [=, this]() { return isReverseFrames; }, [&]() { frames_reverse(); });
    actions.separator();
    actions.add({.label = LABEL_BAKE,
                 .shortcut = SHORTCUT_BAKE,
                 .isEnabled = [=, this]() { return !selectedBakeFrames.empty(); },
                 .run = [&]() { frames_bake(); }});
    actions.add({.label = LABEL_BAKE_INTO_OTHER_FRAMES,
                 .shortcut = -1,
                 .isEnabled = [=, this]() { return !selectedRootFrames.empty(); },
                 .run = [&]() { bakeIntoOtherFramesPopup.open(); }});
    actions.add(
        {.label = LABEL_FIT_ANIMATION_LENGTH,
         .shortcut = SHORTCUT_FIT,
         .isEnabled = [&]() { return animation && animation->frameNum != model::animation_length_get(*animation); },
         .run = [&]() { fit_animation_length(); }});
    actions.separator();
    actions.add({.label = isMakeManyRegions ? LABEL_MAKE_MANY_REGIONS : LABEL_MAKE_REGION,
                 .shortcut = -1,
                 .isEnabled = [=, this]() { return isMakeManyRegions || isMakeRegion; },
                 .run =
                     [&]()
                 {
                   if (!isMakeManyRegions) return make_region();
                   makeManyRegionReferences = selectedRegionFrames;
                   makeManyRegionsPopup.open();
                 }});
    actions.separator();
    actions.add({.label = LABEL_DELETE,
                 .shortcut = SHORTCUT_REMOVE,
                 .isEnabled = [=, this]() { return !selectedFrames.empty(); },
                 .run = [&]() { frames_delete_action(); }});
    actions.separator();
    // Cut, copy and paste each open onto the whole frame, or one property of it.
    auto source = property_source_get();
    auto sourceFrame = source ? model.frame_get(*source) : nullptr;
    auto clipboardFrame = property_clipboard_frame_get();
    auto clipboard_menu_add = [&](ActionType type, std::function<bool()> isFrameEnabled, std::function<void()> frameRun,
                                  int propertyType, std::function<bool(edit::FrameProperty)> is_property_enabled,
                                  std::function<void(edit::FrameProperty)> property_run)
    {
      Action properties{.label = LABEL_PROPERTY};
      for (int i = 0; i < (int)edit::FrameProperty::COUNT; ++i)
      {
        auto property = (edit::FrameProperty)i;
        if (!edit::is_frame_property_valid(property, (ItemType)propertyType)) continue;
        properties.children.push_back({.label = FRAME_PROPERTY_LABELS[i],
                                       .isEnabled = [=]() { return is_property_enabled(property); },
                                       .run = [=]() { property_run(property); }});
      }
      properties.isEnabled = [children = properties.children]()
      { return std::ranges::any_of(children, is_action_enabled); };
      actions.add({.label = ACTION_INFOS[type].label,
                   .children = {{.label = BASIC_FRAME,
                                 .shortcut = ACTION_INFOS[type].shortcut,
                                 .isEnabled = isFrameEnabled,
                                 .run = frameRun},
                                properties}});
    };
    auto sourceType = source ? source->itemType : NONE;
    clipboard_menu_add(
        ACTION_CUT, [=, this]() { return !selectedFrames.empty(); }, [this]() { cut(); }, sourceType,
        [=](edit::FrameProperty) { return sourceFrame != nullptr; },
        [this](edit::FrameProperty property) { property_cut(property); });
    clipboard_menu_add(
        ACTION_COPY, [=, this]() { return !copyFrames.empty(); }, [this]() { copy(); }, sourceType,
        [=](edit::FrameProperty) { return sourceFrame != nullptr; },
        [this](edit::FrameProperty property) { property_copy(property); });
    clipboard_menu_add(
        ACTION_PASTE, [&]() { return !clipboard.is_empty(); }, [this]() { paste(); },
        clipboardFrame ? frameClipboard.itemType : NONE,
        [this](edit::FrameProperty property) { return is_property_pasteable(property); },
        [this](edit::FrameProperty property) { property_paste(property); });
    // The numbered row has its own (marker) menu.
    if (isRulerHovered)
      actions_popup_draw("##Context Menu", actions, settings);
    else
      actions_context_window_draw("##Context Menu", actions, settings, ImGuiPopupFlags_MouseButtonRight,
                                  ImGuiHoveredFlags_ChildWindows);

    ImGui::PopStyleVar(2);
  }

  void TimelineContext::item_base_properties_open(int type, int id)
  {
    manager.item_properties_open(type == LAYER ? ElementType::LAYER_ELEMENT : ElementType::NULL_ELEMENT, id);
  }

  void TimelineContext::group_properties_open(const TimelineRow& row, const model::TrackGroup& group)
  {
    groupName = group.name.empty() ? std::string(localize.get(TEXT_NEW_GROUP)) : group.name;
    groupAnimationIndex = reference.animationIndex;
    groupType = row.type;
    groupId = row.id;
    groupPropertiesPopup.open();
  }

  void TimelineContext::group_properties_update()
  {
    groupPropertiesPopup.trigger();

    if (ImGui::BeginPopupModal(groupPropertiesPopup.label(), &groupPropertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto childSize = child_size_get(1);
      if (ImGui::BeginChild("##Group Properties Child", childSize, ImGuiChildFlags_Borders))
      {
        if (groupPropertiesPopup.isJustOpened) ImGui::SetKeyboardFocusHere();
        input_text_string(localize.get(BASIC_NAME), &groupName);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_NAME));
      }
      ImGui::EndChild();

      auto result = popup_buttons_draw(manager, localize.get(BASIC_CONFIRM));
      if (result == PopupButton::CONFIRM)
      {
        auto targetName = groupName;
        auto targetAnimationIndex = groupAnimationIndex;
        auto targetType = groupType;
        auto targetId = groupId;
        edit_push(EDIT_RENAME_GROUP,
                  [=](model::Model& model)
                  {
                    if (auto group = model.track_group_edit(targetAnimationIndex, targetType, targetId))
                      group->name = targetName;
                  });
      }
      if (result != PopupButton::NONE) groupPropertiesPopup.close();

      ImGui::EndPopup();
    }

    groupPropertiesPopup.end();
  }

  void TimelineContext::item_context_menu()
  {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

    auto& type = reference.itemType;
    auto& id = reference.itemID;
    auto item = selected_item_get();
    auto selectedRows = selected_row_references_get();
    auto selectedGroupableItems = item_references_groupable_get();
    auto copyFrames = copy_frame_references_get();
    TimelineRow selectedGroupRow{};
    const model::TrackGroup* selectedGroup{};
    if (selectedRows.size() == 1 && selectedRows.front().isGroup)
    {
      auto row = selectedRows.front();
      selectedGroupRow = row;
      selectedGroup = row_group_get(selectedGroupRow);
    }
    auto isRemoveAvailable = std::ranges::any_of(selectedRows, [](const TimelineRow& row)
                                                 { return row.type == LAYER || row.type == NULL_; });
    auto item_cut = [&]()
    {
      if (copyFrames.empty() || !isRemoveAvailable) return;
      frame_references_copy(copyFrames);
      item_remove();
    };

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByWindow) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right))
      ImGui::OpenPopup("##Items Context Menu");

    Actions actions{};
    actions_undo_redo_add(actions, manager, document);
    actions.separator();
    actions.add(
        ACTION_PROPERTIES, [&]() { return selectedGroup || (item && (type == LAYER || type == NULL_)); },
        [&]()
        {
          if (selectedGroup)
            group_properties_open(selectedGroupRow, *selectedGroup);
          else
            item_base_properties_open(type, id);
        });
    actions.add(ACTION_ADD, [&]() { return animation; }, [&]() { itemProperties.open(); });
    actions.add(ACTION_REMOVE, [=, this]() { return isRemoveAvailable; }, [&]() { item_remove(); });
    actions.add(ACTION_GROUP, [=, this]() { return !selectedGroupableItems.empty(); }, [&]() { item_group(); });
    actions.separator();
    actions.add(ACTION_CUT, [=, this]() { return !copyFrames.empty() && isRemoveAvailable; }, item_cut);
    actions.add(ACTION_COPY, [=, this]() { return !copyFrames.empty(); }, [this]() { copy(); });
    actions.add(ACTION_PASTE, [&]() { return item && !clipboard.is_empty(); }, [this]() { paste(); });
    if (shortcut(manager.chords[SHORTCUT_CUT], shortcut::FOCUSED) && !copyFrames.empty() && isRemoveAvailable)
      item_cut();
    if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED) && !copyFrames.empty()) copy();
    if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED) && item && !clipboard.is_empty()) paste();
    actions_popup_draw("##Items Context Menu", actions, settings);

    ImGui::PopStyleVar(2);
  }
}
