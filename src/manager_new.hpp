#pragma once

#include <filesystem>
#include <vector>

#include "document_new.hpp"

namespace anm2ed
{
  class ManagerNew
  {
  public:
    std::vector<DocumentNew> documents{};
    int selected{-1};

    DocumentNew* get(int = -1);
    const DocumentNew* get(int = -1) const;
    DocumentNew& open(const std::filesystem::path&, std::string* = nullptr);
    void set(int);
  };
}
