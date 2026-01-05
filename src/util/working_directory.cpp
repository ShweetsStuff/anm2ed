#include "working_directory.h"

#include <format>

#include "log.h"
#include "path_.h"

namespace anm2ed::util
{
  WorkingDirectory::WorkingDirectory(const std::filesystem::path& path, Type type)
  {
    std::error_code ec{};
    previous = std::filesystem::current_path(ec);
    if (ec)
    {
      logger.warning(std::format("Could not query current directory: {}", ec.message()));
      previous.clear();
      ec.clear();
    }

    if (type == FILE)
    {
      std::filesystem::path filePath = path;
      std::filesystem::path parentPath = filePath.parent_path();
      std::filesystem::current_path(parentPath, ec);
      if (ec)
        logger.warning(
            std::format("Could not set current directory to {}: {}", path::to_utf8(parentPath), ec.message()));
    }
    else
    {
      std::filesystem::current_path(path, ec);
      if (ec)
        logger.warning(std::format("Could not set current directory to {}: {}", path::to_utf8(path), ec.message()));
    }
  }

  WorkingDirectory::~WorkingDirectory()
  {
    if (previous.empty()) return;

    std::error_code ec{};
    std::filesystem::current_path(previous, ec);
    if (ec)
      logger.warning(
          std::format("Could not restore current directory to {}: {}", path::to_utf8(previous), ec.message()));
  }
}
