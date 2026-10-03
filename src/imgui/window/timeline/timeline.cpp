#include "context.hpp"

namespace anm2ed::imgui
{
  Timeline::Timeline() = default;
  Timeline::~Timeline() = default;

  TimelineContext::TimelineContext(TimelineState&& state, Manager& manager, Settings& settings, Resources& resources,
                                   Clipboard& clipboard)
      : TimelineState(std::move(state)), manager(manager), settings(settings), resources(resources),
        clipboard(clipboard), document(*manager.get()), model(document.model), playback(document.playback),
        reference(document.reference_get()), region(document.region)
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
}
