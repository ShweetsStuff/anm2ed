#include "window.hpp"

#include <algorithm>
#include <format>

#include "util/imgui/spacing.hpp"

namespace anm2ed::ui_new
{
  bool is_flag_set(ContextMenuOptions options, ContextMenuOptions flag)
  {
    return (static_cast<uint32_t>(options) & static_cast<uint32_t>(flag)) != 0;
  }

  bool is_flag_set(ButtonOptions options, ButtonOptions flag)
  {
    return (static_cast<uint32_t>(options) & static_cast<uint32_t>(flag)) != 0;
  }

  std::vector<ContextMenuOptionEntry> options_get(ContextMenuOptions options)
  {
    std::vector<ContextMenuOptionEntry> entries{};
    for (const auto& entry : CONTEXT_MENU_OPTION_ENTRIES)
      if (is_flag_set(options, entry.option)) entries.push_back(entry);
    return entries;
  }

  std::vector<ButtonOptionEntry> options_get(ButtonOptions options)
  {
    std::vector<ButtonOptionEntry> entries{};
    for (const auto& entry : BUTTON_OPTION_ENTRIES)
      if (is_flag_set(options, entry.option)) entries.push_back(entry);
    return entries;
  }

  int element_selection_value_get(const anm2_new::Anm2::Element& element, std::size_t index)
  {
    return element.id != -1 ? element.id : (int)index;
  }

  Window::Window(std::string label, Type windowType, ButtonOptions buttonOptions,
                 ContextMenuOptions contextMenuOptions, anm2_new::Anm2::Element::Type elementType)
      : label(std::move(label)),
        windowType(windowType),
        buttonOptions(buttonOptions),
        contextMenuOptions(contextMenuOptions),
        elementType(elementType)
  {
  }

  void Window::update(DocumentNew* document)
  {
    if (!ImGui::Begin(label.c_str()))
    {
      ImGui::End();
      return;
    }

    switch (windowType)
    {
      case ELEMENT:
        update_element(document);
        break;

      case ANIMATION_PREVIEW:
      case SPRITESHEET_EDITOR:
      case TIMELINE:
      case FRAME_PROPERTIES:
      case ONIONSKIN:
        break;
    }

    ImGui::End();
  }

  void Window::update_element(DocumentNew* document)
  {
    auto buttonEntries = button_entries_get();
    auto widgetsPerRow = std::min<int>((int)buttonEntries.size(), 3);
    auto footerRowCount = util::imgui::footer_row_count_get((int)buttonEntries.size(), widgetsPerRow);

    if (!document || !document->isOpen || !document->isValid)
    {
      ImGui::BeginChild("##Elements", util::imgui::size_without_footer_get(footerRowCount), ImGuiChildFlags_Borders);
      ImGui::EndChild();
      update_footer();
      return;
    }

    auto* elementState = document->element_state_get(elementType);
    auto elements = document->anm2.elements_get(elementType);
    std::vector<int> selectionIndexMap{};
    selectionIndexMap.reserve(elements.size());
    for (std::size_t i = 0; i < elements.size(); ++i)
      selectionIndexMap.push_back(element_selection_value_get(*elements[i], i));

    if (ImGui::BeginChild("##Elements", util::imgui::size_without_footer_get(footerRowCount), ImGuiChildFlags_Borders))
    {
      if (elementState)
      {
        elementState->selected.set_index_map(&selectionIndexMap);
        elementState->selected.start(elements.size());
      }

      for (std::size_t i = 0; i < elements.size(); ++i)
      {
        auto* element = elements[i];
        auto selectionValue = element_selection_value_get(*element, i);
        auto entry = !element->name.empty()
                         ? element->name
                         : std::format("{} {}", anm2_new::Anm2::Element::tag_get(elementType), i);

        auto isSelected = elementState && elementState->selected.contains(selectionValue);
        ImGui::SetNextItemSelectionUserData((int)i);

        if (ImGui::Selectable(entry.c_str(), isSelected) && elementState)
        {
          elementState->reference = selectionValue;
        }
      }

      if (elementState) elementState->selected.finish();
    }
    ImGui::EndChild();
    update_footer();
  }

  std::vector<ButtonOptionEntry> Window::button_entries_get() const { return options_get(buttonOptions); }

  void Window::update_footer()
  {
    auto buttonEntries = button_entries_get();
    if (buttonEntries.empty()) return;

    constexpr auto MAX_BUTTONS_PER_ROW = 3;
    auto widgetsPerRow = std::min<int>((int)buttonEntries.size(), MAX_BUTTONS_PER_ROW);
    auto footerRowCount = util::imgui::footer_row_count_get((int)buttonEntries.size(), widgetsPerRow);
    auto footerSize = util::imgui::footer_size_get(footerRowCount);

    if (ImGui::BeginChild("##Footer", footerSize, ImGuiChildFlags_None))
    {
      float rowWidth{};
      for (std::size_t i = 0; i < buttonEntries.size(); ++i)
      {
        auto rowIndex = (int)i / widgetsPerRow;
        auto rowStart = rowIndex * widgetsPerRow;
        auto rowItemCount = std::min<int>(widgetsPerRow, (int)buttonEntries.size() - rowStart);

        if ((int)i == rowStart) rowWidth = ImGui::GetContentRegionAvail().x;
        if (i > 0 && rowStart != (int)i) ImGui::SameLine();

        ImGui::Button(buttonEntries[i].label, util::imgui::row_item_size_get(rowItemCount, rowWidth));
      }
    }

    ImGui::EndChild();
  }
}
