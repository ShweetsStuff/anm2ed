#include "documents.hpp"

#include <algorithm>
#include <format>
#include <ranges>
#include <vector>

#include <imgui_internal.h>

#include "log.hpp"
#include "path.hpp"
#include "strings.hpp"
#include "time.hpp"
#include "toast.hpp"
#include "util/imgui/layout.hpp"
#include "util/imgui/shortcut.hpp"

using namespace anm2ed::resource;
using namespace anm2ed::types;
using namespace anm2ed::util;

namespace anm2ed::imgui
{
  struct DocumentTab
  {
    ImGuiID imguiId{};
    uint64_t documentId{};
  };

#define ANM2_DRAG_DROP_MERGE_PRESETS                                                                                    \
  X(FILE_MERGE_PRESET_MERGE_BY_NAME, BASIC_MERGE, TOOLTIP_MERGE_PRESET_MERGE_BY_NAME)                                  \
  X(FILE_MERGE_PRESET_APPEND_AS_NEW, BASIC_APPEND, TOOLTIP_MERGE_PRESET_APPEND_AS_NEW)                                  \
  X(FILE_MERGE_PRESET_REPLACE_MATCHING, BASIC_REPLACE, TOOLTIP_MERGE_PRESET_REPLACE_MATCHING)

  constexpr StringType ANM2_DRAG_DROP_MERGE_PRESET_LABELS[Document::FILE_MERGE_PRESET_COUNT] = {
#define X(preset, label, tooltip) label,
      ANM2_DRAG_DROP_MERGE_PRESETS
#undef X
  };

  constexpr StringType ANM2_DRAG_DROP_MERGE_PRESET_TOOLTIPS[Document::FILE_MERGE_PRESET_COUNT] = {
#define X(preset, label, tooltip) tooltip,
      ANM2_DRAG_DROP_MERGE_PRESETS
#undef X
  };

#undef ANM2_DRAG_DROP_MERGE_PRESETS

  void anm2_drag_drop_merge_queue(Manager& manager, const std::vector<std::filesystem::path>& paths,
                                  Document::FileMergePreset preset)
  {
    manager.command_push({manager.selected,
                          [paths, preset](Manager&, Document& document)
                          {
                            document.anm2_snapshot(localize.get(EDIT_MERGE_ANM2));
                            for (auto& path : paths) document.file_merge(path, preset);
                            document.change(Document::ALL);
                          }});
  }

  void Documents::pending_document_order_apply(Manager& manager)
  {
    if (pendingDocumentTabIds.size() != manager.documents.size())
    {
      pendingDocumentTabIds.clear();
      return;
    }

    std::vector<int> order{};
    order.reserve(manager.documents.size());
    for (auto documentId : pendingDocumentTabIds)
    {
      auto it = std::ranges::find_if(manager.documents, [documentId](const Document& document)
                                     { return document.tabId == documentId; });
      if (it == manager.documents.end())
      {
        pendingDocumentTabIds.clear();
        return;
      }
      order.push_back((int)std::distance(manager.documents.begin(), it));
    }

    bool isOrdered = true;
    for (auto [i, index] : std::views::enumerate(order))
      if ((int)i != index)
      {
        isOrdered = false;
        break;
      }

    if (isOrdered)
    {
      pendingDocumentTabIds.clear();
      return;
    }

    std::vector<int> remap(manager.documents.size(), -1);
    std::vector<Document> documents{};
    documents.reserve(manager.documents.size());
    for (auto [i, index] : std::views::enumerate(order))
    {
      remap[index] = (int)i;
      documents.push_back(std::move(manager.documents[index]));
    }

    auto index_remap = [&](int& index)
    {
      if (index < 0 || index >= (int)remap.size())
        index = -1;
      else
        index = remap[index];
    };

    manager.documents = std::move(documents);
    index_remap(manager.selected);
    index_remap(manager.pendingSelected);
    index_remap(closeDocumentIndex);
    for (auto& index : manager.selectionHistory) index_remap(index);
    for (auto& command : manager.commands)
      if (!command.runManager) index_remap(command.documentIndex);
    std::erase(manager.selectionHistory, -1);

    pendingDocumentTabIds.clear();
  }

