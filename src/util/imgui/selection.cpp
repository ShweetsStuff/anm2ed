#include "selection.hpp"

namespace anm2ed::util::imgui
{
  void external_storage_set(ImGuiSelectionExternalStorage* self, int id, bool isSelected)
  {
    auto* storage = static_cast<MultiSelectStorage*>(self->UserData);
    auto value = storage ? storage->resolve_index(id) : id;
    if (!storage) return;

    if (isSelected)
      storage->insert(value);
    else
      storage->erase(value);
  }

  MultiSelectStorage::MultiSelectStorage() { internal.AdapterSetItemSelected = external_storage_set; }

  void MultiSelectStorage::start(size_t size, ImGuiMultiSelectFlags flags)
  {
    internal.UserData = this;
    io = ImGui::BeginMultiSelect(flags, this->size(), size);
    apply();
  }

  void MultiSelectStorage::apply() { internal.ApplyRequests(io); }

  void MultiSelectStorage::finish()
  {
    io = ImGui::EndMultiSelect();
    apply();
  }

  void MultiSelectStorage::set_index_map(std::vector<int>* map) { indexMap = map; }

  int MultiSelectStorage::resolve_index(int index) const
  {
    if (!indexMap) return index;
    if (index < 0 || index >= (int)indexMap->size()) return index;
    return (*indexMap)[index];
  }
}
