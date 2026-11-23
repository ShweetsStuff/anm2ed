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
    FILE* handle = nullptr;
};

bool file_text_load(const std::filesystem::path&, std::string*, const char* mode = "rb");
// Written to a temporary file beside it, then renamed over it: a failed or cut-short write leaves the old file whole.
bool file_text_save(const std::filesystem::path&, std::string_view);
