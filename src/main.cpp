#include "loader.h"
#include "state.h"

using namespace anm2ed::loader;
using namespace anm2ed::state;

int main(int argc, const char** argv)
{
  Loader loader(argc, argv);

  if (loader.isError) return EXIT_FAILURE;

  State state(loader.window, loader.arguments);

  while (!state.isQuit)
    state.loop(loader.window, loader.settings);

  return EXIT_SUCCESS;
}
