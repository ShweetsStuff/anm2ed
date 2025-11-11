#include "loader.h"
#include "state.h"

int main(int argc, const char** argv)
{
  anm2ed::Loader loader(argc, argv);

  if (loader.isError) return EXIT_FAILURE;

  anm2ed::State state(loader.window, loader.arguments);

  while (!state.isQuit)
    state.loop(loader.window, loader.settings);

  return EXIT_SUCCESS;
}
