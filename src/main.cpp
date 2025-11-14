#include "loader.h"
#include "state.h"

#ifdef _WIN32
  #define SDL_MAIN_HANDLED
  #include <SDL3/SDL_main.h>
  #include <vector>
  #include <windows.h>
#endif

int main(int argc, const char** argv)
{
  anm2ed::Loader loader(argc, argv);

  if (loader.isError) return EXIT_FAILURE;

  anm2ed::State state(loader.window, loader.settings, loader.arguments);

  while (!state.isQuit)
    state.loop(loader.window, loader.settings);

  return EXIT_SUCCESS;
}

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  std::vector<const char*> args(__argc);
  for (int i = 0; i < __argc; ++i)
    args[i] = __argv[i];
  return main(__argc, args.data());
}
#endif
