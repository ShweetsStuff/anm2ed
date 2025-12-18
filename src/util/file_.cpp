#include "file_.h"

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
}