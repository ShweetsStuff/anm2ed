#include "timeline.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <imgui_internal.h>

#include "log.h"
#include "math_.h"
#include "toast.h"

#include "vector_.h"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;
using namespace glm;

namespace anm2ed::imgui
{
  constexpr auto COLOR_HIDDEN_MULTIPLIER = vec4(0.5f, 0.5f, 0.5f, 1.000f);
  constexpr auto FRAME_BORDER_COLOR_DARK = ImVec4(1.0f, 1.0f, 1.0f, 0.15f);
  constexpr auto FRAME_BORDER_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 0.25f);
  constexpr auto FRAME_BORDER_COLOR_REFERENCED_DARK = ImVec4(1.0f, 1.0f, 1.0f, 0.50f);
  constexpr auto FRAME_BORDER_COLOR_REFERENCED_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
  constexpr auto FRAME_BORDER_THICKNESS = 1.0f;
  constexpr auto FRAME_BORDER_THICKNESS_REFERENCED = 2.0f;
  constexpr auto FRAME_MULTIPLE_OVERLAY_COLOR_DARK = ImVec4(1.0f, 1.0f, 1.0f, 0.05f);
  constexpr auto FRAME_MULTIPLE_OVERLAY_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 0.1f);
  constexpr auto FRAME_ROUNDING = 3.0f;
  constexpr auto ICON_TINT_DEFAULT_DARK = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  constexpr auto ICON_TINT_DEFAULT_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  constexpr auto ITEM_TEXT_COLOR_DARK = to_imvec4(color::WHITE);
  constexpr auto ITEM_TEXT_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  constexpr auto PLAYHEAD_ICON_TINT_LIGHT = ImVec4(0.3922f, 0.7843f, 0.5882f, 1.0f);
  constexpr auto PLAYHEAD_LINE_COLOR_DARK = to_imvec4(color::WHITE);
  constexpr auto PLAYHEAD_LINE_COLOR_LIGHT = ImVec4(0.1961f, 0.5882f, 0.3922f, 1.0f);
  constexpr auto PLAYHEAD_LINE_THICKNESS = 4.0f;
  constexpr auto TEXT_MULTIPLE_COLOR_DARK = to_imvec4(color::WHITE);
  constexpr auto TEXT_MULTIPLE_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  constexpr auto TIMELINE_BACKGROUND_COLOR_LIGHT = ImVec4(0.5490f, 0.5490f, 0.5882f, 1.0f);
  constexpr auto TIMELINE_PLAYHEAD_RECT_COLOR_DARK = ImVec4(0.60f, 0.45f, 0.30f, 1.0f);
  constexpr auto TIMELINE_PLAYHEAD_RECT_COLOR_LIGHT = ImVec4(0.8353f, 0.8353f, 0.7294f, 1.0f);
  constexpr auto TIMELINE_TICK_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  constexpr auto TIMELINE_CHILD_BG_COLOR_LIGHT = ImVec4(0.5490f, 0.5490f, 0.5882f, 1.0f);
  constexpr auto TIMELINE_TEXT_COLOR_LIGHT = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

  constexpr glm::vec4 FRAME_COLOR_LIGHT_BASE[] = {{0.80f, 0.80f, 0.80f, 1.0f},
                                                  {0.5216f, 0.7333f, 1.0f, 1.0f},
                                                  {1.0f, 0.9961f, 0.5882f, 1.0f},
                                                  {0.6157f, 1.0f, 0.5882f, 1.0f},
                                                  {1.0f, 0.5882f, 0.8314f, 1.0f}};
  constexpr glm::vec4 FRAME_COLOR_LIGHT_ACTIVE[] = {{0.74f, 0.74f, 0.74f, 1.0f},
                                                    {0.0980f, 0.3765f, 0.6431f, 1.0f},
                                                    {1.0f, 0.5255f, 0.3333f, 1.0f},
                                                    {0.3686f, 0.5765f, 0.2353f, 1.0f},
                                                    {0.6118f, 0.2039f, 0.2745f, 1.0f}};
  constexpr glm::vec4 FRAME_COLOR_LIGHT_HOVERED[] = {{0.84f, 0.84f, 0.84f, 1.0f},
                                                     {0.0752f, 0.2887f, 0.4931f, 1.0f},
                                                     {0.85f, 0.4467f, 0.2833f, 1.0f},
                                                     {0.2727f, 0.4265f, 0.1741f, 1.0f},
                                                     {0.4618f, 0.1539f, 0.2072f, 1.0f}};
  constexpr glm::vec4 ITEM_COLOR_LIGHT_BASE[] = {{0.3059f, 0.3255f, 0.5412f, 1.0f},
                                                 {0.3333f, 0.5725f, 0.8392f, 1.0f},
                                                 {1.0f, 0.5412f, 0.3412f, 1.0f},
                                                 {0.5255f, 0.8471f, 0.4588f, 1.0f},
                                                 {0.7961f, 0.3882f, 0.5412f, 1.0f}};
  constexpr glm::vec4 ITEM_COLOR_LIGHT_ACTIVE[] = {{0.3459f, 0.3655f, 0.5812f, 1.0f},
                                                   {0.3733f, 0.6125f, 0.8792f, 1.0f},
                                                   {0.9106f, 0.4949f, 0.2753f, 1.0f},
                                                   {0.5655f, 0.8871f, 0.4988f, 1.0f},
                                                   {0.8361f, 0.4282f, 0.5812f, 1.0f}};
  constexpr glm::vec4 ITEM_COLOR_LIGHT_SELECTED[] = {{0.74f, 0.74f, 0.74f, 1.0f},
                                                     {0.2039f, 0.4549f, 0.7176f, 1.0f},
                                                     {0.8745f, 0.4392f, 0.2275f, 1.0f},
                                                     {0.3765f, 0.6784f, 0.2980f, 1.0f},
                                                     {0.6353f, 0.2235f, 0.3647f, 1.0f}};

  constexpr auto FRAME_MULTIPLE = 5;
  constexpr auto FRAME_DRAG_PAYLOAD_ID = "Frame Drag Drop";
  constexpr auto FRAME_TOOLTIP_HOVER_DELAY = 0.75f; // Extra delay for frame info tooltip.

