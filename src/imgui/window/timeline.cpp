#include "timeline.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>

#include <imgui_internal.h>

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
  constexpr int ITEM_SELECTION_NULL_FLAG = 1 << 30;

  constexpr auto HELP_FORMAT = R"(- Press {} to decrement time.
- Press {} to increment time.
- Press {} to shorten the selected frame, by one frame.
- Press {} to extend the selected frame, by one frame.
- Press {} to go to the previous frame.
- Press {} to go to the next frame.
- Click and hold on a frame while holding CTRL to change its duration.
- Click and hold on a trigger to change its At Frame.
- Hold Alt while clicking a non-trigger frame to toggle interpolation.)";

  void Timeline::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    auto& document = *manager.get();
    auto& anm2 = document.anm2;
    auto& playback = document.playback;
    auto& reference = document.reference;
    auto& frames = document.frames;
    auto& items = document.items;
    auto& itemSelection = items.selection;
    auto animation = document.animation_get();
    auto item_selection_encode = [&](anm2::Type type, int id) -> int
    {
      if (type == anm2::NULL_) return id | ITEM_SELECTION_NULL_FLAG;
      return id;
    };
    auto item_selection_decode = [&](int value) -> int { return value & ~ITEM_SELECTION_NULL_FLAG; };
    auto item_selection_value_type = [&](int value) -> anm2::Type
    {
      return (value & ITEM_SELECTION_NULL_FLAG) ? anm2::NULL_ : anm2::LAYER;
    };
    std::vector<int> itemSelectionIndexMap{};
    std::unordered_map<int, int> layerSelectionIndex{};
    std::unordered_map<int, int> nullSelectionIndex{};
    if (animation)
    {
      itemSelectionIndexMap.reserve(animation->layerOrder.size() + animation->nullAnimations.size());
      for (auto id : animation->layerOrder)
      {
        layerSelectionIndex[id] = (int)itemSelectionIndexMap.size();
        itemSelectionIndexMap.push_back(item_selection_encode(anm2::LAYER, id));
      }
      for (auto& [id, nullAnimation] : animation->nullAnimations)
      {
        (void)nullAnimation;
        nullSelectionIndex[id] = (int)itemSelectionIndexMap.size();
        itemSelectionIndexMap.push_back(item_selection_encode(anm2::NULL_, id));
      }
      itemSelection.set_index_map(&itemSelectionIndexMap);
    }
    else
      itemSelection.set_index_map(nullptr);

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

    items.hovered = -1;

    auto item_selection_type_get = [&]() -> anm2::Type
    {
      if (itemSelection.empty() || !animation) return anm2::NONE;

      for (auto encoded : itemSelection)
      {
        auto valueType = item_selection_value_type(encoded);
        auto valueID = item_selection_decode(encoded);
        if (valueType == anm2::LAYER && animation->layerAnimations.contains(valueID)) return anm2::LAYER;
        if (valueType == anm2::NULL_ && animation->nullAnimations.contains(valueID)) return anm2::NULL_;
      }

      return anm2::NONE;
    };

    auto item_selection_clear = [&]()
    {
      itemSelection.clear();
      items.reference = -1;
      document.layer.selection.clear();
      document.null.selection.clear();
    };

    auto item_selection_sync = [&]()
    {
      if (itemSelection.empty())
      {
        item_selection_clear();
        return;
      }

      auto type = item_selection_type_get();
      items.reference = (int)type;

      auto assign_selection = [&](MultiSelectStorage& target, anm2::Type assignType)
      {
        target.clear();
        for (auto encoded : itemSelection)
        {
          if (item_selection_value_type(encoded) != assignType) continue;
          target.insert(item_selection_decode(encoded));
        }
      };

      if (type == anm2::LAYER)
        assign_selection(document.layer.selection, anm2::LAYER);
      else if (type == anm2::NULL_)
        assign_selection(document.null.selection, anm2::NULL_);
      else
        item_selection_clear();
    };

    auto item_selection_prune = [&]()
    {
      if (itemSelection.empty())
      {
        items.reference = -1;
        return;
      }

      if (!animation)
      {
        item_selection_clear();
        return;
      }

      auto type = item_selection_type_get();
      if (type != anm2::LAYER && type != anm2::NULL_)
      {
        item_selection_clear();
        return;
      }

      for (auto it = itemSelection.begin(); it != itemSelection.end();)
      {
        if (item_selection_value_type(*it) != type)
        {
          it = itemSelection.erase(it);
          continue;
        }

        auto valueID = item_selection_decode(*it);
        bool exists =
            type == anm2::LAYER ? animation->layerAnimations.contains(valueID) : animation->nullAnimations.contains(valueID);
        if (!exists)
          it = itemSelection.erase(it);
        else
          ++it;
      }

      if (itemSelection.empty())
        item_selection_clear();
      else
        item_selection_sync();
    };

    item_selection_prune();

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

    auto frames_delete = [&]()
    {
      if (auto item = animation->item_get(reference.itemType, reference.itemID); item)
      {
        for (auto it = frames.selection.rbegin(); it != frames.selection.rend(); ++it)
        {
          auto i = *it;
          item->frames.erase(item->frames.begin() + i);
        }

        reference.frameIndex = glm::clamp(--reference.frameIndex, -1, (int)item->frames.size() - 1);
        frames_selection_set_reference();
      }
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

    auto context_menu = [&]()
    {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);

      auto copy = [&]()
      {
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
        DOCUMENT_EDIT(document, "Cut Frame(s)", Document::FRAMES, frames_delete());
      };

      auto paste = [&]()
      {
        if (auto item = animation->item_get(reference.itemType, reference.itemID))
        {
          document.snapshot("Paste Frame(s)");
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
            animation->fit_length();
            document.change(Document::FRAMES);
          }
          else
            toasts.error(std::format("Failed to deserialize frame(s): {}", errorString));
        }
        else
          toasts.error(std::format("Failed to deserialize frame(s): select an item first!"));
      };

      if (shortcut(manager.chords[SHORTCUT_CUT], shortcut::FOCUSED)) cut();
      if (shortcut(manager.chords[SHORTCUT_COPY], shortcut::FOCUSED)) copy();
      if (shortcut(manager.chords[SHORTCUT_PASTE], shortcut::FOCUSED)) paste();

      if (ImGui::BeginPopupContextWindow("##Context Menu", ImGuiPopupFlags_MouseButtonRight))
      {
        if (ImGui::MenuItem("Cut", settings.shortcutCut.c_str(), false, !frames.selection.empty())) cut();
        if (ImGui::MenuItem("Copy", settings.shortcutCopy.c_str(), false, !frames.selection.empty())) copy();
        if (ImGui::MenuItem("Paste", nullptr, false, !clipboard.is_empty())) paste();
        ImGui::EndPopup();
      }

      ImGui::PopStyleVar(2);
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

    auto item_selection_index_get = [&](anm2::Type type, int id)
    {
      if (type == anm2::LAYER)
      {
        if (auto it = layerSelectionIndex.find(id); it != layerSelectionIndex.end()) return it->second;
      }
      else if (type == anm2::NULL_)
      {
        if (auto it = nullSelectionIndex.find(id); it != nullSelectionIndex.end()) return it->second;
      }

      return -1;
    };

    auto item_child = [&](anm2::Type type, int id, int& index)
    {
      ImGui::PushID(index);

      auto item = animation ? animation->item_get(type, id) : nullptr;
      auto isVisible = item ? item->isVisible : false;
      auto& isOnlyShowLayers = settings.timelineIsOnlyShowLayers;
      if (isOnlyShowLayers && type != anm2::LAYER) isVisible = false;
      auto isActive = reference.itemType == type && reference.itemID == id;

      auto label = type == anm2::LAYER   ? std::format(anm2::LAYER_FORMAT, id, anm2.content.layers[id].name,
                                                       anm2.content.layers[id].spritesheetID)
                   : type == anm2::NULL_ ? std::format(anm2::NULL_FORMAT, id, anm2.content.nulls[id].name)
                                         : anm2::TYPE_STRINGS[type];
      auto icon = anm2::TYPE_ICONS[type];
      auto iconTintCurrent = isLightTheme && type == anm2::NONE ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : itemIconTint;
      auto baseColorVec = item_color_vec(type);
      auto activeColorVec = item_color_active_vec(type);
      auto selectionType = item_selection_type_get();
      bool isSelectableItem = type == anm2::LAYER || type == anm2::NULL_;
      auto selectionIndex = item_selection_index_get(type, id);
      int selectionValue = item_selection_encode(type, id);
      bool isMultiSelected = isSelectableItem && selectionType == type && itemSelection.contains(selectionValue);
      bool isTypeNone = type == anm2::NONE;
      auto colorVec = baseColorVec;
      if ((isActive || isMultiSelected) && !isTypeNone)
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

      auto itemSize = ImVec2(ImGui::GetContentRegionAvail().x,
                             ImGui::GetTextLineHeightWithSpacing() + (ImGui::GetStyle().WindowPadding.y * 2));

      if (ImGui::BeginChild(label.c_str(), itemSize, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollWithMouse))
      {
        auto isReferenced = reference.itemType == type && reference.itemID == id;

        auto cursorPos = ImGui::GetCursorPos();

        if (!isTypeNone)
        {
          ImGui::SetCursorPos(to_imvec2(to_vec2(cursorPos) - to_vec2(style.ItemSpacing)));

          ImGui::SetNextItemAllowOverlap();
          ImGui::PushStyleColor(ImGuiCol_Header, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4());
          ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4());
          if (isSelectableItem && selectionIndex != -1) ImGui::SetNextItemSelectionUserData(selectionIndex);
          if (ImGui::Selectable("##Item Button", isSelectableItem && isMultiSelected, ImGuiSelectableFlags_SelectOnNav,
                                itemSize))
          {
            if (isSelectableItem)
            {
              auto previousType = item_selection_type_get();
              bool typeMismatch =
                  !itemSelection.empty() && previousType != anm2::NONE && previousType != type;
              if (typeMismatch)
              {
                itemSelection.clear();
                itemSelection.insert(selectionValue);
              }
              item_selection_sync();
            }

            if (type == anm2::LAYER) document.spritesheet.reference = anm2.content.layers[id].spritesheetID;

            reference_set_item(type, id);
          }
          ImGui::PopStyleColor(3);
          if (ImGui::IsItemHovered()) items.hovered = id;
          if (ImGui::IsItemHovered())
          {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
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
              }
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

                DOCUMENT_EDIT(document, "Move Layer Animation", Document::ITEMS, layer_order_move());
              }
              ImGui::EndDragDropTarget();
            }
          }

          ImGui::SetCursorPos(cursorPos);

          ImGui::Image(resources.icons[icon].id, icon_size_get());
          overlay_icon(resources.icons[icon].id, iconTintCurrent);
          ImGui::SameLine();
          if (isReferenced) ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
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
            DOCUMENT_EDIT(document, "Item Visibility", Document::FRAMES, item->isVisible = !item->isVisible);
          overlay_icon(resources.icons[visibleIcon].id, iconTintCurrent);
          ImGui::SetItemTooltip(isVisible ? "The item is shown. Press to hide." : "The item is hidden. Press to show.");

          if (type == anm2::NULL_)
          {
            auto& null = anm2.content.nulls.at(id);
            auto& isShowRect = null.isShowRect;
            auto rectIcon = isShowRect ? icon::SHOW_RECT : icon::HIDE_RECT;
            ImGui::SetCursorPos(
                ImVec2(itemSize.x - (ImGui::GetTextLineHeightWithSpacing() * 2) - ImGui::GetStyle().ItemSpacing.x,
                       (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
            if (ImGui::ImageButton("##Rect Toggle", resources.icons[rectIcon].id, icon_size_get()))
              DOCUMENT_EDIT(document, "Null Rect", Document::FRAMES, null.isShowRect = !null.isShowRect);
            overlay_icon(resources.icons[rectIcon].id, iconTintCurrent);
            ImGui::SetItemTooltip(isShowRect ? "The null's rect is shown. Press to hide."
                                             : "The null's rect is hidden. Press to show.");
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
          ImGui::SetItemTooltip(isShowUnused ? "Unused layers/nulls are shown. Press to hide."
                                             : "Unused layers/nulls are hidden. Press to show.");

          auto& showLayersOnly = settings.timelineIsOnlyShowLayers;
          auto layersIcon = showLayersOnly ? icon::SHOW_LAYERS : icon::HIDE_LAYERS;
          ImGui::SetCursorPos(
              ImVec2(itemSize.x - (ImGui::GetTextLineHeightWithSpacing() * 2) - ImGui::GetStyle().ItemSpacing.x,
                     (itemSize.y - ImGui::GetTextLineHeightWithSpacing()) / 2));
          if (ImGui::ImageButton("##Layers Toggle", resources.icons[layersIcon].id, icon_size_get()))
            showLayersOnly = !showLayersOnly;
          overlay_icon(resources.icons[layersIcon].id, iconTintCurrent);
          ImGui::SetItemTooltip(showLayersOnly ? "Only layers are visible. Press to show all items."
                                               : "All items are visible. Press to only show layers.");
          ImGui::PopStyleVar(2);
          ImGui::PopStyleColor(3);

          ImGui::SetCursorPos(cursorPos);

          ImGui::BeginDisabled();
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
          ImGui::Text("(?)");
          ImGui::PopStyleColor();
          ImGui::SetItemTooltip("%s", std::format(HELP_FORMAT, settings.shortcutMovePlayheadBack,
                                                  settings.shortcutMovePlayheadForward, settings.shortcutShortenFrame,
                                                  settings.shortcutExtendFrame, settings.shortcutPreviousFrame,
                                                  settings.shortcutNextFrame)
                                          .c_str());
          ImGui::EndDisabled();
        }
      }
      ImGui::EndChild();
      ImGui::PopStyleColor();
      ImGui::PopStyleVar(2);
      index++;

      ImGui::PopID();
    };

    auto items_child = [&]()
    {
      auto itemsChildSize = ImVec2(ImGui::GetTextLineHeightWithSpacing() * 15, ImGui::GetContentRegionAvail().y);

      if (ImGui::BeginChild("##Items Child", itemsChildSize, ImGuiChildFlags_Borders))
      {
        auto itemsListChildSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y -
                                                                               ImGui::GetTextLineHeightWithSpacing() -
                                                                               ImGui::GetStyle().ItemSpacing.y * 2);

        if (ImGui::BeginChild("##Items List Child", itemsListChildSize, ImGuiChildFlags_Borders,
                              ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
        {
          ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2());
          ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0f);
          if (ImGui::BeginTable("##Item Table", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY))
          {
            ImGui::GetCurrentWindow()->Flags |= ImGuiWindowFlags_NoScrollWithMouse;
            ImGui::SetScrollY(scroll.y);

            int index{};

            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("##Items");

            auto item_child_row = [&](anm2::Type type, int id = -1)
            {
              ImGui::TableNextRow();
              ImGui::TableSetColumnIndex(0);
              item_child(type, id, index);
            };

            item_child_row(anm2::NONE);

            if (animation)
            {
              item_selection_prune();
              itemSelection.start(itemSelectionIndexMap.size(), ImGuiMultiSelectFlags_ClearOnEscape);

              item_child_row(anm2::ROOT);

              for (auto& id : animation->layerOrder)
              {
                if (!settings.timelineIsShowUnused && animation->layerAnimations[id].frames.empty()) continue;
                item_child_row(anm2::LAYER, id);
              }

              for (auto& [id, nullAnimation] : animation->nullAnimations)
              {
                if (!settings.timelineIsShowUnused && nullAnimation.frames.empty()) continue;
                item_child_row(anm2::NULL_, id);
              }

              item_child_row(anm2::TRIGGER);

              itemSelection.finish();
              item_selection_sync();
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
        }
        ImGui::EndChild();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);

        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + style.WindowPadding.x, ImGui::GetCursorPosY()));
        auto widgetSize = widget_size_with_row_get(2, ImGui::GetContentRegionAvail().x - style.WindowPadding.x);

        ImGui::BeginDisabled(!animation);
        {
          shortcut(manager.chords[SHORTCUT_ADD]);
          if (ImGui::Button("Add", widgetSize))
          {
            item_properties_reset();
            propertiesPopup.open();
          }
          set_item_tooltip_shortcut("Add a new item to the animation.", settings.shortcutAdd);
          ImGui::SameLine();

          auto selectionType = item_selection_type_get();
          bool hasSelection = !itemSelection.empty() && (selectionType == anm2::LAYER || selectionType == anm2::NULL_);
          bool hasReferenceItem = document.item_get() != nullptr;
          ImGui::BeginDisabled(!hasSelection && !hasReferenceItem);
          {
            shortcut(manager.chords[SHORTCUT_REMOVE]);
            if (ImGui::Button("Remove", widgetSize))
            {
              auto remove = [&]()
              {
                if (hasSelection)
                {
                  std::vector<int> ids{};
                  ids.reserve(itemSelection.size());
                  for (auto value : itemSelection)
                    ids.push_back(item_selection_decode(value));
                  std::sort(ids.begin(), ids.end());
                  for (auto id : ids)
                    animation->item_remove(selectionType, id);
                  item_selection_clear();
                }
                else if (reference.itemType == anm2::LAYER || reference.itemType == anm2::NULL_)
                  animation->item_remove(reference.itemType, reference.itemID);
                reference_clear();
              };

              DOCUMENT_EDIT(document, "Remove Item(s)", Document::ITEMS, remove());
            }
            set_item_tooltip_shortcut("Remove the selected item(s) from the animation.", settings.shortcutRemove);
          }
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

      auto childSize = ImVec2(width, ImGui::GetTextLineHeightWithSpacing() + (ImGui::GetStyle().WindowPadding.y * 2));

      ImGui::PopStyleVar(2);

      ImGui::PushID(index);

      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
      bool isDefaultChild = type == anm2::NONE;
      if (isLightTheme && isDefaultChild) ImGui::PushStyleColor(ImGuiCol_ChildBg, TIMELINE_CHILD_BG_COLOR_LIGHT);

      if (ImGui::BeginChild("##Frames Child", childSize, ImGuiChildFlags_Borders))
      {
        auto drawList = ImGui::GetWindowDrawList();
        auto clipMax = drawList->GetClipRectMax();
        auto length = animation ? animation->frameNum : anm2.animations.length();
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

          if (ImGui::IsMouseReleased(0)) isDragging = false;

          ImGui::SetCursorPos(ImVec2(cursorPos.x + frameSize.x * floorf(playback.time), cursorPos.y));
          ImGui::Image(resources.icons[icon::PLAYHEAD].id, frameSize);
          overlay_icon(resources.icons[icon::PLAYHEAD].id, playheadIconTint, true);
        }
        else if (animation)
        {
          float frameTime{};

          if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
          {
            reference_set_item(type, id);
          }

          for (int i = frameMin; i < frameMax; i++)
          {
            auto frameScreenPos = ImVec2(cursorScreenPos.x + frameSize.x * (float)i, cursorScreenPos.y);
            auto frameRectMax = ImVec2(frameScreenPos.x + frameSize.x, frameScreenPos.y + frameSize.y);

            drawList->AddRect(frameScreenPos, frameRectMax, ImGui::GetColorU32(frameBorderColor));

            if (i % FRAME_MULTIPLE == 0)
              drawList->AddRectFilled(frameScreenPos, frameRectMax, ImGui::GetColorU32(frameMultipleOverlayColor));
          }

          frames.selection.start(item->frames.size(), ImGuiMultiSelectFlags_ClearOnEscape);

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
            if (ImGui::Selectable("##Frame Button", isSelected, ImGuiSelectableFlags_None, buttonSize))
            {
              if (type == anm2::LAYER)
              {
                document.spritesheet.reference = anm2.content.layers[id].spritesheetID;
                document.layer.selection = {id};
              }
              else if (type == anm2::NULL_)
                document.null.selection = {id};

              if (type != anm2::TRIGGER)
              {
                if (ImGui::IsKeyDown(ImGuiMod_Alt))
                  DOCUMENT_EDIT(document, "Frame Interpolation", Document::FRAMES,
                                frame.isInterpolated = !frame.isInterpolated);

                document.frameTime = frameTime;
              }

              reference = frameReference;
              isReferenced = true;
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
                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::Text("%s Frame", anm2::TYPE_STRINGS[type]);
                ImGui::PopFont();

                auto float_text = [&](std::string label, float& value)
                {
                  std::string useFormat = math::float_format_get(value);
                  std::string format(label + ": " + useFormat);
                  ImGui::Text(format.c_str(), value);
                };

                auto vec2_text = [&](std::string label, vec2& value)
                {
                  std::string useFormat = math::vec2_format_get(value);
                  std::string format(label + ": " + useFormat + ", " + useFormat);
                  ImGui::Text(format.c_str(), value.x, value.y);
                };

                ImGui::Text("Index: %i", (int)i);

                if (type == anm2::LAYER)
                {
                  vec2_text("Crop", frame.crop);
                  vec2_text("Size", frame.size);
                }

                vec2_text("Position", frame.size);

                if (type == anm2::LAYER) vec2_text("Pivot", frame.pivot);

                vec2_text("Scale", frame.scale);
                float_text("Rotation", frame.rotation);
                ImGui::Text("Duration: %i", frame.duration);
                ImGui::Text("Tint: %i, %i, %i, %i", math::float_to_uint8(frame.tint.r),
                            math::float_to_uint8(frame.tint.g), math::float_to_uint8(frame.tint.b),
                            math::float_to_uint8(frame.tint.a));
                ImGui::Text("Color Offset: %i, %i, %i", math::float_to_uint8(frame.tint.r),
                            math::float_to_uint8(frame.tint.g), math::float_to_uint8(frame.tint.b));
                ImGui::Text("Visible: %s", frame.isVisible ? "true" : "false");
                ImGui::Text("Interpolated: %s", frame.isInterpolated ? "true" : "false");
              }
              else
              {
                ImGui::PushFont(resources.fonts[font::BOLD].get(), font::SIZE);
                ImGui::Text("%s", anm2::TYPE_STRINGS[type]);
                ImGui::PopFont();

                ImGui::Text("At Frame: %i", frame.atFrame);
                ImGui::Text("Event: %s", document.event.labels[frame.eventID + 1]);
                ImGui::Text("Sound: %s", document.sound.labels[frame.soundID + 1]);
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

                if (frameDragDrop.selection.empty())
                {
                  frameDragDrop.selection.push_back((int)i);
                }

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
                    DOCUMENT_EDIT(document, "Move Frame(s)", Document::FRAMES, {
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
                        document.layer.selection = {id};
                      }
                      else if (type == anm2::NULL_)
                        document.null.selection = {id};
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

          frames.selection.finish();
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

          if (draggedFrame)
          {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            if (!isDraggedFrameSnapshot && hoveredTime != draggedFrameStart)
            {
              isDraggedFrameSnapshot = true;
              document.snapshot(type == anm2::TRIGGER ? "Trigger At Frame" : "Frame Duration");
            }

            if (type == anm2::TRIGGER)
            {
              draggedFrame->atFrame = glm::clamp(
                  hoveredTime, 0, settings.playbackIsClamp ? animation->frameNum - 1 : anm2::FRAME_NUM_MAX - 1);

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
              draggedFrameIndex = -1;
              draggedFrameStart = -1;
              draggedFrameStartDuration = -1;
              isDraggedFrameSnapshot = false;
              if (type == anm2::TRIGGER) item->frames_sort_by_at_frame();
            }
          }
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
      auto itemsChildWidth = ImGui::GetTextLineHeightWithSpacing() * 15;

      auto cursorPos = ImGui::GetCursorPos();
      ImGui::SetCursorPos(ImVec2(cursorPos.x + itemsChildWidth, cursorPos.y));

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

              for (auto& id : animation->layerOrder)
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
          pickerLineDrawList->PushClipRect(rectMin, rectMax);
          pickerLineDrawList->AddRectFilled(linePos, ImVec2(linePos.x + lineSize.x, linePos.y + lineSize.y),
                                            ImGui::GetColorU32(playheadLineColor));
          pickerLineDrawList->PopClipRect();

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
          auto label = playback.isPlaying ? "Pause" : "Play";
          auto tooltip = playback.isPlaying ? "Pause the animation." : "Play the animation.";

          shortcut(manager.chords[SHORTCUT_PLAY_PAUSE]);
          if (ImGui::Button(label, widgetSize)) playback.toggle();
          set_item_tooltip_shortcut(tooltip, settings.shortcutPlayPause);

          ImGui::SameLine();

          auto item = animation->item_get(reference.itemType, reference.itemID);

          ImGui::BeginDisabled(!item);
          {
            shortcut(manager.chords[SHORTCUT_INSERT_FRAME]);
            if (ImGui::Button("Insert", widgetSize))
            {
              auto insert_frame = [&]()
              {
                if (reference.itemType == anm2::TRIGGER)
                {
                  for (auto& trigger : animation->triggers.frames)
                    if (document.frameTime == trigger.atFrame) return;

                  auto addFrame = anm2::Frame();
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

              DOCUMENT_EDIT(document, "Insert Frame", Document::FRAMES, insert_frame());
            }
            set_item_tooltip_shortcut("Insert a frame, based on the current selection.", settings.shortcutInsertFrame);

            ImGui::SameLine();

            ImGui::BeginDisabled(!document.frame_get());
            {
              shortcut(manager.chords[SHORTCUT_REMOVE]);
              if (ImGui::Button("Delete", widgetSize))
                DOCUMENT_EDIT(document, "Delete Frame(s)", Document::FRAMES, frames_delete());
              set_item_tooltip_shortcut("Delete the selected frames.", settings.shortcutRemove);

              ImGui::SameLine();

              if (ImGui::Button("Bake", widgetSize)) bakePopup.open();
              ImGui::SetItemTooltip("Turn interpolated frames into uninterpolated ones.");
            }
            ImGui::EndDisabled();
          }
          ImGui::EndDisabled();

          ImGui::SameLine();

          ImGui::BeginDisabled(!animation || animation->frameNum == animation->length());
          if (ImGui::Button("Fit Animation Length", widgetSize))
            DOCUMENT_EDIT(document, "Fit Animation Length", Document::ANIMATIONS, animation->fit_length());
          ImGui::SetItemTooltip("The animation length will be set to the effective length of the animation.");
          ImGui::EndDisabled();

          ImGui::SameLine();

          auto frameNum = animation ? animation->frameNum : dummy_value<int>();
          ImGui::SetNextItemWidth(widgetSize.x);
          if (input_int_range("Animation Length", frameNum, anm2::FRAME_NUM_MIN, anm2::FRAME_NUM_MAX, STEP, STEP_FAST,
                              !animation ? ImGuiInputTextFlags_DisplayEmptyRefVal : 0))
            DOCUMENT_EDIT(document, "Animation Length", Document::ANIMATIONS, animation->frameNum = frameNum);
          ImGui::SetItemTooltip("Set the animation's length.");

          ImGui::SameLine();

          auto isLoop = animation ? animation->isLoop : dummy_value<bool>();
          ImGui::SetNextItemWidth(widgetSize.x);
          if (ImGui::Checkbox("Loop", &isLoop))
            DOCUMENT_EDIT(document, "Loop", Document::ANIMATIONS, animation->isLoop = isLoop);
          ImGui::SetItemTooltip("Toggle the animation looping.");
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        auto fps = anm2.info.fps;
        ImGui::SetNextItemWidth(widgetSize.x);
        if (input_int_range("FPS", fps, anm2::FPS_MIN, anm2::FPS_MAX))
          DOCUMENT_EDIT(document, "FPS", Document::ANIMATIONS, anm2.info.fps = fps);
        ImGui::SetItemTooltip("Set the FPS of all animations.");

        ImGui::SameLine();

        auto createdBy = anm2.info.createdBy;
        ImGui::SetNextItemWidth(widgetSize.x);
        if (input_text_string("Author", &createdBy))
          DOCUMENT_EDIT(document, "Author", Document::ANIMATIONS, anm2.info.createdBy = createdBy);
        ImGui::SetItemTooltip("Set the author of the document.");

        ImGui::SameLine();

        ImGui::SetNextItemWidth(widgetSize.x);
        ImGui::Checkbox("Sound", &settings.timelineIsSound);
        ImGui::SetItemTooltip("Toggle sounds playing with triggers.\nBind sounds to events in the Events window.");

        ImGui::PopStyleVar();
      }
      ImGui::EndChild();

      ImGui::SetCursorPos(cursorPos);
    };

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
    if (ImGui::Begin("Timeline", &settings.windowIsTimeline))
    {
      isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
      frames_child();
      items_child();
    }
    ImGui::PopStyleVar();
    ImGui::End();

    propertiesPopup.trigger();

    if (ImGui::BeginPopupModal(propertiesPopup.label, &propertiesPopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto item_properties_close = [&]()
      {
        item_properties_reset();
        propertiesPopup.close();
      };

      auto& type = settings.timelineAddItemType;
      auto& locale = settings.timelineAddItemLocale;
      auto& source = settings.timelineAddItemSource;

      auto footerSize = footer_size_get();
      auto optionsSize = child_size_get(11);
      auto itemsSize = ImVec2(0, ImGui::GetContentRegionAvail().y -
                                     (optionsSize.y + footerSize.y + ImGui::GetStyle().ItemSpacing.y * 4));
      if (ImGui::BeginChild("Options", optionsSize, ImGuiChildFlags_Borders))
      {
        ImGui::SeparatorText("Type");

        auto size = ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetFrameHeightWithSpacing());

        if (ImGui::BeginChild("Type Layer", size))
        {
          ImGui::RadioButton("Layer", &type, anm2::LAYER);
          ImGui::SetItemTooltip("Layers are a basic visual element in an animation, used for displaying spritesheets.");
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("Type Null", size))
        {
          ImGui::RadioButton("Null", &type, anm2::NULL_);
          ImGui::SetItemTooltip(
              "Nulls are invisible elements in an animation, used for interfacing with a game engine.");
        }
        ImGui::EndChild();

        ImGui::SeparatorText("Source");

        if (ImGui::BeginChild("Source New", size))
        {
          ImGui::RadioButton("New", &source, source::NEW);
          ImGui::SetItemTooltip("Create a new item to be used.");
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("Source Existing", size))
        {
          auto hasUnusedItems = animation && !unused_items_get((anm2::Type)type).empty();
          ImGui::BeginDisabled(!hasUnusedItems);
          ImGui::RadioButton("Existing", &source, source::EXISTING);
          ImGui::EndDisabled();
          auto tooltip =
              hasUnusedItems ? "Use a pre-existing, presently unused item." : "No unused items are available.";
          ImGui::SetItemTooltip("%s", tooltip);
        }
        ImGui::EndChild();

        ImGui::SeparatorText("Locale");

        if (ImGui::BeginChild("Locale Global", size))
        {
          ImGui::RadioButton("Global", &locale, locale::GLOBAL);
          ImGui::SetItemTooltip("The item will be inserted into all animations, if not already present.");
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("Locale Local", size))
        {
          ImGui::RadioButton("Local", &locale, locale::LOCAL);
          ImGui::SetItemTooltip("The item will only be inserted into this animation.");
        }
        ImGui::EndChild();

        ImGui::SeparatorText("Options");

        ImGui::BeginDisabled(source == source::EXISTING);
        {
          input_text_string("Name", &addItemName);
          ImGui::SetItemTooltip("Set the item's name.");
          ImGui::BeginDisabled(type != anm2::LAYER);
          {
            combo_negative_one_indexed("Spritesheet", &addItemSpritesheetID, document.spritesheet.labels);
            ImGui::SetItemTooltip("Set the layer item's spritesheet.");
          }
          ImGui::EndDisabled();
        }
        ImGui::EndDisabled();
      }
      ImGui::EndChild();

      if (ImGui::BeginChild("Items", itemsSize, ImGuiChildFlags_Borders))
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
              if (ImGui::Selectable(
                      std::format("#{} {} (Spritesheet: #{})", id, layer.name, layer.spritesheetID).c_str(),
                      isSelected))
                addItemID = id;
            }
            else if (type == anm2::NULL_)
            {
              auto& null = anm2.content.nulls[id];
              if (ImGui::Selectable(std::format("#{} {}", id, null.name).c_str(), isSelected)) addItemID = id;
            }

            ImGui::PopID();
          }
        }
      }
      ImGui::EndChild();

      auto widgetSize = widget_size_with_row_get(2);

      ImGui::BeginDisabled(source == source::EXISTING && addItemID == -1);
      if (ImGui::Button("Add", widgetSize))
      {
        anm2::Reference addReference{};

        document.snapshot("Add Item");
        if (type == anm2::LAYER)
          addReference = anm2.layer_animation_add({reference.animationIndex, anm2::LAYER, addItemID}, addItemName,
                                                  addItemSpritesheetID - 1, (locale::Type)locale);
        else if (type == anm2::NULL_)
          addReference = anm2.null_animation_add({reference.animationIndex, anm2::LAYER, addItemID}, addItemName,
                                                 (locale::Type)locale);

        document.change(Document::ITEMS);

        reference = addReference;
        itemSelection.clear();
        if (addReference.itemType == anm2::LAYER || addReference.itemType == anm2::NULL_)
        {
          itemSelection.insert(item_selection_encode(addReference.itemType, addReference.itemID));
          item_selection_sync();
        }

        item_properties_close();
      }
      ImGui::EndDisabled();
      ImGui::SetItemTooltip("Add the item, with the settings specified.");

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) item_properties_close();
      ImGui::SetItemTooltip("Cancel adding an item.");

      ImGui::EndPopup();
    }

    bakePopup.trigger();

    if (ImGui::BeginPopupModal(bakePopup.label, &bakePopup.isOpen, ImGuiWindowFlags_NoResize))
    {
      auto& interval = settings.bakeInterval;
      auto& isRoundRotation = settings.bakeIsRoundRotation;
      auto& isRoundScale = settings.bakeIsRoundScale;

      auto frame = document.frame_get();

      input_int_range("Interval", interval, anm2::FRAME_DURATION_MIN,
                      frame ? frame->duration : anm2::FRAME_DURATION_MIN);
      ImGui::SetItemTooltip("Set the maximum duration of each frame that will be baked.");

      ImGui::Checkbox("Round Rotation", &isRoundRotation);
      ImGui::SetItemTooltip("Rotation will be rounded to the nearest whole number.");

      ImGui::Checkbox("Round Scale", &isRoundScale);
      ImGui::SetItemTooltip("Scale will be rounded to the nearest whole number.");

      auto widgetSize = widget_size_with_row_get(2);

      if (ImGui::Button("Bake", widgetSize))
      {
        auto frames_bake = [&]()
        {
          if (auto item = document.item_get())
            for (auto i : frames.selection | std::views::reverse)
              item->frames_bake(i, interval, isRoundScale, isRoundRotation);

          frames.clear();
        };

        DOCUMENT_EDIT(document, "Bake Frames", Document::FRAMES, frames_bake());

        bakePopup.close();
      }
      ImGui::SetItemTooltip("Bake the selected frame(s) with the options selected.");

      ImGui::SameLine();

      if (ImGui::Button("Cancel", widgetSize)) bakePopup.close();
      ImGui::SetItemTooltip("Cancel baking frames.");

      ImGui::EndPopup();
    }

    if (animation)
    {
      if (shortcut(manager.chords[SHORTCUT_PLAY_PAUSE], shortcut::GLOBAL)) playback.toggle();

      if (shortcut(manager.chords[SHORTCUT_MOVE_PLAYHEAD_BACK], shortcut::GLOBAL, true))
      {
        playback.decrement(settings.playbackIsClamp ? animation->frameNum : anm2::FRAME_NUM_MAX);
        document.frameTime = playback.time;
      }

      if (shortcut(manager.chords[SHORTCUT_MOVE_PLAYHEAD_FORWARD], shortcut::GLOBAL, true))
      {
        playback.increment(settings.playbackIsClamp ? animation->frameNum : anm2::FRAME_NUM_MAX);
        document.frameTime = playback.time;
      }

      if (shortcut(manager.chords[SHORTCUT_SHORTEN_FRAME], shortcut::GLOBAL)) document.snapshot("Shorten Frame");
      if (shortcut(manager.chords[SHORTCUT_SHORTEN_FRAME], shortcut::GLOBAL, true))
      {

        if (auto frame = document.frame_get())
        {
          frame->shorten();
          document.change(Document::FRAMES);
        }
      }

      if (shortcut(manager.chords[SHORTCUT_EXTEND_FRAME], shortcut::GLOBAL)) document.snapshot("Extend Frame");
      if (shortcut(manager.chords[SHORTCUT_EXTEND_FRAME], shortcut::GLOBAL, true))
      {

        if (auto frame = document.frame_get())
        {
          frame->extend();
          document.change(Document::FRAMES);
        }
      }

      if (shortcut(manager.chords[SHORTCUT_PREVIOUS_FRAME], shortcut::GLOBAL, true))
      {
        if (auto item = document.item_get(); !item->frames.empty())
        {
          reference.frameIndex = glm::clamp(--reference.frameIndex, 0, (int)item->frames.size() - 1);
          frames_selection_set_reference();
          document.frameTime = item->frame_time_from_index_get(reference.frameIndex);
        }
      }

      if (shortcut(manager.chords[SHORTCUT_NEXT_FRAME], shortcut::GLOBAL, true))
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
