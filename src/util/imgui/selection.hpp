#pragma once

#include <set>
#include <vector>

#include <imgui/imgui.h>

namespace anm2ed::util::imgui
{
  void external_storage_set(ImGuiSelectionExternalStorage*, int, bool);

  class MultiSelectStorage : public std::set<int>
  {
  public:
    ImGuiSelectionExternalStorage internal{};
    ImGuiMultiSelectIO* io{};
    std::vector<int>* indexMap{};

    using std::set<int>::set;
    using std::set<int>::operator=;
    using std::set<int>::begin;
    using std::set<int>::rbegin;
    using std::set<int>::end;
    using std::set<int>::size;
    using std::set<int>::insert;
    using std::set<int>::erase;
    using std::set<int>::contains;
    using std::set<int>::clear;

    MultiSelectStorage();
    void start(size_t,
               ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ScopeWindow);
    void apply();
    void finish();
    void set_index_map(std::vector<int>*);
    int resolve_index(int) const;
  };
}
