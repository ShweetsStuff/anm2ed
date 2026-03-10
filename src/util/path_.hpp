#pragma once

#include <filesystem>
#include <string>

namespace anm2ed::util::path
{
  std::filesystem::path to_lower(const std::filesystem::path&);
  std::filesystem::path backslash_replace(const std::filesystem::path&);
  std::string to_utf8(const std::filesystem::path&);
  std::filesystem::path from_utf8(const std::string&);
  std::filesystem::path make_relative(const std::filesystem::path&);

  bool is_exist(const std::filesystem::path&);
  bool is_executable(const std::filesystem::path&);
  bool is_extension(const std::filesystem::path&, const std::string&);

  bool ensure_directory(const std::filesystem::path&);

  std::filesystem::path lower_case_backslash_handle(const std::filesystem::path&);
}
