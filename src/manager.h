#pragma once

#include <vector>

#include "document.h"

namespace anm2ed
{
  constexpr auto FILE_LABEL_FORMAT = "{} [{}]";

  class Manager
  {
    std::filesystem::path recent_files_path_get();
    std::filesystem::path autosave_path_get();

  public:
    std::vector<Document> documents{};
    std::vector<std::filesystem::path> recentFiles{};
    std::vector<std::filesystem::path> autosaveFiles{};

    int selected{-1};
    int pendingSelected{-1};

    bool isRecording{};
    bool isRecordingStart{};
    int recordingStart{};
    int recordingEnd{};
    bool isRecordingRange{};

    anm2::Layer editLayer{};
    imgui::PopupHelper layerPropertiesPopup{imgui::PopupHelper("Layer Properties", imgui::POPUP_SMALL_NO_HEIGHT)};

    anm2::Null editNull{};
    imgui::PopupHelper nullPropertiesPopup{imgui::PopupHelper("Null Properties", imgui::POPUP_SMALL_NO_HEIGHT)};

    imgui::PopupHelper progressPopup{imgui::PopupHelper("Rendering...", imgui::POPUP_SMALL_NO_HEIGHT)};

    Manager();
    ~Manager();

    Document* get(int = -1);
    void open(const std::string&, bool = false, bool = true);
    void new_(const std::string&);
    void save(int, const std::string& = {});
    void save(const std::string& = {});
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

    void autosave_files_load();
    void autosave_files_open();
    void autosave_files_write();
    void autosave_files_clear();

    std::filesystem::path autosave_directory_get();
  };
}
