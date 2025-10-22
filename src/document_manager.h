#pragma once

#include <vector>

#include "document.h"

using namespace anm2ed::document;

namespace anm2ed::document_manager
{
  class DocumentManager
  {
  public:
    std::vector<Document> documents{};
    int selected{};
    int pendingSelected{};

    Document* get();
    Document* get(int index);
    bool open(const std::string& path, bool isNew = false);
    bool new_(const std::string& path);
    void save(int index, const std::string& path = {});
    void save(const std::string& path = {});
    void close(int index);
    void spritesheet_add(const std::string& path);
  };
}
