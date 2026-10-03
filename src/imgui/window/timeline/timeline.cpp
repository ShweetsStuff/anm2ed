#include "context.hpp"

namespace anm2ed::imgui
{
  Timeline::Timeline() = default;
  Timeline::~Timeline() = default;

  TimelineContext::TimelineContext(Timeline& timeline, Manager& manager, Settings& settings, Resources& resources,
                                   Clipboard& clipboard)
      : timeline(timeline), isDragging(timeline.isDragging), isWindowHovered(timeline.isWindowHovered),
        isHorizontalScroll(timeline.isHorizontalScroll), itemProperties(timeline.itemProperties),
        bakePopup(timeline.bakePopup), bakeIntoOtherFramesPopup(timeline.bakeIntoOtherFramesPopup),
        makeManyRegionsPopup(timeline.makeManyRegionsPopup),
        bakeIntoOtherFramesTarget(timeline.bakeIntoOtherFramesTarget),
        isBakeIntoOtherFramesLayers(timeline.isBakeIntoOtherFramesLayers),
        isBakeIntoOtherFramesNulls(timeline.isBakeIntoOtherFramesNulls),
        isMakeManyRegionsMapFrames(timeline.isMakeManyRegionsMapFrames),
        makeManyRegionReferences(timeline.makeManyRegionReferences), hoveredTime(timeline.hoveredTime),
        isFrameBoxPending(timeline.isFrameBoxPending), isFrameBoxSelecting(timeline.isFrameBoxSelecting),
        isFrameBoxAdditive(timeline.isFrameBoxAdditive), frameBoxStart(timeline.frameBoxStart),
        frameBoxEnd(timeline.frameBoxEnd), frameBoxSelection(timeline.frameBoxSelection),
        rowSelectionAnchor(timeline.rowSelectionAnchor), isRowSelectionAnchorSet(timeline.isRowSelectionAnchorSet),
        groupPropertiesPopup(timeline.groupPropertiesPopup), groupName(timeline.groupName),
        groupAnimationIndex(timeline.groupAnimationIndex), groupType(timeline.groupType), groupId(timeline.groupId),
        draggedFrameReference(timeline.draggedFrameReference), isDraggedFrameActive(timeline.isDraggedFrameActive),
        draggedFrameType(timeline.draggedFrameType), draggedFrameIndex(timeline.draggedFrameIndex),
        draggedFrameStart(timeline.draggedFrameStart), draggedFrameStartDuration(timeline.draggedFrameStartDuration),
        draggedFrameStartDurations(timeline.draggedFrameStartDurations),
        draggedFrameStartMouseX(timeline.draggedFrameStartMouseX), draggedFrameWidth(timeline.draggedFrameWidth),
        isDraggedFrameSnapshot(timeline.isDraggedFrameSnapshot), frameFocusRequested(timeline.frameFocusRequested),
        frameFocusIndex(timeline.frameFocusIndex), frameMoveDrag(timeline.frameMoveDrag),
        frameSelectionSnapshot(timeline.frameSelectionSnapshot), frameSelectionLocked(timeline.frameSelectionLocked),
        isFrameSelectionLocked(timeline.isFrameSelectionLocked),
        frameSelectionSnapshotReference(timeline.frameSelectionSnapshotReference),
        frameSelectionAnchor(timeline.frameSelectionAnchor), isFrameSelectionAnchorSet(timeline.isFrameSelectionAnchorSet),
        animationLengthEditIndex(timeline.animationLengthEditIndex), rowDragReferences(timeline.rowDragReferences),
        scroll(timeline.scroll), style(timeline.style), manager(manager), settings(settings), resources(resources),
        clipboard(clipboard), document(*manager.get()), anm2(document.anm2), playback(document.playback),
        reference(document.reference), frames(document.frames), region(document.region)
  {
  }

  void TimelineContext::update()
  {
    animation = anm2.element_get(ElementType::ANIMATION, reference.animationIndex);
    rowFrameChildHeight = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 1.5f;

    style = ImGui::GetStyle();
    isLightTheme = settings.theme == theme::LIGHT;
    isTextPushed = false;
    if (isLightTheme)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, TIMELINE_TEXT_COLOR_LIGHT);
      isTextPushed = true;
    }

    commands_bind();
    draw();
  }

  void Timeline::update(Manager& manager, Settings& settings, Resources& resources, Clipboard& clipboard)
  {
    if (!manager.get()) return;
    context = std::make_unique<TimelineContext>(*this, manager, settings, resources, clipboard);
    context->update();
  }
}
