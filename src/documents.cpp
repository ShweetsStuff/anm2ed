#include "documents.h"

#include <vector>

#include "util.h"

using namespace anm2ed::taskbar;
using namespace anm2ed::manager;
using namespace anm2ed::settings;
using namespace anm2ed::resources;
using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed::documents
{
  void Documents::update(Taskbar& taskbar, Manager& manager, Settings& settings, Resources& resources, bool& isQuitting)
  {
    auto viewport = ImGui::GetMainViewport();
    auto windowHeight = ImGui::GetFrameHeightWithSpacing();

    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + taskbar.height));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, windowHeight));

    for (auto& document : manager.documents)
    {
      auto isDirty = document.is_dirty() && document.is_autosave_dirty();
      document.lastAutosaveTime += ImGui::GetIO().DeltaTime;

      if (isDirty && document.lastAutosaveTime > time::SECOND_S) manager.autosave(document);
    }

    if (ImGui::Begin("##Documents", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse))
    {
      height = ImGui::GetWindowSize().y;

      if (ImGui::BeginTabBar("Documents Bar", ImGuiTabBarFlags_Reorderable))
      {
        auto documentsCount = (int)manager.documents.size();
        bool closeShortcut = imgui::shortcut(settings.shortcutClose, shortcut::GLOBAL) && !closePopup.is_open();
        int closeShortcutIndex =
            closeShortcut && manager.selected >= 0 && manager.selected < documentsCount ? manager.selected : -1;

        std::vector<int> closeIndices{};
        closeIndices.reserve(documentsCount);

        for (int i = 0; i < documentsCount; ++i)
        {
          auto& document = manager.documents[i];
          auto isDirty = document.is_dirty() || document.isForceDirty;

          if (!closePopup.is_open())
          {
            if (isQuitting)
              document.isOpen = false;
            else if (i == closeShortcutIndex)
              document.isOpen = false;
          }

          if (!closePopup.is_open() && !document.isOpen)
          {
            if (isDirty)
            {
              closePopup.open();
              closeDocumentIndex = i;
              document.isOpen = true;
            }
            else
            {
              closeIndices.push_back(i);
              continue;
            }
          }

          auto isRequested = i == manager.pendingSelected;

          auto font = isDirty ? font::ITALICS : font::REGULAR;

          auto string = isDirty ? std::format("[Not Saved] {}", document.filename_get().string())
                                : document.filename_get().string();

          auto label = std::format("{}###Document{}", string, i);

          auto flags = isDirty ? ImGuiTabItemFlags_UnsavedDocument : 0;
          if (isRequested) flags |= ImGuiTabItemFlags_SetSelected;

          ImGui::PushFont(resources.fonts[font].get(), font::SIZE);
          if (ImGui::BeginTabItem(label.c_str(), &document.isOpen, flags))
          {
            manager.set(i);
            if (isRequested) manager.pendingSelected = -1;
            ImGui::EndTabItem();
          }
          ImGui::PopFont();
        }

        for (auto it = closeIndices.rbegin(); it != closeIndices.rend(); ++it)
        {
          if (closePopup.is_open() && closeDocumentIndex > *it) --closeDocumentIndex;
          manager.close(*it);
        }

        ImGui::EndTabBar();
      }

      closePopup.trigger();

      if (ImGui::BeginPopupModal(closePopup.label, &closePopup.isOpen, ImGuiWindowFlags_NoResize))
      {
        if (closeDocumentIndex >= 0 && closeDocumentIndex < (int)manager.documents.size())
        {
          auto& closeDocument = manager.documents[closeDocumentIndex];

          ImGui::TextUnformatted(std::format("The document \"{}\" has been modified.\nDo you want to save it?",
                                             closeDocument.filename_get().string())
                                     .c_str());

          auto widgetSize = imgui::widget_size_with_row_get(3);

          auto close = [&]()
          {
            closeDocumentIndex = -1;
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

          if (ImGui::Button("Cancel", widgetSize))
          {
            isQuitting = false;
            close();
          }
        }
        else
        {
          closeDocumentIndex = -1;
          closePopup.close();
        }

        ImGui::EndPopup();
      }
    }

    ImGui::End();
  }
}
