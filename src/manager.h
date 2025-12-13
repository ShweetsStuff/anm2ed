#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "document.h"
#include "settings.h"
#include "strings.h"

namespace anm2ed
{
  constexpr auto FILE_LABEL_FORMAT = "{} [{}]";

  class Manager
  {
    std::filesystem::path recent_files_path_get();
    std::filesystem::path autosave_path_get();
    void selection_history_push(int);
    void selection_history_cleanup(int removedIndex);
    void recent_files_trim();

  public:
    std::vector<Document> documents{};
    std::map<std::string, std::size_t, std::less<>> recentFiles{};
    std::size_t recentFilesCounter{};
    std::vector<std::filesystem::path> autosaveFiles{};

    int selected{-1};
    int pendingSelected{-1};

    bool isRecording{};
    bool isRecordingStart{};
    int recordingStart{};
    int recordingEnd{};
    bool isRecordingRange{};

    ImGuiKeyChord chords[SHORTCUT_COUNT]{};

    std::vector<std::filesystem::path> anm2DragDropPaths{};
    bool isAnm2DragDrop{};
    imgui::PopupHelper anm2DragDropPopup{
        imgui::PopupHelper(LABEL_MANAGER_ANM2_DRAG_DROP, imgui::POPUP_NORMAL, imgui::POPUP_BY_CURSOR)};

    std::filesystem::path spritesheetDragDropPath{};
    bool isSpritesheetDragDrop{};

    anm2::Layer editLayer{};
    imgui::PopupHelper layerPropertiesPopup{
        imgui::PopupHelper(LABEL_MANAGER_LAYER_PROPERTIES, imgui::POPUP_SMALL_NO_HEIGHT)};

    anm2::Null editNull{};
    imgui::PopupHelper nullPropertiesPopup{
        imgui::PopupHelper(LABEL_MANAGER_NULL_PROPERTIES, imgui::POPUP_SMALL_NO_HEIGHT)};

    imgui::PopupHelper progressPopup{
        imgui::PopupHelper(LABEL_MANAGER_RENDERING_PROGRESS, imgui::POPUP_SMALL_NO_HEIGHT)};

    std::vector<int> selectionHistory{};

    Manager();
    ~Manager();

    Document* get(int = -1);
    void open(const std::filesystem::path&, bool = false, bool = true);
    void new_(const std::filesystem::path&);
    void save(int, const std::filesystem::path& = {});
    void save(const std::filesystem::path& = {});
    void autosave(Document&);
    void set(int);
    void close(int);
    void layer_properties_open(int = -1);
    void layer_properties_trigger();
    void layer_properties_end();
    void layer_properties_close();
    void null_properties_open(int = -1);
    void null_properties_trigger();
    void null_properties_end();
    void null_properties_close();

    void recent_files_load();
    void recent_files_write();
    void recent_files_clear();
    void recent_file_add(const std::filesystem::path&);
    std::vector<std::filesystem::path> recent_files_ordered() const;

    void autosave_files_load();
    void autosave_files_open();
    void autosave_files_write();
    void autosave_files_clear();

    void chords_set(Settings&);
  };
}
