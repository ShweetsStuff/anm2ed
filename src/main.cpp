#include "loader.hpp"
#include "log.hpp"
#include "state.hpp"

#ifdef _WIN32
  #define SDL_MAIN_HANDLED
  #include <SDL3/SDL_main.h>
  #include <string>
  #include <string_view>
  #include <vector>
  #include <windows.h>
  #include <shellapi.h>
#endif

#ifdef _WIN32
namespace anm2ed::main
{
  struct CommandLineArguments
  {
    std::vector<std::string> strings{};
    std::vector<const char*> pointers{};
  };

  std::string wstring_utf8_get(std::wstring_view value)
  {
    if (value.empty()) return {};
    auto size = WideCharToMultiByte(CP_UTF8, 0, value.data(), (int)value.size(), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};

    std::string utf8((std::size_t)size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), (int)value.size(), utf8.data(), size, nullptr, nullptr);
    return utf8;
  }

  CommandLineArguments command_line_arguments_get()
  {
    int count{};
    auto wideArguments = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!wideArguments) return {};
    if (count <= 0)
    {
      LocalFree(wideArguments);
      return {};
    }

    CommandLineArguments arguments{};
    arguments.strings.reserve((std::size_t)count);
    for (int i = 0; i < count; ++i)
      arguments.strings.push_back(wstring_utf8_get(wideArguments[i]));
    LocalFree(wideArguments);

    arguments.pointers.reserve(arguments.strings.size());
    for (auto& argument : arguments.strings)
      arguments.pointers.push_back(argument.c_str());

    return arguments;
  }
}
#endif

namespace anm2ed
{
  int application_run(int argc, const char** argv)
  {
    return log_exceptions([&]()
    {
      Loader loader(argc, argv);

      if (loader.isError) return EXIT_FAILURE;

      State state(loader.window, loader.settings, loader.arguments);

      while (!state.isQuit)
        state.loop(loader.window, loader.settings);

      return EXIT_SUCCESS;
    });
  }
}

int main(int argc, const char** argv)
{
#ifdef _WIN32
  auto arguments = anm2ed::main::command_line_arguments_get();
  if (!arguments.pointers.empty())
    return anm2ed::application_run((int)arguments.pointers.size(), arguments.pointers.data());
#endif
  return anm2ed::application_run(argc, argv);
}

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  auto arguments = anm2ed::main::command_line_arguments_get();
  return anm2ed::application_run((int)arguments.pointers.size(), arguments.pointers.data());
}
#endif
