#include "file_.hpp"

#include <array>
#include <cstring>

namespace anm2ed::util
{
  File::File(const std::filesystem::path& path, const char* mode) { open(path, mode); }

  File::~File() { close(); }

  bool File::open(const std::filesystem::path& path, const char* mode)
  {
    close();

#ifdef _WIN32
    std::wstring wideMode{};
    if (mode)
      wideMode.assign(mode, mode + std::strlen(mode));
    else
      wideMode = L"rb";
    handle = _wfopen(path.native().c_str(), wideMode.c_str());
#else
    handle = std::fopen(path.string().c_str(), mode);
#endif

    return handle != nullptr;
  }

  int File::close()
  {
    if (handle)
    {
      auto result = std::fclose(handle);
      handle = nullptr;
      return result;
    }
    return -1;
  }

  FILE* File::get() const { return handle; }

  File::operator bool() const { return handle != nullptr; }

  namespace file
  {
    bool read_to_string(const std::filesystem::path& path, std::string* output, const char* mode)
    {
      if (!output) return false;

      File file(path, mode);
      if (!file) return false;

      output->clear();
      std::array<char, 4096> buffer{};
      while (true)
      {
        auto read = std::fread(buffer.data(), 1, buffer.size(), file.get());
        if (read > 0) output->append(buffer.data(), read);
        if (read < buffer.size()) return std::feof(file.get()) != 0;
      }
    }

    bool write_string(const std::filesystem::path& path, std::string_view data, const char* mode)
    {
      File file(path, mode);
      if (!file) return false;

      return std::fwrite(data.data(), 1, data.size(), file.get()) == data.size();
    }
  }
}
