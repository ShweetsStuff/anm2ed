#include "document_manager.h"

#include "util.h"

using namespace anm2ed::util;

namespace anm2ed::document_manager
{
  Document* DocumentManager::get()
  {
    return vector::find(documents, selected);
  }

  Document* DocumentManager::get(int index)
  {
    return vector::find(documents, index);
  }

  bool DocumentManager::open(const std::string& path, bool isNew)
  {
    std::string errorString{};
    Document document = Document(path, isNew, &errorString);
    if (document.is_valid())
    {
      documents.emplace_back(std::move(document));
      selected = documents.size() - 1;
      pendingSelected = selected;
      return true;
    }
    return false;
  }

  bool DocumentManager::new_(const std::string& path)
  {
    return open(path, true);
  }

  void DocumentManager::save(int index, const std::string& path)
  {
    auto document = get(index);
    if (!document) return;
    std::string errorString{};

    document->path = !path.empty() ? path : document->path.string();

    document->save(document->path, &errorString);
  }

  void DocumentManager::save(const std::string& path)
  {
    save(selected, path);
  }

  void DocumentManager::close(int index)
  {
    documents.erase(documents.begin() + index);
  }
}
