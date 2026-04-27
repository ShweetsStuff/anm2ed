#pragma once

#include <filesystem>
#include <new>
#include <string>
#include <unordered_map>

#include "snapshots_new.hpp"
#include "util/imgui/selection.hpp"
#include "util/imgui/widgets.hpp"

namespace anm2ed
{
  class DocumentNew
  {
  public:
    struct ElementState
    {
      int reference{-1};
      util::imgui::MultiSelectStorage selected{};
      util::imgui::RenameState renameState{util::imgui::RENAME_SELECTABLE};
      int renameId{-1};
    };

    std::filesystem::path path{};
    SnapshotsNew snapshots{};
    SnapshotNew& current = snapshots.current;
    anm2_new::Anm2& anm2 = current.anm2;
    std::string& message = current.message;
    std::unordered_map<anm2_new::Anm2::Element::Type, ElementState> selectedElements{};
    bool isOpen{false};
    bool isValid{false};

    DocumentNew() = default;
    explicit DocumentNew(const std::filesystem::path&, std::string* = nullptr);
    DocumentNew(const DocumentNew&) = delete;
    DocumentNew& operator=(const DocumentNew&) = delete;
    DocumentNew(DocumentNew&&) noexcept;
    DocumentNew& operator=(DocumentNew&&) noexcept;

    ElementState* element_state_get(anm2_new::Anm2::Element::Type);
    const ElementState* element_state_get(anm2_new::Anm2::Element::Type) const;
    bool save(const std::filesystem::path& = {}, std::string* = nullptr,
              anm2_new::Anm2::Compatibility = anm2_new::Anm2::ANM2ED);
    std::filesystem::path filename_get() const;
    void snapshot(const std::string&);
    void undo();
    void redo();
    bool is_able_to_undo();
    bool is_able_to_redo();
  };
}