  void Documents::update(Taskbar& taskbar, Manager& manager, Settings& settings, Resources& resources, bool& isQuitting)
  {
    pending_document_order_apply(manager);

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

    for (auto i = 0; i < (int)manager.documents.size(); ++i)
    {
      auto& document = manager.documents[i];
      auto isDirty = (document.is_dirty() || document.isForceDirty) && document.is_autosave_dirty();
      if (isDirty)
      {
        document.lastAutosaveTime += ImGui::GetIO().DeltaTime;
        if (document.lastAutosaveTime > time::SECOND_M)
        {
          auto options = settings.anm2_options_get();
          manager.command_push({i, [options](Manager& manager, Document& document)
                                { manager.autosave(document, options); }});
        }
      }
    }

    if (ImGui::Begin("##Documents", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse))
    {
      height = ImGui::GetWindowSize().y;

      if (ImGui::BeginTabBar("##Documents Bar", ImGuiTabBarFlags_Reorderable))
      {
        auto documentsCount = (int)manager.documents.size();
        bool isCloseShortcut = shortcut(manager.chords[SHORTCUT_CLOSE], shortcut::GLOBAL) && !closePopup.is_open();
        int closeShortcutIndex =
            isCloseShortcut && manager.selected >= 0 && manager.selected < documentsCount ? manager.selected : -1;

        std::vector<int> closeIndices{};
        closeIndices.reserve(documentsCount);
        std::vector<DocumentTab> documentTabs{};
        documentTabs.reserve(documentsCount);

        for (int i = 0; i < documentsCount; ++i)
        {
          auto& document = manager.documents[i];
          auto isDocumentDirty = document.is_dirty() || document.isForceDirty;
          auto isSpritesheetDirty = document.spritesheet_any_dirty() || document.overlay_any_dirty();
          auto isDirty = isDocumentDirty || isSpritesheetDirty;

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
          auto font = isDocumentDirty ? font::ITALICS : font::REGULAR;
          auto filename = path::to_utf8(document.filename_get());
          auto string = isDocumentDirty ? std::vformat(localize.get(FORMAT_NOT_SAVED), std::make_format_args(filename))
                                        : filename;
          auto label = std::format("{}###Document{}", string, document.tabId);
          auto imguiId = ImGui::GetCurrentWindow()->GetID(label.c_str());
          documentTabs.push_back({imguiId, document.tabId});

          auto flags = isDocumentDirty ? ImGuiTabItemFlags_UnsavedDocument : 0;
          if (isRequested) flags |= ImGuiTabItemFlags_SetSelected;

          ImGui::PushFont(resources.fonts[font].get(), font::SIZE);
          if (ImGui::BeginTabItem(label.c_str(), &document.isOpen, flags))
          {
            if (manager.selected != i) manager.set(i);

            if (isRequested) manager.pendingSelected = -1;

            ImGui::EndTabItem();
          }
          auto pathUtf8 = path::to_utf8(document.path);
          ImGui::SetItemTooltip("%s", pathUtf8.c_str());

          ImGui::PopFont();
        }

        if (auto tabBar = ImGui::GetCurrentTabBar(); tabBar && documentTabs.size() == manager.documents.size())
        {
          std::vector<uint64_t> documentTabIds{};
          documentTabIds.reserve(manager.documents.size());
          for (auto& tab : tabBar->Tabs)
          {
            auto it = std::ranges::find_if(documentTabs, [&](const DocumentTab& documentTab)
                                           { return documentTab.imguiId == tab.ID; });
            if (it != documentTabs.end()) documentTabIds.push_back(it->documentId);
          }

          if (documentTabIds.size() == manager.documents.size())
          {
            bool isOrdered = true;
            for (auto [i, documentId] : std::views::enumerate(documentTabIds))
              if (manager.documents[i].tabId != documentId)
              {
                isOrdered = false;
                break;
              }
            if (!isOrdered) pendingDocumentTabIds = std::move(documentTabIds);
          }
        }

        for (auto it = closeIndices.rbegin(); it != closeIndices.rend(); ++it)
        {
          if (closePopup.is_open() && closeDocumentIndex > *it) --closeDocumentIndex;
          auto index = *it;
          manager.command_push({.runManager = [index](Manager& manager) { manager.close(index); }});
        }

        ImGui::EndTabBar();
      }

      closePopup.trigger();

      if (ImGui::BeginPopupModal(closePopup.label(), &closePopup.isOpen, ImGuiWindowFlags_NoResize))
      {
        if (closeDocumentIndex >= 0 && closeDocumentIndex < (int)manager.documents.size())
        {
          auto& closeDocument = manager.documents[closeDocumentIndex];

          auto filename = path::to_utf8(closeDocument.filename_get());
          auto isDocumentDirty = closeDocument.is_dirty() || closeDocument.isForceDirty;
          auto isSpritesheetDirty = closeDocument.spritesheet_any_dirty() || closeDocument.overlay_any_dirty();
          auto promptLabel =
              isDocumentDirty && isSpritesheetDirty
                  ? LABEL_DOCUMENT_AND_SPRITESHEETS_MODIFIED_PROMPT
                  : (isDocumentDirty ? LABEL_DOCUMENT_MODIFIED_PROMPT : LABEL_SPRITESHEETS_MODIFIED_PROMPT);
          auto prompt = std::vformat(localize.get(promptLabel), std::make_format_args(filename));
          ImGui::TextUnformatted(prompt.c_str());

          auto widgetSize = imgui::widget_size_with_row_get(3);

          auto close = [&]()
          {
            closeDocumentIndex = -1;
            closePopup.close();
          };

          shortcut(manager.chords[SHORTCUT_CONFIRM]);
          if (ImGui::Button(localize.get(BASIC_YES), widgetSize))
          {
            bool isSaved = true;
            if (isDocumentDirty) isSaved = taskbar.save_manual(manager, settings, closeDocumentIndex);

            if (!isSaved)
            {
              ImGui::EndPopup();
              return;
            }

            if (isSpritesheetDirty)
            {
              auto spritesheets = closeDocument.anm2.element_get(ElementType::SPRITESHEETS);
              if (spritesheets)
              {
                for (auto& spritesheet : spritesheets->children)
                {
                  if (spritesheet.type != ElementType::SPRITESHEET) continue;
                  auto id = spritesheet.id;
                  auto texture = closeDocument.texture_get(id);
                  if (texture && closeDocument.spritesheet_is_dirty(id))
                  {
                    auto pathString = path::to_utf8(spritesheet.path);
                    auto savePath = closeDocument.directory_get() / spritesheet.path;
                    path::ensure_directory(savePath.parent_path());
                    if (texture->write_png(savePath))
                    {
                      closeDocument.spritesheet_hash_set_saved(id);
                      toasts.push(
                          std::vformat(localize.get(TOAST_SAVE_SPRITESHEET), std::make_format_args(id, pathString)));
                      logger.info(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET, anm2ed::ENGLISH),
                                               std::make_format_args(id, pathString)));
                    }
                    else
                    {
                      toasts.push(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET_FAILED),
                                               std::make_format_args(id, pathString)));
                      logger.error(std::vformat(localize.get(TOAST_SAVE_SPRITESHEET_FAILED, anm2ed::ENGLISH),
                                                std::make_format_args(id, pathString)));
                    }
                  }

                  for (auto& overlay : spritesheet.children)
                  {
                    if (overlay.type != ElementType::OVERLAY) continue;
                    auto overlayTexture = closeDocument.overlay_texture_get(overlay.id);
                    if (!overlayTexture || !closeDocument.overlay_is_dirty(overlay.id)) continue;
                    auto pathString = path::to_utf8(overlay.path);
                    auto savePath = closeDocument.directory_get() / overlay.path;
                    path::ensure_directory(savePath.parent_path());
                    if (overlayTexture->write_png(savePath))
                    {
                      closeDocument.overlay_hash_set_saved(overlay.id);
                      toasts.push(std::vformat(localize.get(TOAST_SAVE_OVERLAY),
                                               std::make_format_args(overlay.id, pathString)));
                      logger.info(std::vformat(localize.get(TOAST_SAVE_OVERLAY, anm2ed::ENGLISH),
                                               std::make_format_args(overlay.id, pathString)));
                    }
                    else
                    {
                      toasts.push(std::vformat(localize.get(TOAST_SAVE_OVERLAY_FAILED),
                                               std::make_format_args(overlay.id, pathString)));
                      logger.error(std::vformat(localize.get(TOAST_SAVE_OVERLAY_FAILED, anm2ed::ENGLISH),
                                                std::make_format_args(overlay.id, pathString)));
                    }
                  }
                }
              }
            }
            auto index = closeDocumentIndex;
            manager.command_push({.runManager = [index](Manager& manager) { manager.close(index); }});
            close();
          }

