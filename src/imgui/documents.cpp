#include "documents.h"

#include <vector>

#include "time_.h"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed::imgui
{
  void Documents::update(Taskbar& taskbar, Manager& manager, Settings& settings, Resources& resources, bool& isQuitting)
  {
    auto viewport = ImGui::GetMainViewport();
    auto windowHeight = ImGui::GetFrameHeightWithSpacing();
    bool isLightTheme = settings.theme == theme::LIGHT;
    bool pushedStyle = false;

    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + taskbar.height));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, windowHeight));
    if (isLightTheme)
    {
      ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive));
      pushedStyle = true;
    }

    for (auto& document : manager.documents)
    {
      auto isDirty = document.is_dirty() && document.is_autosave_dirty();
      if (isDirty)
      {
        document.lastAutosaveTime += ImGui::GetIO().DeltaTime;
        if (document.lastAutosaveTime > settings.fileAutosaveTime * time::SECOND_M) manager.autosave(document);
      }
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
        bool isCloseShortcut = shortcut(manager.chords[SHORTCUT_CLOSE], shortcut::GLOBAL) && !closePopup.is_open();
        int closeShortcutIndex =
            isCloseShortcut && manager.selected >= 0 && manager.selected < documentsCount ? manager.selected : -1;

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
            if (manager.selected != i) manager.set(i);

            if (isRequested) manager.pendingSelected = -1;

            ImGui::EndTabItem();
          }
          ImGui::SetItemTooltip("%s", document.path.c_str());

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
    if (pushedStyle) ImGui::PopStyleColor();

    if (manager.isAnm2DragDrop)
    {
      auto drag_drop_reset = [&]()
      {
        manager.isAnm2DragDrop = false;
        manager.anm2DragDropPaths.clear();
        manager.anm2DragDropPopup.close();
      };

      if (manager.anm2DragDropPaths.empty())
        drag_drop_reset();
      else
      {
        if (!manager.anm2DragDropPopup.is_open()) manager.anm2DragDropPopup.open();

        manager.anm2DragDropPopup.trigger();

        if (ImGui::BeginPopupContextWindow(manager.anm2DragDropPopup.label, ImGuiPopupFlags_None))
        {
          auto document = manager.get();
          if (ImGui::MenuItem(manager.anm2DragDropPaths.size() > 1 ? "Open Many Documents" : "Open New Document"))
          {
            for (auto& path : manager.anm2DragDropPaths)
              manager.open(path);
            drag_drop_reset();
          }

          if (ImGui::MenuItem("Merge into Current Document", nullptr, false,
                              document && !manager.anm2DragDropPaths.empty()))
          {
            if (document)
            {
              auto merge_anm2s = [&]()
              {
                for (auto& path : manager.anm2DragDropPaths)
                {
                  anm2::Anm2 source(path.string());
                  document->anm2.merge(source, document->directory_get(), path.parent_path());
                }
              };

              DOCUMENT_EDIT_PTR(document, "Merge Anm2", Document::ALL, merge_anm2s());
              drag_drop_reset();
            }
          }

          if (ImGui::MenuItem("Cancel")) drag_drop_reset();

          manager.anm2DragDropPopup.end();
          ImGui::EndPopup();
        }
        else if (!ImGui::IsPopupOpen(manager.anm2DragDropPopup.label))
          drag_drop_reset();
      }
    }
  }

}
