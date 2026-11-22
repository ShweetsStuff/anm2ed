#pragma once

#include <filesystem>
#include <string>
#include <string_view>

  class File
  {
  public:
    File() = default;
    File(const std::filesystem::path&, const char* mode);
    ~File();

    bool open(const std::filesystem::path&, const char* mode);
    int close();
    FILE* get() const;
    explicit operator bool() const;

  private:
    FILE* handle{};
  };

    bool file_text_load(const std::filesystem::path&, std::string*, const char* mode = "rb");
    bool file_text_save(const std::filesystem::path&, std::string_view, const char* mode = "wb");
