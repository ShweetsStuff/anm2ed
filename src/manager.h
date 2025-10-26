#pragma once

#include <vector>

#include "document.h"
#include "imgui.h"

using namespace anm2ed::document;

namespace anm2ed::manager
{
  class Manager
  {
  public:
    std::vector<Document> documents{};
    int selected{};
    int pendingSelected{};

    anm2::Layer editLayer{};
    imgui::PopupHelper layerPropertiesPopup{imgui::PopupHelper("Layer Properties", imgui::POPUP_SMALL, true)};

    anm2::Null editNull{};
    imgui::PopupHelper nullPropertiesPopup{imgui::PopupHelper("Null Properties", imgui::POPUP_SMALL, true)};

    Document* get(int = -1);
    void open(const std::string&, bool = false);
    void new_(const std::string&);
    void save(int, const std::string& = {});
    void save(const std::string& = {});
    void close(int);
    void layer_properties_open(int = -1);
    void layer_properties_trigger();
    void layer_properties_end();
    void layer_properties_close();
    void null_properties_open(int = -1);
    void null_properties_trigger();
    void null_properties_end();
    void null_properties_close();
  };
}
