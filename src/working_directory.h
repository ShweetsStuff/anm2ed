#pragma once

#include <filesystem>

namespace anm2ed::util
{
  class WorkingDirectory
  {
  public:
    enum Type
    {
      FILE,
      DIRECTORY
    };

    std::filesystem::path previous{};

    WorkingDirectory(const std::filesystem::path&, Type type = DIRECTORY);
    ~WorkingDirectory();
  };
}