#define ITEM_FRAME_CHILD_HEIGHT ImGui::GetTextLineHeightWithSpacing() + (ImGui::GetStyle().WindowPadding.y * 1.5)
#define ITEM_CHILD_WIDTH ImGui::GetTextLineHeightWithSpacing() * 12.5

  void Timeline::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& playback = document.playback;
    auto& reference = document.reference;
    auto& frames = document.frames;
    auto animation = document.animation_get();

    style = ImGui::GetStyle();
    auto isLightTheme = settings.theme == theme::LIGHT;
    bool isTextPushed = false;
    if (isLightTheme)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, TIMELINE_TEXT_COLOR_LIGHT);
      isTextPushed = true;
    }

    auto type_index = [](anm2::Type type) { return std::clamp((int)type, 0, (int)anm2::TRIGGER); };

    auto type_color_base_vec = [&](anm2::Type type)
    { return isLightTheme ? FRAME_COLOR_LIGHT_BASE[type_index(type)] : anm2::TYPE_COLOR[type]; };

    auto type_color_active_vec = [&](anm2::Type type)
    {
      if (isLightTheme) return FRAME_COLOR_LIGHT_ACTIVE[type_index(type)];
      return anm2::TYPE_COLOR_ACTIVE[type];
    };

    auto type_color_hovered_vec = [&](anm2::Type type)
    {
      if (isLightTheme) return FRAME_COLOR_LIGHT_HOVERED[type_index(type)];
      return anm2::TYPE_COLOR_HOVERED[type];
    };

    auto item_color_vec = [&](anm2::Type type)
    {
      if (!isLightTheme) return anm2::TYPE_COLOR[type];
      return ITEM_COLOR_LIGHT_BASE[type_index(type)];
    };

    auto item_color_active_vec = [&](anm2::Type type)
    {
      if (!isLightTheme) return anm2::TYPE_COLOR_ACTIVE[type];
      return ITEM_COLOR_LIGHT_ACTIVE[type_index(type)];
    };

    auto iconTintDefault = isLightTheme ? ICON_TINT_DEFAULT_LIGHT : ICON_TINT_DEFAULT_DARK;
    auto itemIconTint = isLightTheme ? ICON_TINT_DEFAULT_LIGHT : iconTintDefault;
    auto frameBorderColor = isLightTheme ? FRAME_BORDER_COLOR_LIGHT : FRAME_BORDER_COLOR_DARK;
    auto frameBorderColorReferenced =
        isLightTheme ? FRAME_BORDER_COLOR_REFERENCED_LIGHT : FRAME_BORDER_COLOR_REFERENCED_DARK;
    auto frameMultipleOverlayColor =
        isLightTheme ? FRAME_MULTIPLE_OVERLAY_COLOR_LIGHT : FRAME_MULTIPLE_OVERLAY_COLOR_DARK;
    auto textMultipleColor = isLightTheme ? TEXT_MULTIPLE_COLOR_LIGHT : TEXT_MULTIPLE_COLOR_DARK;
    auto playheadLineColor = isLightTheme ? PLAYHEAD_LINE_COLOR_LIGHT : PLAYHEAD_LINE_COLOR_DARK;
    auto playheadIconTint = isLightTheme ? PLAYHEAD_ICON_TINT_LIGHT : iconTintDefault;
    auto timelineBackgroundColor =
        isLightTheme ? TIMELINE_BACKGROUND_COLOR_LIGHT : ImGui::GetStyleColorVec4(ImGuiCol_Header);
    auto timelinePlayheadRectColor =
        isLightTheme ? TIMELINE_PLAYHEAD_RECT_COLOR_LIGHT : TIMELINE_PLAYHEAD_RECT_COLOR_DARK;
    auto timelineTickColor = isLightTheme ? TIMELINE_TICK_COLOR_LIGHT : frameBorderColor;
    auto itemTextColor = isLightTheme ? ITEM_TEXT_COLOR_LIGHT : ITEM_TEXT_COLOR_DARK;

    auto overlay_icon = [&](GLuint textureId, ImVec4 tint, bool isForced = false)
    {
      if (!isForced && !isLightTheme) return;
      auto min = ImGui::GetItemRectMin();
      auto max = ImGui::GetItemRectMax();
      ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)textureId, min, max, ImVec2(0, 0), ImVec2(1, 1),
                                           ImGui::GetColorU32(tint));
    };

    auto frames_selection_set_reference = [&]()
    {
      frames.selection.clear();
      if (reference.frameIndex >= 0) frames.selection.insert(reference.frameIndex);
      frameSelectionSnapshot.assign(frames.selection.begin(), frames.selection.end());
      frameSelectionSnapshotReference = reference;
      frameSelectionLocked.clear();
      isFrameSelectionLocked = false;
      frameFocusIndex = reference.frameIndex;
      frameFocusRequested = reference.frameIndex >= 0;
    };

    auto frame_insert = [&](anm2::Item* item)
    {
      if (!item) return;

      auto behavior = [&, item]()
      {
        if (reference.itemType == anm2::TRIGGER)
        {
          for (auto& trigger : animation->triggers.frames)
            if (document.frameTime == trigger.atFrame) return;

          anm2::Frame addFrame{};
          addFrame.atFrame = document.frameTime;
          item->frames.push_back(addFrame);
          item->frames_sort_by_at_frame();
          reference.frameIndex = item->frame_index_from_at_frame_get(addFrame.atFrame);
        }
        else
        {
          auto frame = document.frame_get();
          if (frame)
          {
            auto addFrame = *frame;
            item->frames.insert(item->frames.begin() + reference.frameIndex, addFrame);
            reference.frameIndex++;
          }
          else if (!item->frames.empty())
          {
            auto addFrame = item->frames.back();
            item->frames.emplace_back(addFrame);
            reference.frameIndex = (int)(item->frames.size()) - 1;
          }
          else
          {
            item->frames.emplace_back(anm2::Frame());
            reference.frameIndex = 0;
          }
        }

        frames_selection_set_reference();
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_INSERT_FRAME), Document::FRAMES, behavior());
    };

    auto frames_delete = [&]()
    {
      if (auto item = animation->item_get(reference.itemType, reference.itemID); item)
      {
        for (auto it = frames.selection.rbegin(); it != frames.selection.rend(); ++it)
        {
          auto i = *it;
          item->frames.erase(item->frames.begin() + i);
        }

        if (item->frames.empty())
          reference.frameIndex = -1;
        else
          reference.frameIndex = glm::clamp(--reference.frameIndex, 0, (int)item->frames.size() - 1);
        frames_selection_set_reference();
      }
    };

    auto frames_delete_action = [&]()
    {
      if (!document.frame_get()) return;
      DOCUMENT_EDIT(document, localize.get(EDIT_DELETE_FRAMES), Document::FRAMES, frames_delete());
    };

    auto frames_bake = [&]()
    {
      auto behavior = [&]()
      {
        if (auto item = document.item_get())
          for (auto i : frames.selection | std::views::reverse)
            item->frames_bake(i, settings.bakeInterval, settings.bakeIsRoundScale, settings.bakeIsRoundRotation);

        frames.clear();
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_BAKE_FRAMES), Document::FRAMES, behavior());
    };

    auto frame_split = [&]()
    {
      auto behavior = [&]()
      {
        if (reference.itemType == anm2::TRIGGER) return;

        auto item = document.item_get();
        auto frame = document.frame_get();

        if (!item || !frame) return;

        auto originalDuration = frame->duration;
        if (originalDuration <= 1) return;

        auto frameStartTime = item->frame_time_from_index_get(reference.frameIndex);
        int frameStart = (int)std::round(frameStartTime);
        int playheadTime = (int)std::floor(playback.time);
        int firstDuration = playheadTime - frameStart + 1;

        if (firstDuration <= 0 || firstDuration >= originalDuration) return;

        int secondDuration = originalDuration - firstDuration;
        anm2::Frame splitFrame = *frame;
        splitFrame.duration = secondDuration;

        auto nextFrame = vector::in_bounds(item->frames, reference.frameIndex + 1)
                             ? &item->frames[reference.frameIndex + 1]
                             : nullptr;
        if (frame->isInterpolated && nextFrame)
        {
          float interpolation = (float)firstDuration / (float)originalDuration;
          splitFrame.rotation = glm::mix(frame->rotation, nextFrame->rotation, interpolation);
          splitFrame.position = glm::mix(frame->position, nextFrame->position, interpolation);
          splitFrame.scale = glm::mix(frame->scale, nextFrame->scale, interpolation);
          splitFrame.colorOffset = glm::mix(frame->colorOffset, nextFrame->colorOffset, interpolation);
          splitFrame.tint = glm::mix(frame->tint, nextFrame->tint, interpolation);
        }

        frame->duration = firstDuration;
        item->frames.insert(item->frames.begin() + reference.frameIndex + 1, splitFrame);
        frames_selection_set_reference();
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_SPLIT_FRAME), Document::FRAMES, behavior());
    };

    auto frames_selection_reset = [&]()
    {
      frames.clear();
      frameSelectionSnapshot.clear();
      frameSelectionLocked.clear();
      isFrameSelectionLocked = false;
      frameFocusRequested = false;
      frameFocusIndex = -1;
      frameSelectionSnapshotReference = reference;
    };

    auto reference_clear = [&]()
    {
      reference = {reference.animationIndex};
      frames_selection_reset();
    };

    auto reference_set_item = [&](anm2::Type type, int id)
    {
      reference = {reference.animationIndex, type, id};
      frames_selection_reset();
    };

    auto item_remove = [&]()
    {
      auto behavior = [&]()
      {
        if (!animation) return;
        if (reference.itemType == anm2::LAYER || reference.itemType == anm2::NULL_)
          animation->item_remove(reference.itemType, reference.itemID);
        reference_clear();
      };

      DOCUMENT_EDIT(document, localize.get(EDIT_REMOVE_ITEMS), Document::ITEMS, behavior());
    };

    auto fit_animation_length = [&]()
    {
      if (!animation) return;

      auto behavior = [&]() { animation->fit_length(); };

      DOCUMENT_EDIT(document, localize.get(EDIT_FIT_ANIMATION_LENGTH), Document::ANIMATIONS, behavior());
    };

    auto context_menu = [&]()
    {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      auto copy = [&]()
      {
        if (frames.selection.empty()) return;

        if (auto item = animation->item_get(reference.itemType, reference.itemID); item)
        {
          std::string clipboardString{};
          for (auto& i : frames.selection)
          {
            if (!vector::in_bounds(item->frames, i)) break;
            clipboardString += item->frames[i].to_string(reference.itemType);
          }
          clipboard.set(clipboardString);
        }
      };

      auto cut = [&]()
      {
        copy();
        DOCUMENT_EDIT(document, localize.get(EDIT_CUT_FRAMES), Document::FRAMES, frames_delete());
      };

      auto paste = [&]()
      {
        if (clipboard.is_empty()) return;

        auto behavior = [&]()
        {
          if (auto item = animation->item_get(reference.itemType, reference.itemID))
          {
            document.snapshot(localize.get(EDIT_PASTE_FRAMES));
            std::set<int> indices{};
            std::string errorString{};
            int insertIndex = (int)item->frames.size();
            if (!frames.selection.empty())
              insertIndex = std::min((int)item->frames.size(), *frames.selection.rbegin() + 1);
            else if (reference.frameIndex >= 0 && reference.frameIndex < (int)item->frames.size())
              insertIndex = reference.frameIndex + 1;

            auto start = reference.itemType == anm2::TRIGGER ? hoveredTime : insertIndex;
            if (item->frames_deserialize(clipboard.get(), reference.itemType, start, indices, &errorString))
            {
              frames.selection.clear();
              for (auto i : indices)
                frames.selection.insert(i);
              reference.frameIndex = *indices.begin();
              document.change(Document::FRAMES);
            }
            else
            {
              toasts.push(std::format("{} {}", localize.get(TOAST_DESERIALIZE_FRAMES_FAILED), errorString));
              logger.error(
                  std::format("{} {}", localize.get(TOAST_DESERIALIZE_FRAMES_FAILED, anm2ed::ENGLISH), errorString));
            }
          }
          else
          {
            toasts.push(localize.get(TOAST_DESERIALIZE_FRAMES_NO_SELECTION));
            logger.warning(localize.get(TOAST_DESERIALIZE_FRAMES_NO_SELECTION, anm2ed::ENGLISH));
          }
        };

        DOCUMENT_EDIT(document, localize.get(EDIT_PASTE_FRAMES), Document::FRAMES, behavior());
      };

      if (shortcut(manager.chords[SHORTCUT_CUT], shortcut::FOCUSED)) cut();
      if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
      if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste();
      if (shortcut(manager.chords[SHORTCUT_SPLIT], shortcut::FOCUSED)) frame_split();
      if (shortcut(manager.chords[SHORTCUT_BAKE], shortcut::FOCUSED)) frames_bake();
      if (shortcut(manager.chords[SHORTCUT_FIT], shortcut::FOCUSED)) fit_animation_length();

      if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
      {
        auto item = animation ? animation->item_get(reference.itemType, reference.itemID) : nullptr;

        if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_UNDO), settings.shortcutUndo.c_str(), false,
                            document.is_able_to_undo()))
          document.undo();

        if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_REDO), settings.shortcutRedo.c_str(), false,
                            document.is_able_to_redo()))
          document.redo();

        ImGui::Separator();

        auto label = playback.isPlaying ? localize.get(LABEL_PAUSE) : localize.get(LABEL_PLAY);
        if (ImGui::MenuItem(label, settings.shortcutPlayPause.c_str())) playback.toggle();

        if (ImGui::MenuItem(localize.get(LABEL_INSERT), settings.shortcutInsertFrame.c_str(), false, item))
          frame_insert(item);

        if (ImGui::MenuItem(localize.get(LABEL_DELETE), settings.shortcutRemove.c_str(), false, document.frame_get()))
          frames_delete_action();

        if (ImGui::MenuItem(localize.get(LABEL_BAKE), settings.shortcutBake.c_str(), false, !frames.selection.empty()))
          frames_bake();

        if (ImGui::MenuItem(localize.get(LABEL_FIT_ANIMATION_LENGTH), settings.shortcutFit.c_str(), false,
                            animation && animation->frameNum != animation->length()))
          fit_animation_length();

        if (ImGui::MenuItem(localize.get(LABEL_SPLIT), settings.shortcutSplit.c_str(), false,
                            frames.selection.size() == 1))
          frame_split();

        ImGui::Separator();

        if (ImGui::MenuItem(localize.get(BASIC_CUT), settings.shortcutCut.c_str(), false, !frames.selection.empty()))
          cut();
        if (ImGui::MenuItem(localize.get(BASIC_COPY), settings.shortcutCopy.c_str(), false, !frames.selection.empty()))
          copy();
        if (ImGui::MenuItem(localize.get(BASIC_PASTE), settings.shortcutPaste.c_str(), false, !clipboard.is_empty()))
          paste();

        ImGui::EndPopup();
      }

      ImGui::PopStyleVar(2);
    };

    auto item_base_properties_open = [&](anm2::Type type, int id)
    {
      switch (type)
      {
        case anm2::LAYER:
          manager.layer_properties_open(id);
          break;
        case anm2::NULL_:
          manager.null_properties_open(id);
        default:
          break;
      };
    };

    auto item_properties_reset = [&]()
    {
      addItemName.clear();
      addItemSpritesheetID = {};
      addItemID = -1;
    };

    auto unused_items_get = [&](anm2::Type type)
    {
      if (!animation) return std::set<int>{};
      if (type == anm2::LAYER) return anm2.layers_unused(*animation);
      if (type == anm2::NULL_) return anm2.nulls_unused(*animation);
      return std::set<int>{};
    };

    auto item_context_menu = [&]()
    {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      auto& type = reference.itemType;
      auto& id = reference.itemID;
      auto item = document.item_get();

      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByWindow) &&
          ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##Items Context Menu");

      if (ImGui::BeginPopup("##Items Context Menu"))
      {
        if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_UNDO), settings.shortcutUndo.c_str(), false,
                            document.is_able_to_undo()))
          document.undo();

        if (ImGui::MenuItem(localize.get(SHORTCUT_STRING_REDO), settings.shortcutRedo.c_str(), false,
                            document.is_able_to_redo()))
          document.redo();

        ImGui::Separator();

        if (ImGui::MenuItem(localize.get(BASIC_PROPERTIES), nullptr, false,
                            item && (type == anm2::LAYER || type == anm2::NULL_)))
          item_base_properties_open(type, id);

        if (ImGui::MenuItem(localize.get(BASIC_ADD), settings.shortcutAdd.c_str(), false, animation))
        {
          item_properties_reset();
          propertiesPopup.open();
        }

        if (ImGui::MenuItem(localize.get(BASIC_REMOVE), settings.shortcutRemove.c_str(), false, item)) item_remove();

        ImGui::EndPopup();
      }

      ImGui::PopStyleVar(2);
    };

    auto item_child = [&](anm2::Type type, int id, int index)
    {
      ImGui::PushID(index);

      auto item = animation ? animation->item_get(type, id) : nullptr;
      auto isVisible = item ? item->isVisible : false;
      auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;
      if (isOnlyShowLayers && type != anm2::LAYER) isVisible = false;
      auto isReferenced = reference.itemType == type && reference.itemID == id;

      auto label = type == anm2::LAYER ? std::vformat(localize.get(FORMAT_LAYER),
                                                      std::make_format_args(id, anm2.content.layers[id].name,
                                                                            anm2.content.layers[id].spritesheetID))
                   : type == anm2::NULL_
                       ? std::vformat(localize.get(FORMAT_NULL), std::make_format_args(id, anm2.content.nulls[id].name))
                       : localize.get(anm2::TYPE_STRINGS[type]);
      auto icon = anm2::TYPE_ICONS[type];
      auto iconTintCurrent = isLightTheme && type == anm2::NONE ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : itemIconTint;
      auto baseColorVec = item_color_vec(type);
      auto activeColorVec = item_color_active_vec(type);
      auto colorVec = baseColorVec;
      if (isReferenced && type != anm2::NONE)
      {
        if (isLightTheme)
          colorVec = ITEM_COLOR_LIGHT_SELECTED[type_index(type)];
        else
          colorVec = activeColorVec;
      }
      auto color = to_imvec4(colorVec);
      color = !isVisible ? to_imvec4(colorVec * COLOR_HIDDEN_MULTIPLIER) : color;
      ImGui::PushStyleColor(ImGuiCol_ChildBg, color);

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

      auto itemSize = ImVec2(ImGui::GetContentRegionAvail().x, ITEM_FRAME_CHILD_HEIGHT);

      if (ImGui::BeginChild(label.c_str(), itemSize, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollWithMouse))
      {
        auto cursorPos = ImGui::GetCursorPos();

        if (type != anm2::NONE)
        {
          ImGui::SetCursorPos(to_imvec2(to_vec2(cursorPos) - to_vec2(style.ItemSpacing)));
          ImGui::SetNextItemAllowOverlap();
          ImGui::PushStyleColor(ImGuiCol_Header, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4());
          ImGui::SetNextItemStorageID(id);
          if (ImGui::Selectable("##Item Button", isReferenced, ImGuiSelectableFlags_SelectOnClick, itemSize))
          {
            if (type == anm2::LAYER) document.spritesheet.reference = anm2.content.layers[id].spritesheetID;
            reference_set_item(type, id);
          }
          ImGui::PopStyleColor(3);
          if (ImGui::IsItemHovered())
          {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) item_base_properties_open(type, id);

            auto& imguiStyle = ImGui::GetStyle();
            auto previousTooltipFlags = imguiStyle.HoverFlagsForTooltipMouse;
            auto previousTooltipDelay = imguiStyle.HoverDelayNormal;
            imguiStyle.HoverFlagsForTooltipMouse = ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_DelayNormal |
                                                   ImGuiHoveredFlags_AllowWhenDisabled |
                                                   ImGuiHoveredFlags_NoSharedDelay;
            imguiStyle.HoverDelayNormal = FRAME_TOOLTIP_HOVER_DELAY;
            bool showItemTooltip = ImGui::BeginItemTooltip();
            imguiStyle.HoverFlagsForTooltipMouse = previousTooltipFlags;
            imguiStyle.HoverDelayNormal = previousTooltipDelay;

            if (showItemTooltip)
            {
              auto yesNoLabel = [&](bool value) { return value ? localize.get(BASIC_YES) : localize.get(BASIC_NO); };
              auto visibleLabel = yesNoLabel(isVisible);
              auto framesCount = item ? (int)item->frames.size() : 0;

              switch (type)
              {
                case anm2::ROOT:
                {
                  ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                  ImGui::TextUnformatted(localize.get(BASIC_ROOT));
                  ImGui::PopFont();

                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_VISIBLE), std::make_format_args(visibleLabel)).c_str());
                  auto transformLabel = yesNoLabel(settings.previewIsRootTransform);
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_TRANSFORM), std::make_format_args(transformLabel)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_FRAMES_COUNT), std::make_format_args(framesCount)).c_str());
                  break;
                }
                case anm2::LAYER:
                {
                  auto& layer = anm2.content.layers[id];
                  ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                  ImGui::TextUnformatted(layer.name.c_str());
                  ImGui::PopFont();

                  ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_SPRITESHEET_ID), std::make_format_args(layer.spritesheetID))
                          .c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_VISIBLE), std::make_format_args(visibleLabel)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_FRAMES_COUNT), std::make_format_args(framesCount)).c_str());
                  break;
                }
                case anm2::NULL_:
                {
                  auto& nullInfo = anm2.content.nulls[id];
                  ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                  ImGui::TextUnformatted(nullInfo.name.c_str());
                  ImGui::PopFont();

                  auto rectLabel = yesNoLabel(nullInfo.isShowRect);
                  ImGui::TextUnformatted(std::vformat(localize.get(FORMAT_ID), std::make_format_args(id)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_RECT), std::make_format_args(rectLabel)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_VISIBLE), std::make_format_args(visibleLabel)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_FRAMES_COUNT), std::make_format_args(framesCount)).c_str());
                  break;
                }
                case anm2::TRIGGER:
                {
                  ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                  ImGui::TextUnformatted(localize.get(BASIC_TRIGGERS));
                  ImGui::PopFont();

                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_VISIBLE), std::make_format_args(visibleLabel)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_TRIGGERS_COUNT), std::make_format_args(framesCount)).c_str());
                  break;
                }
                default:
                  break;
              }

              ImGui::EndTooltip();
            }
          }

          if (type == anm2::LAYER)
          {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip))
            {
              ImGui::SetDragDropPayload("Layer Animation Drag Drop", &id, sizeof(int));
              ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget())
            {
              if (auto payload = ImGui::AcceptDragDropPayload("Layer Animation Drag Drop"))
              {
                auto droppedID = *(int*)payload->Data;

                auto layer_order_move = [&]()
                {
                  int source = vector::find_index(animation->layerOrder, droppedID);
                  int destination = vector::find_index(animation->layerOrder, id);

                  if (source != -1 && destination != -1) vector::move_index(animation->layerOrder, source, destination);
                };

                DOCUMENT_EDIT(document, localize.get(EDIT_MOVE_LAYER_ANIMATION), Document::ITEMS, layer_order_move());
              }
              ImGui::EndDragDropTarget();
            }
          }

          ImGui::SetCursorPos(cursorPos);

          ImGui::Image(resources.icons[icon].id, icon_size_get());
          overlay_icon(resources.icons[icon].id, iconTintCurrent);
          ImGui::SameLine();
          if (isReferenced) ImGui::PushFont(resources.fonts[font::ITALICS].get(), font::SIZE);
          ImGui::PushStyleColor(ImGuiCol_Text, itemTextColor);
          ImGui::TextUnformatted(label.c_str());
          ImGui::PopStyleColor();
          if (isReferenced) ImGui::PopFont();

          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
          ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

          ImGui::SetCursorPos(
              ImVec2(itemSize.x - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x,
                     (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
          int visibleIcon = item->isVisible ? icon::VISIBLE : icon::INVISIBLE;
          if (ImGui::ImageButton("##Visible Toggle", resources.icons[visibleIcon].id, icon_size_get()))
            DOCUMENT_EDIT(document, localize.get(EDIT_TOGGLE_ITEM_VISIBILITY), Document::FRAMES,
                          item->isVisible = !item->isVisible);
          overlay_icon(resources.icons[visibleIcon].id, iconTintCurrent);
          ImGui::SetItemTooltip("%s", isVisible ? localize.get(TOOLTIP_ITEM_VISIBILITY_SHOWN)
                                                : localize.get(TOOLTIP_ITEM_VISIBILITY_HIDDEN));

          if (type == anm2::NULL_)
          {
            auto& null = anm2.content.nulls.at(id);
            auto& isShowRect = null.isShowRect;
            auto rectIcon = isShowRect ? icon::SHOW_RECT : icon::HIDE_RECT;
            ImGui::SetCursorPos(
                ImVec2(itemSize.x - (ImGui::GetTextLineHeightWithSpacing() * 2) - ImGui::GetStyle().ItemSpacing.x,
                       (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
            if (ImGui::ImageButton("##Rect Toggle", resources.icons[rectIcon].id, icon_size_get()))
              DOCUMENT_EDIT(document, localize.get(EDIT_TOGGLE_NULL_RECT), Document::FRAMES,
                            null.isShowRect = !null.isShowRect);
            overlay_icon(resources.icons[rectIcon].id, iconTintCurrent);
            ImGui::SetItemTooltip("%s", isShowRect ? localize.get(TOOLTIP_NULL_RECT_SHOWN)
                                                   : localize.get(TOOLTIP_NULL_RECT_HIDDEN));
          }

          ImGui::PopStyleVar(2);
          ImGui::PopStyleColor(3);
        }
        else
        {
          auto cursorPos = ImGui::GetCursorPos();
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
          ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
          ImGui::SetCursorPos(
              ImVec2(itemSize.x - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x,
                     (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));

          auto& isShowUnused = settings.timelineIsShowUnused;
          auto unusedIcon = isShowUnused ? icon::SHOW_UNUSED : icon::HIDE_UNUSED;
          if (ImGui::ImageButton("##Unused Toggle", resources.icons[unusedIcon].id, icon_size_get()))
            isShowUnused = !isShowUnused;
          overlay_icon(resources.icons[unusedIcon].id, iconTintCurrent);
          ImGui::SetItemTooltip("%s", isShowUnused ? localize.get(TOOLTIP_UNUSED_ITEMS_SHOWN)
                                                   : localize.get(TOOLTIP_UNUSED_ITEMS_HIDDEN));

          auto& showLayersOnly = settings.timelineIsOnlyShowLayers;
          auto layersIcon = showLayersOnly ? icon::SHOW_LAYERS : icon::HIDE_LAYERS;
          ImGui::SetCursorPos(
              ImVec2(itemSize.x - (ImGui::GetTextLineHeightWithSpacing() * 2) - ImGui::GetStyle().ItemSpacing.x,
                     (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
          if (ImGui::ImageButton("##Layers Toggle", resources.icons[layersIcon].id, icon_size_get()))
            showLayersOnly = !showLayersOnly;
          overlay_icon(resources.icons[layersIcon].id, iconTintCurrent);
          ImGui::SetItemTooltip("%s", showLayersOnly ? localize.get(TOOLTIP_ONLY_LAYERS_VISIBLE)
                                                     : localize.get(TOOLTIP_ALL_ITEMS_VISIBLE));
          ImGui::PopStyleVar(2);
          ImGui::PopStyleColor(3);

          ImGui::SetCursorPos(cursorPos);

          ImGui::BeginDisabled();
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
          ImGui::Text("(?)");
          ImGui::PopStyleColor();
          auto tooltipShortcuts = std::vformat(
              localize.get(TOOLTIP_TIMELINE_SHORTCUTS),
              std::make_format_args(settings.shortcutMovePlayheadBack, settings.shortcutMovePlayheadForward,
                                    settings.shortcutShortenFrame, settings.shortcutExtendFrame,
                                    settings.shortcutPreviousFrame, settings.shortcutNextFrame));
          ImGui::SetItemTooltip("%s", tooltipShortcuts.c_str());
          ImGui::EndDisabled();
        }
      }
      ImGui::EndChild();
      ImGui::PopStyleColor();
      ImGui::PopStyleVar(2);

      ImGui::PopID();
    };

    auto items_child = [&]()
    {
      auto itemsChildSize = ImVec2(ITEM_CHILD_WIDTH, ImGui::GetContentRegionAvail().y);

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
      bool isItemsChildOpen = ImGui::BeginChild("##Items Child", itemsChildSize, ImGuiChildFlags_Borders);
      ImGui::PopStyleVar();
      if (isItemsChildOpen)
      {
        auto itemsListChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y -
                                                                               ImGui::GetTextLineHeightWithSpacing() -
                                                                               ImGui::GetStyle().ItemSpacing.y * 2);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
        if (ImGui::BeginChild("##Items List Child", itemsListChildSize, ImGuiChildFlags_Borders,
                              ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
        {
          ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2());
          ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0f);
          if (ImGui::BeginTable("##Item Table", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY))
          {
            ImGui::GetCurrentWindow()->Flags |= ImGuiWindowFlags_NoScrollWithMouse;
            ImGui::SetScrollY(scroll.y);

            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("##Items");

            auto item_child_row = [&](anm2::Type type, int id = -1, int index = 0)
            {
              ImGui::TableNextRow();
              ImGui::TableSetColumnIndex(0);
              item_child(type, id, index);
            };

            item_child_row(anm2::NONE);

            int index{};
            if (animation)
            {
              item_child_row(anm2::ROOT, -1, index++);

              for (auto& id : animation->layerOrder | std::views::reverse)
              {
                if (!settings.timelineIsShowUnused && animation->layerAnimations[id].frames.empty()) continue;
                item_child_row(anm2::LAYER, id, index++);
              }

              for (auto& [id, nullAnimation] : animation->nullAnimations)
              {
                if (!settings.timelineIsShowUnused && nullAnimation.frames.empty()) continue;
                item_child_row(anm2::NULL_, id, index++);
              }

              item_child_row(anm2::TRIGGER);
            }

            if (isHorizontalScroll && ImGui::GetCurrentWindow()->ScrollbarY)
            {
              ImGui::TableNextRow();
              ImGui::TableSetColumnIndex(0);
              ImGui::Dummy(ImVec2(0, style.ScrollbarSize));
            }

            ImGui::EndTable();
          }
          ImGui::PopStyleVar(2);

          item_context_menu();
        }
        ImGui::PopStyleVar(2);
        ImGui::EndChild();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + style.WindowPadding.x, ImGui::GetCursorPosY()));
        auto widgetSize = widget_size_with_row_get(2, ImGui::GetContentRegionAvail().x - style.WindowPadding.x);

        ImGui::BeginDisabled(!animation);
        {
          shortcut(manager.chords[SHORTCUT_ADD]);
          if (ImGui::Button(localize.get(BASIC_ADD), widgetSize))
          {
            item_properties_reset();
            propertiesPopup.open();
          }
          set_item_tooltip_shortcut(localize.get(TOOLTIP_ADD_ITEM), settings.shortcutAdd);
          ImGui::SameLine();

          ImGui::BeginDisabled(!document.item_get());
          shortcut(manager.chords[SHORTCUT_REMOVE]);
          if (ImGui::Button(localize.get(BASIC_REMOVE), widgetSize)) item_remove();
          set_item_tooltip_shortcut(localize.get(TOOLTIP_REMOVE_ITEMS), settings.shortcutRemove);
          ImGui::EndDisabled();
        }
        ImGui::EndDisabled();

        ImGui::PopStyleVar();
      }
      ImGui::EndChild();
    };

    auto frame_child = [&](anm2::Type type, int id, int& index, float width)
    {
      auto item = animation ? animation->item_get(type, id) : nullptr;
      auto isVisible = item ? item->isVisible : false;
      auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;
      if (isOnlyShowLayers && type != anm2::LAYER) isVisible = false;

      auto colorVec = type_color_base_vec(type);
      auto colorActiveVec = type_color_active_vec(type);
      auto colorHoveredVec = type_color_hovered_vec(type);
      auto color = to_imvec4(colorVec);
      auto colorActive = to_imvec4(colorActiveVec);
      auto colorHovered = to_imvec4(colorHoveredVec);
      auto colorHidden = to_imvec4(colorVec * COLOR_HIDDEN_MULTIPLIER);
      auto colorActiveHidden = to_imvec4(colorActiveVec * COLOR_HIDDEN_MULTIPLIER);
      auto colorHoveredHidden = to_imvec4(colorHoveredVec * COLOR_HIDDEN_MULTIPLIER);

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

      auto childSize = ImVec2(width, ITEM_FRAME_CHILD_HEIGHT);

      ImGui::PopStyleVar(2);

      ImGui::PushID(index);

      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
      bool isDefaultChild = type == anm2::NONE;
      if (isLightTheme && isDefaultChild) ImGui::PushStyleColor(ImGuiCol_ChildBg, TIMELINE_CHILD_BG_COLOR_LIGHT);

      if (ImGui::BeginChild("##Frames Child", childSize, ImGuiChildFlags_Borders))
      {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused))
        {
          if (!frames.selection.empty())
          {
            reference.frameIndex = -1;
            frames_selection_set_reference();
          }
          else if (reference.itemType != anm2::NONE || reference.itemID != -1)
            reference_clear();
        }

        auto drawList = ImGui::GetWindowDrawList();
        auto clipMax = drawList->GetClipRectMax();
        auto length = animation ? animation->frameNum : 0;
        auto frameSize = ImVec2(ImGui::GetTextLineHeight(), ImGui::GetContentRegionAvail().y);
        auto framesSize = ImVec2(frameSize.x * length, frameSize.y);
        auto cursorPos = ImGui::GetCursorPos();
        auto cursorScreenPos = ImGui::GetCursorScreenPos();
        auto border = glm::max(0.5f, ImGui::GetStyle().FrameBorderSize * 0.5f);
        auto borderLineLength = frameSize.y / 5;
        auto frameMin = std::max(0, (int)std::floor(scroll.x / frameSize.x) - 1);
        auto frameMax = std::min(anm2::FRAME_NUM_MAX, (int)std::ceil((scroll.x + clipMax.x) / frameSize.x) + 1);
        pickerLineDrawList = drawList;

        if (type == anm2::NONE)
        {
          if (length > 0)
          {
            if (isLightTheme)
            {
              auto totalMax = ImVec2(cursorScreenPos.x + framesSize.x, cursorScreenPos.y + framesSize.y);
              drawList->AddRectFilled(cursorScreenPos, totalMax, ImGui::GetColorU32(timelineBackgroundColor));
              float animationWidth = std::min(framesSize.x, frameSize.x * (float)length);
              drawList->AddRectFilled(cursorScreenPos,
                                      ImVec2(cursorScreenPos.x + animationWidth, cursorScreenPos.y + framesSize.y),
                                      ImGui::GetColorU32(timelinePlayheadRectColor));
            }
            else
            {
              drawList->AddRectFilled(cursorScreenPos,
                                      ImVec2(cursorScreenPos.x + framesSize.x, cursorScreenPos.y + framesSize.y),
                                      ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_Header)));
            }
          }

          for (int i = frameMin; i < frameMax; i++)
          {
            auto frameScreenPos = ImVec2(cursorScreenPos.x + frameSize.x * (float)i, cursorScreenPos.y);

            drawList->AddRect(frameScreenPos, ImVec2(frameScreenPos.x + border, frameScreenPos.y + borderLineLength),
                              ImGui::GetColorU32(timelineTickColor), 0, 0, 0.5f);

            drawList->AddRect(ImVec2(frameScreenPos.x, frameScreenPos.y + frameSize.y - borderLineLength),
                              ImVec2(frameScreenPos.x + border, frameScreenPos.y + frameSize.y),
                              ImGui::GetColorU32(timelineTickColor), 0, 0, 0.5);

            if (i % FRAME_MULTIPLE == 0)
            {
              auto string = std::to_string(i);
              auto textSize = ImGui::CalcTextSize(string.c_str());
              auto textPos = ImVec2(frameScreenPos.x + (frameSize.x - textSize.x) / 2,
                                    frameScreenPos.y + (frameSize.y - textSize.y) / 2);

              drawList->AddRectFilled(frameScreenPos,
                                      ImVec2(frameScreenPos.x + frameSize.x, frameScreenPos.y + frameSize.y),
                                      ImGui::GetColorU32(frameMultipleOverlayColor));

              drawList->AddText(textPos, ImGui::GetColorU32(textMultipleColor), string.c_str());
            }
          }

          if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseDown(0))
            isDragging = true;

          auto childPos = ImGui::GetWindowPos();
          auto mousePos = ImGui::GetIO().MousePos;
          auto localMousePos = ImVec2(mousePos.x - childPos.x, mousePos.y - childPos.y);
          hoveredTime = floorf(localMousePos.x / frameSize.x);

          if (isDragging)
          {
            playback.time = hoveredTime;
            document.frameTime = playback.time;
          }

          playback.clamp(settings.playbackIsClamp ? length : anm2::FRAME_NUM_MAX);

          if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) isDragging = false;

          if (length > 0)
          {
            ImGui::SetCursorPos(ImVec2(cursorPos.x + frameSize.x * floorf(playback.time), cursorPos.y));
            ImGui::Image(resources.icons[icon::PLAYHEAD].id, frameSize);
            overlay_icon(resources.icons[icon::PLAYHEAD].id, playheadIconTint, true);
          }
        }
        else if (animation)
        {
          float frameTime{};

          if (ImGui::IsWindowHovered() &&
              (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Right)) &&
              !ImGui::IsAnyItemHovered())
            reference_set_item(type, id);

          for (int i = frameMin; i < frameMax; i++)
          {
            auto frameScreenPos = ImVec2(cursorScreenPos.x + frameSize.x * (float)i, cursorScreenPos.y);
            auto frameRectMax = ImVec2(frameScreenPos.x + frameSize.x, frameScreenPos.y + frameSize.y);

            drawList->AddRect(frameScreenPos, frameRectMax, ImGui::GetColorU32(frameBorderColor));

            if (i % FRAME_MULTIPLE == 0)
              drawList->AddRectFilled(frameScreenPos, frameRectMax, ImGui::GetColorU32(frameMultipleOverlayColor));
          }

          if (type != anm2::TRIGGER) frames.selection.start(item->frames.size(), ImGuiMultiSelectFlags_ClearOnEscape);

          for (auto [i, frame] : std::views::enumerate(item->frames))
          {
            ImGui::PushID((int)i);

            auto frameReference = anm2::Reference{reference.animationIndex, type, id, (int)i};
            auto isFrameVisible = isVisible && frame.isVisible;
            auto isReferenced = reference == frameReference;
            auto isSelected =
                (frames.selection.contains((int)i) && reference.itemType == type && reference.itemID == id);

            if (type == anm2::TRIGGER) frameTime = frame.atFrame;

            auto buttonSize =
                type == anm2::TRIGGER ? frameSize : to_imvec2(vec2(frameSize.x * frame.duration, frameSize.y));
            auto frameStart = type == anm2::TRIGGER ? frame.atFrame : frameTime;
            auto frameEnd = type == anm2::TRIGGER ? frameStart + 1.0f : frameStart + frame.duration;
            if (frameEnd <= (float)frameMin || frameStart >= (float)frameMax)
            {
              if (type != anm2::TRIGGER) frameTime += frame.duration;
              ImGui::PopID();
              continue;
            }
            auto buttonPos = ImVec2(cursorPos.x + (frameTime * frameSize.x), cursorPos.y);

            if (frameFocusRequested && frameFocusIndex == (int)i && reference == frameReference)
            {
              ImGui::SetKeyboardFocusHere();
              frameFocusRequested = false;
            }

            ImGui::SetCursorPos(buttonPos);

            auto buttonScreenPos = ImGui::GetCursorScreenPos();
            auto fillColor = isSelected ? (isFrameVisible ? colorActive : colorActiveHidden)
                                        : (isFrameVisible ? color : colorHidden);
            drawList->AddRectFilled(buttonScreenPos,
                                    ImVec2(buttonScreenPos.x + buttonSize.x, buttonScreenPos.y + buttonSize.y),
                                    ImGui::GetColorU32(fillColor), FRAME_ROUNDING);

            ImGui::PushStyleColor(ImGuiCol_Header, isFrameVisible ? colorActive : colorActiveHidden);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, isFrameVisible ? colorActive : colorActiveHidden);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, isFrameVisible ? colorHovered : colorHoveredHidden);
            ImGui::PushStyleColor(ImGuiCol_NavCursor, isFrameVisible ? colorHovered : colorHoveredHidden);

            ImGui::SetNextItemAllowOverlap();
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING);
            ImGui::SetNextItemSelectionUserData((int)i);
            bool isDifferentItem = reference.itemType != type || reference.itemID != id;
            if (ImGui::Selectable("##Frame Button", isSelected, ImGuiSelectableFlags_None, buttonSize))
            {
              if (type == anm2::LAYER) document.spritesheet.reference = anm2.content.layers[id].spritesheetID;

              if (type != anm2::TRIGGER)
              {
                if (ImGui::IsKeyDown(ImGuiMod_Alt))
                  DOCUMENT_EDIT(document, localize.get(EDIT_FRAME_INTERPOLATION), Document::FRAMES,
                                frame.isInterpolated = !frame.isInterpolated);

                document.frameTime = frameTime;
              }

              reference = frameReference;
              isReferenced = true;
              if (isDifferentItem) frames_selection_set_reference();
            }
            ImGui::PopStyleVar();

            ImGui::PopStyleColor(4);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
            auto& imguiStyle = ImGui::GetStyle();
            auto previousTooltipFlags = imguiStyle.HoverFlagsForTooltipMouse;
            auto previousTooltipDelay = imguiStyle.HoverDelayNormal;
            imguiStyle.HoverFlagsForTooltipMouse = ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_DelayNormal |
                                                   ImGuiHoveredFlags_AllowWhenDisabled |
                                                   ImGuiHoveredFlags_NoSharedDelay;
            imguiStyle.HoverDelayNormal = FRAME_TOOLTIP_HOVER_DELAY;
            bool showFrameTooltip = ImGui::BeginItemTooltip();
            imguiStyle.HoverFlagsForTooltipMouse = previousTooltipFlags;
            imguiStyle.HoverDelayNormal = previousTooltipDelay;

            if (showFrameTooltip)
            {
              if (type != anm2::TRIGGER)
              {
                auto cropX = frame.crop.x;
                auto cropY = frame.crop.y;
                auto sizeX = frame.size.x;
                auto sizeY = frame.size.y;
                auto pivotX = frame.pivot.x;
                auto pivotY = frame.pivot.y;
                auto scaleX = frame.scale.x;
                auto scaleY = frame.scale.y;
                auto rotationValue = frame.rotation;
                auto durationValue = frame.duration;
                auto tintR = math::float_to_uint8(frame.tint.r);
                auto tintG = math::float_to_uint8(frame.tint.g);
                auto tintB = math::float_to_uint8(frame.tint.b);
                auto tintA = math::float_to_uint8(frame.tint.a);
                auto visibleLabel = frame.isVisible ? localize.get(BASIC_YES) : localize.get(BASIC_NO);
                auto interpolatedLabel = frame.isInterpolated ? localize.get(BASIC_YES) : localize.get(BASIC_NO);

                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::Text("%s %s", localize.get(anm2::TYPE_STRINGS[type]), localize.get(BASIC_FRAME));
                ImGui::PopFont();

                auto indexValue = (int)i;
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_INDEX), std::make_format_args(indexValue)).c_str());

                if (type == anm2::LAYER)
                {

                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_CROP), std::make_format_args(cropX, cropY)).c_str());
                  ImGui::TextUnformatted(
                      std::vformat(localize.get(FORMAT_SIZE), std::make_format_args(sizeX, sizeY)).c_str());
                }

                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_POSITION), std::make_format_args(sizeX, sizeY)).c_str());

                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_PIVOT), std::make_format_args(pivotX, pivotY)).c_str());

                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_SCALE), std::make_format_args(scaleX, scaleY)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_ROTATION), std::make_format_args(rotationValue)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_DURATION), std::make_format_args(durationValue)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_TINT), std::make_format_args(tintR, tintG, tintB, tintA)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_COLOR_OFFSET), std::make_format_args(tintR, tintG, tintB))
                        .c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_VISIBLE), std::make_format_args(visibleLabel)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_INTERPOLATED), std::make_format_args(interpolatedLabel)).c_str());
              }
              else
              {
                auto atFrameValue = frame.atFrame;
                auto eventLabel = document.event.labels[frame.eventID + 1];
                auto soundLabel = document.sound.labels[frame.soundID + 1];

                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::Text("%s %s", localize.get(anm2::TYPE_STRINGS[type]), localize.get(BASIC_FRAME));
                ImGui::PopFont();

                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_AT_FRAME), std::make_format_args(atFrameValue)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_EVENT_LABEL), std::make_format_args(eventLabel)).c_str());
                ImGui::TextUnformatted(
                    std::vformat(localize.get(FORMAT_SOUND_LABEL), std::make_format_args(soundLabel)).c_str());
              }

              ImGui::EndTooltip();
            }
            ImGui::PopStyleVar(2);

            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
              if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
              {
                if (type == anm2::TRIGGER || ImGui::IsKeyDown(ImGuiMod_Ctrl))
                {
                  draggedFrame = &frame;
                  draggedFrameType = type;
                  draggedFrameIndex = (int)i;
                  draggedFrameStart = hoveredTime;
                  if (type != anm2::TRIGGER) draggedFrameStartDuration = draggedFrame->duration;
                }
              }
            }

            if (type != anm2::TRIGGER)
            {
              if (!draggedFrame && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip))
              {
                frameDragDrop = {};
                frameDragDrop.type = type;
                frameDragDrop.itemID = id;
                frameDragDrop.animationIndex = reference.animationIndex;

                auto append_valid_indices = [&](const auto& container)
                {
                  for (auto idx : container)
                    if (idx >= 0 && idx < (int)item->frames.size()) frameDragDrop.selection.push_back(idx);
                };

                if (isReferenced) append_valid_indices(frames.selection);

                auto contains_index = [&](const std::vector<int>& container, int index)
                { return std::find(container.begin(), container.end(), index) != container.end(); };

                if ((!contains_index(frameDragDrop.selection, (int)i) || frameDragDrop.selection.size() <= 1) &&
                    frameSelectionSnapshotReference.animationIndex == reference.animationIndex &&
                    frameSelectionSnapshotReference.itemType == type && frameSelectionSnapshotReference.itemID == id &&
                    contains_index(frameSelectionSnapshot, (int)i))
                {
                  frameDragDrop.selection = frameSelectionSnapshot;
                  frames.selection.clear();
                  for (int idx : frameSelectionSnapshot)
                    if (idx >= 0 && idx < (int)item->frames.size()) frames.selection.insert(idx);
                  frameSelectionLocked = frameDragDrop.selection;
                  isFrameSelectionLocked = true;
                }

                if (frameDragDrop.selection.empty()) frameDragDrop.selection.push_back((int)i);

                std::sort(frameDragDrop.selection.begin(), frameDragDrop.selection.end());
                frameDragDrop.selection.erase(
                    std::unique(frameDragDrop.selection.begin(), frameDragDrop.selection.end()),
                    frameDragDrop.selection.end());

                ImGui::SetDragDropPayload(FRAME_DRAG_PAYLOAD_ID, &frameDragDrop, sizeof(FrameDragDrop));
                ImGui::EndDragDropSource();
              }

              if (!draggedFrame && ImGui::BeginDragDropTarget())
              {
                if (auto payload = ImGui::AcceptDragDropPayload(FRAME_DRAG_PAYLOAD_ID))
                {
                  auto source = static_cast<const FrameDragDrop*>(payload->Data);
                  auto sameAnimation = source && source->animationIndex == reference.animationIndex;
                  auto sourceItem =
                      sameAnimation && animation ? animation->item_get(source->type, source->itemID) : nullptr;
                  auto targetItem = animation ? animation->item_get(type, id) : nullptr;

                  auto time_from_index = [&](anm2::Item* target, int index)
                  {
                    if (!target || target->frames.empty()) return 0.0f;
                    index = std::clamp(index, 0, (int)target->frames.size());
                    float timeAccum = 0.0f;
                    for (int n = 0; n < index && n < (int)target->frames.size(); ++n)
                      timeAccum += target->frames[n].duration;
                    return timeAccum;
                  };

                  if (source && sourceItem && targetItem && source->type != anm2::TRIGGER && type != anm2::TRIGGER)
                  {
                    std::vector<int> indices = source->selection;
                    if (indices.empty()) indices.push_back((int)i);
                    std::sort(indices.begin(), indices.end());
                    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

                    int insertPosResult = -1;
                    int insertedCount = 0;
                    DOCUMENT_EDIT(document, localize.get(EDIT_MOVE_FRAMES), Document::FRAMES, {
                      std::vector<anm2::Frame> movedFrames;
                      movedFrames.reserve(indices.size());

                      for (int i : indices)
                        if (i >= 0 && i < (int)sourceItem->frames.size())
                          movedFrames.push_back(std::move(sourceItem->frames[i]));

                      for (auto it = indices.rbegin(); it != indices.rend(); ++it)
                        if (*it >= 0 && *it < (int)sourceItem->frames.size())
                          sourceItem->frames.erase(sourceItem->frames.begin() + *it);

                      const int dropIndex = (int)i;
                      int desired = std::clamp(dropIndex + 1, 0, (int)targetItem->frames.size());
                      if (sourceItem == targetItem)
                      {
                        if (dropIndex < indices.front())
                          desired = dropIndex;
                        else if (dropIndex > indices.back())
                          desired = dropIndex + 1;
                        else
                          desired = indices.front();

                        int removedBefore = 0;
                        for (int i : indices)
                          if (i < desired) ++removedBefore;
                        desired -= removedBefore;
                      }
                      desired = std::clamp(desired, 0, (int)targetItem->frames.size());

                      insertPosResult = desired;
                      insertedCount = (int)movedFrames.size();
                      targetItem->frames.insert(targetItem->frames.begin() + insertPosResult,
                                                std::make_move_iterator(movedFrames.begin()),
                                                std::make_move_iterator(movedFrames.end()));
                    });

                    if (insertedCount > 0)
                    {
                      frames.selection.clear();
                      for (int offset = 0; offset < insertedCount; ++offset)
                        frames.selection.insert(insertPosResult + offset);

                      reference = {reference.animationIndex, type, id, insertPosResult};
                      document.frameTime = time_from_index(targetItem, reference.frameIndex);
                      if (type == anm2::LAYER)
                      {
                        document.spritesheet.reference = anm2.content.layers[id].spritesheetID;
                      }
                    }
                  }
                }

                ImGui::EndDragDropTarget();
              }
            }

            auto rectMin = ImGui::GetItemRectMin();
            auto rectMax = ImGui::GetItemRectMax();
            auto borderColor = isReferenced ? frameBorderColorReferenced : frameBorderColor;
            auto borderThickness = isReferenced ? FRAME_BORDER_THICKNESS_REFERENCED : FRAME_BORDER_THICKNESS;
            drawList->AddRect(rectMin, rectMax, ImGui::GetColorU32(borderColor), FRAME_ROUNDING, 0, borderThickness);

            auto icon = type == anm2::TRIGGER  ? icon::TRIGGER
                        : frame.isInterpolated ? icon::INTERPOLATED
                                               : icon::UNINTERPOLATED;
            auto iconPos = ImVec2(cursorPos.x + (frameTime * frameSize.x),
                                  cursorPos.y + (frameSize.y / 2) - (icon_size_get().y / 2));
            ImGui::SetCursorPos(iconPos);
            ImGui::Image(resources.icons[icon].id, icon_size_get());
            overlay_icon(resources.icons[icon].id, iconTintDefault);

            if (type != anm2::TRIGGER) frameTime += frame.duration;

            ImGui::PopID();
          }

          if (type != anm2::TRIGGER) frames.selection.finish();

          if (isFrameSelectionLocked)
          {
            frames.selection.clear();
            for (int idx : frameSelectionLocked)
              frames.selection.insert(idx);
            isFrameSelectionLocked = false;
            frameSelectionLocked.clear();
          }
          if (reference.itemType == type && reference.itemID == id)
          {
            frameSelectionSnapshot.assign(frames.selection.begin(), frames.selection.end());
            frameSelectionSnapshotReference = reference;
          }
        }
      }

      if (draggedFrame)
      {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        if (!isDraggedFrameSnapshot && hoveredTime != draggedFrameStart)
        {
          isDraggedFrameSnapshot = true;
          document.snapshot(draggedFrameType == anm2::TRIGGER ? localize.get(EDIT_TRIGGER_AT_FRAME)
                                                              : localize.get(EDIT_FRAME_DURATION));
        }

        if (draggedFrameType == anm2::TRIGGER)
        {
          draggedFrame->atFrame =
              glm::clamp(hoveredTime, 0, settings.playbackIsClamp ? animation->frameNum - 1 : anm2::FRAME_NUM_MAX - 1);

          for (auto [i, trigger] : std::views::enumerate(animation->triggers.frames))
          {
            if ((int)i == draggedFrameIndex) continue;
            if (trigger.atFrame == draggedFrame->atFrame) draggedFrame->atFrame--;
          }
        }
        else
        {
          draggedFrame->duration = glm::clamp(draggedFrameStartDuration + (hoveredTime - draggedFrameStart),
                                              anm2::FRAME_DURATION_MIN, anm2::FRAME_DURATION_MAX);
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
          document.change(Document::FRAMES);
          draggedFrame = nullptr;
          draggedFrameType = anm2::NONE;
          draggedFrameIndex = -1;
          draggedFrameStart = -1;
          draggedFrameStartDuration = -1;
          isDraggedFrameSnapshot = false;
          if (type == anm2::TRIGGER) item->frames_sort_by_at_frame();
        }
      }

      context_menu();

      ImGui::EndChild();
      if (isLightTheme && isDefaultChild) ImGui::PopStyleColor();
      ImGui::PopStyleVar();

      index++;
      ImGui::PopID();
    };

    auto frames_child = [&]()
    {
      auto cursorPos = ImGui::GetCursorPos();
      ImGui::SetCursorPos(ImVec2(cursorPos.x + ITEM_CHILD_WIDTH, cursorPos.y));

      auto framesChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);

      if (ImGui::BeginChild("##Frames Child", framesChildSize, ImGuiChildFlags_Borders))
      {
        auto viewListChildSize =
            ImVec2(ImGui::GetContentRegionAvail().x,
                   ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() - style.ItemSpacing.y * 2);

        auto childWidth = ImGui::GetContentRegionAvail().x > anm2.animations.length() * ImGui::GetTextLineHeight()
                              ? ImGui::GetContentRegionAvail().x
                              : anm2.animations.length() * ImGui::GetTextLineHeight();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
        if (ImGui::BeginChild("##Frames List Child", viewListChildSize, true, ImGuiWindowFlags_HorizontalScrollbar))
        {
          auto cursorScreenPos = ImGui::GetCursorScreenPos();

          ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2());
          if (ImGui::BeginTable("##Frames List Table", 1,
                                ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY))
          {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            window->Flags |= ImGuiWindowFlags_NoScrollWithMouse;

            scroll.x = ImGui::GetScrollX();
            scroll.y = ImGui::GetScrollY();

            isHorizontalScroll = window->ScrollbarX;

            if (isWindowHovered)
            {
              auto& io = ImGui::GetIO();
              auto lineHeight = ImGui::GetTextLineHeightWithSpacing() * 2;

              scroll.x -= io.MouseWheelH * lineHeight;
              scroll.y -= io.MouseWheel * lineHeight;
            }

            ImGui::SetScrollX(scroll.x);
            ImGui::SetScrollY(scroll.y);

            int index{};

            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("##Frames");

            auto frames_child_row = [&](anm2::Type type, int id = -1)
            {
              ImGui::TableNextRow();
              ImGui::TableSetColumnIndex(0);
              frame_child(type, id, index, childWidth);
            };

            frames_child_row(anm2::NONE);

            if (animation)
            {
              ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

              frames_child_row(anm2::ROOT);

              for (auto& id : animation->layerOrder | std::views::reverse)
              {
                if (auto item = animation->item_get(anm2::LAYER, id); item)
                  if (!settings.timelineIsShowUnused && item->frames.empty()) continue;

                frames_child_row(anm2::LAYER, id);
              }

              for (const auto& entry : animation->nullAnimations)
              {
                auto id = entry.first;
                if (auto item = animation->item_get(anm2::NULL_, id); item)
                  if (!settings.timelineIsShowUnused && item->frames.empty()) continue;
                frames_child_row(anm2::NULL_, id);
              }

              frames_child_row(anm2::TRIGGER);

              ImGui::PopStyleVar();
            }
            ImGui::EndTable();
          }

          ImDrawList* windowDrawList = ImGui::GetWindowDrawList();

          auto frameSize = ImVec2(ImGui::GetTextLineHeight(),
                                  ImGui::GetTextLineHeightWithSpacing() + (ImGui::GetStyle().WindowPadding.y * 2));
          auto playheadIndex = std::floor(playback.time);
          auto lineCenterX = cursorScreenPos.x + frameSize.x * (playheadIndex + 0.6f) - scroll.x;
          float lineOffsetY = frameSize.y;
          auto linePos =
              ImVec2(lineCenterX - (PLAYHEAD_LINE_THICKNESS * 0.5f), cursorScreenPos.y + frameSize.y + lineOffsetY);
          auto lineSize = ImVec2((PLAYHEAD_LINE_THICKNESS / 2.0f),
                                 viewListChildSize.y - frameSize.y - lineOffsetY -
                                     (isHorizontalScroll ? ImGui::GetStyle().ScrollbarSize : 0.0f));

          auto rectMin = windowDrawList->GetClipRectMin();
          auto rectMax = windowDrawList->GetClipRectMax();
          if (pickerLineDrawList)
          {
            pickerLineDrawList->PushClipRect(rectMin, rectMax);
            pickerLineDrawList->AddRectFilled(linePos, ImVec2(linePos.x + lineSize.x, linePos.y + lineSize.y),
                                              ImGui::GetColorU32(playheadLineColor));
            pickerLineDrawList->PopClipRect();
          }

          ImGui::PopStyleVar();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

        ImGui::SetCursorPos(
            ImVec2(ImGui::GetStyle().WindowPadding.x, ImGui::GetCursorPos().y + ImGui::GetStyle().ItemSpacing.y));

        auto widgetSize = widget_size_with_row_get(10);

        ImGui::BeginDisabled(!animation);
        {
          auto label = playback.isPlaying ? localize.get(LABEL_PAUSE) : localize.get(LABEL_PLAY);
          auto tooltip =
              playback.isPlaying ? localize.get(TOOLTIP_PAUSE_ANIMATION) : localize.get(TOOLTIP_PLAY_ANIMATION);

          shortcut(manager.chords[SHORTCUT_PLAY_PAUSE]);
          if (ImGui::Button(label, widgetSize)) playback.toggle();
          set_item_tooltip_shortcut(tooltip, settings.shortcutPlayPause);

          ImGui::SameLine();

          auto item = animation->item_get(reference.itemType, reference.itemID);

          ImGui::BeginDisabled(!item);
          {
            shortcut(manager.chords[SHORTCUT_INSERT_FRAME]);
            if (ImGui::Button(localize.get(LABEL_INSERT), widgetSize)) frame_insert(item);
            set_item_tooltip_shortcut(localize.get(TOOLTIP_INSERT_FRAME), settings.shortcutInsertFrame);

            ImGui::SameLine();

            ImGui::BeginDisabled(!document.frame_get());
            {
              shortcut(manager.chords[SHORTCUT_REMOVE]);
              if (ImGui::Button(localize.get(LABEL_DELETE), widgetSize)) frames_delete_action();
              set_item_tooltip_shortcut(localize.get(TOOLTIP_DELETE_FRAMES), settings.shortcutRemove);

              ImGui::SameLine();

              if (ImGui::Button(localize.get(LABEL_BAKE), widgetSize)) bakePopup.open();
              set_item_tooltip_shortcut(localize.get(TOOLTIP_BAKE_FRAMES), settings.shortcutBake);
            }
            ImGui::EndDisabled();
          }
          ImGui::EndDisabled();

          ImGui::SameLine();

          ImGui::BeginDisabled(!animation || animation->frameNum == animation->length());
          shortcut(manager.chords[SHORTCUT_FIT]);
          if (ImGui::Button(localize.get(LABEL_FIT_ANIMATION_LENGTH), widgetSize)) fit_animation_length();
          set_item_tooltip_shortcut(localize.get(TOOLTIP_FIT_ANIMATION_LENGTH), settings.shortcutFit);
          ImGui::EndDisabled();

          ImGui::SameLine();

          auto frameNum = animation ? animation->frameNum : dummy_value<int>();
          ImGui::SetNextItemWidth(widgetSize.x);
          if (input_int_range(localize.get(LABEL_ANIMATION_LENGTH), frameNum, anm2::FRAME_NUM_MIN, anm2::FRAME_NUM_MAX,
                              STEP, STEP_FAST, !animation ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0))
            DOCUMENT_EDIT(document, localize.get(EDIT_ANIMATION_LENGTH), Document::ANIMATIONS,
                          animation->frameNum = frameNum);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ANIMATION_LENGTH));

          ImGui::SameLine();

          auto isLoop = animation ? animation->isLoop : dummy_value<bool>();
          ImGui::SetNextItemWidth(widgetSize.x);
          if (ImGui::Checkbox(localize.get(LABEL_LOOP), &isLoop))
            DOCUMENT_EDIT(document, localize.get(EDIT_LOOP), Document::ANIMATIONS, animation->isLoop = isLoop);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LOOP_ANIMATION));
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        auto fps = anm2.info.fps;
        ImGui::SetNextItemWidth(widgetSize.x);
        if (input_int_range(localize.get(LABEL_FPS), fps, anm2::FPS_MIN, anm2::FPS_MAX))
          DOCUMENT_EDIT(document, localize.get(EDIT_FPS), Document::ANIMATIONS, anm2.info.fps = fps);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_FPS));

        ImGui::SameLine();

        auto createdBy = anm2.info.createdBy;
        ImGui::SetNextItemWidth(widgetSize.x);
        if (input_text_string(localize.get(LABEL_AUTHOR), &createdBy))
          DOCUMENT_EDIT(document, localize.get(EDIT_AUTHOR), Document::ANIMATIONS, anm2.info.createdBy = createdBy);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_AUTHOR));

        ImGui::SameLine();

        ImGui::SetNextItemWidth(widgetSize.x);
        ImGui::Checkbox(localize.get(LABEL_SOUND), &settings.timelineIsSound);
        ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_SOUND));

        ImGui::PopStyleVar();
      }
      ImGui::EndChild();

      ImGui::SetCursorPos(cursorPos);
    };

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
    if (ImGui::Begin(localize.get(LABEL_TIMELINE_WINDOW), &settings.windowIsTimeline))
    {
      isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
      frames_child();
      items_child();
    }
    ImGui::PopStyleVar();
    ImGui::End();

    propertiesPopup.trigger();

    if (ImGui::BeginPopupModal(propertiesPopup.label(), &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto item_properties_close = [&]()
      {
        item_properties_reset();
        propertiesPopup.close();
      };

      auto& type = settings.timelineAddItemType;
      auto& destination = settings.timelineAddItemDestination;
      auto& source = settings.timelineAddItemSource;

      auto footerSize = footer_size_get();
      auto optionsSize = child_size_get(11);
      auto itemsSize = ImVec2(0, ImGui::GetContentRegionAvail().y -
                                     (optionsSize.y + footerSize.y + ImGui::GetStyle().ItemSpacing.y * 4));
      if (ImGui::BeginChild("##Options", optionsSize, ImGuiChildFlags_Borders))
      {
        ImGui::SeparatorText(localize.get(LABEL_TYPE));

        auto size = ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetFrameHeightWithSpacing());

        if (ImGui::BeginChild("##Type 1", size))
        {
          ImGui::RadioButton(localize.get(LABEL_LAYER), &type, anm2::LAYER);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_TYPE));
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("##Type 2", size))
        {
          ImGui::RadioButton(localize.get(LABEL_NULL), &type, anm2::NULL_);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NULL_TYPE));
        }
        ImGui::EndChild();

        ImGui::SeparatorText(localize.get(LABEL_SOURCE));

        if (ImGui::BeginChild("##Source 1", size))
        {
          ImGui::RadioButton(localize.get(LABEL_NEW), &source, source::NEW);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_NEW_ITEM));
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("##Source 2", size))
        {
          auto isUnusedItems = animation && !unused_items_get((anm2::Type)type).empty();
          ImGui::BeginDisabled(!isUnusedItems);
          ImGui::RadioButton(localize.get(LABEL_EXISTING), &source, source::EXISTING);
          ImGui::EndDisabled();
          ImGui::SetItemTooltip("%s", isUnusedItems ? localize.get(TOOLTIP_USE_EXISTING_ITEM)
                                                    : localize.get(TOOLTIP_NO_UNUSED_ITEMS));
        }
        ImGui::EndChild();

        ImGui::SeparatorText(localize.get(LABEL_DESTINATION));

        if (ImGui::BeginChild("##Destination 1", size))
        {
          ImGui::RadioButton(localize.get(LABEL_ALL_ANIMATIONS), &destination, destination::ALL);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_ALL_ANIMATIONS));
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("##Destination 2", size))
        {
          ImGui::RadioButton(localize.get(LABEL_THIS_ANIMATION), &destination, destination::THIS);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_THIS_ANIMATION));
        }
        ImGui::EndChild();

        ImGui::SeparatorText(localize.get(LABEL_OPTIONS));

        ImGui::BeginDisabled(source == source::EXISTING);
        {
          input_text_string(localize.get(BASIC_NAME), &addItemName);
          ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ITEM_NAME));
          ImGui::BeginDisabled(type != anm2::LAYER);
          {
            combo_negative_one_indexed(localize.get(LABEL_SPRITESHEET), &addItemSpritesheetID,
                                       document.spritesheet.labels);
            ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_LAYER_SPRITESHEET));
          }
          ImGui::EndDisabled();
        }
        ImGui::EndDisabled();
      }
      ImGui::EndChild();

      if (ImGui::BeginChild("##Items", itemsSize, ImGuiChildFlags_Borders))
      {
        if (animation && source == source::EXISTING)
        {
          auto unusedItems = unused_items_get((anm2::Type)type);
          if (addItemID != -1 && !unusedItems.contains(addItemID)) addItemID = -1;

          for (auto id : unusedItems)
          {
            auto isSelected = addItemID == id;

            ImGui::PushID(id);

            if (type == anm2::LAYER)
            {
              auto& layer = anm2.content.layers[id];
              auto label =
                  std::vformat(localize.get(FORMAT_LAYER), std::make_format_args(id, layer.name, layer.spritesheetID));
              if (ImGui::Selectable(label.c_str(), isSelected)) addItemID = id;
            }
            else if (type == anm2::NULL_)
            {
              auto& null = anm2.content.nulls[id];
              auto label = std::vformat(localize.get(FORMAT_NULL), std::make_format_args(id, null.name));
              if (ImGui::Selectable(label.c_str(), isSelected)) addItemID = id;
            }

            ImGui::PopID();
          }
        }
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      ImGui::BeginDisabled(source == source::EXISTING && addItemID == -1);
      if (ImGui::Button(localize.get(BASIC_ADD), widgetSize))
      {
        anm2::Reference addReference{};

        document.snapshot(localize.get(EDIT_ADD_ITEM));
        if (type == anm2::LAYER)
          addReference = anm2.layer_animation_add({reference.animationIndex, anm2::LAYER, addItemID}, addItemName,
                                                  addItemSpritesheetID - 1, (destination::Type)destination);
        else if (type == anm2::NULL_)
          addReference = anm2.null_animation_add({reference.animationIndex, anm2::LAYER, addItemID}, addItemName,
                                                 (destination::Type)destination);

        document.change(Document::ITEMS);

        reference_set_item(addReference.itemType, addReference.itemID);

        item_properties_close();
      }
      ImGui::EndDisabled();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ADD_ITEM));

      ImGui::SameLine();

      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) item_properties_close();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CANCEL_ADD_ITEM));

      ImGui::EndPopup();
    }

    bakePopup.trigger();

    if (ImGui::BeginPopupModal(bakePopup.label(), &bakePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto& interval = settings.bakeInterval;
      auto& isRoundRotation = settings.bakeIsRoundRotation;
      auto& isRoundScale = settings.bakeIsRoundScale;

      auto frame = document.frame_get();

      input_int_range(localize.get(LABEL_INTERVAL), interval, anm2::FRAME_DURATION_MIN,
                      frame ? frame->duration : anm2::FRAME_DURATION_MIN);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_INTERVAL));

      ImGui::Checkbox(localize.get(LABEL_ROUND_ROTATION), &isRoundRotation);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROUND_ROTATION));

      ImGui::Checkbox(localize.get(LABEL_ROUND_SCALE), &isRoundScale);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_ROUND_SCALE));

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button(localize.get(LABEL_BAKE), widgetSize))
      {
        frames_bake();
        bakePopup.close();
      }
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_BAKE_FRAMES_OPTIONS));

      ImGui::SameLine();

      if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) bakePopup.close();
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CANCEL_BAKE_FRAMES));

      ImGui::EndPopup();
    }

    if (animation)
    {
      if (shortcut(manager.chords[SHORTCUT_PLAY_PAUSE], shortcut::GLOBAL)) playback.toggle();

      if (shortcut(manager.chords[SHORTCUT_MOVE_PLAYHEAD_BACK], shortcut::GLOBAL))
      {
        playback.decrement(settings.playbackIsClamp ? animation->frameNum : anm2::FRAME_NUM_MAX);
        document.frameTime = playback.time;
      }

      if (shortcut(manager.chords[SHORTCUT_MOVE_PLAYHEAD_FORWARD], shortcut::GLOBAL))
      {
        playback.increment(settings.playbackIsClamp ? animation->frameNum : anm2::FRAME_NUM_MAX);
        document.frameTime = playback.time;
      }

      static bool isShortenChordHeld = false;
      auto isShortenFrame = shortcut(manager.chords[SHORTCUT_SHORTEN_FRAME], shortcut::GLOBAL);

      if (isShortenFrame && !isShortenChordHeld) document.snapshot(localize.get(EDIT_SHORTEN_FRAME));
      if (isShortenFrame)
      {

        if (auto frame = document.frame_get())
        {
          frame->shorten();
          document.change(Document::FRAMES);
        }
      }
      isShortenChordHeld = isShortenFrame;

      static bool isExtendChordHeld = false;
      auto isExtendFrame = shortcut(manager.chords[SHORTCUT_EXTEND_FRAME], shortcut::GLOBAL);
      if (isExtendFrame && !isExtendChordHeld) document.snapshot(localize.get(EDIT_EXTEND_FRAME));
      if (isExtendFrame)
      {

        if (auto frame = document.frame_get())
        {
          frame->extend();
          document.change(Document::FRAMES);
        }
      }
      isExtendChordHeld = isExtendFrame;

      if (shortcut(manager.chords[SHORTCUT_PREVIOUS_FRAME], shortcut::GLOBAL))
      {
        if (auto item = document.item_get(); !item->frames.empty())
        {
          reference.frameIndex = glm::clamp(--reference.frameIndex, 0, (int)item->frames.size() - 1);
          frames_selection_set_reference();
          document.frameTime = item->frame_time_from_index_get(reference.frameIndex);
        }
      }

      if (shortcut(manager.chords[SHORTCUT_NEXT_FRAME], shortcut::GLOBAL))
      {
        if (auto item = document.item_get(); !item->frames.empty())
        {
          reference.frameIndex = glm::clamp(++reference.frameIndex, 0, (int)item->frames.size() - 1);
          frames_selection_set_reference();
          document.frameTime = item->frame_time_from_index_get(reference.frameIndex);
        }
      }
    }

    if (isTextPushed) ImGui::PopStyleColor();
  }
}
