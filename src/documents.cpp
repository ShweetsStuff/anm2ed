#include "documents.h"

#include <ranges>

#include "imgui.h"

using namespace anm2ed::taskbar;
using namespace anm2ed::manager;
using namespace anm2ed::resources;

namespace anm2ed::documents
{
  void Documents::update(Taskbar& taskbar, Manager& manager, Resources& resources)
  {
    auto viewport = ImGui::GetMainViewport();
    auto windowHeight = ImGui::GetFrameHeightWithSpacing();

    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + taskbar.height));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, windowHeight));

    if (ImGui::Begin("##Documents", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse))
    {
      height = ImGui::GetWindowSize().y;

      if (ImGui::BeginTabBar("Documents Bar", ImGuiTabBarFlags_Reorderable))
      {
        for (auto [i, document] : std::views::enumerate(manager.documents))
        {
          auto isDirty = document.is_dirty();
          auto isRequested = i == manager.pendingSelected;

          auto font = isDirty ? font::ITALICS : font::REGULAR;

          auto string = isDirty ? std::format("[Not Saved] {}", document.filename_get()) : document.filename_get();

          auto label = std::format("{}###Document{}", string, i);

          auto flags = isDirty ? ImGuiTabItemFlags_UnsavedDocument : 0;
          if (isRequested) flags |= ImGuiTabItemFlags_SetSelected;

          ImGui::PushFont(resources.fonts[font].get(), font::SIZE);
          if (ImGui::BeginTabItem(label.c_str(), &document.isOpen, flags))
          {
            manager.selected = i;
            if (isRequested) manager.pendingSelected = -1;
            ImGui::EndTabItem();
          }
          ImGui::PopFont();

          if (!document.isOpen)
          {
            if (isDirty)
            {
              isCloseDocument = true;
              isOpenCloseDocumentPopup = true;
              closeDocumentIndex = i;
              document.isOpen = true;
            }
            else
              manager.close(i);
          }
        }

        ImGui::EndTabBar();
      }

      closePopup.trigger();

      if (isCloseDocument)
      {
        if (ImGui::BeginPopupModal(closePopup.label, &closePopup.isOpen, ImGuiWindowFlags_NoResize))
        {
          auto closeDocument = manager.get(closeDocumentIndex);

          ImGui::TextUnformatted(std::format("The document \"{}\" has been modified.\nDo you want to save it?",
                                             closeDocument->filename_get())
                                     .c_str());

          auto widgetSize = imgui::widget_size_with_row_get(3);

          auto close = [&]()
          {
            closeDocumentIndex = 0;
            closePopup.close();
          };

          if (ImGui::Button("Yes", widgetSize))
          {
            manager.save(closeDocumentIndex);
            manager.close(closeDocumentIndex);
            close();
          }

          ImGui::SameLine();

          if (ImGui::Button("No", widgetSize))
          {
            manager.close(closeDocumentIndex);
            close();
          }

          ImGui::SameLine();

          if (ImGui::Button("Cancel", widgetSize)) close();

          ImGui::EndPopup();
        }
      }
    }

    ImGui::End();
  }
}
