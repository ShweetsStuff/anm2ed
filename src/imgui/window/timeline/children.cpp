#include "context.hpp"

// Animation layers in the timeline: their child's tracks under them (read only, timed as the parent plays them),
// stepping into the child, and the breadcrumb back out.
namespace anm2ed::imgui
{
  constexpr auto BREADCRUMB_SEPARATOR = ">";

  // The child animation's tracks (root, layers top-down, nulls, triggers) when an animation layer row is expanded.
  std::vector<PeekRow> TimelineContext::peek_rows_get(const TimelineRow& row)
  {
    std::vector<PeekRow> rows{};
    if (row.isGroup || row.type != LAYER) return rows;
    auto track = item_get(LAYER, row.id, row.groupType, row.groupId);
    auto layer = model::item_get(model.content.layers, row.id);
    if (!track || !layer || !layer->is_animation() || !expandedLayerTracks.contains(track->uid)) return rows;
    auto [source, child] = model::child_get(model, *layer, {}, document.library_resolver_get());
    if (!child) return rows;

    auto row_add = [&](const model::Track& childTrack)
    {
      if (settings.timelineIsShowUnused || !childTrack.frames.empty()) rows.push_back({source, child, &childTrack});
    };
    row_add(child->root);
    std::vector<const model::Track*> layers{};
    model::tracks_each(child->layers, [&](const model::Track& childTrack, auto*) { layers.push_back(&childTrack); });
    for (auto it = layers.rbegin(); it != layers.rend(); ++it)
      row_add(**it);
    model::tracks_each(child->nulls, [&](const model::Track& childTrack, auto*) { row_add(childTrack); });
    row_add(child->triggers);
    return rows;
  }

  void TimelineContext::child_step_in(const TimelineRow& row)
  {
    manager.command_push({.runManager = [index = manager.selected, layer = row_item_reference_get(row)](
                                            Manager& manager) { manager.child_step_in(index, layer); }});
  }

