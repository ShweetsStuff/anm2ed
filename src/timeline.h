#pragma once

#include "anm2.h"
#include "clipboard.h"
#include "document.h"
#include "manager.h"
#include "resources.h"
#include "settings.h"

namespace anm2ed::timeline
{
  class Timeline
  {
    bool isDragging{};
    bool isWindowHovered{};
    bool isHorizontalScroll{};
    imgui::PopupHelper propertiesPopup{imgui::PopupHelper("Item Properties")};
    imgui::PopupHelper bakePopup{imgui::PopupHelper("Bake", imgui::POPUP_SMALL, true)};
    std::string addItemName{};
    int addItemSpritesheetID{};
    bool addItemIsRect{};
    int addItemID{-1};
    bool isUnusedItemsSet{};
    std::set<int> unusedItems{};
    glm::vec2 scroll{};
    ImDrawList* pickerLineDrawList{};
    ImGuiStyle style{};

    void context_menu(document::Document&, settings::Settings&, clipboard::Clipboard&);
    void item_child(manager::Manager&, Document&, anm2::Animation*, settings::Settings&, resources::Resources&,
                    clipboard::Clipboard&, anm2::Type, int, int&);
    void items_child(manager::Manager&, Document&, anm2::Animation*, settings::Settings&, resources::Resources&,
                     clipboard::Clipboard&);
    void frame_child(document::Document&, anm2::Animation*, settings::Settings&, resources::Resources&,
                     clipboard::Clipboard&, anm2::Type, int, int&, float);
    void frames_child(document::Document&, anm2::Animation*, settings::Settings&, resources::Resources&,
                      clipboard::Clipboard&);

    void popups(document::Document&, anm2::Animation*, settings::Settings&);

  public:
    void update(manager::Manager&, settings::Settings&, resources::Resources&, clipboard::Clipboard&);
  };
}
