#include "manager_new.hpp"

namespace anm2ed
{
  DocumentNew* ManagerNew::get(int index)
  {
    if (index < 0) index = selected;
    if (index < 0 || index >= (int)documents.size()) return nullptr;
    return &documents[index];
  }

  const DocumentNew* ManagerNew::get(int index) const
  {
    if (index < 0) index = selected;
    if (index < 0 || index >= (int)documents.size()) return nullptr;
    return &documents[index];
  }

  DocumentNew& ManagerNew::open(const std::filesystem::path& path, std::string* errorString)
  {
    documents.emplace_back(path, errorString);
    selected = (int)documents.size() - 1;
    return documents.back();
  }

  void ManagerNew::set(int index)
  {
    if (index < 0 || index >= (int)documents.size()) return;
    selected = index;
  }
}
