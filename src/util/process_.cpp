#include "process_.h"

#ifdef WIN32
  #include <string>
  #include <windows.h>
#endif

namespace anm2ed::util
{
#ifdef WIN32
  namespace
  {
    std::wstring utf8_to_wstring(const char* text)
    {
      if (!text) return {};
      auto sizeRequired = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
      if (sizeRequired <= 0) return {};
      std::wstring wide(static_cast<std::size_t>(sizeRequired - 1), L'\0');
      MultiByteToWideChar(CP_UTF8, 0, text, -1, wide.data(), sizeRequired);
      return wide;
    }
  }
#endif

  Process::Process(const char* command, const char* mode) { open(command, mode); }

  Process::~Process() { close(); }

  bool Process::open(const char* command, const char* mode)
  {
    close();

#ifdef WIN32
    auto wideCommand = utf8_to_wstring(command);
    auto wideMode = utf8_to_wstring(mode);
    pipe = _wpopen(wideCommand.c_str(), wideMode.c_str());
#else
    pipe = popen(command, mode);
#endif
    return pipe != nullptr;
  }

  int Process::close()
  {
    if (pipe)
    {
#ifdef WIN32
      auto result = _pclose(pipe);
#else
      auto result = pclose(pipe);
#endif
      pipe = nullptr;

      return result;
    }

    return -1;
  }

  std::string Process::output_get_and_close()
  {
    std::string output{};
    if (!pipe) return {};

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe))
      output += buffer;

    close();

    return output;
  }

  FILE* Process::get() const { return pipe; }

  Process::operator bool() const { return pipe != nullptr; }
}
