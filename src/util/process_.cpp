#include "process_.h"

#ifdef WIN32
  #include <cstddef>
  #include <string>
  #include <windows.h>
#endif

namespace anm2ed::util
{
  Process::Process(const char* command, const char* mode) { open(command, mode); }

  Process::~Process() { close(); }

  bool Process::open(const char* command, const char* mode)
  {
    close();

#ifdef WIN32
    pipe = _popen(command.c_str(), mode.c_str());
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
