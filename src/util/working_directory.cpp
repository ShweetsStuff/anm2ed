#include "working_directory.h"

namespace anm2ed::util
{
  WorkingDirectory::WorkingDirectory(const std::filesystem::path& path, Type type)
  {
    previous = std::filesystem::current_path();
    if (type == FILE)
    {
      std::filesystem::path filePath = path;
      std::filesystem::path parentPath = filePath.parent_path();
      std::filesystem::current_path(parentPath);
    }
    else
      std::filesystem::current_path(path);
  }

  WorkingDirectory::~WorkingDirectory() { std::filesystem::current_path(previous); }
}