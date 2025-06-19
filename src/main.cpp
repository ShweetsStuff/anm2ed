#include "main.h"

s32
main(s32 argc, char* argv[])
{
	State state;

	/* If anm2 given on command line, set state argument to that (will be loaded) */
	if (argc > 0 && argv[1])
	{
		strncpy(state.argument, argv[1], PATH_MAX - 1);
		state.isArgument = true;
	}

	init(&state);

	while (state.isRunning)
		loop(&state);

	quit(&state);

	return EXIT_SUCCESS;
}