  void TimelineContext::peek_item_child(const TimelineRow& row, const PeekRow& peek, int index)
  {
    auto type = (int)peek.track->type;
    auto label = std::string(localize.get(TYPE_STRINGS[type]));
    if (type == LAYER)
      if (auto layer = model::item_get(peek.source->content.layers, peek.track->id)) label = layer_label_get(*layer);
    if (type == NULL_)
      if (auto null = model::item_get(peek.source->content.nulls, peek.track->id)) label = null->name;

    ImGui::PushID(index);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, to_imvec4(color_get(COLOR_ITEM_BASE, type) * COLOR_HIDDEN_MULTIPLIER));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
    if (ImGui::BeginChild("##Peek Item Child", ImVec2(ImGui::GetContentRegionAvail().x, rowFrameChildHeight),
                          ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollWithMouse))
    {
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (float)(row.depth + 1) * ImGui::GetTextLineHeightWithSpacing());
      ImGui::BeginDisabled();
      ImGui::Image(resources.icon_id_get(TYPE_ICONS[type]), icon_size_get());
      ImGui::SameLine();
      ImGui::TextUnformatted(label.c_str());
      ImGui::EndDisabled();
      if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) child_step_in(row);
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_CHILD_TRACK));
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    ImGui::PopID();
  }

  // The child track's keyframes where the parent reaches them: a run of parent frames showing the same keyframe is one
  // cell (a trigger, one cell on the frame it fires).
  void TimelineContext::peek_frame_child(const TimelineRow& row, const PeekRow& peek, int& index, float width)
  {
    ImGui::PushID(index++);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
    if (ImGui::BeginChild("##Peek Frames Child", ImVec2(width, rowFrameChildHeight), ImGuiChildFlags_Borders) &&
        animation)
    {
      auto drawList = ImGui::GetWindowDrawList();
      auto origin = ImGui::GetCursorScreenPos();
      auto frameSize = ImVec2(ImGui::GetTextLineHeight(), ImGui::GetContentRegionAvail().y);
      auto track = item_get(LAYER, row.id, row.groupType, row.groupId);
      auto layer = model::item_get(model.content.layers, row.id);
      auto type = (int)peek.track->type;
      auto isTrigger = type == TRIGGER;
      auto color = ImGui::GetColorU32(to_imvec4(color_get(COLOR_FRAME_BASE, type) * COLOR_HIDDEN_MULTIPLIER));
      auto libraryGet = document.library_resolver_get();

      auto key_get = [&](int time)
      {
        auto frame = model::frame_generate(*track, (float)time);
        if (!frame.isVisible || model::child_get(model, *layer, frame, libraryGet).second != peek.child) return -1;
        auto childTime = model::child_time_get(*track, (float)time, peek.child->frameNum, layer->id);
        return isTrigger ? model::frame_index_from_at_frame_get(*peek.track, (int)childTime)
                         : model::frame_index_from_time_get(*peek.track, childTime);
      };
      auto cell_draw = [&](int start, int end, int key)
      {
        auto min = ImVec2(origin.x + frameSize.x * (float)start, origin.y);
        auto max = ImVec2(origin.x + frameSize.x * (float)end, origin.y + frameSize.y);
        drawList->AddRectFilled(min, max, color, FRAME_ROUNDING);
        drawList->AddRect(min, max, ImGui::GetColorU32(frameBorderColor), FRAME_ROUNDING);
        auto icon = isTrigger ? icon::TRIGGER : INTERPOLATION_ICONS[(int)peek.track->frames[key].interpolation];
        auto iconSize = icon_size_get();
        auto iconMin = ImVec2(min.x, origin.y + (frameSize.y - iconSize.y) / 2);
        drawList->AddImage((ImTextureID)(intptr_t)resources.icon_id_get(icon), iconMin,
                           ImVec2(iconMin.x + iconSize.x, iconMin.y + iconSize.y), ImVec2(0, 0), ImVec2(1, 1),
                           ImGui::GetColorU32(iconTintDefault));
      };

      int runStart{};
      int runKey{-1};
      for (int time = 0; time <= animation->frameNum; ++time)
      {
        auto key = time < animation->frameNum && track && layer ? key_get(time) : -1;
        if (key == runKey) continue;
        if (runKey != -1 && !isTrigger) cell_draw(runStart, time, runKey);
        if (key != -1 && isTrigger) cell_draw(time, time + 1, key);
        runStart = time;
        runKey = key;
      }

      if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) child_step_in(row);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopID();
  }

  // Where the animation being edited was stepped into from: each step leads back.
  void TimelineContext::breadcrumb_draw()
  {
    auto& steps = document.childSteps;
    if (!steps.empty() && steps.back().childAnimationIndex != reference.animationIndex) steps.clear();
    if (steps.empty()) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.WindowPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing);
    ImGui::SetCursorPos(ImVec2(style.WindowPadding.x, ImGui::GetCursorPosY() + style.ItemSpacing.y));
    auto crumb_draw = [&](const std::string& label, int stepIndex)
    {
      ImGui::PushID(stepIndex);
      if (ImGui::SmallButton(label.c_str()))
        manager.command_push({.runManager = [index = manager.selected, stepIndex](Manager& manager)
                              { manager.child_step_out(index, stepIndex); }});
      ImGui::SetItemTooltip("%s", localize.get(TOOLTIP_STEP_OUT));
      ImGui::PopID();
      ImGui::SameLine();
      ImGui::TextUnformatted(BREADCRUMB_SEPARATOR);
      ImGui::SameLine();
    };
    crumb_draw(steps.front().animationName, 0);
    for (int i = 0; i + 1 < (int)steps.size(); ++i)
      crumb_draw(steps[i].layerName, i + 1);
    ImGui::TextUnformatted(steps.back().layerName.c_str());
    ImGui::PopStyleVar(2);
  }
}
