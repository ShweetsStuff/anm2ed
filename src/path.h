#pragma once

#include <filesystem>
#include <string>

  std::filesystem::path path_to_lower(const std::filesystem::path&);
  std::filesystem::path path_backslash_replace(const std::filesystem::path&);
  std::string path_to_utf8(const std::filesystem::path&);
  std::filesystem::path path_from_utf8(const std::string&);
  std::filesystem::path path_make_relative(const std::filesystem::path&);
  std::filesystem::path path_make_relative(const std::filesystem::path&, const std::filesystem::path&);

  bool is_path_exist(const std::filesystem::path&);
  bool is_path_executable(const std::filesystem::path&);
  bool is_path_extension(const std::filesystem::path&, const std::string&);

  bool path_ensure_directory(const std::filesystem::path&);

  std::filesystem::path path_backslash_handle(const std::filesystem::path&);
  std::filesystem::path path_case_insensitive_find(const std::filesystem::path&);