          ImGui::SameLine();

          if (ImGui::Button(localize.get(BASIC_NO), widgetSize))
          {
            auto index = closeDocumentIndex;
            manager.command_push({.runManager =
                                      [index](Manager& manager)
                                      {
                                        manager.autosave_file_clear(index);
                                        manager.close(index);
                                      }});
            close();
          }

          ImGui::SameLine();

          shortcut(manager.chords[SHORTCUT_CANCEL]);
          if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize))
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
        manager.anm2DragDropMergePopup.close();
        manager.anm2DragDropMergePreset = Document::FILE_MERGE_PRESET_MERGE_BY_NAME;
      };

      if (manager.anm2DragDropPaths.empty())
        drag_drop_reset();
      else if (manager.anm2DragDropMergePopup.is_open())
      {
        manager.anm2DragDropMergePopup.trigger();

        if (ImGui::BeginPopupModal(manager.anm2DragDropMergePopup.label(), &manager.anm2DragDropMergePopup.isOpen,
                                   ImGuiWindowFlags_NoResize))
        {
          auto document = manager.get();
          manager.anm2DragDropMergePreset =
              std::clamp(manager.anm2DragDropMergePreset, 0, Document::FILE_MERGE_PRESET_COUNT - 1);

          for (int preset = 0; preset < Document::FILE_MERGE_PRESET_COUNT; ++preset)
          {
            ImGui::PushID(preset);
            ImGui::RadioButton(localize.get(ANM2_DRAG_DROP_MERGE_PRESET_LABELS[preset]),
                               &manager.anm2DragDropMergePreset, preset);
            ImGui::SetItemTooltip("%s", localize.get(ANM2_DRAG_DROP_MERGE_PRESET_TOOLTIPS[preset]));
            ImGui::PopID();
          }

          auto widgetSize = widget_size_with_row_get(2);
          shortcut(manager.chords[SHORTCUT_CONFIRM]);
          ImGui::BeginDisabled(!document);
          if (ImGui::Button(localize.get(BASIC_MERGE), widgetSize))
          {
            auto paths = manager.anm2DragDropPaths;
            auto preset = (Document::FileMergePreset)manager.anm2DragDropMergePreset;
            anm2_drag_drop_merge_queue(manager, paths, preset);
            drag_drop_reset();
          }
          ImGui::EndDisabled();

          ImGui::SameLine();

          shortcut(manager.chords[SHORTCUT_CANCEL]);
          if (ImGui::Button(localize.get(BASIC_CANCEL), widgetSize)) drag_drop_reset();

          manager.anm2DragDropMergePopup.end();
          ImGui::EndPopup();

          if (manager.isAnm2DragDrop && !manager.anm2DragDropMergePopup.is_open()) drag_drop_reset();
        }
        else if (!ImGui::IsPopupOpen(manager.anm2DragDropMergePopup.label()))
          drag_drop_reset();
      }
      else
      {
        if (!manager.anm2DragDropPopup.is_open()) manager.anm2DragDropPopup.open();

        manager.anm2DragDropPopup.trigger();

        if (ImGui::BeginPopup(manager.anm2DragDropPopup.label(), ImGuiWindowFlags_NoMove))
        {
          auto document = manager.get();
          if (ImGui::MenuItem(manager.anm2DragDropPaths.size() > 1 ? localize.get(LABEL_DOCUMENTS_OPEN_MANY)
                                                                   : localize.get(LABEL_DOCUMENTS_OPEN_NEW)))
          {
            auto paths = manager.anm2DragDropPaths;
            manager.command_push({.runManager =
                                      [paths](Manager& manager)
                                      {
                                        for (auto& path : paths) manager.open(path);
                                      }});
            drag_drop_reset();
          }

          if (ImGui::MenuItem(localize.get(LABEL_DOCUMENTS_MERGE_INTO_CURRENT), nullptr, false,
                              document && !manager.anm2DragDropPaths.empty()))
          {
            if (document)
            {
              manager.anm2DragDropPopup.close();
              manager.anm2DragDropMergePopup.open();
              ImGui::CloseCurrentPopup();
            }
          }

          ImGui::Separator();

          if (ImGui::MenuItem(localize.get(BASIC_CANCEL))) drag_drop_reset();

          manager.anm2DragDropPopup.end();
          ImGui::EndPopup();
        }
        else if (!ImGui::IsPopupOpen(manager.anm2DragDropPopup.label()))
          drag_drop_reset();
      }
    }
  }

}
